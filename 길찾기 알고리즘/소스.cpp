#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include <unordered_set>
#include "Astar.h"
#include "JPS.h"

#define GRID_ROWS 18 * 2
#define GRID_COLS 24 * 2
int CELL_SIZE = 40;  // 확대/축소 가능

int offsetX = 0, offsetY = 0;
bool isDragging = false;
POINT lastMousePos = { 0,0 };

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void UpdateScrollBars(HWND hWnd, int clientW, int clientH)
{
    SCROLLINFO si = { sizeof(si) };
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

    // 가로 스크롤
    si.nMin = 0;
    si.nMax = GRID_COLS * CELL_SIZE;
    si.nPage = clientW;
    if (offsetX > si.nMax - (int)si.nPage) offsetX = si.nMax - si.nPage;
    if (offsetX < 0) offsetX = 0;
    si.nPos = offsetX;
    SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

    // 세로 스크롤
    si.nMin = 0;
    si.nMax = GRID_ROWS * CELL_SIZE;
    si.nPage = clientH;
    if (offsetY > si.nMax - (int)si.nPage) offsetY = si.nMax - si.nPage;
    if (offsetY < 0) offsetY = 0;
    si.nPos = offsetY;
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"AStarWin";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW); // 기본 화살표 커서 지정

    RegisterClass(&wc);

    HWND hWnd = CreateWindowEx(
        0, L"AStarWin", L"A* Pathfinding Grid",
        WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

// 시작 노드, 도착 노드 (화면에 보일)
Node startNode{ 2, 1 };
Node goalNode = { GRID_COLS / 2, 1 };
bool draggingStart = false;
bool draggingGoal = false;

// 장애물
AStarSolver solver(GRID_COLS, GRID_ROWS);
//JpsSolver solver(GRID_COLS, GRID_ROWS);
bool isDrawingObstacle = false;  // 드래그로 장애물 추가 중인지
bool isErasingObstacle = false;  // 드래그로 장애물 제거 중인지

// 경로탐색
std::vector<Node*> currentPath;
bool isSolving = false;

// 퍼즈 기능
bool isPaused = false;

int speed = 400;
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        // WM_KEYDOWN
    case WM_KEYDOWN:
        if (wParam == 'G' && !isSolving) {
            currentPath.clear();
            solver.reset();  // 내부 상태 초기화
            isSolving = true;
            isPaused = false;                 // 시작 시 일시정지 해제
            SetTimer(hWnd, 1, speed, NULL); //  step 진행 속도
        }
        else if (wParam == VK_SPACE && isSolving) {
            isPaused = !isPaused;             // 토글
            if (isPaused) {
                KillTimer(hWnd, 1);           // 멈춤
            }
            else {
                SetTimer(hWnd, 1, speed, NULL);  // 재개
            }
        }
        else if (wParam == 'C') {
            // 전체 초기화: 경로, 방문 흔적, 장애물, 탐색 상태 모두 리셋
            currentPath.clear();
            solver.reset();
            solver.obstacles.clear();
            solver.openMap.clear();
            solver.closedList.clear();
            isSolving = false;
            isPaused = false;
            KillTimer(hWnd, 1);
            InvalidateRect(hWnd, NULL, TRUE);
            
        }
        else if (wParam == VK_OEM_PLUS)
        {
            speed -= 10;
            InvalidateRect(hWnd, NULL, TRUE);   // 클라이언트 영역 전체를 무효화 (배경 지우고 다시 그림)
            UpdateWindow(hWnd);                 // 바로 WM_PAINT 메시지를 처리하게 함
        }
        else if (wParam == VK_OEM_MINUS)
        {
            speed += 10;
            InvalidateRect(hWnd, NULL, TRUE);   // 클라이언트 영역 전체를 무효화 (배경 지우고 다시 그림)
            UpdateWindow(hWnd);                 // 바로 WM_PAINT 메시지를 처리하게 함
        }
        break;

        // WM_TIMER
    case WM_TIMER: {

        auto result = solver.findPath(startNode.x, startNode.y, goalNode.x, goalNode.y);

        if (!result.empty()) {
            if (result[0] == nullptr) {
                // 탐색 실패
                KillTimer(hWnd, 1);
                MessageBox(hWnd, L"경로를 찾을 수 없습니다!", L"A*", MB_OK | MB_ICONWARNING);
                solver.reset();
            }
            else {
                // 탐색 성공
                currentPath = result;
            }

            isSolving = false;
            KillTimer(hWnd, 1);
        }

        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case WM_RBUTTONDOWN: {
        int mouseX = GET_X_LPARAM(lParam);
        int mouseY = GET_Y_LPARAM(lParam);

        int col = (mouseX + offsetX) / CELL_SIZE;
        int row = (mouseY + offsetY) / CELL_SIZE;

        if ((col == startNode.x && row == startNode.y) ||
            (col == goalNode.x && row == goalNode.y)) {
            break;
        }

        auto pos = std::make_pair(col, row);

        if (GetKeyState(VK_SHIFT) & 0x8000) {
            // Shift 눌림 → 무조건 지우기 모드
            solver.obstacles.erase(pos);
            isErasingObstacle = true;
            isDrawingObstacle = false;
        }
        else if (solver.obstacles.count(pos)) {
            // 이미 장애물이 있으면 지우기
            solver.obstacles.erase(pos);
            isErasingObstacle = true;
            isDrawingObstacle = false;
        }
        else {
            // 없으면 그리기
            solver.obstacles.insert(pos);
            isDrawingObstacle = true;
            isErasingObstacle = false;
        }

        if (isSolving && !isPaused) {
            isPaused = true;
            KillTimer(hWnd, 1);
        }

        InvalidateRect(hWnd, NULL, FALSE);
        SetCapture(hWnd); // 드래그 시작
        break;
    }
    case WM_LBUTTONDOWN: {
        int mouseX = GET_X_LPARAM(lParam);
        int mouseY = GET_Y_LPARAM(lParam);

        int col = (mouseX + offsetX) / CELL_SIZE;
        int row = (mouseY + offsetY) / CELL_SIZE;

        if (col == startNode.x && row == startNode.y) {
            draggingStart = true;
        }
        else if (col == goalNode.x && row == goalNode.y) {
            draggingGoal = true;
        }
        SetCapture(hWnd); // 마우스 캡처
        break;
    }
    case WM_MOUSEMOVE: {
        if (wParam & MK_LBUTTON) {
            int mouseX = GET_X_LPARAM(lParam);
            int mouseY = GET_Y_LPARAM(lParam);

            int col = (mouseX + offsetX) / CELL_SIZE;
            int row = (mouseY + offsetY) / CELL_SIZE;

            // 격자 범위 체크
            if (col >= 0 && col < GRID_COLS && row >= 0 && row < GRID_ROWS) {
                if (draggingStart) {
                    if (!(col == goalNode.x && row == goalNode.y)) { // Goal과 겹치면 무시
                        startNode.x = col;
                        startNode.y = row;
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                }
                else if (draggingGoal) {
                    if (!(col == startNode.x && row == startNode.y)) { // Start와 겹치면 무시
                        goalNode.x = col;
                        goalNode.y = row;
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                }
            }
        }
        else if (wParam & MK_RBUTTON) {
        int mouseX = GET_X_LPARAM(lParam);
        int mouseY = GET_Y_LPARAM(lParam);

        int col = (mouseX + offsetX) / CELL_SIZE;
        int row = (mouseY + offsetY) / CELL_SIZE;

        if (col < 0 || row < 0 || col >= GRID_COLS || row >= GRID_ROWS) break;
        if ((col == startNode.x && row == startNode.y) ||
            (col == goalNode.x && row == goalNode.y)) {
            break;
        }

        auto pos = std::make_pair(col, row);

        if (isDrawingObstacle) {
            solver.obstacles.insert(pos);
        } else if (isErasingObstacle) {
            solver.obstacles.erase(pos);
        }

        // 탐색 중이면 강제 일시정지
        if (isSolving && !isPaused) {
            isPaused = true;
            KillTimer(hWnd, 1);
        }

        InvalidateRect(hWnd, NULL, FALSE);
        }   
        break;
    }

    case WM_LBUTTONUP: {
        draggingStart = false;
        draggingGoal = false;
        ReleaseCapture();
        break;
    }
    case WM_SIZE: {
        int clientW = LOWORD(lParam);
        int clientH = HIWORD(lParam);
        UpdateScrollBars(hWnd, clientW, clientH);
        InvalidateRect(hWnd, nullptr, TRUE);
        break;
    }
    case WM_HSCROLL: {
        SCROLLINFO si = { sizeof(si), SIF_ALL };
        GetScrollInfo(hWnd, SB_HORZ, &si);
        int pos = si.nPos;

        switch (LOWORD(wParam)) {
        case SB_LINELEFT: si.nPos -= 10; break;
        case SB_LINERIGHT: si.nPos += 10; break;
        case SB_PAGELEFT: si.nPos -= si.nPage; break;
        case SB_PAGERIGHT: si.nPos += si.nPage; break;
        case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
        }
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
        GetScrollInfo(hWnd, SB_HORZ, &si);
        offsetX = si.nPos;
        InvalidateRect(hWnd, nullptr, TRUE);
        break;
    }
    case WM_VSCROLL: {
        SCROLLINFO si = { sizeof(si), SIF_ALL };
        GetScrollInfo(hWnd, SB_VERT, &si);
        int pos = si.nPos;

        switch (LOWORD(wParam)) {
        case SB_LINEUP: si.nPos -= 10; break;
        case SB_LINEDOWN: si.nPos += 10; break;
        case SB_PAGEUP: si.nPos -= si.nPage; break;
        case SB_PAGEDOWN: si.nPos += si.nPage; break;
        case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
        }
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
        GetScrollInfo(hWnd, SB_VERT, &si);
        offsetY = si.nPos;
        InvalidateRect(hWnd, nullptr, TRUE);
        break;
    }
    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0 && CELL_SIZE < 100) CELL_SIZE += 2;  // 확대
        else if (delta < 0 && CELL_SIZE > 4) CELL_SIZE -= 2; // 축소

        RECT rc;
        GetClientRect(hWnd, &rc);
        UpdateScrollBars(hWnd, rc.right - rc.left, rc.bottom - rc.top);

        InvalidateRect(hWnd, nullptr, TRUE);
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rc;
        GetClientRect(hWnd, &rc);

        // 왼쪽 격자 영역 (전체의 80%)
        int gridWidth = (rc.right - rc.left) * 0.8;
        RECT gridArea = { rc.left, rc.top, rc.left + gridWidth, rc.bottom };

        // 오른쪽 안내 패널 영역 (20%)
        RECT infoArea = { rc.left + gridWidth, rc.top, rc.right, rc.bottom };

        // ===== 배경 =====
        FillRect(hdc, &gridArea, (HBRUSH)(COLOR_WINDOW + 1));

        // ===== 상태 있는 셀 =====
        HBRUSH startBrush = CreateSolidBrush(RGB(0, 128, 0));     // 진한 초록
        HBRUSH goalBrush = CreateSolidBrush(RGB(200, 0, 0));     // 빨강
        HBRUSH obstacleBrush = CreateSolidBrush(RGB(0, 0, 0));       // 검정
        HBRUSH openBrush = CreateSolidBrush(RGB(144, 238, 144)); // 연두
        HBRUSH closedBrush = CreateSolidBrush(RGB(173, 216, 230)); // 하늘색
        HBRUSH pathBrush = CreateSolidBrush(RGB(128, 128, 0));   // 노랑

        auto fillCell = [&](int x, int y, HBRUSH brush) {
            RECT r = {
                x * CELL_SIZE - offsetX,
                y * CELL_SIZE - offsetY,
                (x + 1) * CELL_SIZE - offsetX,
                (y + 1) * CELL_SIZE - offsetY
            };
            FillRect(hdc, &r, brush);
            };
        // closed list (확정)
        for (auto& c : solver.closedList) {
            fillCell(c.first, c.second, closedBrush);
        }

        // open list (후보) → openMap 사용
        for (auto& kv : solver.openMap) {
            auto coord = kv.first;
            if (solver.closedList.count(coord)) continue; // closed 우선
            fillCell(coord.first, coord.second, openBrush);
        }

        // path (최적 경로, 노랑)
        for (auto& p : currentPath) {
            fillCell(p->x, p->y, pathBrush);
        }

        // start, goal (최우선)
        fillCell(startNode.x, startNode.y, startBrush);
        fillCell(goalNode.x, goalNode.y, goalBrush);

        // ===== 부모 방향 화살표 (빨강, 얇은 선) =====
        HPEN redPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
        HPEN oldPen = (HPEN)SelectObject(hdc, redPen);

        for (auto& kv : solver.openMap) {
            Node* node = kv.second;
            if (!node->parent) continue;

            int cx = node->x * CELL_SIZE + CELL_SIZE / 2 - offsetX;
            int cy = node->y * CELL_SIZE + CELL_SIZE / 2 - offsetY;
            int px = node->parent->x * CELL_SIZE + CELL_SIZE / 2 - offsetX;
            int py = node->parent->y * CELL_SIZE + CELL_SIZE / 2 - offsetY;

            MoveToEx(hdc, cx, cy, NULL);
            LineTo(hdc, px, py);

            // 화살촉
            double angle = atan2(py - cy, px - cx);
            int len = 6;
            POINT arrow[3] = {
                { px, py },
                { (int)(px - len * cos(angle - 3.14159 / 6)), (int)(py - len * sin(angle - 3.14159 / 6)) },
                { (int)(px - len * cos(angle + 3.14159 / 6)), (int)(py - len * sin(angle + 3.14159 / 6)) }
            };
            Polygon(hdc, arrow, 3);
        }

        SelectObject(hdc, oldPen);
        DeleteObject(redPen);

        // ===== 최적 경로 굵은 노란 선 =====
        if (!currentPath.empty()) {
            HPEN yellowPen = CreatePen(PS_SOLID, 3, RGB(255, 255, 0));
            oldPen = (HPEN)SelectObject(hdc, yellowPen);

            for (size_t i = 1; i < currentPath.size(); i++) {
                int x1 = currentPath[i - 1]->x * CELL_SIZE + CELL_SIZE / 2 - offsetX;
                int y1 = currentPath[i - 1]->y * CELL_SIZE + CELL_SIZE / 2 - offsetY;
                int x2 = currentPath[i]->x * CELL_SIZE + CELL_SIZE / 2 - offsetX;
                int y2 = currentPath[i]->y * CELL_SIZE + CELL_SIZE / 2 - offsetY;

                MoveToEx(hdc, x1, y1, NULL);
                LineTo(hdc, x2, y2);
            }

            SelectObject(hdc, oldPen);
            DeleteObject(yellowPen);
        }

        // 장애물
        for (auto& obs : solver.obstacles) {
            fillCell(obs.first, obs.second, obstacleBrush);
        }

        // === 확대된 경우 노드 G/H/F 값 출력 ===
        if (CELL_SIZE >= 30) {
            HFONT hFont = CreateFont(
                CELL_SIZE / 3, 0, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_SWISS, L"Consolas"
            );
            HFONT oldFont3 = (HFONT)SelectObject(hdc, hFont);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 0));

            for (auto& kv : solver.openMap) {
                Node* n = kv.second;

                int left = n->x * CELL_SIZE - offsetX;
                int top = n->y * CELL_SIZE - offsetY;

                wchar_t buf[32];
                RECT rc;

                // G (좌상단)
                wsprintf(buf, L"G:%d", n->g);
                rc = { left + 2, top + 2, left + CELL_SIZE, top + CELL_SIZE };
                DrawText(hdc, buf, -1, &rc, DT_LEFT | DT_TOP | DT_SINGLELINE);

                // H (좌측 중단)
                wsprintf(buf, L"H:%d", n->h);
                rc = { left + 2, top, left + CELL_SIZE, top + CELL_SIZE };
                DrawText(hdc, buf, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                // F (좌하단)
                wsprintf(buf, L"F:%d", n->f());
                rc = { left + 2, top, left + CELL_SIZE, top + CELL_SIZE - 2 };
                DrawText(hdc, buf, -1, &rc, DT_LEFT | DT_BOTTOM | DT_SINGLELINE);
            }

            SelectObject(hdc, oldFont3);
            DeleteObject(hFont);
        }

        DeleteObject(startBrush);
        DeleteObject(goalBrush);
        DeleteObject(obstacleBrush);
        DeleteObject(openBrush);
        DeleteObject(closedBrush);
        DeleteObject(pathBrush);

        // ====== 마지막에 격자 그리기 ======
        int startCol = offsetX / CELL_SIZE;
        int startRow = offsetY / CELL_SIZE;
        int endCol = (offsetX + (gridArea.right - gridArea.left)) / CELL_SIZE + 1;
        int endRow = (offsetY + (gridArea.bottom - gridArea.top)) / CELL_SIZE + 1;

        if (endCol > GRID_COLS) endCol = GRID_COLS;
        if (endRow > GRID_ROWS) endRow = GRID_ROWS;

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        HPEN oldPen2 = (HPEN)SelectObject(hdc, hPen);

        for (int y = startRow; y < endRow; y++) {
            for (int x = startCol; x < endCol; x++) {
                MoveToEx(hdc, x * CELL_SIZE - offsetX, y * CELL_SIZE - offsetY, NULL);
                LineTo(hdc, (x + 1) * CELL_SIZE - offsetX, y * CELL_SIZE - offsetY);
                LineTo(hdc, (x + 1) * CELL_SIZE - offsetX, (y + 1) * CELL_SIZE - offsetY);
                LineTo(hdc, x * CELL_SIZE - offsetX, (y + 1) * CELL_SIZE - offsetY);
                LineTo(hdc, x * CELL_SIZE - offsetX, y * CELL_SIZE - offsetY);
            }
        }

        SelectObject(hdc, oldPen2);
        DeleteObject(hPen);

        // ====== 안내 패널 ======
        HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240));
        FillRect(hdc, &infoArea, hBrush);
        DeleteObject(hBrush);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));
        DrawText(hdc, L"기능 안내", -1, &infoArea, DT_TOP | DT_CENTER);

        RECT textRect = infoArea;
        textRect.top += 40;
        DrawText(hdc, L"G 키: 탐색 실행", -1, &textRect, DT_TOP | DT_CENTER);

        textRect.top += 30;
        DrawText(hdc, L"드래그: 시작/도착 이동", -1, &textRect, DT_TOP | DT_CENTER);

        textRect.top += 30;
        DrawText(hdc, L"우클릭: 장애물 추가/삭제", -1, &textRect, DT_TOP | DT_CENTER);

        textRect.top += 30;
        DrawText(hdc, L"휠: 확대/축소", -1, &textRect, DT_TOP | DT_CENTER);

        textRect.top += 30;
        DrawText(hdc, L"C 키: 리셋", -1, &textRect, DT_TOP | DT_CENTER);

        textRect.top += 30;
        DrawText(hdc, L"SPACE 키: 일시 정지", -1, &textRect, DT_TOP | DT_CENTER);

        textRect.top += 30;
        DrawText(hdc, L"+/- 키: 탐색 속도 조절", -1, &textRect, DT_TOP | DT_CENTER);


        {
            wchar_t buf[128];
            if (speed >= 1000)
                speed = 1000;
            else if (speed <= 10)
            {
                speed = 10;
            }
            wsprintf(buf, L"현재 탐색 속도: %d.%d", 1000/speed, 1000%speed);
            textRect.top += 30;
            DrawText(hdc, buf, -1, &textRect, DT_TOP | DT_CENTER);
        }

        // ==== 탐색 결과 ====
        if (!currentPath.empty()) {
            wchar_t buf[128];
            int pathLength = (int)currentPath.back()->g;

            wsprintf(buf, L"경로 길이: %d.%d", pathLength / 10, pathLength % 10);
            textRect.top += 50;
            DrawText(hdc, buf, -1, &textRect, DT_TOP | DT_CENTER);
        }
        EndPaint(hWnd, &ps);
        break;
    }





    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
