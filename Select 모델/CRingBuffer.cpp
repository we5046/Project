#include <iostream>
#include "CRingBuffer.h"
#include <Windows.h>

CRingBuffer::CRingBuffer() : CRingBuffer(8192) {}

CRingBuffer::CRingBuffer(int iBufferSize)
{
	this->bufferSize = iBufferSize;
	this->useSize = 0;
	this->front = 0;
	this->rear = 0;
	buffer = new char[this->bufferSize];
}

int CRingBuffer::GetBufferSize()
{
	return this->bufferSize;
}

CRingBuffer::~CRingBuffer()
{
	delete[] buffer;
}

int CRingBuffer::GetUseSize()
{
	return this->useSize;
}

int CRingBuffer::GetFreeSize()
{
	return this->bufferSize - this->useSize;
}

int CRingBuffer::Enqueue(const char* chpData, int iSize)
{
	int insertSize = min(iSize, GetFreeSize());
	int spaceToEnd = this->bufferSize - this->rear;
	if (insertSize <= spaceToEnd)
	{
		memcpy(&buffer[this->rear], chpData, insertSize);
	}
	else
	{
		memcpy(&buffer[this->rear], chpData, spaceToEnd);
		memcpy(buffer, chpData + spaceToEnd, insertSize - spaceToEnd);
	}
	this->rear = (this->rear + insertSize) % this->bufferSize;
	this->useSize += insertSize;
	return insertSize;
}

int CRingBuffer::Dequeue(char* chpData, int iSize)
{
	int removeSize = min(iSize, GetUseSize());
	int spaceToEnd = this->bufferSize - this->front;
	if (removeSize <= spaceToEnd)
	{
		memcpy(chpData, &buffer[this->front], removeSize);
	}
	else
	{
		memcpy(chpData, &buffer[this->front], spaceToEnd);
		memcpy(chpData + spaceToEnd, buffer, removeSize - spaceToEnd);
	}
	this->front = (this->front + removeSize) % this->bufferSize;
	this->useSize -= removeSize;
	return removeSize;
}

int CRingBuffer::Peek(char* chpDest, int iSize)
{
	int removeSize = min(iSize, GetUseSize());
	int spaceToEnd = this->bufferSize - this->front;
	if (removeSize <= spaceToEnd)
	{
		memcpy(chpDest, &buffer[this->front], removeSize);
	}
	else
	{
		memcpy(chpDest, &buffer[this->front], spaceToEnd);
		memcpy(chpDest + spaceToEnd, buffer, removeSize - spaceToEnd);
	}
	return removeSize;
}

void CRingBuffer::ClearBuffer(void)
{
	this->front = this->rear = this->useSize = 0;
}

int CRingBuffer::DirectEnqueueSize()
{
	if (this->front <= this->rear)
	{
		return this->bufferSize - this->rear;
	}
	else
		return this->front - this->rear;
}

int CRingBuffer::DirectDequeueSize()
{
	if (this->front <= this->rear)
		return this->rear - this->front;	//사실상 useSize와 같음
	else
		return this->bufferSize - this->front;
}

int CRingBuffer::MoveRear(int iSize)
{
	int moveSize = min(iSize, GetFreeSize());
	this->rear = (this->rear + moveSize) % this->bufferSize;
	this->useSize += moveSize;
	return moveSize;
}

int CRingBuffer::MoveFront(int iSize)
{
	int moveSize = min(iSize, GetUseSize());
	this->front = (this->front + moveSize) % this->bufferSize;
	this->useSize -= moveSize;
	return moveSize;
}

char* CRingBuffer::GetFrontBufferPtr(void)
{
	return buffer + front;
}

char* CRingBuffer::GetRearBufferPtr(void)
{
	return buffer + rear;
}
