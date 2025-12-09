#pragma once
#include <Winsock2.h>
#include <deque>
#include <vector>

#include "RingBuffer.h"
#include <string>
#include <atomic>
constexpr size_t BUFFER_SIZE = 1024;

class Session
{
private:
	SOCKET sock;		// USER SOCKET

	bool isClosing;
	std::atomic<long> pendingIO;
	// ---- RECV ----
	WSAOVERLAPPED recvOverlapped;	// recvI/O용 OVERLAPPED
	WSABUF recvWsabuf;				// .buf: recvBuffer 포인터, .len: 길이
	char recvBuffer[BUFFER_SIZE];	// 커널 -> 유저 공간으로 1차로 받아 놓는 임시 버퍼
	RingBuffer recvRing;			// TCP 스트림을 쌓아두고 패킷 단위로 잘라내는 링버퍼

	// ---- SEND ----
	WSAOVERLAPPED sendOverlapped;	// sendI/O용 OVERLAPPED
	WSABUF sendWsabuf;				// 현재 보내는 패킷의 data, len 임시 담는 용도, 초기값 세팅 필요 X -> 보내기 직전에 PostSend 함수에서 설정
	std::deque<std::vector<char>> sendQueue;		// 보낼 메시지를 쌓아두는 FIFO, 실제 WSASend의 대상이 되는 버퍼들
	bool isSending = false;			// 지금 Send 중인지 판단 근거



public:
	std::string name;					// 닉네임, 유저정보
	Session(SOCKET s);
	void PostRecv();
	void PostSend();
	void OnRecvComplete(DWORD bytes);
	void OnSendComplete(DWORD bytes);
	void RequestClose(); // isClosing = true 등
	void SendPacket(uint16_t id, const void* data, uint16_t dataSize);
	bool IsClosing() const { return isClosing; }

	WSAOVERLAPPED* GetRecvOverlapped();
	WSAOVERLAPPED* GetSendOverlapped(); 
};