
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

 FILENAME:	link_table.cc

  DESCRIPTION:	链表方式内存结构实现

  这里仅提取出数据存储动态数组部分，内存管理和索引组织由上层进行

 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "link_table.h"

//初始化链式表，用户必须自己分配LinkTableHead和Block所需的buffer
//可以调用EvalSize计算Block需要的buffer大小
int LinkTable::Init(void *pBuffer, ull64_t ddwBufferSize, ull64_t ddwBlockCount, ull64_t ddwBlockSize)
{
	if(pBuffer==NULL || ddwBufferSize<EvalBufferSize(ddwBlockCount, ddwBlockSize))
	{
		snprintf(errmsg, sizeof(errmsg), "Bad argument: %s",
			pBuffer==NULL? "buffer is null": "not enough buffer size");
		return -1;
	}

	header = (volatile LinkTableHead *)pBuffer;
	blocks = ((char*)pBuffer)+sizeof(LinkTableHead);

	memset((void*)header, 0, sizeof(*header));
	header->ddwAllBlockCount = header->ddwFreeBlockCount = (ddwBlockCount - 1);
	header->ddwBlockSize = ddwBlockSize;
	header->ddwLastEmpty = 1; // the first block (pos==0) is not used
	header->dwRecyclePoolSize = LT_DEF_RECYCLE_POOL_SIZE;

	return 0;
}

int LinkTable::InitExisting(void *pBuffer, ull64_t ddwBufferSize, ull64_t ddwBlockCount, ull64_t ddwBlockSize)
{
	if(pBuffer==NULL || ddwBufferSize<EvalBufferSize(ddwBlockCount, ddwBlockSize))
	{
		snprintf(errmsg, sizeof(errmsg), "Bad argument: %s",
			pBuffer==NULL? "buffer is null": "not enough buffer size");
		return -1;
	}

	header = (volatile LinkTableHead *)pBuffer;
	blocks = ((char*)pBuffer)+sizeof(LinkTableHead);

	if(header->ddwAllBlockCount != (ddwBlockCount - 1))
	{
		snprintf(errmsg, sizeof(errmsg), "Total block count mismatched: existing %llu, expected %llu", header->ddwAllBlockCount, (ddwBlockCount - 1));
		return -2;
	}

	if(header->ddwBlockSize != ddwBlockSize)
	{
		snprintf(errmsg, sizeof(errmsg), "Block size mismatched: existing %llu, expected %llu", header->ddwBlockSize, ddwBlockSize);
		return -3;
	}

	return 0;
}


#define _GET_ELE_AT(elist, idx, sz) ((LtBlock *)(((char *)(elist)) + ((idx) * (sz))))

// 返回索引所指向的Block
#define LT_ELE_AT(idx) _GET_ELE_AT(blocks, idx, header->ddwBlockSize)

// 释放链表的元素并放到free链表中
int LinkTable::FreeBlock(ull64_t ddwStartPos)
{
	ull64_t ddwCurPos=0, ddwNextPos=0;
	int iCount=0;
	LtBlock *pBlock;

	if(header==NULL || blocks==NULL)
	{
		strncpy(errmsg, "LinkTable not initialized", sizeof(errmsg));
		return -1;
	}

	if((ddwStartPos==0) || (ddwStartPos>=header->ddwAllBlockCount))
	{
		snprintf(errmsg, sizeof(errmsg), "Position %llu gone insane", ddwStartPos);
		return -2;
	}

	ddwNextPos=ddwStartPos;
	while((ddwNextPos) &&(iCount<=MAX_BLOCK_COUNT))
	{
		ddwCurPos=ddwNextPos;
		if(ddwCurPos>=header->ddwAllBlockCount)
		{
			snprintf(errmsg, sizeof(errmsg), "Position %llu gone insane", ddwStartPos);
			return -3;
		}

		pBlock=LT_ELE_AT(ddwCurPos);
		ddwNextPos=pBlock->ddwNext;
		memset(pBlock, 0, header->ddwBlockSize);

		// 把节点放入Free列表中
		pBlock->ddwNext=header->ddwFirstFreePos;
		header->ddwFirstFreePos=ddwCurPos;

		header->ddwFreeBlockCount++;
		iCount++;
	}

	if(ddwNextPos!=0)
	{
		snprintf(errmsg, sizeof(errmsg), "Too many blocks at %llu", ddwStartPos);
		return -4;
	}

	return 0;
}


// 预回收池的主要算法实现：
// 用一个游标dwRecycleIndex循环遍历预回收池指针数组addwRecyclePool，没次预回收遍历一个指针，
// 将本次希望回收的起始地址挂到addwRecyclePool[ddwRecycleIndex]上，并回收上次挂到这的地方的元素链表
int LinkTable::RecycleBlock(ull64_t ddwStartPos)
{
	int iRet;
	ull64_t ddwOldPos, ddwIdx;

	if(header==NULL || blocks==NULL)
	{
		strncpy(errmsg, "LinkTable not initialized", sizeof(errmsg));
		return -1;
	}

	ddwIdx = header->ddwRecycleIndex;
	if(ddwIdx>=header->dwRecyclePoolSize)
	{
		ddwIdx = 0;
		header->ddwRecycleIndex = 0;
	}

	// 先放到预回收池中，再回收同个位置中的旧数据
	ddwOldPos = header->addwRecyclePool[ddwIdx];
	header->addwRecyclePool[ddwIdx] = ddwStartPos;
	header->ddwRecycleIndex = (ddwIdx+1)%header->dwRecyclePoolSize;

	if(ddwOldPos)
	{
		iRet = FreeBlock(ddwOldPos);
		if(iRet<0)
		{
			// 如果回收失败，将导致单元不能被重新利用，这里不处理
		}
	}

	return 0;
}

// 回收全部预回收池中的元素
void LinkTable::EmptyRecyclePool()
{
	int i;

	if(header==NULL || blocks==NULL)
		return;

	for(i=0; i<(int)header->dwRecyclePoolSize; i++)
	{
		if(header->addwRecyclePool[i])
		{
			int iRet;

			iRet = FreeBlock(header->addwRecyclePool[i]);
			if(iRet<0)
			{
				// 如果回收失败，将导致单元不能被重新利用，这里不处理
			}

			header->addwRecyclePool[i] = 0;
		}
	}

	header->ddwRecycleIndex = 0;
}

// ---------- 下面的 ATTR_API 属性需要重新申请

//使用数组方式管理自由空间，分配和使用自由空间采用步进方式
// 返回 -100 表示空间不足
int LinkTable::GetEmptyBlock(int iCount, ull64_t &ddwOutPos)
{
	ull64_t  ddwStartPos,ddwCurPos;
	int i,j;
#ifdef EMPTY_RECYCLE_ON_OUT_OF_SPACE
	int iTrySecond=0;
#endif
	//float fUsedRate;
	LtBlock *pBlock;

	if(header==NULL || blocks==NULL)
	{
		strncpy(errmsg, "LinkTable not initialized", sizeof(errmsg));
		return -1;
	}

	// 只要空闲块数小于预设比例就告警
	//fUsedRate = (header->ddwFreeBlockCount*100.0/header->ddwAllBlockCount);

#ifdef EMPTY_RECYCLE_ON_OUT_OF_SPACE
restart:
	if(iTrySecond) // 清空预回收池后重新分配
#endif
	if((iCount<0) ||
		(iCount > MAX_BLOCK_COUNT) ||
		(iCount > (int)header->ddwFreeBlockCount))
	{
		// Not enough space
#ifdef EMPTY_RECYCLE_ON_OUT_OF_SPACE
		if(iTrySecond==0)
		{
			iTrySecond = 1;
			EmptyRecyclePool();
			goto restart;
		}
#endif
		strncpy(errmsg, "Not enough space", sizeof(errmsg));
		return -100;
	}

	// 先从Free列表搜索，搜不到再挨个搜索
	ddwStartPos=0;
	ddwCurPos=header->ddwFirstFreePos;
	i=0;
	while(i<iCount && ddwCurPos>0 &&
		  ddwCurPos<header->ddwAllBlockCount &&
		  (pBlock=LT_ELE_AT(ddwCurPos))->ddwLengthInBlock==0)
	{
		ull64_t ddwNext;
		i++;
		ddwNext = pBlock->ddwNext;
		if(i==iCount) // 把最后一个的Next置零
			pBlock->ddwNext = 0;
		ddwCurPos=ddwNext;
	}

	if(i==iCount) // 找到了
	{
		ddwOutPos = header->ddwFirstFreePos;
		header->ddwFirstFreePos = ddwCurPos;
		return 0;
	}

	// FIXME：这里存在一个不是很重要的问题：就是当从Free链表中找到一些节点，但无法满足请求的需要，
	// 这时需要通过遍历搜索来分配，而遍历搜索可能会找到free链表中的节点，这将导致free链表中的节点被
	// 遍历分配了，使得Free链表断链，断链后的链表就只能由遍历分配器来分配了

	// 从header->ddwLastEmpty开始进行挨个搜索
	ddwStartPos=0;
	ddwCurPos=header->ddwLastEmpty;
	i=0;
	for(j=0; (i<iCount) && (j<(int)header->ddwAllBlockCount);j++)
	{
		if((pBlock=LT_ELE_AT(ddwCurPos))->ddwLengthInBlock==0)
		{
			pBlock->ddwNext=ddwStartPos;
			ddwStartPos=ddwCurPos;
			i++;
		}
		ddwCurPos++;

		// Wrap around
		if(ddwCurPos>=header->ddwAllBlockCount)
		{
			ddwCurPos=1;
		}
	}

	if(i<iCount)
	{
		// Not enough space
#ifdef EMPTY_RECYCLE_ON_OUT_OF_SPACE
		if(iTrySecond==0)
		{
			iTrySecond = 1;
			EmptyRecyclePool();
			goto restart;
		}
#endif
		strncpy(errmsg, "Not enough space", sizeof(errmsg));
		return -100;
	}

	ddwOutPos=ddwStartPos;
	header->ddwLastEmpty=ddwCurPos;
	return 0;
}

#define BLOCK_DATA_LEN() (header->ddwBlockSize - (unsigned long)(((LtBlock*)0)->bufData))

//返回ddwPos指向的用户数据
int LinkTable::GetData(ull64_t ddwPos, void *sDataBuf, int &iOutDataLen)
{
	ull64_t  ddwCurPos=0,ddwNextPos=0;
	int iDataLen=0,iBufLen=iOutDataLen;
	LtBlock *pBlock;

	if(sDataBuf==NULL ||
		iBufLen < (int)BLOCK_DATA_LEN()) // buffer大小不足一个块
	{
		strncpy(errmsg, "Bad argument", sizeof(errmsg));
		return -1;
	}

	if(header==NULL || blocks==NULL)
	{
		strncpy(errmsg, "LinkTable not initialized", sizeof(errmsg));
		return -1;
	}

	iDataLen=0;

	ddwNextPos=ddwPos;
	while(ddwNextPos)
	{
		ddwCurPos=ddwNextPos;
		if(ddwCurPos>=header->ddwAllBlockCount)
		{
			strncpy(errmsg, "Position gone insane", sizeof(errmsg));
			return -21;
		}
		if((iDataLen+(int)BLOCK_DATA_LEN()) > iBufLen)
		{
			strncpy(errmsg, "Not enough space for data", sizeof(errmsg));
			return -4;
		}
		pBlock = LT_ELE_AT(ddwCurPos);
		uint16_t wBlockLen;
		if(pBlock->ddwNext) // not the last block
		{
			wBlockLen = BLOCK_DATA_LEN();
		}
		else // the last block
		{
			wBlockLen = pBlock->ddwLengthInBlock;
			if(wBlockLen>BLOCK_DATA_LEN())
				wBlockLen = BLOCK_DATA_LEN();
		}
		memcpy(((char*)sDataBuf)+iDataLen,pBlock->bufData, wBlockLen);
		iDataLen+=wBlockLen;
		ddwNextPos=pBlock->ddwNext;
	}
	iOutDataLen=iDataLen;
	return 0;
}


//设置数据，如果ddwPos指向的数据有效，删除已有数据
//执行成功时，ddwPos 返回新插入的数据位置
// 返回 -100 表示空间不足
int LinkTable::SetData(ull64_t &ddwPos, const void *sDataBuf, int iDataLen)
{
	ull64_t ddwCurPos=0,ddwNextPos=0,ddwStartPos=0,ddwOldPos=ddwPos;
	int iCount=0,iLeftLen=0,iCopyLen=0;
	int iRet=0;
	LtBlock *pBlock;

	if(sDataBuf==NULL || iDataLen<0)
	{
 		strncpy(errmsg, "Bad argument", sizeof(errmsg));
		return -1;
	}

	if(header==NULL || blocks==NULL)
	{
		strncpy(errmsg, "LinkTable not initialized", sizeof(errmsg));
		return -1;
	}

	iCount=(iDataLen+BLOCK_DATA_LEN()-1)/BLOCK_DATA_LEN();
	if(iCount>MAX_BLOCK_COUNT)
	{
 		snprintf(errmsg, sizeof(errmsg), "Data too large to fit in max of %d blocks", MAX_BLOCK_COUNT);
		return -2;
	}

	//先构造新数据
	iRet=GetEmptyBlock(iCount, ddwStartPos);
	if(iRet<0)
	{
		if(iRet==-100)
			return iRet;
		return -7;
	}

	ddwNextPos=ddwStartPos;
	iLeftLen=iDataLen-iCopyLen;
	while((ddwNextPos) && (iLeftLen>0))
	{
		ddwCurPos=ddwNextPos;
		if(ddwCurPos>=header->ddwAllBlockCount)
		{
		 	strncpy(errmsg, "Next position is bad", sizeof(errmsg));
			return -8;
		}

		pBlock = LT_ELE_AT(ddwCurPos);
		if(iLeftLen > (int)BLOCK_DATA_LEN())
		{
			memcpy(pBlock->bufData,
				((char*)sDataBuf)+iCopyLen,BLOCK_DATA_LEN());
			iCopyLen+=BLOCK_DATA_LEN();
			pBlock->ddwLengthInBlock = BLOCK_DATA_LEN();
		}
		else
		{
			memcpy(pBlock->bufData,
					((char*)sDataBuf)+iCopyLen,(unsigned)iLeftLen);
			iCopyLen+=iLeftLen;
			pBlock->ddwLengthInBlock = iLeftLen;
		}

		ddwNextPos=pBlock->ddwNext;
		header->ddwFreeBlockCount--;
		iLeftLen=iDataLen-iCopyLen;
	}

	if(iLeftLen!=0)
	{
		//bug
		snprintf(errmsg, sizeof(errmsg),
			"Allocated blocks(%d) not enough for data(len=%d, bs=%llu)",
			iCount, iDataLen, header->ddwBlockSize);
		return -9;
	}

	LT_ELE_AT(ddwCurPos)->ddwNext=0;
	LT_ELE_AT(0)->ddwNext=ddwNextPos;
	ddwPos=ddwStartPos;

	//删除旧数据
	if(ddwOldPos!=0)
	{
		iRet=RecycleBlock(ddwOldPos);
		if(iRet<0)
		{
			// No way of turning back
		}
	}

	return 0;
}

//清除某个Key对应的数据
int LinkTable::EraseData(ull64_t ddwPos)
{
	if(header==NULL || blocks==NULL)
	{
		strncpy(errmsg, "LinkTable not initialized", sizeof(errmsg));
		return -1;
	}

	return RecycleBlock(ddwPos);
}

// 允许用户改变预回收池的大小，sz必须在1到LT_MAX_PREFREE_POOL_SIZE之间
int LinkTable::SetRecyclePoolSize(int sz)
{
	if(sz<1 || (unsigned int)sz>LT_MAX_RECYCLE_POOL_SIZE)
	{
		strncpy(errmsg, "Invalid argument", sizeof(errmsg));
		return -1;
	}
	if(header==NULL || blocks==NULL)
	{
		strncpy(errmsg, "LinkTable not initialized", sizeof(errmsg));
		return -2;
	}

	EmptyRecyclePool();
	header->dwRecyclePoolSize = sz;

	return 0;
}

// 访问头部预留的用户空间
int LinkTable::GetHeaderData(void *pBuff, int iSize)
{
	if(iSize<=0 || pBuff==NULL)
	{
		strncpy(errmsg, "Invalid argument", sizeof(errmsg));
		return -1;
	}
	if(header==NULL || blocks==NULL)
	{
		strncpy(errmsg, "LinkTable not initialized", sizeof(errmsg));
		return -2;
	}

    if(iSize>(int)header->dwUserDataLen)
        iSize = (int)header->dwUserDataLen;

    memcpy(pBuff, (void*)header->sUserData, iSize);
    return iSize;
}

int LinkTable::SetHeaderData(const void *pData, int iLen)
{
	if(iLen<0 || (iLen&&pData==NULL))
	{
		strncpy(errmsg, "Invalid argument", sizeof(errmsg));
		return -1;
	}
	if(header==NULL || blocks==NULL)
	{
		strncpy(errmsg, "LinkTable not initialized", sizeof(errmsg));
		return -2;
	}
    if(iLen>(int)sizeof(header->sUserData))
        iLen = (int)sizeof(header->sUserData);

    if(iLen>0)
        memcpy((void*)header->sUserData, pData, iLen);

    header->dwUserDataLen = iLen;
    return iLen;
}

//调试
int LinkTable::PrintInfo(const string &prefix)
{
    long i=0;
    unsigned long total=0;

    if(header==NULL || blocks==NULL)
    {
        cout << prefix << "Linktable not initialized" << endl;
        return -1;
    }

    cout << prefix << "ddwAllBlockCount:  " << header->ddwAllBlockCount << endl;
    cout << prefix << "ddwFreeBlockCount: " << header->ddwFreeBlockCount << endl;
    cout << prefix << "ddwBlockSize:      " << header->ddwBlockSize << endl;
    cout << prefix << "ddwFirstFreePos:     " << header->ddwFirstFreePos << endl;
    cout << prefix << "ddwRecycleIndex:     " << header->ddwRecycleIndex << endl;

    for(total=0,i=0; i<header->dwRecyclePoolSize; i++)
    {
        if(header->addwRecyclePool[i])
            total ++;
    }
    cout << prefix << "dwRecycledCount:     " << total << endl;
    cout << prefix << "dwTableUsage:        " << GetUsage() << endl;

    return 0;
}


