#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

void RecvThread(SOCKET sock)
{
    char buffer[1024];

    while (true)
    {
        int recvBytes = recv(sock, buffer, sizeof(buffer) - 1, 0);

        if (recvBytes <= 0)
        {
            cout << "\n서버 연결 종료됨." << endl;
            return;
        }

        buffer[recvBytes] = 0;

        // 입력 중 깨끗하게 메시지를 다시 그려주도록 처리
        cout << "\r"              // 현재 입력 줄 제거
            << "[서버] " << buffer << "\n"
            << "> " << flush;    // 입력 프롬프트 복구
    }
}

int main()
{
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);



    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000);
    InetPton(AF_INET, L"127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cout << "서버 접속 실패." << endl;
        return 1;
    }

    cout << "서버 접속 완료.\n";

    // recv 전용 스레드 생성
    thread t(RecvThread, sock);
    t.detach();

    string name;
    cout << "사용할 닉네임을 작성해주세요" << "\n" << ">>";
    getline(cin, name);

    // 서버로 닉네임 패킷 전송
    string nicknamePacket = "[name]" + name;
    send(sock, nicknamePacket.c_str(), nicknamePacket.size(), 0);

    while (true)
    {
        string msg;
        cout << "> ";
        getline(cin, msg);

        if (msg == "quit" || msg == "exit")
        {
            break;
        }

        send(sock, msg.c_str(), msg.size(), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
