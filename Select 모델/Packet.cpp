#include "Packet.h"
#include <Windows.h>
CPacket::CPacket():CPacket(eBUFFER_DEFAULT){}

CPacket::CPacket(int iBufferSize)
{
	this->m_iBufferSize = iBufferSize;
	this->m_chpBuffer = new char[this->m_iBufferSize];
	Clear();
}

CPacket::~CPacket()
{
	delete[] this->m_chpBuffer;
}

void CPacket::Clear(void)
{
	this->m_iDataSize = 0;
	this->m_iReadPos = 0;
	this->m_iWritePos = 0;
	ZeroMemory(this->m_chpBuffer, this->m_iBufferSize);
}

int CPacket::PutData(char* chpSrc, int iSrcSize)
{
	if (m_iWritePos + iSrcSize > m_iBufferSize)
	{
		//printf("넣을수 없는 크기의 데이터입니다.\n");
		return 0;
	}

	memcpy(m_chpBuffer + m_iWritePos, chpSrc, iSrcSize);
	this->m_iWritePos += iSrcSize;
	this->m_iDataSize += iSrcSize;
	return iSrcSize;
}

int CPacket::GetData(char* chpDest, int iSize)
{
	if (this->m_iReadPos + iSize > this->m_iWritePos)
	{
		// 불가
		return 0;
	}

	memcpy(chpDest, this->m_iReadPos + this->m_chpBuffer, iSize);
	m_iReadPos += iSize;
	m_iDataSize -= iSize;
	return iSize;
}

int CPacket::MoveWritePos(int iSize)
{
	if (iSize < 0)
		return -1;
	if (m_iWritePos + iSize > m_iBufferSize)
		return -2;

	m_iWritePos += iSize;
	m_iDataSize += iSize;
	return iSize;
}

int CPacket::MoveReadPos(int iSize)
{
	if (iSize < 0)
		return -1;
	if (m_iReadPos + iSize > m_iBufferSize)
		return -2;

	m_iReadPos += iSize;
	m_iDataSize -= iSize;
	return iSize;
}

CPacket& CPacket::operator=(CPacket& clSrcPacket)
{
	if (this == &clSrcPacket)
		return *this; // 자기 자신이면 그대로 반환

	// 기존 버퍼 해제
	delete[] m_chpBuffer;

	// 새 버퍼 크기 맞춰서 할당
	m_iBufferSize = clSrcPacket.m_iBufferSize;
	m_chpBuffer = new char[m_iBufferSize];

	// 데이터 복사
	memcpy(m_chpBuffer, clSrcPacket.m_chpBuffer, m_iBufferSize);

	// 상태 값 복사
	m_iDataSize = clSrcPacket.m_iDataSize;
	m_iReadPos = clSrcPacket.m_iReadPos;
	m_iWritePos = clSrcPacket.m_iWritePos;

	return *this;
}

CPacket& CPacket::operator<<(unsigned char byValue)
{
	PutData((char*)&byValue, sizeof(byValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator<<(char chValue)
{
	PutData((char*)&chValue, sizeof(chValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}
CPacket& CPacket::operator<<(char* chValue)
{
	char len = strlen(chValue);
	PutData(&len, 1);
	PutData(chValue, len);
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator<<(short shValue)
{
	PutData((char*)&shValue, sizeof(shValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator<<(unsigned short wValue)
{
	PutData((char*)&wValue, sizeof(wValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator<<(int iValue)
{
	PutData((char*)&iValue, sizeof(iValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator<<(long lValue)
{
	PutData((char*)&lValue, sizeof(lValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator<<(float fValue)
{
	PutData((char*)&fValue, sizeof(fValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator<<(__int64 iValue)
{
	PutData((char*)&iValue, sizeof(iValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator<<(double dValue)
{
	PutData((char*)&dValue, sizeof(dValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator>>(unsigned char& byValue)
{
	GetData((char*)&byValue, sizeof(byValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator>>(char& chValue)
{
	GetData((char*)&chValue, sizeof(chValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator>>(char* chValue)
{
	int len;
	GetData((char*)&len, sizeof(len));
	GetData(chValue, len);
	chValue[len] = '\0'; // null terminator 추가
	return *this;
}


CPacket& CPacket::operator>>(short& shValue)
{
	GetData((char*)&shValue, sizeof(shValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator>>(unsigned short& wValue)
{
	GetData((char*)&wValue, sizeof(wValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator>>(int& iValue)
{
	GetData((char*)&iValue, sizeof(iValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator>>(unsigned long& dwValue)
{
	GetData((char*)&dwValue, sizeof(dwValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator>>(float& fValue)
{
	GetData((char*)&fValue, sizeof(fValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator>>(__int64& iValue)
{
	GetData((char*)&iValue, sizeof(iValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator>>(double& dValue)
{
	GetData((char*)&dValue, sizeof(dValue));
	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}


