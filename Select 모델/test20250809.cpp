#include <iostream>
#include <winSock2.h>
#include <WS2tcpip.h>	//InetPton을 위해 필요함
#include <windows.h>
#include <list>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "CRingBuffer.h"
#include "PacketDefine.h"
#include "settingScreen.h"
#define SERVERPORT 5000

#pragma pack(push, 1)
struct st_SESSION
{
	SOCKET			Socket = INVALID_SOCKET;
	unsigned int	dwSessionID = 0;
	CRingBuffer*	RecvQ = new CRingBuffer;
	CRingBuffer*	SendQ = new CRingBuffer;

	unsigned int	dwAction = 0;
	char			byDirection = dfPACKET_MOVE_DIR_LL;
	short			shX = dfRANGE_MOVE_RIGHT/2;
	short			shY = dfRANGE_MOVE_BOTTOM/2;
	char			chHP = 100;

	bool			isMoving = false;                         // 이동중 여부
	bool			disconnectFlag = false;
};
#pragma pack(pop, 1)

bool g_bShutdown = false;
SOCKET g_ListenSocket;
std::list<st_SESSION*> g_sessionList;
unsigned int g_dwSessionID = 0;
///Session


#pragma region 함수
void netIOProcess(void);
int Initialize(void);
int netProc_Accept(void);
int netProc_Recv(st_SESSION* session);
void netProc_Send(st_SESSION* session);
void mpMoveStart(st_SC_MOVE_START* pPacket, int SessionID, unsigned char Direction, short X, short Y);
void mpMoveStop(st_SC_MOVE_STOP* pPacket, int SessionID, unsigned char Direction, short X, short Y);
void SendBroadCast(st_SESSION* except, const void* msg, int msglen);
void SendUnicast(st_SESSION* except, const void* msg, int msglen);
bool netPacketProc_MoveStart(st_SESSION* pSession, char* pPacket);
bool netPacketProc_MoveStop(st_SESSION* pSession, char* pPacket);
void Disconnect(st_SESSION* session);
bool PacketProc(st_SESSION* pSession, BYTE byPacketType, char* pPacket);
void Update();
bool netPacketProc_Attack1(st_SESSION* pSession, char* pPacket);
bool netPacketProc_Attack2(st_SESSION* pSession, char* pPacket);
bool netPacketProc_Attack3(st_SESSION* pSession, char* pPacket);
#pragma endregion

LONGLONG  g_LastUpdateTime = 0;
LONGLONG  g_Frequency = 0;

int main()
{
	srand((unsigned int)time(NULL));
	timeBeginPeriod(1);

	QueryPerformanceFrequency((LARGE_INTEGER*)&g_Frequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&g_LastUpdateTime);


	//초기화
	int initial = Initialize();
	if (initial != 0)
	{
		printf("Listen Socket 설정 실패\n");
		return 0;
	}
	while (!g_bShutdown)
	{
		netIOProcess();
		LONGLONG now;
		QueryPerformanceCounter((LARGE_INTEGER*)&now);
		double deltaTime = (double)(now - g_LastUpdateTime) / g_Frequency;

		if (deltaTime >= 1.0 / 25.0) // 25FPS 기준, 40ms 마다
		{
			Update();
			g_LastUpdateTime = now;
		}
		Sleep(1);
	}
	WSACleanup();
	timeEndPeriod(1);
	closesocket(g_ListenSocket);
	return 0;
}

int Initialize()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return -1;
	}
	else
	{
		printf("WSAStartup #\n");
	}
	g_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (g_ListenSocket == INVALID_SOCKET)
	{
		printf("ERROR\n");
		return -1;
	}

	SOCKADDR_IN serverAddr;
	int AddrSize = sizeof(serverAddr);
	ZeroMemory(&serverAddr, AddrSize);
	serverAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "0.0.0.0", &serverAddr.sin_addr);
	serverAddr.sin_port = htons(SERVERPORT);

	int retval_bind = bind(g_ListenSocket, (SOCKADDR*)&serverAddr, AddrSize);
	if (retval_bind != 0)
	{
		return -1;
	}
	else
	{
		printf("Bind OK # Port:%d\n", SERVERPORT);
	}

	int retval_listen = listen(g_ListenSocket, SOMAXCONN);
	if (retval_listen != 0)
	{
		return -1;
	}
	else
	{
		printf("Listen OK #\n");
	}

	u_long mode_on = 1;
	int retval_ioctl = ioctlsocket(g_ListenSocket, FIONBIO, &mode_on);
	if (retval_ioctl != 0)
	{
		return -1;
	}
	return 0;
}

void netIOProcess(void)
{
	st_SESSION* pSession;

	FD_SET Rset;
	FD_SET Wset;
	FD_ZERO(&Rset);
	FD_ZERO(&Wset);

	FD_SET(g_ListenSocket, &Rset);

	for (auto it = g_sessionList.begin(); it != g_sessionList.end(); ++it)
	{
		pSession = *it;
		FD_SET(pSession->Socket, &Rset);
		if (pSession->SendQ->GetUseSize() > 0)
		{
			FD_SET(pSession->Socket, &Wset);
		}
	}

	TIMEVAL timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	int retval_select = select(0, &Rset, &Wset, NULL, &timeout);
	if (retval_select > 0)
	{
		if (FD_ISSET(g_ListenSocket, &Rset))
		{
			--retval_select;
			int retval_accept = netProc_Accept();
			if (retval_accept != 0)
			{
				printf("netProc_Accept Failed..\n");
				return;
			}
		}

		for (auto it = g_sessionList.begin(); it != g_sessionList.end(); ++it)
		{
			pSession = *it;
			if (FD_ISSET(pSession->Socket, &Rset))
			{
				--retval_select;
				int recvResult = netProc_Recv(pSession);
				if (recvResult == -1)  // recv() 에서 종료 신호 받음
				{
					pSession->disconnectFlag = true;
				}
			}

			if (FD_ISSET(pSession->Socket, &Wset))
			{
				--retval_select;
				netProc_Send(pSession);
			}
		}

		std::list<st_SESSION*>::iterator it = g_sessionList.begin();
		for (; it != g_sessionList.end();)
		{
			if ((*it)->disconnectFlag == true)
			{
				st_SESSION* temp = (*it);
				it = g_sessionList.erase(it);
				Disconnect(temp);
			}
			else
			{
				++it;
			}
		}
	}
	else if (retval_select == SOCKET_ERROR)
	{
		printf("SOCKET_ERROR : CONNECT ERROR\n");
	}
}

const char* DirectionToString(unsigned char dir)
{
	switch (dir)
	{
	case dfPACKET_MOVE_DIR_LL: return "LL";
	case dfPACKET_MOVE_DIR_RR: return "RR";
	case dfPACKET_MOVE_DIR_UU: return "UU";
	case dfPACKET_MOVE_DIR_DD: return "DD";
	case dfPACKET_MOVE_DIR_LU: return "LU";
	case dfPACKET_MOVE_DIR_RU: return "RU";
	case dfPACKET_MOVE_DIR_LD: return "LD";
	case dfPACKET_MOVE_DIR_RD: return "RD";
	default: return "??";
	}
}

void Update()
{
	// 프레임 당 이동량
	const short MOVE_X = 6;
	const short MOVE_Y = 4;

	for (auto it : g_sessionList)
	{
		st_SESSION* pSession = it;
		if (pSession->chHP <= 0)
		{
			pSession->disconnectFlag = true;
			continue;
		}

		if (!pSession->isMoving)
			continue;

		short prevX = pSession->shX;
		short prevY = pSession->shY;

		// 방향별 이동
		switch (pSession->dwAction)
		{
		case dfPACKET_MOVE_DIR_LL: pSession->shX -= MOVE_X; break;
		case dfPACKET_MOVE_DIR_LU: pSession->shX -= MOVE_X; pSession->shY -= MOVE_Y; break;
		case dfPACKET_MOVE_DIR_UU: pSession->shY -= MOVE_Y; break;
		case dfPACKET_MOVE_DIR_RU: pSession->shX += MOVE_X; pSession->shY -= MOVE_Y; break;
		case dfPACKET_MOVE_DIR_RR: pSession->shX += MOVE_X; break;
		case dfPACKET_MOVE_DIR_RD: pSession->shX += MOVE_X; pSession->shY += MOVE_Y; break;
		case dfPACKET_MOVE_DIR_DD: pSession->shY += MOVE_Y; break;
		case dfPACKET_MOVE_DIR_LD: pSession->shX -= MOVE_X; pSession->shY += MOVE_Y; break;
		}

		// 이동 위치 변경 로그 출력 (gameRun:방향)
		{
			//printf 막기
			#define printf(...) ((void)0)
			
			printf("# gameRun:%s # SessionID:%d / X:%d / Y:%d\n",
				DirectionToString(pSession->dwAction), pSession->dwSessionID, pSession->shX, pSession->shY);
			
			#undef printf
		}
		// 경계 체크
		if (pSession->shX <= dfRANGE_MOVE_LEFT ||
			pSession->shX >= dfRANGE_MOVE_RIGHT ||
			pSession->shY <= dfRANGE_MOVE_TOP ||
			pSession->shY >= dfRANGE_MOVE_BOTTOM)
		{
			// 이전 좌표로 되돌림
			pSession->shX = prevX;
			pSession->shY = prevY;

			// 멈춤 처리
			pSession->isMoving = false;

			st_SC_MOVE_STOP stopPacket;
			mpMoveStop(&stopPacket, pSession->dwSessionID, pSession->byDirection, pSession->shX, pSession->shY);
			SendBroadCast(pSession, &stopPacket, sizeof(st_SC_MOVE_STOP));
			continue;
		}
		// ------------------------------
		// 순간이동/속도핵 검출 (한 프레임 내 이동량이 비정상적이면 끊음)
		// (프레임당 이동량은 MOVE_X/MOVE_Y 이므로 dfERROR_RANGE 초과면 명백한 이상)
		// ------------------------------
		if (abs(pSession->shX - prevX) > dfERROR_RANGE ||
			abs(pSession->shY - prevY) > dfERROR_RANGE)
		{
			// 비정상 이동 -> 접속 끊기
			printf("비정상 적인 이동 감지 \n");
			pSession->disconnectFlag = true;
			continue;
		}
	}

}



int netProc_Accept(void)
{
	SOCKADDR_IN clientAddr;
	int addrlen = sizeof(clientAddr);
	SOCKET clientSock = accept(g_ListenSocket, (SOCKADDR*)&clientAddr, &addrlen);
	if (clientSock == INVALID_SOCKET)
	{
		printf("accept socket Error : %d\n", WSAGetLastError());
		return -1;
	}
	u_long mode = 1;
	int retval_ioctl = ioctlsocket(clientSock, FIONBIO, &mode);
	if (retval_ioctl != 0)
	{
		printf("ioctlsocket ERROR : %d\n", WSAGetLastError());
		return -1;
	}
	st_SESSION* newSession = new st_SESSION;
	newSession->Socket = clientSock;
	newSession->dwSessionID = ++g_dwSessionID;

	int randomX = rand() % 220;
	int randomY = rand() % 150;
	int plusminus = rand() % 4;
	switch (plusminus)
	{
	case 0:
		newSession->shX += randomX;
		newSession->shY += randomY;
		break;
	case 1:
		newSession->shX -= randomX;
		newSession->shY += randomY;
		break;
	case 2:
		newSession->shX += randomX;
		newSession->shY -= randomY;
		break;
	case 3:
		newSession->shX -= randomX;
		newSession->shY -= randomY;
		break;
	}

	g_sessionList.push_back(newSession);

	printf("NEW CLIENT Connected: %d\n", newSession->dwSessionID);
	
	// 나의 캐릭터 생성
	st_SC_CREATE_MY_CHARACTER createMe;
	createMe.header.byCode = 0x89;
	createMe.header.bySize = sizeof(st_SC_CREATE_MY_CHARACTER) - sizeof(st_PACKET_HEADER);
	createMe.header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;
	createMe.ID = newSession->dwSessionID;
	createMe.byDirection = newSession->byDirection;
	createMe.shX = newSession->shX;
	createMe.shY = newSession->shY;
	createMe.HP = newSession->chHP;
	SendUnicast(newSession, &createMe, sizeof(st_SC_CREATE_MY_CHARACTER));

	// 새로운 세션이 들어왔음을 다른 클라이언트에게 알리기
	st_SC_CREATE_OTHER_CHARACTER crMsg;
	crMsg.header.byCode = 0x89;
	crMsg.header.bySize = sizeof(st_SC_CREATE_OTHER_CHARACTER) - sizeof(st_PACKET_HEADER);
	crMsg.header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

	crMsg.ID = newSession->dwSessionID;
	crMsg.shX = newSession->shX;
	crMsg.shY = newSession->shY;
	crMsg.byDirection = newSession->byDirection;
	crMsg.HP = newSession->chHP;
	SendBroadCast(newSession, &crMsg, sizeof(st_SC_CREATE_OTHER_CHARACTER));

	// 다른 클라이언트의 정보를 newSession에게 보내기
	for (auto& s : g_sessionList)
	{
		if (s == newSession) continue;
		st_SC_CREATE_OTHER_CHARACTER other;
		other.header.byCode = 0x89;
		other.header.bySize = sizeof(st_SC_CREATE_OTHER_CHARACTER) - sizeof(st_PACKET_HEADER);
		other.header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

		other.ID = s->dwSessionID;
		other.byDirection = s->byDirection;
		other.shX = s->shX;
		other.shY = s->shY;
		other.HP = s->chHP;

		SendUnicast(newSession, &other, sizeof(st_SC_CREATE_OTHER_CHARACTER));
	}
	return 0;
}

int netProc_Recv(st_SESSION* session)
{
	//링버퍼의 남은 여유 공간으로 연속적 사용가능한 영역 크기 계산
	while(true)
	{
		int freeSize = session->RecvQ->GetFreeSize();
		if (freeSize == 0)
		{
			printf("RecvQ가 꽉참\n");
			return -1;
		}

		int contFreeSize = session->RecvQ->DirectEnqueueSize();
		int recvSize = min(freeSize, contFreeSize);

		char* writePtr = session->RecvQ->GetRearBufferPtr();
		int retval_recv = recv(session->Socket, writePtr, recvSize, 0);
		if (retval_recv == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				printf("WOULDBLOCK\n");
				return 0;
			}
			else
			{
				printf("recv ERROR : %d\n", WSAGetLastError());
				return -1;
			}
		}
		else if (retval_recv == 0)
		{
			//Client 연결 종료
			return -1;
		}

		session->RecvQ->MoveRear(retval_recv);
		if (retval_recv < recvSize)
		{
			break;
		}
	}
#pragma region MyRegion
//임시 수신버퍼
//char buffer[10000];

//int retval_recv = recv(session->Socket, buffer, sizeof(buffer), 0);
//if (retval_recv == SOCKET_ERROR)
//{
//	if (WSAGetLastError() == WSAEWOULDBLOCK)
//	{
//		printf("WOULDBLOCK\n");
//		return 0;
//	}
//	return -1;
//}
//else if (retval_recv == 0)
//{
//	return -1;
//}

//int retval_enqueue = session->RecvQ->Enqueue(buffer, retval_recv);
//if (retval_enqueue == 0)
//{
//	printf("RecvQ가 꽉참\n");
//	return -1;
//}
#pragma endregion

	// * 완료패킷 처리 부
	// 완료패킷 처리 부분은 RecvQ 에 들어있는 모든 완성패킷을 처리 해야 함.
	while (true)
	{
		//1.	RecvQ 에 최소한의 사이즈가 있는지 확인. 조건 - 헤더사이즈 초과
		if (session->RecvQ->GetUseSize() <= sizeof(st_PACKET_HEADER))
		{
			break;
		}
		st_PACKET_HEADER stHeader;
		//2.	RecvQ 에서 헤더를 Peek 
		int retval_peek = session->RecvQ->Peek((char*)&stHeader, sizeof(st_PACKET_HEADER));
		if (retval_peek < sizeof(st_PACKET_HEADER))
		{
			printf("PEEK에 아무것도 잡히지않음\n");
			return -1;
		}
		//3.	헤더의 Code 확인
		if (stHeader.byCode != 0x89)
		{
			printf("Header byCode is Wrong : %d\n", stHeader.byCode);
			return -1;
		}
		//4.	헤더의 Len 값과 RecvQ 의 데이터 사이즈 비교
		int totalSize = stHeader.bySize + retval_peek;
		if (session->RecvQ->GetUseSize() < totalSize)
		{
			break;
		}
		//- 완성패킷 사이즈 : 헤더 + Len
		char* PacketBuffer = new char[totalSize];
		//5. 의심
		if (stHeader.bySize <= 0 || stHeader.bySize + retval_peek >= 8192)
		{
			printf("의심스러운 패킷 크기\n");
			return -1;
		}
		//6.	RecvQ 에서 Len 만큼 임시 패킷버퍼로 뽑음.
		session->RecvQ->Dequeue(PacketBuffer, totalSize);
		//7.	헤더의 타입에 따른 분기를 위해 패킷 프로시저 호출.
		if (!PacketProc(session, stHeader.byType, PacketBuffer))
		{
			printf("PacketProc Failed\n");
			return -1;
		}
		delete[] PacketBuffer;
	}
	return 0;
}
void netProc_Send(st_SESSION* s)
{
	char buf[4096];
	int size = s->SendQ->Peek(buf, sizeof(buf));
	if (size <= 0) return;

	int sent = send(s->Socket, buf, size, 0);
	if (sent > 0) {
		s->SendQ->MoveFront(sent);
	}
	else if (sent == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAEWOULDBLOCK)
		{
			return;
		}
		else
		{
			printf("send Failed : %d\n", WSAGetLastError());
			s->disconnectFlag = true;
			return;
		}
	}
	else if (sent <= 0)
	{
		s->disconnectFlag = true;
		return;
	}


}


bool PacketProc(st_SESSION* pSession, BYTE byPacketType, char* pPacket)
{
	switch (byPacketType)
	{
	case dfPACKET_CS_MOVE_START:
	{
		//st_CS_MOVE_START* pMoveStart = (st_CS_MOVE_START*)pPacket;
		//printf("[PacketProc] MOVE_START Received: SessionID:%d, Dir:%d, X:%d, Y:%d\n",
		//	pSession->dwSessionID, pMoveStart->byDirection, pMoveStart->shX, pMoveStart->shY);
		return netPacketProc_MoveStart(pSession, pPacket);
	}
		break;
	case dfPACKET_CS_MOVE_STOP:
		return netPacketProc_MoveStop(pSession, pPacket);
		break;
	case dfPACKET_CS_ATTACK1:
		return netPacketProc_Attack1(pSession, pPacket);
		break;
	case dfPACKET_CS_ATTACK2:
		return netPacketProc_Attack2(pSession, pPacket);
		break;
	case dfPACKET_CS_ATTACK3:
		return netPacketProc_Attack3(pSession, pPacket);
		break;
	}
	return TRUE;
}

bool netPacketProc_MoveStart(st_SESSION* pSession, char* pPacket)
{
	st_CS_MOVE_START* pMoveStart = (st_CS_MOVE_START*)pPacket;
	printf("# PACKET_MOVESTART # SessionID:%d / Direction:%d / X:%d / Y:%d\n",
		pSession->dwSessionID, pMoveStart->byDirection, pMoveStart->shX, pMoveStart->shY);

	// 메시지 수신 로그 확인
	//-----------------------------------------------------
	// 서버의 위치와 받은 패킷의 위치값이 너무 큰 차이가 난다면 끊어버림
	// 본 게임의 좌표 동기화 구조가 단순한 키보드 조작 (클라이언트의 선처리, 서버의 후 반영) 방식으로
	// 클라이언트의 좌표를 그대로 믿는 방식을 택하고 있음.
	// 실제 온라인 게임이라면 클라이언트에서 목적지를 공유하는 방식을 택해야 함.
	// 지금은 간단한 구현을 목적으로 하고 있으므로 오차범위 내에서 클라이언트 좌표를 믿도록 한다
	//-----------------------------------------------------
	if (abs(pSession->shX - pMoveStart->shX) > dfERROR_RANGE ||
		abs(pSession->shY - pMoveStart->shY) > dfERROR_RANGE)
	{
		printf("[MoveStart] Position mismatch too large. Disconnecting SessionID:%d\n", pSession->dwSessionID);
		pSession->disconnectFlag = true;
		// 로그출력
		return false;
	}
	//-----------------------------------------------------
	// 동작을 변경. 지금 구현에선 동작번호가 방향값이다
	//-----------------------------------------------------
	pSession->dwAction = pMoveStart->byDirection;
	pSession->isMoving = true;                     // 이동중 상태로 변경

	//-----------------------------------------------------
	// 방향을 변경.
	//-----------------------------------------------------
	switch (pMoveStart->byDirection)
	{
	case dfPACKET_MOVE_DIR_RR:
		[[fallthrough]];
	case dfPACKET_MOVE_DIR_RU:
		[[fallthrough]];
	case dfPACKET_MOVE_DIR_RD:
		pSession->byDirection = dfPACKET_MOVE_DIR_RR;
		break;
	case dfPACKET_MOVE_DIR_LU:
		[[fallthrough]];
	case dfPACKET_MOVE_DIR_LL:
		[[fallthrough]];
	case dfPACKET_MOVE_DIR_LD:
		pSession->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	}
	pSession->shX = pMoveStart->shX;
	pSession->shY = pMoveStart->shY;
	//-----------------------------------------------------
	// 현재 접속중인 사용자에게 모든 패킷을 뿌린다. 당사자 제외
	//-----------------------------------------------------
	st_SC_MOVE_START SendMsg;
	mpMoveStart(&SendMsg, pSession->dwSessionID, pMoveStart->byDirection, pSession->shX, pSession->shY);
	SendBroadCast(pSession, &SendMsg, sizeof(st_SC_MOVE_START)); // 당사자만 제외하고 모두에게 전송시키는 함수
	return true;
}

void mpMoveStart(st_SC_MOVE_START* pPacket, int SessionID, unsigned char Direction, short X, short Y)
{
	pPacket->header.byCode = dfNETWORK_PACKET_CODE;
	pPacket->header.bySize = sizeof(st_SC_MOVE_START) - sizeof(st_PACKET_HEADER);
	pPacket->header.byType = dfPACKET_SC_MOVE_START;

	pPacket->ID = SessionID;
	pPacket->byDirection = Direction;
	pPacket->shX = X;
	pPacket->shY = Y;
}

bool netPacketProc_MoveStop(st_SESSION* pSession, char* pPacket)
{
	st_CS_MOVE_STOP* Packet = (st_CS_MOVE_STOP*)pPacket;

	printf("# PACKET_MOVESTOP # SessionID:%d / Direction:%d / X:%d / Y:%d\n",
		pSession->dwSessionID, Packet->byDirection, Packet->shX, Packet->shY);

	//-----------------------------------------------------
// 동작을 변경. 지금 구현에선 동작번호가 방향값이다
//-----------------------------------------------------
	pSession->dwAction = Packet->byDirection;
	pSession->isMoving = false;                // 이동중 상태 해제
	//-----------------------------------------------------
	// 방향을 변경.
	//-----------------------------------------------------
	switch (Packet->byDirection)
	{
	case dfPACKET_MOVE_DIR_RR:
		[[fallthrough]];
	case dfPACKET_MOVE_DIR_RU:
		[[fallthrough]];
	case dfPACKET_MOVE_DIR_RD:
		pSession->byDirection = dfPACKET_MOVE_DIR_RR;
		break;
	case dfPACKET_MOVE_DIR_LU:
		[[fallthrough]];
	case dfPACKET_MOVE_DIR_LL:
		[[fallthrough]];
	case dfPACKET_MOVE_DIR_LD:
		pSession->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	}
	pSession->shX = Packet->shX;
	pSession->shY = Packet->shY;
	//-----------------------------------------------------
	// 현재 접속중인 사용자에게 모든 패킷을 뿌린다. 당사자 제외
	//-----------------------------------------------------
	st_SC_MOVE_STOP SendMsg;
	mpMoveStop(&SendMsg, pSession->dwSessionID, pSession->byDirection, pSession->shX, pSession->shY);
	SendBroadCast(pSession, &SendMsg, sizeof(st_SC_MOVE_STOP));
	return true;
}
void mpMoveStop(st_SC_MOVE_STOP* pPacket, int SessionID, unsigned char Direction, short X, short Y)
{
	pPacket->header.byCode = dfNETWORK_PACKET_CODE;
	pPacket->header.bySize = sizeof(st_SC_MOVE_STOP) - sizeof(st_PACKET_HEADER);
	pPacket->header.byType = dfPACKET_SC_MOVE_STOP;

	pPacket->ID = SessionID;
	pPacket->byDirection = Direction;
	pPacket->shX = X;
	pPacket->shY = Y;
}

bool netPacketProc_Attack1(st_SESSION* pSession, char* pPacket)
{
	st_CS_ATTACK1* pAttack = (st_CS_ATTACK1*)pPacket;
	printf("# PACKET_ATTACK1 # SessionID:%d / Direction:%d / X:%d / Y:%d\n",
		pSession->dwSessionID, pAttack->byDirection, pAttack->shX, pAttack->shY);

	// 1) 위치 동기화, 방향 갱신
	pSession->byDirection = pAttack->byDirection;
	pSession->shX = pAttack->shX;
	pSession->shY = pAttack->shY;

	// 2) 공격 애니메이션 브로드캐스트
	st_SC_ATTACK1 sendMsg;
	sendMsg.header.byCode = dfNETWORK_PACKET_CODE;
	sendMsg.header.bySize = sizeof(st_SC_ATTACK1) - sizeof(st_PACKET_HEADER);
	sendMsg.header.byType = dfPACKET_SC_ATTACK1;
	sendMsg.ID = pSession->dwSessionID;
	sendMsg.byDirection = pAttack->byDirection;
	sendMsg.shX = pAttack->shX;
	sendMsg.shY = pAttack->shY;
	SendBroadCast(pSession, &sendMsg, sizeof(st_SC_ATTACK1));

	// 3) 공격 범위 내 플레이어 찾아 피해 주기 (예시: dfATTACK1_RANGE_X/Y)
	for (auto targetSession : g_sessionList)
	{
		if (targetSession == pSession) continue; // 자기 자신 제외
		int dx = abs(targetSession->shX - pSession->shX);
		int dy = abs(targetSession->shY - pSession->shY);
		if (dx <= dfATTACK1_RANGE_X && dy <= dfATTACK1_RANGE_Y)
		{
			// 피해 처리 (임시로 HP 10 감소)
			if (targetSession->chHP > 10)
				targetSession->chHP -= 10;
			else
				targetSession->chHP = 0;

			// 피해 패킷 전송
			st_SC_DAMAGE dmgPacket;
			dmgPacket.header.byCode = dfNETWORK_PACKET_CODE;
			dmgPacket.header.bySize = sizeof(st_SC_DAMAGE) - sizeof(st_PACKET_HEADER);
			dmgPacket.header.byType = dfPACKET_SC_DAMAGE;
			dmgPacket.AttackID = pSession->dwSessionID;
			dmgPacket.DamageID = targetSession->dwSessionID;
			dmgPacket.DamageHP = targetSession->chHP;
			SendBroadCast(nullptr, &dmgPacket, sizeof(st_SC_DAMAGE)); // 모두에게 알림
		}
	}

	return true;
}

bool netPacketProc_Attack2(st_SESSION* pSession, char* pPacket)
{
	st_CS_ATTACK2* pAttack = (st_CS_ATTACK2*)pPacket;
	printf("# PACKET_ATTACK2 # SessionID:%d / Direction:%d / X:%d / Y:%d\n",
		pSession->dwSessionID, pAttack->byDirection, pAttack->shX, pAttack->shY);

	// 1) 위치 동기화, 방향 갱신
	pSession->byDirection = pAttack->byDirection;
	pSession->shX = pAttack->shX;
	pSession->shY = pAttack->shY;

	// 2) 공격 애니메이션 브로드캐스트
	st_SC_ATTACK2 sendMsg;
	sendMsg.header.byCode = dfNETWORK_PACKET_CODE;
	sendMsg.header.bySize = sizeof(st_SC_ATTACK2) - sizeof(st_PACKET_HEADER);
	sendMsg.header.byType = dfPACKET_SC_ATTACK2;
	sendMsg.ID = pSession->dwSessionID;
	sendMsg.byDirection = pAttack->byDirection;
	sendMsg.shX = pAttack->shX;
	sendMsg.shY = pAttack->shY;
	SendBroadCast(pSession, &sendMsg, sizeof(st_SC_ATTACK2));

	// 3) 공격 범위 내 플레이어 찾아 피해 주기 (예시: dfATTACK2_RANGE_X/Y)
	for (auto targetSession : g_sessionList)
	{
		if (targetSession == pSession) continue; // 자기 자신 제외
		int dx = abs(targetSession->shX - pSession->shX);
		int dy = abs(targetSession->shY - pSession->shY);
		if (dx <= dfATTACK2_RANGE_X && dy <= dfATTACK2_RANGE_Y)
		{
			// 피해 처리 (임시로 HP 10 감소)
			if (targetSession->chHP > 15)
				targetSession->chHP -= 15;
			else
				targetSession->chHP = 0;

			// 피해 패킷 전송
			st_SC_DAMAGE dmgPacket;
			dmgPacket.header.byCode = dfNETWORK_PACKET_CODE;
			dmgPacket.header.bySize = sizeof(st_SC_DAMAGE) - sizeof(st_PACKET_HEADER);
			dmgPacket.header.byType = dfPACKET_SC_DAMAGE;
			dmgPacket.AttackID = pSession->dwSessionID;
			dmgPacket.DamageID = targetSession->dwSessionID;
			dmgPacket.DamageHP = targetSession->chHP;
			SendBroadCast(nullptr, &dmgPacket, sizeof(st_SC_DAMAGE)); // 모두에게 알림
		}
	}

	return true;
}

bool netPacketProc_Attack3(st_SESSION* pSession, char* pPacket)
{
	st_CS_ATTACK3* pAttack = (st_CS_ATTACK3*)pPacket;
	printf("# PACKET_ATTACK3 # SessionID:%d / Direction:%d / X:%d / Y:%d\n",
		pSession->dwSessionID, pAttack->byDirection, pAttack->shX, pAttack->shY);

	// 1) 위치 동기화, 방향 갱신
	pSession->byDirection = pAttack->byDirection;
	pSession->shX = pAttack->shX;
	pSession->shY = pAttack->shY;

	// 2) 공격 애니메이션 브로드캐스트
	st_SC_ATTACK3 sendMsg;
	sendMsg.header.byCode = dfNETWORK_PACKET_CODE;
	sendMsg.header.bySize = sizeof(st_SC_ATTACK3) - sizeof(st_PACKET_HEADER);
	sendMsg.header.byType = dfPACKET_SC_ATTACK3;
	sendMsg.ID = pSession->dwSessionID;
	sendMsg.byDirection = pAttack->byDirection;
	sendMsg.shX = pAttack->shX;
	sendMsg.shY = pAttack->shY;
	SendBroadCast(pSession, &sendMsg, sizeof(st_SC_ATTACK3));

	// 3) 공격 범위 내 플레이어 찾아 피해 주기 (예시: dfATTACK3_RANGE_X/Y)
	for (auto targetSession : g_sessionList)
	{
		if (targetSession == pSession) continue; // 자기 자신 제외
		int dx = abs(targetSession->shX - pSession->shX);
		int dy = abs(targetSession->shY - pSession->shY);
		if (dx <= dfATTACK3_RANGE_X && dy <= dfATTACK3_RANGE_Y)
		{
			// 피해 처리 (임시로 HP 10 감소)
			if (targetSession->chHP > 10)
				targetSession->chHP -= 20;
			else
				targetSession->chHP = 0;

			// 피해 패킷 전송
			st_SC_DAMAGE dmgPacket;
			dmgPacket.header.byCode = dfNETWORK_PACKET_CODE;
			dmgPacket.header.bySize = sizeof(st_SC_DAMAGE) - sizeof(st_PACKET_HEADER);
			dmgPacket.header.byType = dfPACKET_SC_DAMAGE;
			dmgPacket.AttackID = pSession->dwSessionID;
			dmgPacket.DamageID = targetSession->dwSessionID;
			dmgPacket.DamageHP = targetSession->chHP;
			SendBroadCast(nullptr, &dmgPacket, sizeof(st_SC_DAMAGE)); // 모두에게 알림
		}
	}

	return true;
}

void Disconnect(st_SESSION* session)
{
	st_SC_DELETE_CHARACTER dlmsg;
	dlmsg.header.byCode = 0x89;
	dlmsg.header.bySize = sizeof(st_SC_DELETE_CHARACTER) - sizeof(st_PACKET_HEADER);
	dlmsg.header.byType = dfPACKET_SC_DELETE_CHARACTER;

	dlmsg.ID = session->dwSessionID;
	SendBroadCast(session, &dlmsg, sizeof(st_SC_DELETE_CHARACTER));
	printf("DisConnect : %d\n", session->dwSessionID);
	closesocket(session->Socket);

	delete session->RecvQ;
	delete session->SendQ;

	delete session;
}

void SendUnicast(st_SESSION* session, const void* msg, int msglen)
{
	// send() 직접 호출 대신 SendQ에 데이터를 넣는다
	session->SendQ->Enqueue((const char*)msg, msglen);
}

void SendBroadCast(st_SESSION* except, const void* msg, int msglen)
{
	for (auto session : g_sessionList)
	{
		if (session == except) continue;

		// send() 직접 호출 대신 SendQ에 데이터를 넣는다
		session->SendQ->Enqueue((const char*)msg, msglen);
	}
}