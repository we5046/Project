#pragma once
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

// ---------------- Node Á¤ÀÇ ----------------
struct Node {
    int x, y;
    int g, h;
    Node* parent;

    Node(int x_, int y_);
    int f() const;
};

// ---------------- ÁÂÇ¥ ÇØ½Ã ----------------
struct PairHash {
    size_t operator()(const std::pair<int, int>& p) const;
};

// ---------------- A* Solver ----------------
class AStarSolver {
private:
    struct CompareNode {
        bool operator()(Node* a, Node* b);
    };

    int width, height;

    Node* createNode(int x, int y);
    int heuristic(Node* a, Node* b);
    std::vector<std::pair<int, int>> getNeighbors(Node* node);
    std::vector<Node*> reconstructPath(Node* goal);

public:
    std::unordered_map<std::pair<int, int>, Node*, PairHash> openMap;
    std::unordered_set<std::pair<int, int>, PairHash> obstacles;
    std::unordered_set<std::pair<int, int>, PairHash> closedList;
    std::priority_queue<Node*, std::vector<Node*>, CompareNode> openList;

    AStarSolver(int w, int h);
    ~AStarSolver();

    std::vector<Node*> findPath(int sx, int sy, int gx, int gy);

    void reset();

    // ¸â¹ö º¯¼ö
    Node* startNodePtr = nullptr;
    Node* goalNodePtr = nullptr;
    bool initialized = false;

};
