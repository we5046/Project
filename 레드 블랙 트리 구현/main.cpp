#include <windows.h>
#include <iostream>
#include "RedBlackTree.h"
#include <conio.h>
#include <string>
using namespace std;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
RedBlackTree g_tree;
HWND g_hWnd;
string inputBuffer;
char currentCmd = 'i';

void RedirectIOToConsole() {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONIN$", "r", stdin);

}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    RedirectIOToConsole();
    cout << "레드블랙트리 시각화 시작!\n";
    cout << "명령어 예시: (삽입) i | (삭제) d | (검색) s \n";
    cout << "모드 선택 후 숫자 입력\n";
    cout << "트리쪽 화면에서 Q누를 시 중위순회 실행\n";
    cout << "> ";

    // 윈도우 클래스 등록
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("RBTVisualizer");
    RegisterClass(&wc);

    // 윈도우 생성
    g_hWnd = CreateWindow(wc.lpszClassName, TEXT("Red Black Tree Visualizer"),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1920, 1080,
        nullptr, nullptr, hInstance, nullptr);
    ShowWindow(g_hWnd, nCmdShow);

    MSG msg;
    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // 명령어 뒤에 숫자를 붙여서 한 줄로 입력(i 10) 가능
        // 또는 명령어만 입력 후, 다음에 숫자만 입력하는 방식도 지원
        if (_kbhit()) {
            char ch = _getch();

            if (ch == '\r') { // 엔터
                if (!inputBuffer.empty()) {
                    // 입력 파싱
                    char cmd = inputBuffer[0];
                    int value = 0;
                    bool hasValue = false; // 숫자 입력 여부 체크

                    if (isdigit(cmd)) {
                        // 명령어 안 적었을 경우 → 이전 currentCmd 사용
                        value = stoi(inputBuffer);
                        cmd = currentCmd;
                        hasValue = true;
                    }
                    else {
                        // 명령어 적은 경우 (예: i 10)
                        if (inputBuffer.size() > 2)
                        {
                            value = stoi(inputBuffer.substr(2));
                            hasValue = true;
                        }
                        currentCmd = cmd; // 새 명령어 저장

                        // 모드 변경 안내
                        if (cmd == 'i') cout << "\n[삽입 모드 전환]" << "\n";
                        else if (cmd == 'd') cout << "\n[삭제 모드 전환]" << "\n";
                        else if (cmd == 's') cout << "\n[검색 모드 전환]" << "\n";
                    }

                    // 숫자가 있을 때만 실행
                    if (hasValue) {

                        if (cmd == 'i') {
                            g_tree.insert(value);
                            cout << "\n삽입: " << value << "\n";
                        }
                        else if (cmd == 'd') {
                            if (g_tree.remove(value))
                                cout << "\n삭제 성공: " << value << "\n";
                            else
                                cout << "\n삭제 실패 (존재하지 않음): " << value << "\n";
                        }
                        else if (cmd == 's') {
                            auto node = g_tree.search(value);
                            if (node)
                                cout << "검색 성공: " << value << "\n";
                            else
                                cout << "검색 실패: " << value << "\n";
                        }

                        InvalidateRect(g_hWnd, nullptr, TRUE);
                    }
                    inputBuffer.clear();
                    cout << "> ";
                }
            }
            else if (ch == '\b' && !inputBuffer.empty()) { // 백스페이스
                inputBuffer.pop_back();
                cout << "\b \b";
            }
            else {
                inputBuffer += ch;
                cout << ch;
            }
        }
    }
    return (int)msg.wParam;
}

// WndProc 구현
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_KEYDOWN:
    {
        if (wParam == 'Q')
            g_tree.printInorder();
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &ps.rcPaint, hBrush); // 화면을 흰색으로 채운다

        g_tree.draw(hdc);
        EndPaint(hWnd, &ps);
    } break;
    case WM_SIZE: {
        // 윈도우 크기가 변경되면 화면 전체를 무효화하여 WM_PAINT 메시지를 발생시킴
        InvalidateRect(hWnd, NULL, TRUE);
    } break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}