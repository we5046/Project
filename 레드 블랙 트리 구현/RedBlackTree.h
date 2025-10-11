#pragma once
#include <windows.h>
enum Color {
	BLACK = 0,
	RED
};

struct Node
{
	int Data;
	Color color;
	Node* parent;
	Node* left;
	Node* right;

	// MODIFIED: 생성자에 color 파라미터 추가 (원래는 기본으로 BLACK만 넣음 -> 혼동 소지)
	// 이제 노드 생성 시 명시적으로 색을 지정 가능하게 변경.
	Node(int k = 0, Color c = BLACK) : Data(k), color(c), left(nullptr), right(nullptr), parent(nullptr) {}
};


class RedBlackTree
{
private:
	Node* root;
	Node* NIL; // 모든 리프노드를 가리키는 sentinel 노드

	void rotateLeft(Node* x);
	void rotateRight(Node* x);
	void insertFixup(Node* x);

	// 삭제 관련 유틸
	void transplant(Node* u, Node* v);
	Node* treeMinimum(Node* x);
	void deleteFixup(Node* x);
	void drawTree(HDC hdc, Node* node, int x, int y, int xOffset);

public:
	RedBlackTree();
	~RedBlackTree();
	void insert(int key);
	bool remove(int key);
	Node* search(int key);
	void inorder(Node* x);
	Node* getRoot() { return root; }

	void printInorder();  // 콘솔 출력용
	void draw(HDC hdc);   // GUI 출력용
};