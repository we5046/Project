#pragma once
#include <cstdint>


/*---------------------------------------------------------------

패킷데이터 정의.


자신의 캐릭터에 대한 패킷을 서버에게 보낼 때, 모두 자신이 먼저
액션을 취함과 동시에 패킷을 서버로 보내주도록 한다.

- 이동 키 입력 시 이동동작을 취함과 동시에 이동 패킷을 보내도록 한다.
- 공격키 입력 시 공격 동작을 취하면서 패킷을 보낸다.
- 충돌 처리 및 데미지에 대한 정보는 서버에서 처리 후 통보하게 된다.


---------------------------------------------------------------*/

#ifndef __PACKET_DEFINE__
#define __PACKET_DEFINE__


//---------------------------------------------------------------
// 패킷헤더.
//
//---------------------------------------------------------------
#pragma pack(push, 1)
struct st_PACKET_HEADER
{
    unsigned char byCode;   // 0x89 고정
    unsigned char bySize;   // 패킷 크기
    unsigned char byType;   // 패킷 타입
};


#define	dfPACKET_SC_CREATE_MY_CHARACTER			0
struct st_SC_CREATE_MY_CHARACTER
{
    st_PACKET_HEADER header;
    unsigned int ID;       // 4 bytes
    unsigned char byDirection; // 1 byte (LL/RR)
    short shX;        // 2 bytes
    short shY;        // 2 bytes
    unsigned char HP;        // 1 byte
};


#define	dfPACKET_SC_CREATE_OTHER_CHARACTER		1
struct st_SC_CREATE_OTHER_CHARACTER
{
    st_PACKET_HEADER header;
    unsigned int ID;       // 4 bytes
    unsigned char byDirection; // 1 byte (LL/RR)
    short shX;        // 2 bytes
    short shY;        // 2 bytes
    unsigned char HP;        // 1 byte
};


#define	dfPACKET_SC_DELETE_CHARACTER			2
struct st_SC_DELETE_CHARACTER
{
    st_PACKET_HEADER header;
    unsigned int ID; // 4 bytes
};



#define	dfPACKET_CS_MOVE_START					10
struct st_CS_MOVE_START
{
    st_PACKET_HEADER header;
    unsigned char byDirection; // 1 byte (8방향)
    short shX;        // 2 bytes
    short shY;        // 2 bytes
};

#define dfPACKET_MOVE_DIR_LL					0
#define dfPACKET_MOVE_DIR_LU					1
#define dfPACKET_MOVE_DIR_UU					2
#define dfPACKET_MOVE_DIR_RU					3
#define dfPACKET_MOVE_DIR_RR					4
#define dfPACKET_MOVE_DIR_RD					5
#define dfPACKET_MOVE_DIR_DD					6
#define dfPACKET_MOVE_DIR_LD					7




#define	dfPACKET_SC_MOVE_START					11
struct st_SC_MOVE_START
{
    st_PACKET_HEADER header;
    unsigned int ID;       // 4 bytes
    unsigned char byDirection; // 1 byte
    short shX;        // 2 bytes
    short shY;        // 2 bytes
};




#define	dfPACKET_CS_MOVE_STOP					12
struct st_CS_MOVE_STOP
{
    st_PACKET_HEADER header;
    unsigned char byDirection; // 1 byte (좌/우)
    short shX;        // 2 bytes
    short shY;        // 2 bytes
};


#define	dfPACKET_SC_MOVE_STOP					13
struct st_SC_MOVE_STOP
{
    st_PACKET_HEADER header;
    unsigned int ID;       // 4 bytes
    unsigned char byDirection; // 1 byte (좌/우)
    short shX;        // 2 bytes
    short shY;        // 2 bytes
};



#define	dfPACKET_CS_ATTACK1						20
struct st_CS_ATTACK1
{
    st_PACKET_HEADER header;
    unsigned char byDirection; // 1 byte (좌/우)
    short shX;        // 2 bytes
    short shY;        // 2 bytes
};

#define	dfPACKET_SC_ATTACK1						21
struct st_SC_ATTACK1
{
    st_PACKET_HEADER header;
    unsigned int ID;       // 4 bytes
    unsigned char byDirection; // 1 byte (좌/우)
    short shX;        // 2 bytes
    short shY;        // 2 bytes
};



#define	dfPACKET_CS_ATTACK2						22
struct st_CS_ATTACK2
{
    st_PACKET_HEADER header;
    unsigned char byDirection; // 1 byte (좌/우)
    short shX;        // 2 bytes
    short shY;        // 2 bytes
};

#define	dfPACKET_SC_ATTACK2						23
struct st_SC_ATTACK2
{
    st_PACKET_HEADER header;
    unsigned int ID;       // 4 bytes
    unsigned char byDirection; // 1 byte (좌/우)
    short shX;        // 2 bytes
    short shY;        // 2 bytes
};

#define	dfPACKET_CS_ATTACK3						24
struct st_CS_ATTACK3
{
    st_PACKET_HEADER header;
    unsigned char byDirection; // 1 byte (좌/우)
    short shX;        // 2 bytes
    short shY;        // 2 bytes
};

#define	dfPACKET_SC_ATTACK3						25
struct st_SC_ATTACK3
{
    st_PACKET_HEADER header;
    unsigned int ID;       // 4 bytes
    unsigned char byDirection; // 1 byte (좌/우)
    short shX;        // 2 bytes
    short shY;        // 2 bytes
};





#define	dfPACKET_SC_DAMAGE						30
struct st_SC_DAMAGE
{
    st_PACKET_HEADER header;
    unsigned int AttackID; // 4 bytes (공격자 ID)
    unsigned int DamageID; // 4 bytes (피해자 ID)
    unsigned char DamageHP;  // 1 byte (남은 HP)
};







// 사용안함...
#define	dfPACKET_CS_SYNC						250
struct st_CS_SYNC
{
    st_PACKET_HEADER header;
    short shX;
    short shY;
};


#define	dfPACKET_SC_SYNC						251
struct st_SC_SYNC
{
    st_PACKET_HEADER header;
    unsigned int ID;
    short shX;
    short shY;
};
#pragma pack(pop, 1)


#endif
