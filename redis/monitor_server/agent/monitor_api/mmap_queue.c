
/**
 * Tencent is pleased to support the open source community by making MSEC available.
 *
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the GNU General Public License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. You may 
 * obtain a copy of the License at
 *
 *     https://opensource.org/licenses/GPL-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the 
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions
 * and limitations under the License.
 */


/*
 * mmap_queue.c
 * Implementation of a mmap queue
 *
 */
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <sys/file.h>
#include "mmap_queue.h"
#include "opt_time.h"

#define TOKEN_NO_DATA    0
#define TOKEN_SKIPPED    0xdb030000 // token to mark the node is skipped
#define TOKEN_HAS_DATA   0x0000db03 // token to mark the valid start of a node

#define SQ_MAX_CONFLICT_TRIES	10 // maximum number of reading attempts for read-write conflict detection

#define CAS32(ptr, val_old, val_new)({ char ret; __asm__ __volatile__("lock; cmpxchgl %2,%0; setz %1": "+m"(*ptr), "=q"(ret): "r"(val_new),"a"(val_old): "memory"); ret;})
#define wmb() __asm__ __volatile__("sfence":::"memory")
#define rmb() __asm__ __volatile__("lfence":::"memory")

static char errmsg[256];

const char *mq_errorstr(struct mmap_queue *mq)
{
	return mq? mq->errmsg : errmsg;
}

// Increase head/tail by val
#define SQ_ADD_HEAD(queue, val) 	(((queue)->head_pos+(val))%((queue)->ele_count+1))
#define SQ_ADD_TAIL(queue, val) 	(((queue)->tail_pos+(val))%((queue)->ele_count+1))

// Next position after head/tail
#define SQ_NEXT_HEAD(queue) 	SQ_ADD_HEAD(queue, 1)
#define SQ_NEXT_TAIL(queue) 	SQ_ADD_TAIL(queue, 1)

#define SQ_ADD_POS(queue, pos, val)     (((pos)+(val))%((queue)->ele_count+1))

#define SQ_IS_QUEUE_FULL(queue) 	(SQ_NEXT_TAIL(queue)==(queue)->head_pos)
#define SQ_IS_QUEUE_EMPTY(queue)	((queue)->tail_pos==(queue)->head_pos)

#define SQ_EMPTY_NODES(queue) 	(((queue)->head_pos+(queue)->ele_count-(queue)->tail_pos) % ((queue)->ele_count+1))
#define SQ_USED_NODES(queue) 	((queue)->ele_count - SQ_EMPTY_NODES(queue))

#define SQ_EMPTY_NODES2(queue, head) (((head)+(queue)->ele_count-(queue)->tail_pos) % ((queue)->ele_count+1)) 
#define SQ_USED_NODES2(queue, head) ((queue)->ele_count - SQ_EMPTY_NODES2(queue, head))

// The size of a node
#define SQ_NODE_SIZE_ELEMENT(ele_size)	(sizeof(struct mq_node_head_t)+ele_size)
#define SQ_NODE_SIZE(queue)            	(SQ_NODE_SIZE_ELEMENT((queue)->ele_size))

// Convert an index to a node_head pointer
#define SQ_GET(queue, idx) ((struct mq_node_head_t *)(((char*)(queue)->nodes) + (idx)*SQ_NODE_SIZE(queue)))

// Estimate how many nodes are needed by this length
#define SQ_NUM_NEEDED_NODES(queue, datalen) 	((datalen) + sizeof(struct mq_node_head_t) + SQ_NODE_SIZE(queue) -1) / SQ_NODE_SIZE(queue)


// optimized gettimeofday
#include "opt_time.h"

// mmap operation wrapper
inline off_t file_length(const char* szFile)
{
	struct stat s;
	if(0 != stat(szFile, &s))
		return 0;
	return s.st_size;
}

static char *open_mmap(const char* mmap_file, size_t length, int *bCreate)
{
	//get full path
	if(strlen(mmap_file) >= PATH_MAX)
		return NULL;
	char szFullPath[PATH_MAX*2];
	if(mmap_file[0] != '/')
	{
		if(NULL == getcwd(szFullPath, PATH_MAX))
			return NULL;
		int len = strlen(szFullPath);
		char c = szFullPath[len - 1];
		if(c != '/')
		{
			snprintf(szFullPath + len, sizeof(szFullPath)-len, "/%s", mmap_file);
		}
		else
		{
			snprintf(szFullPath + len, sizeof(szFullPath)-len, "%s", mmap_file);
		}
	}
	else
	{		
		snprintf(szFullPath, sizeof(szFullPath), "%s", mmap_file);
	}

	size_t fl = file_length(szFullPath);
	
	int creating = *bCreate, created = 0;
	char* buf;

	if(fl == 0)
	{
		if(length == 0) //not created, return ERR
			return NULL;
		mode_t old_mask = umask( 011 );
		int fd2 = open(szFullPath, O_RDWR|O_APPEND|O_CREAT, 0666);
		umask( old_mask );
		if(fd2 == -1)
		{
			return NULL;
		}
		const size_t l = 4096;
		char pp[l];
		memset(pp, 0, l);
		size_t len = length - fl;
		for(; len >= l; len -= l)
			write(fd2, pp, l);
		if(len > 0)
			write(fd2, pp, len);
		fl = length;
		created = 1;
	}
	else if(creating)
	{
		if(fl != length)
		{
			printf("filesize mismatched(existing %lu, creating %lu)\n", fl, length);
			return NULL;
		}
	}
	mode_t old_mask = umask( 011 );
	int fd = open(szFullPath, O_RDWR, 0666);
	buf = mmap(0, fl, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	umask( old_mask );
	
	*bCreate = created;
	*(int*)buf = fl;

	return buf;
}

// mmap operation wrapper
static struct mq_head_t *open_mmap_queue(const char* mmap_file, long ele_size, long ele_count, int create)
{
	long allocate_size;
	struct mq_head_t *mmap;

	if(create)
	{
		ele_size = (((ele_size + 7)>>3) << 3); // align to 8 bytes
		// We need an extra element for ending control
		allocate_size = sizeof(struct mq_head_t) + SQ_NODE_SIZE_ELEMENT(ele_size)*(ele_count+1);
		// Align to 4MB boundary
		allocate_size = (allocate_size + (4UL<<20) - 1) & (~((4UL<<20)-1));
		//printf("mmap size needed for queue - %lu.\n", allocate_size);
	}
	else
	{
		allocate_size = 0;
	}

	int created = create;
	if (!(mmap = (struct mq_head_t *)open_mmap(mmap_file, allocate_size, &created)))
	{
		return NULL;
	}

	if(created)
	{
		mmap->ele_size = ele_size;
		mmap->ele_count = ele_count;
	}
	else if(create)// verify parameters if open for writing
	{
		if(mmap->ele_size!=ele_size || mmap->ele_count!=ele_count)
		{
			printf("mmap parameters mismatched: \n");
			printf("    given:  ele_size=%ld, ele_count=%ld\n", ele_size, ele_count);
			printf("    in mmap: ele_size=%d, ele_count=%d\n", mmap->ele_size, mmap->ele_count);
			munmap(mmap, mmap->file_size);
			return NULL;
		}
	}

	return mmap;
}


#define SQ_LOCK_FILE	"/tmp/.mmap_queue_lock"

static int exc_lock(int iUnlocking, int *fd, const char* mmap_file)
{
	char sLockFile[1024];
	int idx = strlen(mmap_file)-1;
	for(; idx >=0; idx--)
	{
		if(mmap_file[idx] == '/')
			break;
	}
	snprintf(sLockFile, sizeof(sLockFile), "%s_%s", SQ_LOCK_FILE, mmap_file+idx+1);

	if(*fd <= 0)
	{
		mode_t old_mask = umask( 011 );
		*fd = open(sLockFile, O_CREAT, 666);
		umask(old_mask);
	}
	if(*fd < 0)
	{
		printf("open lock file %s failed: %s\n", sLockFile, strerror(errno));
		return -1;
	}

	int ret = flock(*fd, iUnlocking? LOCK_UN:LOCK_EX);
	if(ret < 0)
	{
		printf("%s file %s failed: %s\n", iUnlocking? "Unlock":"Lock", SQ_LOCK_FILE, strerror(errno));
		return -2;
	}
	return 0;
}


// Create a mmap queue
// Parameters:
//     mmap_file      - mmap file
//     ele_size     - preallocated size for each element
//     ele_count    - preallocated number of elements
// Returns a mmap queue pointer or NULL if failed
struct mmap_queue *mq_create(const char* mmap_file, int ele_size, int ele_count)
{
	int fd = -1;
	signal(SIGPIPE, SIG_IGN);

	exc_lock(0, &fd, mmap_file); // lock, if failed, printf and ignore

	struct mmap_queue *queue = calloc(1, sizeof(struct mmap_queue));
	if(queue==NULL)
	{
		snprintf(errmsg, sizeof(errmsg), "Out of memory");
		exc_lock(1, &fd, mmap_file); // ulock
		return NULL;
	}

	if(ele_size<=0 || ele_count<=RESERVE_BLOCK_COUNT || mmap_file == NULL) // invalid parameter
	{
		free(queue);
		if(ele_count<=RESERVE_BLOCK_COUNT)
			snprintf(errmsg, sizeof(errmsg), "Bad argument: ele_count(%d) should be greater than RESERVE_BLOCK_COUNT(%d)", ele_count, RESERVE_BLOCK_COUNT);
		else
			snprintf(errmsg, sizeof(errmsg), "Bad argument");
		exc_lock(1, &fd, mmap_file); // ulock
		return NULL;
	}

	queue->head = open_mmap_queue(mmap_file, ele_size, ele_count, 1);
	if(queue->head==NULL)
	{
		free(queue);
		snprintf(errmsg, sizeof(errmsg), "Get mmap failed");
		exc_lock(1, &fd, mmap_file); // ulock
		return NULL;
	}
	exc_lock(1, &fd, mmap_file); // ulock
	return queue;
}

// Open an existing mmap queue for reading data
struct mmap_queue *mq_open(const char* mmap_file)
{
	signal(SIGPIPE, SIG_IGN);

	struct mmap_queue *queue = calloc(1, sizeof(struct mmap_queue));
	if(queue==NULL)
	{
		snprintf(errmsg, sizeof(errmsg), "Out of memory");
		return NULL;
	}

	queue->head = open_mmap_queue(mmap_file, 0, 0, 0);
	if(queue->head==NULL)
	{
		free(queue);
		snprintf(errmsg, sizeof(errmsg), "Open mmap failed");
		return NULL;
	}
	return queue;
}

// Destroy TP created by mmap_create()
void mq_destroy(struct mmap_queue *queue)
{
	munmap(queue->head, queue->head->file_size);
	free(queue);
}

// Add data to end of mmap queue
// Returns 0 on success or
//     -1 - invalid parameter
//     -2 - mmap queue is full
int mq_put(struct mmap_queue *mq, void *data, int datalen)
{
	u32_t idx;
	struct mq_node_head_t *node;
	int nr_nodes;
	int old_tail, new_tail;
	struct mq_head_t *queue = mq->head;

	if(queue==NULL || data==NULL || datalen<=0 || datalen>MAX_SQ_DATA_LENGTH)
	{
		snprintf(mq->errmsg, sizeof(mq->errmsg), "Bad argument");
		return -1;
	}


	while(1)
	{
		rmb(); // sync read
		old_tail = queue->tail_pos;

		// calculate the number of nodes needed
		nr_nodes = SQ_NUM_NEEDED_NODES(queue, datalen);
	
		if(SQ_EMPTY_NODES(queue)<nr_nodes+RESERVE_BLOCK_COUNT)
		{
			snprintf(mq->errmsg, sizeof(mq->errmsg), "Not enough for new data");
			return -2;
		}
	
		idx = old_tail;
		node = SQ_GET(queue, idx);
		new_tail = SQ_ADD_TAIL(queue, nr_nodes);
	
		if(new_tail < old_tail) // wrapped back
		{
			// We need a set of continuous nodes
			// So skip the empty nodes at the end, and begin allocation at index 0
			idx = 0;
			new_tail = nr_nodes;
			node = SQ_GET(queue, 0);
	
			if(queue->head_pos-1 < nr_nodes)
			{
				snprintf(mq->errmsg, sizeof(mq->errmsg), "Not enough for new data");
				return -2; // not enough empty nodes
			}
		}

		if(!CAS32(&queue->tail_pos, old_tail, new_tail)) // CAS contest fail, try again
			continue;

		if(idx==0 && old_tail) // it's been wrapped around
		{
			// mark all the skipped blocks as being skipped
			// so that the reader process can identify whether it is
			// skipped or is being written
			struct mq_node_head_t *n;
			do
			{
				n = SQ_GET(queue, old_tail);
				n->start_token = TOKEN_SKIPPED;
				old_tail = SQ_ADD_POS(queue, old_tail, 1);
			}
			while(old_tail);
		}

		// initialize the new node
		node->datalen = datalen;
#ifdef __x86_64__
		struct timeval tv;
		opt_gettimeofday(&tv, NULL);
		node->enqueue_time.tv_sec = tv.tv_sec;
		node->enqueue_time.tv_usec = tv.tv_usec;
#else
		opt_gettimeofday(&node->enqueue_time, NULL);
#endif
		memcpy(node->data, data, datalen);
		node->start_token = TOKEN_HAS_DATA; // mark data ready for reading
		wmb(); // sync write with other processors
		break;
	}
	return 0;
}

int mq_get_usage(struct mmap_queue *mq)
{
	if(mq==NULL || mq->head==NULL) return 0;
	struct mq_head_t *queue = mq->head;
	return queue->ele_count? ((SQ_USED_NODES(queue))*100)/queue->ele_count : 0;
}

int mq_get_used_blocks(struct mmap_queue *mq)
{
	if(mq==NULL || mq->head==NULL) return 0;
	struct mq_head_t *queue = mq->head;
	return SQ_USED_NODES(queue);
}

// Retrieve data
// On success, buf is filled with the first queue data
// Returns the data length or
//     0  - no data in queue
//     -1 - invalid parameter
int mq_get(struct mmap_queue *mq, void *buf, int buf_sz, struct timeval *enqueue_time)
{
	struct mq_node_head_t *node;

	int nr_nodes, datalen;
	int old_head, new_head, head;
	struct mq_head_t *queue = mq->head;

	if(queue==NULL || buf==NULL || buf_sz<1)
	{
		snprintf(mq->errmsg, sizeof(mq->errmsg), "Bad argument");
		return -1;
	}

	rmb();
	head = old_head = queue->head_pos;
	do
	{
		if(queue->tail_pos==head) // end of queue
		{
			if(head!=old_head && CAS32(&queue->head_pos, old_head, head))
			{
				wmb();
				new_head = head;
				datalen = 0;
				break;
			}
			// head_pos not advanced or changed by someone else, simply returns
			mq->rw_conflict = 0;
			return 0;
		}

		node = SQ_GET(queue, head);
		if(node->start_token!=TOKEN_HAS_DATA) // read-write conflict or corruption of data
		{
			// if read-write conflict happens, we (the reader) will
			// try at most SQ_MAX_CONFLICT_TRIES times for the
			// writer to finish, and if the writer is unable to
			// finish in SQ_MAX_CONFLICT_TRIES consecutive reads,
			// we will treat it as node corruption
			if(node->start_token==TOKEN_NO_DATA && mq->rw_conflict<SQ_MAX_CONFLICT_TRIES)
			{
				// Attension:
				// this node may have been read by some other process,
				// if so, the header position should have been updated
				rmb();
				if(old_head!=queue->head_pos)
				{
					// read by others, start all over again
					head = old_head = queue->head_pos;
					continue;
				}
				mq->rw_conflict++;
				return 0; // returns no data
			}
			// check start_token once again in case the writer may have already finished writting for now
			// in this case, we should not deem it corrupted
			// special thanks to jiffychen for pointing out this situation.
			rmb();
			if(node->start_token!=TOKEN_HAS_DATA)
			{
				// treat it as data corruption and skip this corrupted node
				head = SQ_ADD_POS(queue, head, 1);
				continue;
			}
		}
		datalen = node->datalen;
		nr_nodes = SQ_NUM_NEEDED_NODES(queue, datalen);
		if(SQ_USED_NODES2(queue, head) < nr_nodes)
		{
			head = SQ_ADD_POS(queue, head, 1);
			continue;
		}
		new_head = SQ_ADD_POS(queue, head, nr_nodes);
		if(CAS32(&queue->head_pos, old_head, new_head))
		{
			wmb();
			if(enqueue_time)
			{
				enqueue_time->tv_sec = node->enqueue_time.tv_sec;
				enqueue_time->tv_usec = node->enqueue_time.tv_usec;
			}
			if(datalen > buf_sz)
			{
				snprintf(mq->errmsg, sizeof(mq->errmsg), "Data length(%u) exceeds supplied buffer size of %u", datalen, buf_sz);
				return -2;
			}
			memcpy(buf, node->data, datalen);
			break;
		}
		else // head_pos changed by someone else, start over
		{
			old_head = queue->head_pos;
			head = old_head;
		}
	} while(1);

	while(old_head!=new_head)
	{
		node = SQ_GET(queue, old_head);
		// reset start_token so that this node will not be treated as a starting node of data
		node->start_token = 0;
		old_head = SQ_ADD_POS(queue, old_head, 1);
	}

	wmb();
	mq->rw_conflict = 0;
	return datalen;
}

#if SQ_FOR_TEST

#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

//
// Below is a test program, please compile with:
// gcc -o mqtest -DSQ_FOR_TEST mmap_queue.c
//

static char m[1024*1024];

void test_put(struct mmap_queue *queue, int proc_count, int count, char *msg)
{
	int i;
	int pid = 0;

	for(i=1; i<=proc_count; i++)
	{
		if(fork()==0)
		{
			pid=i;
			break;
		}
	}

	for(i=0; pid && i<count; i++)
	{
		snprintf(m, 1024, "[%d:%d] %s", pid, i, msg);
		if(mq_put(queue, m, strlen(m))<0)
		{
			printf("put msg[%d] failed: %s\n", i, mq_errorstr(queue));
			return;
		}
	}
	if(pid) exit(0);
	while(wait(NULL)>0);
	printf("put successfully\n");
}

void test_get(struct mmap_queue *queue, int proc_count, int count)
{
	int i;
	int pid = 0;

	for(i=1; i<=proc_count; i++)
	{
		if(fork()==0)
		{
			pid=i;
			break;
		}
	}

	if(pid)
	{
		for(i=0; i<count; i++)
		{
			struct timeval tv;
			int l = mq_get(queue, m, sizeof(m), &tv);
			if(l<0)
			{
				printf("mq_get failed: %s\n", mq_errorstr(queue));
				break;
			}
			if(l==0)// no data
			{
				printf("no data\n");
				break;
			}
			// if we are able to retrieve data from queue, always
			// try it without sleeping
			m[l] = 0;
			printf("pid[%d] msg[%d] len[%d]: %s\n", pid, i, l, m);
		}
		exit(0);
	}
	while(wait(NULL)>0);
}

void press_test(struct mmap_queue *queue, uint32_t record_count, uint32_t record_size)
{
	struct timeval tv;
	int put_count=0,get_count=0;
	for(; put_count<record_count; )
	{
		while(put_count<record_count && mq_put(queue, m, record_size)==0) put_count ++;
		while(mq_get(queue, m, sizeof(m), &tv)>0) get_count ++;
	}
	printf("put %u, get %u finished\n", put_count, get_count);
}

int main(int argc, char *argv[])
{
	struct mmap_queue *queue;

	if(argc<3)
	{
badarg:
		printf("usage: \n");
		printf("     %s open <file>\n", argv[0]);
		printf("     %s create <file> <element_size> <element_count>\n", argv[0]);
		printf("     %s press <file> <record_count> <record_size>\n", argv[0]);
		printf("\n");
		return -1;
	}

	if(strcmp(argv[1], "open")==0 || strcmp(argv[1], "press")==0)
	{
		queue = mq_open(argv[2]);
	}
	else if(strcmp(argv[1], "create")==0 && argc==5)
	{
		queue = mq_create(argv[2], strtoul(argv[3], NULL, 10), strtoul(argv[4], NULL, 10));
	}
	else
	{
		goto badarg;
	}

	if(queue==NULL)
	{
		printf("failed to open mmap queue: %s\n", mq_errorstr(NULL));
		return -1;
	}

	if(strcmp(argv[1], "press")==0)
	{
		if(argc!=5) goto badarg;
		press_test(queue, strtoul(argv[3], NULL, 10), strtoul(argv[4], NULL, 10));
		return 0;
	}

	while(1)
	{
		static char cmd[1024*1024];
		printf("available commands: \n");
		printf("  put <concurrent_proc_count> <msg_count> <msg>\n");
		printf("  get <concurrent_proc_count> <msg_count>\n");
		printf("  quit\n");
		printf("cmd>"); fflush(stdout);
		if(fgets(cmd, sizeof(cmd), stdin)==NULL)
			return 0;
		if(strncmp(cmd, "put ", 4)==0)
		{
			char *pstr = cmd + 4;
			while(isspace(*pstr)) pstr ++;
			int proc_count = atoi(pstr);
			if(proc_count<1) proc_count = 1;
			while(isdigit(*pstr)) pstr ++;
			while(isspace(*pstr)) pstr ++;
			int count = atoi(pstr);
			if(count<1) count = 1;
			while(isdigit(*pstr)) pstr ++;
			while(isspace(*pstr)) pstr ++;
			test_put(queue, proc_count, count, pstr);
		}
		else if(strncmp(cmd, "get ", 4)==0)
		{
			char *pstr = cmd + 4;
			while(isspace(*pstr)) pstr ++;
			int proc_count = atoi(pstr);
			if(proc_count<1) proc_count = 1;
			while(isdigit(*pstr)) pstr ++;
			while(isspace(*pstr)) pstr ++;
			int count = atoi(pstr);
			if(count<1) count = 1;
			test_get(queue, proc_count, count);
		}
		else if(strncmp(cmd, "quit", 4)==0 || strncmp(cmd, "exit", 4)==0)
		{
			return 0;
		}
	}
	return 0;
}

#endif
