#pragma once
class CRingBuffer
{
private:
	char* buffer;
	int bufferSize;
	int useSize;
	int front;
	int rear;
public:
	CRingBuffer();
	CRingBuffer(int iBufferSize);
	~CRingBuffer();
	int GetBufferSize();
	int GetUseSize();
	int GetFreeSize();
	int Enqueue(const char* chpData, int iSize);
	int Dequeue(char* chpData, int iSize);
	int Peek(char* chpDest, int iSize);
	void ClearBuffer(void);
	int DirectEnqueueSize();
	int DirectDequeueSize();
	int MoveRear(int iSize);
	int MoveFront(int iSize);
};