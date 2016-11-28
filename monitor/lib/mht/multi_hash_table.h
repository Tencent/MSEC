
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


/******************************************************************************

  FILENAME:	multi_hash_table.h

  DESCRIPTION:	多阶Hash表的一种实现：索引和数据分离，数据层使用LinkTable存储

 ******************************************************************************/
#ifndef __MULTI_HASH_TABLE__
#define __MULTI_HASH_TABLE__

#include <stdlib.h>
#include "link_table.h"


#define HASH_MAX_ROW           100
#define HASH_MAX_KEY_LENGTH    64
#define HASH_MAX_DATA_LENGTH   (1*1024*1024)

typedef ull64_t MhtIterator;

typedef struct
{
	union {
		uint32_t access_time;
		uint32_t access_seq;
	};

	uint32_t klen;
	uint8_t key[HASH_MAX_KEY_LENGTH];

	uint32_t dlen;
	uint8_t data[HASH_MAX_DATA_LENGTH];
}MhtData;

class shm_adapter;

struct MhtNode{
	uint32_t dwHashKey; // 32 bit hash for acKey[cKeyLen]
	union {
		uint32_t dwAccessTime;
		volatile uint32_t dwAccessSeq;
	};
	ull64_t ddwLtPos : 48; // Position in LinkTable
	ull64_t cKeyLen  : 8;
	ull64_t cDataVer  : 8;	// Use for data collision detection
	int32_t prePos;
	int32_t postPos;
	uint8_t acKey[0];
} ;

typedef struct {
	ull64_t ddwTotalShmSize; // total size allocated in shm (used if created from shm)
	ull64_t ddwHashNodeNum; // used for validity checking
	ull64_t ddwHashNodeSize;  // hash node size
	ull64_t ddwDataBlockSize; // data block size in link table
	ull64_t ddwDataBlockNum;  // data block num in link table
	ull64_t ddwRowNum;
	ull64_t auRowsNodeNum[HASH_MAX_ROW]; // each row's node num
	ull64_t auRowsStartNode[HASH_MAX_ROW]; // each row's starting node index, pre-calculated to speed up searching
	ull64_t ddwMaxUsedRow;   // max row index (start at 0) in use
	ull64_t ddwEraseIterator; // used to iterate through the hash table for erasing old key
	uint8_t cUseAccessSeq; // 0: use access_time for auto erasing, 1: use access_seq
	uint8_t acReserved[1024*1024 - 1];
	uint16_t wExpiryRatio;
	int32_t FirstPos;
	int32_t LastPos;
} MhtHeader;

#define DEFAULT_MAX_KEY_LENGTH    8
#define DEFAULT_ROW_NUM           20
#define DEFAULT_EXPIRY_RATIO		95
#define DEFAULT_INDEX_RATIO       50 // 1 hash node - 2 data block
#define DEFAULT_BLOCK_SIZE        512
#define DEFAULT_USE_HUGEPAGE      0  // not using huge page

extern "C" 
{
	unsigned int DefaultHashFunc(const void *, int);
	typedef unsigned int (*CalcHashKeyFunc)(const void *,int);
}

// Memset to use default values, and set the arguments you are interested in
typedef struct {
	ull64_t ddwShmKey;     // required: shm key
	ull64_t ddwBufferSize; // required: how much memory can be used by us
	uint8_t cMaxKeyLen;    // optional: max key length, use as small value as possible to save index space (default to 8)
	uint16_t wRowNum;      // optional: row num, level of MultiHashTable, in range [1-HASH_MAX_ROW] (default to 20)
	uint32_t dwExpiryRatio;	// optional: control the overall usage ratio while inserting data (default to 95)
	uint32_t dwIndexRatio; // optional: hash node num * 100 / data block num, controls the index-data ratio (default to 50)
	ull64_t ddwBlockSize;  // optional: data block size in LinkTable (default to 128)
	bool bUseHugePage;     // optional: whether use huge page to speed up attaching/detaching
	                       // you should make sure /proc/sys/vm/nr_hugepages has enough value (in tlinux, each page is 2MB)
	bool bUseAccessSeq;    // optional: whether use seq for auto-erase instead of access time
	CalcHashKeyFunc HashFunc;
} MhtInitParam;

class MultiHashTable
{
private: // Buffer structure: MhtHeader + HtBody[] + LinkTableHead + LtBody[]
	MhtHeader *header;
	void *ht;
	LinkTable lt;
	shm_adapter *pShmAdpt;
	char errmsg[1024];
	CalcHashKeyFunc hashfunc;

private:
	// HashNodeSize=MaxKeyLen+sizeof(MhtNode)
	ull64_t EvaluateHashNodeSize(ull64_t ddwMaxKeyLen) { return ddwMaxKeyLen+sizeof(MhtNode); }
	ull64_t EvaluateHashNodeNum(ull64_t ddwMemsize, ull64_t ddwHashNodeSize, ull64_t ddwHashMemRatio)
	{
		ull64_t ddwDataMemSize = ddwMemsize * 100 / ( 100 + ddwHashMemRatio); // mem size for link table part
		ull64_t hashsz = ddwMemsize - ddwDataMemSize; // mem size for hash table part
		return hashsz / ddwHashNodeSize;
	}
	ull64_t NodeNumRatio2MemSizeRatio(ull64_t ddwNodeSize, ull64_t ddwBlockSize, uint64_t ddwIndexRatio)
	{
		int ratio =  ddwIndexRatio * ddwNodeSize / ddwBlockSize;
		return (ratio == 0) ? 1: ratio;
	}
	// calculate each row's node num, return the real total node num
	ull64_t CalcNodeNumForEachRow(ull64_t ddwRowNum, ull64_t ddwMaxNodeNum, ull64_t auRowsNodeNum[], ull64_t auRowsStartNode[]);

	// Search and return a node for pKey, if no such key found, returns NULL, optionally
	// 	ppEmptyNode returns the first empty node encountered
	//  ppOldestNode returns the oldest node in the search path
	//  piEmptyRow returns ppEmptyNode's row index
	MhtNode *Search(const void *pKey, int iKeyLen, uint32_t dwHashKey,
	        MhtNode **ppEmptyNode=NULL, MhtNode **ppOldestNode=NULL, int *piEmptyRow=NULL);

	// A brutal method for cache replacement:
	// iterate through the hash table and find the oldest node in a thousand to erase,
	// repeat this operation until enough space is needed
	int SearchAndEraseOldestOne(MhtNode *pDataNode);

	int Erase(MhtNode* pDataNode);

public:
	MultiHashTable():header(NULL),ht(NULL),lt(),pShmAdpt(NULL),errmsg(),hashfunc(DefaultHashFunc) {}
	~MultiHashTable();

	// Note: before a MultiHashTable can be used, you are responsible to call one of the 3 init methods below
	// and check the returned values, all methods returning an int have the
	// same meaning:
	//     < 0 -- failed, please get the reason by GetErrorMsg()
	//     = 0 -- successful

	// Init from user supplied buffer
	//     pBuffer        -- user supplied buffer
	//     ddwBufferSize  -- size of pBuffer
	//     cMaxKeyLen     -- max key length, use as small value as possible to save index space
	//     wRowNum        -- hash table row num, level of MultiHashTable, should be in range [1-HASH_MAX_ROW]
	//     dwIndexRatio   -- hash node num * 100 / data block num, controlling the ratio between index and data spaces
	//     ddwBlockSize   -- data block size in LinkTable
	//     HashFunc       -- function entry for calculating the hash key
	//     bUseAccessSeq  -- whether use seq for auto-erase instead of access time
	int InitFromBuffer(void *pBuffer, ull64_t ddwBufferSize, // required arguments 
			uint8_t cMaxKeyLen = DEFAULT_MAX_KEY_LENGTH,
			uint16_t wRowNum = DEFAULT_ROW_NUM,
			uint32_t dwExpiryRatio = DEFAULT_EXPIRY_RATIO,
			uint32_t dwIndexRatio = DEFAULT_INDEX_RATIO,
			ull64_t ddwBlockSize = DEFAULT_BLOCK_SIZE,
			CalcHashKeyFunc HashFunc = NULL,
			bool bUseAccessSeq = false);

	// Init from existing shm, parameters are read from shm
	int InitFromShm(ull64_t ddwShmKey, CalcHashKeyFunc HashFunc = NULL);

	// Create a shm and init hash table, paramenters(except ddwShmKey/bUseHugePage) have the same meaning as InitFromBuffer()
	int CreateFromShm(const MhtInitParam &param);

	bool HasKey(const void *pKey, int iKeyLen);
	int GetData(const void *pKey, int iKeyLen, char *sDataBuf, int &iDataLen, int *DataVer = NULL, bool bUpdateAccessTime = true, time_t *pdwLastAccessTime = NULL);
	int SetData(const void *pKey, int iKeyLen, const char *sDataBuf, int iDataLen, int DataVer = -1, bool bRemoveOldIfNoSpace = false);
	int EraseData(const void *pKey, int iKeyLen);

	// get data without effecting access time
	int PeekData(const void *pKey, int iKeyLen, char *sDataBuf, int &iDataLen) { return GetData(pKey, iKeyLen, sDataBuf, iDataLen, NULL, false); }
	// set data with bRemoveOldIfNoSpace=true
	int ForceSetData(const void *pKey, int iKeyLen, char *sDataBuf, int iDataLen) { return SetData(pKey, iKeyLen, sDataBuf, iDataLen, -1, true); }

	// Iteration interfaces
	MhtIterator Begin() { return Next(End()); }
	MhtIterator End() { return (MhtIterator)-1; }
	MhtIterator Next(MhtIterator it);
	int Get(MhtIterator it, MhtData &data, bool withdata = true);

	int GetMaxKeyLen();
	int GetDataBlockSize();
	int GetHashRowNum();
	int PrintInfo(const string &prefix = "");
	
	int EraseBadNode(const void *pKey, int iKeyLen);
	const char *GetErrorMsg() { return errmsg; }
	MhtNode *GetNode(int32_t pos) { return (pos>=0)?(MhtNode*)((char*)ht+pos):NULL;}
	MhtNode *GetFirstNode() { return GetNode(header->FirstPos); }
};

#endif
