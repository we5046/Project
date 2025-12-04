#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <map>

#pragma comment(lib,"ws2_32.lib")

#define BUFFER_SIZE 1024
using namespace std;

CRITICAL_SECTION g_lock;
vector<SOCKET>g_sessions;
map<SOCKET, string>g_names;

enum IO_TYPE
{
	IO_RECV,
	IO_SEND
};
struct PER_IO_DATA
{
	WSAOVERLAPPED overlapped;
	WSABUF wsabuf;
	char buffer[BUFFER_SIZE];
	IO_TYPE io_type;

};



void Broadcast(const char* msg, int len, SOCKET except)
{
	EnterCriticalSection(&g_lock);
	for (SOCKET k : g_sessions)
	{
		if (k == except) continue;

		PER_IO_DATA* sendData = new PER_IO_DATA;
		ZeroMemory(&sendData->overlapped, sizeof(WSAOVERLAPPED));
		memcpy(sendData->buffer, msg, len);
		sendData->wsabuf.buf = sendData->buffer;
		sendData->wsabuf.len = len;
		sendData->io_type = IO_SEND;

		int ret = WSASend(
			k,
			&sendData->wsabuf,
			1,
			nullptr,
			0,
			&sendData->overlapped,
			nullptr);

		if (ret == SOCKET_ERROR && GetLastError() != WSA_IO_PENDING)
		{
			cout << "SEND 실패" << "\n";
			delete sendData;
		}

	}
	LeaveCriticalSection(&g_lock);

}

DWORD WINAPI WorkerThread(LPVOID lpParam)
{
	HANDLE hIocp = lpParam;
	while (true)
	{
		DWORD bytesTransferred = 0;
		PER_IO_DATA* data = nullptr;
		ULONG_PTR key;
		bool ret = GetQueuedCompletionStatus(
			hIocp,
			&bytesTransferred,
			&key,
			(LPOVERLAPPED*)&data,
			INFINITE);

		SOCKET client = (SOCKET)key;
		if (!ret || bytesTransferred == 0)
		{
			cout << " 클라이언트 종료 " << "\n";
			EnterCriticalSection(&g_lock);
			g_sessions.erase(remove(g_sessions.begin(), g_sessions.end(), client), g_sessions.end());
			g_names.erase(client);
			LeaveCriticalSection(&g_lock);

			closesocket(client);
			delete data;
			continue;
		}

		if (data->io_type == IO_SEND)
		{
			delete data;
			continue;
		}

		// recv
		data->buffer[bytesTransferred] = 0;
		string msg = data->buffer;
		
		if (msg.rfind("[name]", 0) == 0)
		{
			string name = msg.substr(6);

			EnterCriticalSection(&g_lock);
			g_names[client] = name;
			LeaveCriticalSection(&g_lock);
			cout << client << "의 닉네임 : " << name << "\n";
		}
		else
		{
			
			cout << client << "] : " << data->buffer << "\n";
			// broadcast
			string finalMsg = g_names[client] + " : " + data->buffer;
			Broadcast(finalMsg.c_str(), finalMsg.size(), client);
		}



		// recv 재등록
		ZeroMemory(&data->overlapped, sizeof(WSAOVERLAPPED));
		data->io_type = IO_RECV;
		data->wsabuf.buf = data->buffer;
		data->wsabuf.len = sizeof(data->buffer);

		DWORD flags = 0;
		int retR = WSARecv(
			client,
			&data->wsabuf,
			1,
			nullptr,
			&flags,
			&data->overlapped,
			nullptr);

		if (retR == SOCKET_ERROR && GetLastError() != WSA_IO_PENDING)
		{
			cout << "RECV 재등록 실패" << "\n";
			EnterCriticalSection(&g_lock);
			g_sessions.erase(remove(g_sessions.begin(), g_sessions.end(), client), g_sessions.end());
			g_names.erase(client);
			LeaveCriticalSection(&g_lock);

			closesocket(client);
			delete data;
			continue;
		}

	}


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
	HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

	CreateThread(nullptr, 0, WorkerThread, hIOCP, 0, nullptr);

	while (true)
	{
		SOCKET client = accept(listenSock, nullptr, nullptr);

		EnterCriticalSection(&g_lock);
		g_sessions.push_back(client);
		LeaveCriticalSection(&g_lock);

		// 서버에 접속한 클라이언트를 IOCP로 감시해주세요.
		CreateIoCompletionPort((HANDLE)client, hIOCP, (ULONG_PTR)client, 0);

		// 첫 WSARECV 등록
		PER_IO_DATA* recvData = new PER_IO_DATA;
		ZeroMemory(&recvData->overlapped, sizeof(WSAOVERLAPPED));
		recvData->wsabuf.buf = recvData->buffer;
		recvData->wsabuf.len = BUFFER_SIZE;
		recvData->io_type = IO_RECV;

		DWORD flags = 0;
		int retR = WSARecv(
			client,
			&recvData->wsabuf,
			1,
			nullptr,
			&flags,
			&recvData->overlapped,
			nullptr);

		if (retR == SOCKET_ERROR && GetLastError() != WSA_IO_PENDING)
		{
			cout << "RECV 등록 실패" << "\n";
			EnterCriticalSection(&g_lock);
			g_sessions.erase(remove(g_sessions.begin(), g_sessions.end(), client), g_sessions.end());
			g_names.erase(client);
			LeaveCriticalSection(&g_lock);

			closesocket(client);
			delete recvData;
		}
	}

	DeleteCriticalSection(&g_lock);
	WSACleanup();
	return 0;

}
