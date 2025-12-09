#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <iostream>
#include <deque>
#include <vector>
#include <map>
#include <thread>
#include "RingBuffer.h"
#include "GameServer.h"
#include "Protocol.h"
#include "Session.h"
#include "Network.h"

#pragma comment(lib,"ws2_32.lib")

#define BUFFER_SIZE 1024

CRITICAL_SECTION g_lock;
std::vector<HANDLE> gWorkerThreads;
unsigned int GetWorkerThreadCount();
bool StartWorkerThreads();


std::map<SOCKET, Session*> g_sessions;
HANDLE gIOCP;


bool IsKnownPacketId(uint16_t id);

DWORD WINAPI WorkerThread(LPVOID lpParam);


void Network_Broadcast(Session* sender, uint16_t packetId, const void* data, uint16_t size)
{
	EnterCriticalSection(&g_lock);
	for (auto& s : g_sessions)
	{
		Session* target = s.second;
		if (target == sender)
			continue;

		target->SendPacket(packetId, data, size);
	}
	LeaveCriticalSection(&g_lock);
}

// 아마도 하드웨어 스레드 개수 구하는 것 같은데 
unsigned int GetWorkerThreadCount()
{
	unsigned int n = std::thread::hardware_concurrency();
	return (n == 0) ? 4 : n;
}

bool StartWorkerThreads()
{
	unsigned int count = GetWorkerThreadCount();
	gWorkerThreads.reserve(count);

	for (unsigned int i = 0; i < count; ++i)
	{
		HANDLE hThread = CreateThread(
			nullptr,
			0,
			WorkerThread,
			gIOCP,
			0,
			nullptr
		);

		if (hThread == nullptr)
		{
			std::cout << "CreateThread failed: " << GetLastError();
			return false;
		}

		gWorkerThreads.push_back(hThread);
	}
	std::cout << "WorkerThreads Created: " << count << "\n";
	return true;
}

DWORD WINAPI WorkerThread(LPVOID lpParam)
{
	HANDLE hIocp = lpParam;

	while (true)
	{
		DWORD bytesTransferred = 0;
		Session* session = nullptr;
		LPOVERLAPPED overlapped = nullptr;	

		bool ret = GetQueuedCompletionStatus(
			hIocp,
			&bytesTransferred,
			(PULONG_PTR)&session,
			&overlapped,
			INFINITE);

		if (session == nullptr)
			continue;

		// 1) 에러 또는 0바이트 → 연결 끊김
		if (!ret || bytesTransferred == 0)
		{
			if (!ret)
			{
				// Error 종료
				int err = GetLastError();	// ERROR 64는 상대방 강제 종료
				std::cout <<"[Client " << session->name << "] GQCS Error: " << err << "\n";
			}
			else
			{
				// 정상 종료
				std::cout << "client 정상 종료\n";
			}

			session->OnRecvComplete(0);
			continue;
		}

		// recv
		if (overlapped == session->GetRecvOverlapped())
		{
			session->OnRecvComplete(bytesTransferred);
			printf("[Thread %d] RecvComplete\n", GetCurrentThreadId());
		}
		// send
		else if (overlapped == session->GetSendOverlapped())
		{
			session->OnSendComplete(bytesTransferred);
			printf("[Thread %d] SendComplete\n", GetCurrentThreadId());
		}
		else
		{
			// 예상치 못한 overlapped -> 로그 기록
		}
	}
	return 0;

}

int main()
{
	InitializeCriticalSection(&g_lock);
	WSAData wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN listenAddr;
	listenAddr.sin_family = AF_INET;
	listenAddr.sin_port = htons(9000);
	InetPton(AF_INET, L"127.0.0.1", &listenAddr.sin_addr);

	bind(listenSock, (SOCKADDR*)&listenAddr, sizeof(listenAddr));
	listen(listenSock, SOMAXCONN);

	// 인자 넣는거 못외움
	gIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

	StartWorkerThreads();

	while (true)
	{
		SOCKET client = accept(listenSock, nullptr, nullptr);
		if (client == INVALID_SOCKET)
			continue;

		Session* session = new Session(client);

		std::cout << "Client " << client << "접속\n";

		EnterCriticalSection(&g_lock);
		g_sessions.emplace(client, session);
		LeaveCriticalSection(&g_lock);

		// 서버에 접속한 클라이언트를 IOCP로 감시해주세요. 다만 KEY는 이제부터 Session*
		CreateIoCompletionPort((HANDLE)client, gIOCP, (ULONG_PTR)session, 0);

		session->PostRecv();
	}

	DeleteCriticalSection(&g_lock);
	WSACleanup();
	return 0;

}