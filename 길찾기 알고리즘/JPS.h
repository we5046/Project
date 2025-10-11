#pragma once
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include "Astar.h" // Node, PairHash 정의 사용

class JpsSolver {
private:
    struct CompareNode {
        bool operator()(Node* a, Node* b) {
            if (a->f() == b->f()) return a->h > b->h;
            return a->f() > b->f();
        }
    };

    int width, height;
    Node* createNode(int x, int y);
    int heuristic(Node* a, Node* b);
    std::pair<int, int> jump(int x, int y, int dx, int dy);
    std::vector<std::pair<int, int>> pruneNeighbors(Node* node);
    void identifySuccessors(Node* node);
    std::vector<Node*> reconstructPath(Node* goal);

    // 헬퍼
    bool isInside(int x, int y) const;
    bool isWalkable(int x, int y) const;

public:
    std::unordered_map<std::pair<int, int>, Node*, PairHash> openMap;       //priority_queue로 특정 노드 좌표 찾기어려워서 빠르게찾을 수있는 좌표 테이블
    std::unordered_set<std::pair<int, int>, PairHash> closedList;           // openList에서 꺼낸 최단 거리 노드 저장
    std::unordered_set<std::pair<int, int>, PairHash> obstacles;            // 장애물 좌표 모음
    std::priority_queue<Node*, std::vector<Node*>, CompareNode> openList;   //가장 낮은 f값을 찾기위한 우선순위 큐 삽입/삭제 O(log n), 최소값 O(1)

    JpsSolver(int w, int h);
    ~JpsSolver();

    std::vector<Node*> findPath(int sx, int sy, int gx, int gy);
    void reset();

    Node* startNodePtr = nullptr;
    Node* goalNodePtr = nullptr;
    bool initialized = false;
};
