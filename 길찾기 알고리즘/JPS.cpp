#include "JPS.h"
#include <cmath>
#include <algorithm>
#include <climits>

JpsSolver::JpsSolver(int w, int h) : width(w), height(h) {}

JpsSolver::~JpsSolver() {
    for (auto& kv : openMap) delete kv.second;
}

Node* JpsSolver::createNode(int x, int y) {
    Node* node = new Node(x, y);
    openMap[{x, y}] = node;
    return node;
}

int JpsSolver::heuristic(Node* a, Node* b) {
    int dx = std::abs(a->x - b->x);
    int dy = std::abs(a->y - b->y);
    return 10 * (dx + dy); // 맨해튼 거리 (10 단위)
}

// 최종 경로 제작
std::vector<Node*> JpsSolver::reconstructPath(Node* goal) {
    std::vector<Node*> path;
    for (Node* n = goal; n != nullptr; n = n->parent) path.push_back(n);
    std::reverse(path.begin(), path.end());
    return path;
}

void JpsSolver::reset() {
    initialized = false;
}

// 헬퍼들
bool JpsSolver::isInside(int x, int y) const {
    return (x >= 0 && y >= 0 && x < width && y < height);
}
bool JpsSolver::isWalkable(int x, int y) const {
    if (!isInside(x, y)) return false;
    return (obstacles.count({ x, y }) == 0);
}

// ---------------- JPS 핵심 ----------------
std::pair<int, int> JpsSolver::jump(int x, int y, int dx, int dy) {
    int nx = x + dx;
    int ny = y + dy;

    if (!isInside(nx, ny) || !isWalkable(nx, ny))
        return { -1, -1 };

    if (nx == goalNodePtr->x && ny == goalNodePtr->y)
        return { nx, ny };

    // 대각선 이동
    if (dx != 0 && dy != 0) {
        if ((!isWalkable(nx - dx, ny) && isWalkable(nx - dx, ny + dy)) ||
            (!isWalkable(nx, ny - dy) && isWalkable(nx + dx, ny - dy))) {
            return { nx, ny };
        }
        // 대각선이면 직선 jump도 체크
        if (jump(nx, ny, dx, 0).first != -1) return { nx, ny };
        if (jump(nx, ny, 0, dy).first != -1) return { nx, ny };
    }
    else if (dx != 0) { // 수평
        if ((!isWalkable(nx, ny + 1) && isWalkable(nx + dx, ny + 1)) ||
            (!isWalkable(nx, ny - 1) && isWalkable(nx + dx, ny - 1))) {
            return { nx, ny };
        }
    }
    else if (dy != 0) { // 수직
        if ((!isWalkable(nx + 1, ny) && isWalkable(nx + 1, ny + dy)) ||
            (!isWalkable(nx - 1, ny) && isWalkable(nx - 1, ny + dy))) {
            return { nx, ny };
        }
    }

    // 계속 직진
    return jump(nx, ny, dx, dy);
}

std::vector<std::pair<int, int>> JpsSolver::pruneNeighbors(Node* node) {
    std::vector<std::pair<int, int>> dirs;

    if (!node->parent) {
        // 시작 노드는 8방향 다 가능
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                dirs.push_back({ dx, dy });
            }
        }
    }
    else {
        int dx = (node->x - node->parent->x);
        int dy = (node->y - node->parent->y);
        dx = (dx > 0 ? 1 : (dx < 0 ? -1 : 0));
        dy = (dy > 0 ? 1 : (dy < 0 ? -1 : 0));

        // 대각선 이동
        if (dx != 0 && dy != 0) {
            dirs.push_back({ dx, dy });
            dirs.push_back({ dx, 0 });
            dirs.push_back({ 0, dy });

            // forced neighbor 체크 (PathFinding.js 규칙)
            if (!isWalkable(node->x - dx, node->y) && isWalkable(node->x - dx, node->y + dy))
                dirs.push_back({ -dx, dy });
            if (!isWalkable(node->x, node->y - dy) && isWalkable(node->x + dx, node->y - dy))
                dirs.push_back({ dx, -dy });
        }
        // 수평 이동
        else if (dx != 0) {
            dirs.push_back({ dx, 0 });
            if (!isWalkable(node->x, node->y + 1) && isWalkable(node->x + dx, node->y + 1))
                dirs.push_back({ dx, 1 });
            if (!isWalkable(node->x, node->y - 1) && isWalkable(node->x + dx, node->y - 1))
                dirs.push_back({ dx, -1 });
        }
        // 수직 이동
        else if (dy != 0) {
            dirs.push_back({ 0, dy });
            if (!isWalkable(node->x + 1, node->y) && isWalkable(node->x + 1, node->y + dy))
                dirs.push_back({ 1, dy });
            if (!isWalkable(node->x - 1, node->y) && isWalkable(node->x - 1, node->y + dy))
                dirs.push_back({ -1, dy });
        }
    }
    return dirs;
}

void JpsSolver::identifySuccessors(Node* node) {
    auto dirs = pruneNeighbors(node);
    for (auto [dx, dy] : dirs) {

        // jump를 통해 다음 의사노드(보여줄 실제 노드) 찾기
        auto jp = jump (node->x, node->y, dx, dy);
        if (jp.first == -1) continue;

        int jx = jp.first, jy = jp.second;

        // 이미 closedList라면 무시
        if (closedList.count({ jx, jy })) continue;

        // 이동 거리 (점프 칸 수 × 비용)
        int steps = std::max(std::abs(jx - node->x), std::abs(jy - node->y));
        int stepCost = (dx != 0 && dy != 0) ? 14 : 10;
        int dist = steps * stepCost;
        int tentative_g = node->g + dist;

        if (!openMap.count({ jx, jy })) {
            Node* neighborNode = createNode(jx, jy);
            neighborNode->g = tentative_g;
            neighborNode->h = heuristic(neighborNode, goalNodePtr);
            neighborNode->parent = node;
            openList.push(neighborNode);
        }
        else {
            Node* neighborNode = openMap[{ jx, jy}];
            if (tentative_g < neighborNode->g) {
                neighborNode->g = tentative_g;
                neighborNode->parent = node;
                openList.push(neighborNode);
            }
        }
    }
}

std::vector<Node*> JpsSolver::findPath(int sx, int sy, int gx, int gy) {
    if (!initialized) {
        // 초기화: 이전 메모리 정리
        while (!openList.empty()) openList.pop();
        for (auto& kv : openMap) delete kv.second;
        openMap.clear();
        closedList.clear();

        // 시작/목표 노드 생성
        startNodePtr = createNode(sx, sy);
        goalNodePtr = createNode(gx, gy);

        startNodePtr->g = 0;
        startNodePtr->h = heuristic(startNodePtr, goalNodePtr);
        openList.push(startNodePtr);
        initialized = true;
    }

    if (openList.empty()) {
        initialized = false;
        return { nullptr }; // 실패
    }

    Node* current = openList.top();
    openList.pop();

    // 이미 closed에 있으면 skip (우선순위 큐 중복 아이템 방지)
    if (closedList.count({ current->x, current->y })) {
        return {}; // 계속 탐색
    }

    // 목표 도달
    if (current->x == goalNodePtr->x && current->y == goalNodePtr->y) {
        initialized = false;
        return reconstructPath(current);
    }

    closedList.insert({ current->x, current->y });
    identifySuccessors(current);
    return {}; // 아직 탐색 중
}
