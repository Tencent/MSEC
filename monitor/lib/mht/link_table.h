
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

 FILENAME:	LinkTable.h

  DESCRIPTION:	链表方式内存结构接口声明

  这里仅提取出数据存储动态数组部分，内存管理和索引组织由上层进行

 ******************************************************************************/

#ifndef _LINK_TABLE_H_
#define _LINK_TABLE_H_

#include <stdint.h>
#include <string>

using namespace std;

// 一个key最大允许的块数，仅用于错误检测
#define MAX_BLOCK_COUNT 10000

// 预回收池最多占用256K
#define LT_MAX_RECYCLE_POOL_SIZE   (256UL*1024/8)
#define LT_DEF_RECYCLE_POOL_SIZE   100
// 预留给数据结构扩展的空间
#define LT_MAX_RESERVE_LEN      (256UL*1024)
#define LT_MAX_USRDATA_LEN      ((512UL*1024)-4)

// 空间不足的时候是否需要回收预回收池中的元素再重试
//#define EMPTY_RECYCLE_ON_OUT_OF_SPACE

typedef unsigned long long ull64_t;

typedef struct
{
    ull64_t ddwNext : 48;
    ull64_t ddwLengthInBlock : 16; // data length in the block, the block is empty if this is 0
    uint8_t bufData[0];
}LtBlock;


typedef struct
{
    ull64_t ddwAllBlockCount;
    ull64_t ddwFreeBlockCount;
    ull64_t ddwBlockSize;
    ull64_t ddwLastEmpty; // 顺序分配时最后分配的为空的索引
    ull64_t ddwFirstFreePos; // All free blocks are linked together by ddwNext
    ull64_t ddwRecycleIndex; // Current index to addwRecyclePool
    uint32_t dwRecyclePoolSize; // 预回收池的大小，可配置，不大于LT_MAX_RECYCLE_POOL_SIZE
    ull64_t addwRecyclePool[LT_MAX_RECYCLE_POOL_SIZE]; // All blocks to be deleted are put here for delayed deletion
    uint8_t  sReserved2[LT_MAX_RESERVE_LEN];  // Reserved for future extension
    uint32_t dwUserDataLen;
    uint8_t  sUserData[LT_MAX_USRDATA_LEN];  // Reserved for application use
}LinkTableHead;


class LinkTable
{
private:
	volatile LinkTableHead *header;
	void *blocks;

	char errmsg[1024];

public:
	LinkTable():header(NULL), blocks(NULL) { errmsg[0] = 0; }
	~LinkTable() {}

	ull64_t EvalBufferSize(ull64_t uBlockCount, ull64_t uBlockSize) { return sizeof(LinkTableHead) + (uBlockCount * uBlockSize); }
	ull64_t EvalBlockCount(ull64_t uBuffSize, ull64_t uBlockSize) { return (uBuffSize - sizeof(LinkTableHead)) / uBlockSize; }

	//初始化链式表，用户必须自己分配LinkTableHead和Block所需的buffer
	//可以调用EvalBufferSize根据块大小和块数计算链表需要的buffer大小
	int Init(void *pBuffer, ull64_t ddwBufferSize, ull64_t ddwBlockCount, ull64_t ddwBlockSize);
	int InitExisting(void *pBuffer, ull64_t ddwBufferSize, ull64_t ddwBlockCount, ull64_t ddwBlockSize);

	//返回ddwPos指向的用户数据
	int GetData(ull64_t ddwPos, void *sDataBuf, int &iDataLen);

	//设置数据，如果ddwPos执行的数据有效，删除已有数据
	//执行成功时，ddwPos 返回新插入的数据位置
	//  返回 -100 表示空间不足
	int SetData(ull64_t &ddwPos, const void *sDataBuf, int iDataLen);

	//清除某个由ddwPos开始的数据
	int EraseData(ull64_t ddwPos);

	// 允许用户改变预回收池的大小，sz必须在1到LT_MAX_RECYCLE_POOL_SIZE之间
	int SetRecyclePoolSize(int sz);

	// 访问头部预留的用户空间
	int GetHeaderData(void *pBuff, int iSize);
	int SetHeaderData(const void *pData, int iLen);

	//调试
	const char *GetErrorMsg() { return errmsg; }
	int PrintInfo(const string &prefix = "");

	// 返回0~100，表示当前使用百分比
	int GetUsage() { return header? (100 - ((header->ddwFreeBlockCount * 100) / header->ddwAllBlockCount)) : 0; }

private:
	int FreeBlock(ull64_t ddwStartPos); // 回收链表并放到free链表中
	int RecycleBlock(ull64_t ddwStartPos); // 放入回收池
	void EmptyRecyclePool(); // 清空回收池，回收池里面的所有元素
	int GetEmptyBlock(int iCount, ull64_t &ddwStartPos); // 获取iCount个空节点，并返回起始位置
};

#endif
