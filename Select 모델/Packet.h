#pragma once
class CPacket
{
public:
	enum en_PACKET
	{
		eBUFFER_DEFAULT	= 1400
	};
	//생성자 소멸자
	CPacket();
	CPacket(int iBufferSize);
	virtual ~CPacket();

	// 패킷 청소
	void Clear(void);

	//버퍼 사이즈 얻기
	int GetBufferSize(void) { return m_iBufferSize; }

	//사용중 사이즈
	int GetDataSize(void) { return m_iDataSize; }

	//버퍼 포인터
	char* GetBufferPtr(void) { return m_chpBuffer; }

	//버퍼 Pos 이동
	int MoveWritePos(int iSize);
	int MoveReadPos(int iSize);

	//연산자 오버로딩
	CPacket& operator = (CPacket& clSrcPacket);

	//넣기
	CPacket& operator << (unsigned char byValue);
	CPacket& operator << (char chValue);

	CPacket& operator<<(char* chValue);

	CPacket& operator << (short shValue);
	CPacket& operator << (unsigned short wValue);

	CPacket& operator << (int iValue);
	CPacket& operator << (long lValue);
	CPacket& operator << (float fValue);
	
	CPacket& operator << (__int64 iValue);
	CPacket& operator << (double dValue);

	//빼기
	CPacket& operator >> (unsigned char& byValue);
	CPacket& operator >> (char& chValue);

	CPacket& operator>>(char* chValue);

	CPacket& operator >> (short& shValue);
	CPacket& operator >> (unsigned short& wValue);

	CPacket& operator >> (int& iValue);
	CPacket& operator >> (unsigned long& dwValue);
	CPacket& operator >> (float& fValue);

	CPacket& operator >> (__int64& iValue);
	CPacket& operator >> (double& dValue);

	// 데이타 얻기.
	int		GetData(char* chpDest, int iSize);

	// 데이타 삽입.
	int		PutData(char* chpSrc, int iSrcSize);

protected:
	int m_iBufferSize;  // 전체 버퍼 크기
	int m_iDataSize;    // 현재 저장된 데이터 크기
	char* m_chpBuffer;  // 실제 데이터 저장 공간
	int m_iReadPos;     // 읽기 위치
	int m_iWritePos;    // 쓰기 위치
};