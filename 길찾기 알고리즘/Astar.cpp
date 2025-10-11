#include "AStar.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <climits>

// ---------------- Node 구현 ----------------
Node::Node(int x_, int y_) : x(x_), y(y_), g(INT_MAX), h(0), parent(nullptr) {}
int Node::f() const { return g + h; }

// ---------------- PairHash 구현 ----------------
size_t PairHash::operator()(const std::pair<int, int>& p) const {
    return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
}

// ---------------- CompareNode ----------------
bool AStarSolver::CompareNode::operator()(Node* a, Node* b) {
    if (a->f() == b->f()) {
        return a->h > b->h; // F 같으면 H 작은 쪽을 우선
    }
    return a->f() > b->f();
}

// ---------------- AStarSolver ----------------
AStarSolver::AStarSolver(int w, int h) : width(w), height(h) {}

AStarSolver::~AStarSolver() {
    for (auto& kv : openMap) {
        delete kv.second;
    }
}

Node* AStarSolver::createNode(int x, int y) {
    Node* node = new Node(x, y);
    openMap[{x, y}] = node;
    return node;
}

int AStarSolver::heuristic(Node* a, Node* b) {
    // 맨해튼 거리 (직선=10 단위 사용)
    int dx = std::abs(a->x - b->x);
    int dy = std::abs(a->y - b->y);
    return 10 * (dx + dy);
}

std::vector<std::pair<int, int>> AStarSolver::getNeighbors(Node* node) {
    return {
        {node->x + 1, node->y}, {node->x - 1, node->y},
        {node->x, node->y + 1}, {node->x, node->y - 1},
        {node->x + 1, node->y + 1}, {node->x - 1, node->y + 1},
        {node->x + 1, node->y - 1}, {node->x - 1, node->y - 1}
    };
}

std::vector<Node*> AStarSolver::reconstructPath(Node* goal) {
    std::vector<Node*> path;
    for (Node* n = goal; n != nullptr; n = n->parent) {
        path.push_back(n);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

void AStarSolver::reset()
{
    initialized = false;
}

std::vector<Node*> AStarSolver::findPath(int sx, int sy, int gx, int gy) {
    // 초기화 (첫 실행일 때만)
    if (!initialized) {
        while (!openList.empty()) openList.pop();
        for (auto& kv : openMap) delete kv.second;
        openMap.clear();
        closedList.clear();

        startNodePtr = createNode(sx, sy);
        goalNodePtr = createNode(gx, gy);

        startNodePtr->g = 0;
        startNodePtr->h = heuristic(startNodePtr, goalNodePtr);
        openList.push(startNodePtr);
        openMap[{sx, sy}] = startNodePtr;

        initialized = true;
    }

    // === 종료 조건 1: 더 이상 탐색할 게 없음 (실패) ===
    if (openList.empty()) {
        initialized = false;
        return { nullptr };  // 실패 표시용 (빈 {}와 구분)
    }

    Node* current = openList.top();
    openList.pop();

    // === 종료 조건 2: 목표 도착 (성공) ===
    if (current->x == goalNodePtr->x && current->y == goalNodePtr->y) {
        initialized = false;
        return reconstructPath(current); // 경로 반환
    }

    // === 탐색 진행 ===
    closedList.insert({ current->x, current->y });

    for (auto neighbor : getNeighbors(current)) {

        int nx = neighbor.first;
        int ny = neighbor.second;

        if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
        if (closedList.count({ nx, ny })) continue;
        if (obstacles.count({ nx, ny })) continue;

        int moveCost = (nx == current->x || ny == current->y) ? 10 : 14;
        int tentative_g = current->g + moveCost;

        if (!openMap.count({ nx, ny })) {
            Node* neighborNode = createNode(nx, ny);
            neighborNode->g = tentative_g;
            neighborNode->h = heuristic(neighborNode, goalNodePtr);
            neighborNode->parent = current;
            openList.push(neighborNode);
            openMap[{nx, ny}] = neighborNode;
        }
        else {
            Node* neighborNode = openMap[{nx, ny}];
            if (tentative_g < neighborNode->g) {
                neighborNode->g = tentative_g;
                neighborNode->parent = current;
                openList.push(neighborNode);
            }
        }
    }

    return {}; // 아직 탐색 중
}