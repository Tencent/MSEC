
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
 * mmap_queue.h
 * Declaration of a mmap queue
 *
 *  Based on transaction pool, features:
 *  1) support multiple writer and multiple reader processes/threads
 *  2) support timestamping for each data
 *  3) support auto detecting and skipping corrupted elements
 *  4) support variable user data size
 *  5) use highly optimized gettimeofday() to speedup sys time
 */

#ifndef __MMAP_QUEUE_HEADER__
#define __MMAP_QUEUE_HEADER__

#ifndef BOOL
#define BOOL int
#endif

#ifndef NULL
#define NULL 0
#endif

// Switch on this macro for compiling a test program
#ifndef SQ_FOR_TEST
#define SQ_FOR_TEST	0
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short u16_t;
typedef unsigned int u32_t;
typedef unsigned long long u64_t;

// Maximum bytes allowed for a queue data
#define MAX_SQ_DATA_LENGTH	65536

// number of blocks that will be reserved to avoid write-read conflict
// if your project restricts the use of memory, you can adjust this number
// down to 1, but the probabily of write-read conflict will also be increased
#define RESERVE_BLOCK_COUNT	100

//
// time structs in 32/64 bit environments are different
// these code makes time_t/timeval 32bit compatible, so that
// the writer compiled in 32bit can comunicate with the reader
// in 64bit environment, and vice versa.
//
#ifdef __x86_64__
#define time32_t int32_t
struct timeval32
{
	time32_t tv_sec;
	time32_t tv_usec;
};
#else
#define time32_t time_t
#define timeval32 timeval
#endif

struct mq_node_head_t
{
	u32_t start_token; // 0x0000db03, if the head position is corrupted, find next start token
	u32_t datalen; // length of stored data in this node
	struct timeval32 enqueue_time;

	// the actual data are stored here
	unsigned char data[0];

} __attribute__((packed));

struct mq_head_t
{
	int file_size;
	int ele_size;
	int ele_count;

	volatile int head_pos; // head position in the queue, pointer for reading
	volatile int tail_pos; // tail position in the queue, pointer for writting

	uint8_t reserved[1024*1024*4]; // 4MB of reserved space

	struct mq_node_head_t nodes[0];
};

struct mmap_queue
{
	struct mq_head_t *head;
	int rw_conflict; // read-write conflict counter
	char errmsg[256];
};


// Create a mmap queue
// Parameters:
//     mmap_file      - mmap file
//     ele_size     - preallocated size for each element
//     ele_count    - preallocated number of elements, this count should be greater than RESERVE_BLOCK_COUNT,
//                    and the real usable element count is (ele_count-RESERVE_BLOCK_COUNT)
// Returns a mmap queue pointer or NULL if failed
struct mmap_queue *mq_create(const char* mmap_file, int ele_size, int ele_count);

// Open an existing mmap queue for reading data
struct mmap_queue *mq_open(const char* mmap_file);

// Destroy queue created by mq_create()
void mq_destroy(struct mmap_queue *queue);

// Add data to end of mmap queue
// Returns 0 on success or
//     -1 - invalid parameter
//     -2 - mmap queue is full
int mq_put(struct mmap_queue *queue, void *data, int datalen);

// Retrieve data
// On success, buf is filled with the first queue data
// this function is multi-thread/multi-process safe
// Returns the data length or
//      0 - no data in queue
//     -1 - invalid parameter
int mq_get(struct mmap_queue *queue, void *buf, int buf_sz, struct timeval *enqueue_time);

// Get usage rate
// Returns a number from 0 to 99
int mq_get_usage(struct mmap_queue *queue);

// Get number of used blocks
int mq_get_used_blocks(struct mmap_queue *queue);

// If a queue operation failed, call this function to get an error reason
// Error msg for mq_create()/mq_open() can be retrieved by calling mq_errorstr(NULL)
const char *mq_errorstr(struct mmap_queue *queue);

#ifdef __cplusplus
}
#endif

#endif

