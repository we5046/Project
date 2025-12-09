#include "Session.h"
#include <string>
#include <iostream>
#include "Packet.h"
#include "Protocol.h"


Session::Session(SOCKET s)
{
	sock = s;
	ZeroMemory(&recvOverlapped, sizeof(WSAOVERLAPPED));
	ZeroMemory(&sendOverlapped, sizeof(WSAOVERLAPPED));

	recvWsabuf.buf = recvBuffer;
	recvWsabuf.len = BUFFER_SIZE;

	sendWsabuf.buf = nullptr;		// 실제 전송할때 sendQueue.front().data()로 설정함.
	sendWsabuf.len = 0;				// 실제 전송할때 sendQueue.front().size()로 설정함. 

	ZeroMemory(recvBuffer, BUFFER_SIZE);

	isClosing = false;
	pendingIO = 0;
}

void Session::PostRecv()
{
	// 이미 종료 중이면 더 이상 Recv 등록 안함
	if (isClosing)
	{
		return;
	}

	DWORD flags = 0;
	DWORD bytes = 0;

	// I/O 하나 등록 → pendingIO 증가
	pendingIO.fetch_add(1, std::memory_order_relaxed);


	int ret = WSARecv(
		sock,
		&recvWsabuf,
		1,
		&bytes,
		&flags,
		&recvOverlapped,
		nullptr);

	if (ret == SOCKET_ERROR && GetLastError() != WSA_IO_PENDING)
	{
		std::cout << "WSARECV 등록 실패 " << "\n";
		// 등록 실패했으니 다시 되돌림
		pendingIO.fetch_sub(1, std::memory_order_relaxed);
		RequestClose();
	}
}

void Session::PostSend()
{
	if (isClosing|| isSending || sendQueue.empty())
	{
		return;
	}

	auto& pkt = sendQueue.front();

	// 메시지를 sendBuffer에 복사

	sendWsabuf.buf = pkt.data();
	sendWsabuf.len = (ULONG)pkt.size();

	DWORD bytes = 0;
	// I/O 등록 → pendingIO 증가
	pendingIO.fetch_add(1, std::memory_order_relaxed);

	int ret = WSASend(
		sock,
		&sendWsabuf,
		1,
		&bytes,
		0,
		&sendOverlapped,
		nullptr);

	if (ret == SOCKET_ERROR && GetLastError() != WSA_IO_PENDING)
	{
		std::cout << "WSASEND 실패" << "\n";
		pendingIO.fetch_sub(1, std::memory_order_relaxed);
		RequestClose();
	}
}

void Session::OnRecvComplete(DWORD bytes)
{
	pendingIO.fetch_sub(1, std::memory_order_relaxed);

	if (bytes == 0)
	{
		RequestClose();
		return;
	}

	// 이번에 Recv된 데이터를 RingBuffer에 Write
	if (!recvRing.Write(recvBuffer, bytes))
	{
		// 버퍼 오버플로우 상황 (너무 많이 받아서 링버퍼가 꽉 찬 경우)
		// 실제 서버라면 로그 남기고 세션 끊어도 됨.
		// 여기선 그냥 조용히 무시하거나, 필요 시 강제 종료.
		// TODO: overflow 처리
		RequestClose();
		return;
	}

	// 패킷 조립 & 처리
	while (true)
	{
		if (!recvRing.Has(sizeof(PacketHeader)))
			break;

		Packet pkt;
		recvRing.Peek(&pkt.header, sizeof(PacketHeader));

		if (pkt.header.size < sizeof(PacketHeader) ||
			pkt.header.size > MAX_PACKET_SIZE)
		{
			RequestClose();
			return;
		}

		if (!recvRing.Has(pkt.header.size))
			break;
		recvRing.Skip(sizeof(PacketHeader));
		pkt.body = recvRing.GetReadPtr();
		ProcessPacket(this, pkt);

		recvRing.Skip(pkt.header.size - sizeof(PacketHeader));
	}
	PostRecv();
}

void Session::OnSendComplete(DWORD bytes)
{
	pendingIO.fetch_sub(1, std::memory_order_relaxed);

	if (!sendQueue.empty())
		sendQueue.pop_front();

	isSending = false;

	if (isClosing)
		return;

	if (!sendQueue.empty())
		PostSend();
}

void Session::RequestClose()
{
	if (isClosing)
		return;

	isClosing = true;

	shutdown(sock, SD_BOTH);
}

WSAOVERLAPPED* Session::GetRecvOverlapped() { return &recvOverlapped; }
WSAOVERLAPPED* Session::GetSendOverlapped() { return &sendOverlapped; }

void Session::SendPacket(uint16_t id, const void* data, uint16_t dataSize)
{
	uint16_t packetSize = sizeof(PacketHeader) + dataSize;

	// 1) 패킷 하나를 통째로 담을 buffer 생성
	std::vector<char> packet(packetSize);

	// 2) 헤더 채우기
	PacketHeader header = { packetSize, id };

	// 3) [헤더][바디] 복사
	memcpy(packet.data(), &header, sizeof(header));
	memcpy(packet.data() + sizeof(header), data, dataSize);

	// 4) 큐에 push
	sendQueue.push_back(std::move(packet));

	// 로그
	printf("SendPacket: id=%u size=%u to [%s]\n", id, packetSize, name.c_str());

	// 5) 지금 아무 것도 안 보내는 중이면 전송 시작
	if (!isSending)
	{
		PostSend();
	}
}