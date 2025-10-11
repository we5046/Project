#include "RedBlackTree.h"
#include <iostream>
// 좌회전
void RedBlackTree::rotateLeft(Node* x) {
	Node* y = x->right;
	if (y == nullptr) return;

	x->right = y->left;
	if (y->left != NIL) {
		y->left->parent = x;
	}
	y->parent = x->parent;
	if (x->parent == NIL) {
		root = y;
	}
	else if (x == x->parent->left) {
		x->parent->left = y;
	}
	else {
		x->parent->right = y;
	}
	y->left = x;
	x->parent = y;
}

// 우회전
void RedBlackTree::rotateRight(Node* x) {
	Node* y = x->left;
	if (y == nullptr) return;

	x->left = y->right;
	if (y->right != NIL) {
		y->right->parent = x;
	}
	y->parent = x->parent;
	if (x->parent == NIL) {
		root = y;
	}
	else if (x == x->parent->right) {
		x->parent->right = y;
	}
	else {
		x->parent->left = y;
	}
	y->right = x;
	x->parent = y;
}

// 삽입 후 색/구조 복구
void RedBlackTree::insertFixup(Node* z) {
	while (z->parent->color == RED) {
		if (z->parent == z->parent->parent->left) {
			Node* y = z->parent->parent->right; // 삼촌
			if (y->color == RED) {
				// Case 1: 부모와 삼촌이 RED
				z->parent->color = BLACK;
				y->color = BLACK;
				z->parent->parent->color = RED;
				z = z->parent->parent;
			}
			else {
				if (z == z->parent->right) {
					// Case 2: z가 부모의 오른쪽 자식
					z = z->parent;
					rotateLeft(z);
				}
				// Case 3:
				z->parent->color = BLACK;
				z->parent->parent->color = RED;
				rotateRight(z->parent->parent);
			}
		}
		else {
			// 대칭 케이스: 부모가 조부모의 오른쪽 자식인 경우
			Node* y = z->parent->parent->left; // 삼촌
			if (y->color == RED) {
				// Case 1
				z->parent->color = BLACK;
				y->color = BLACK;
				z->parent->parent->color = RED;
				z = z->parent->parent;
			}
			else {
				if (z == z->parent->left) {
					// Case 2
					z = z->parent;
					rotateRight(z);
				}
				// Case 3
				z->parent->color = BLACK;
				z->parent->parent->color = RED;
				rotateLeft(z->parent->parent);
			}
		}
	}
	root->color = BLACK; // 항상 루트는 BLACK
}

// transplant: u를 v로 교체 (u의 parent가 NIL이면 루트 갱신)
void RedBlackTree::transplant(Node* u, Node* v) {
	if (u->parent == NIL) root = v;
	else if (u == u->parent->left) u->parent->left = v;
	else u->parent->right = v;
	// MODIFIED: v가 NIL일지라도 NIL->parent가 항상 유효하도록 NIL->parent를 설정해둠
	v->parent = u->parent;
}

// tree minimum
Node* RedBlackTree::treeMinimum(Node* x) {
	while (x->left != NIL) x = x->left;
	return x;
}

// deleteFixup: x는 replace된 노드 (NIL 가능). y_original_color이 BLACK이면 호출
void RedBlackTree::deleteFixup(Node* x) {
	while (x != root && x->color == BLACK) {
		if (x == x->parent->left) {
			Node* w = x->parent->right; // 형제
			// Case 2.2: 형제가 RED
			if (w->color == RED) {
				w->color = BLACK;
				x->parent->color = RED;
				rotateLeft(x->parent);
				w = x->parent->right;
			}
			// Case 2.3: 형제가 BLACK이고 형제의 왼/오 모두 BLACK
			if (w->left->color == BLACK && w->right->color == BLACK) {
				w->color = RED;
				x = x->parent;
			}
			else {
				// Case 2.4: 형제가 BLACK이고 형제의 오른쪽이 BLACK, 왼쪽이 RED
				if (w->right->color == BLACK) {
					w->left->color = BLACK;
					w->color = RED;
					rotateRight(w);
					w = x->parent->right;
				}
				// Case 2.5: 형제가 BLACK이고 형제의 오른쪽이 RED
				w->color = x->parent->color;
				x->parent->color = BLACK;
				w->right->color = BLACK;
				rotateLeft(x->parent);
				x = root;
			}
		}
		else {
			// 대칭 케이스
			Node* w = x->parent->left;
			if (w->color == RED) {
				w->color = BLACK;
				x->parent->color = RED;
				rotateRight(x->parent);
				w = x->parent->left;
			}
			if (w->right->color == BLACK && w->left->color == BLACK) {
				w->color = RED;
				x = x->parent;
			}
			else {
				if (w->left->color == BLACK) {
					w->right->color = BLACK;
					w->color = RED;
					rotateLeft(w);
					w = x->parent->left;
				}
				w->color = x->parent->color;
				x->parent->color = BLACK;
				w->left->color = BLACK;
				rotateRight(x->parent);
				x = root;
			}
		}
	}
	x->color = BLACK;
}

RedBlackTree::RedBlackTree()
{
	// MODIFIED: NIL sentinel을 자기 자신을 가리키도록 설정 (원래: nullptr)
	// 이렇게 하면 deleteFixup 등에서 NIL->parent 접근 시 nullptr deref 방지
	NIL = new Node(0, BLACK);
	NIL->left = NIL->right = NIL->parent = NIL;
	root = NIL;
}

// 소멸자: 간단히 노드들을 순회하면서 삭제 (재귀)
// MODIFIED: deleteSubtree에서 NILnode는 삭제하지 않도록 명확히 처리
static void deleteSubtree(Node* x, Node* NILnode) {
	if (x == nullptr || x == NILnode) return;
	deleteSubtree(x->left, NILnode);
	deleteSubtree(x->right, NILnode);
	delete x;
}

RedBlackTree::~RedBlackTree()
{
	// MODIFIED: root가 NIL이면 아무것도 안함 (NIL은 마지막에 따로 삭제)
	deleteSubtree(root, NIL);
	delete NIL;
}

// public 삽입 함수: BST 삽입 + fixup
void RedBlackTree::insert(int key) {
	Node* z = new Node(key);
	z->left = z->right = z->parent = NIL;
	z->color = RED;

	Node* y = NIL;
	Node* x = root;

	// BST 삽입 위치 탐색
	while (x != NIL) {
		y = x;
		if (z->Data == x->Data) {
			// 이미 존재하면 삽입하지 않고 종료
			delete z;
			return;
		}
		else if (z->Data < x->Data) x = x->left;
		else x = x->right;
	}

	z->parent = y;
	if (y == NIL) {
		root = z; // 트리가 비어있음
	}
	else if (z->Data < y->Data) {
		y->left = z;
	}
	else {
		y->right = z;
	}

	z->left = NIL;
	z->right = NIL;
	z->color = RED;

	// 색/구조 복구
	insertFixup(z);
}

// remove: key가 존재하면 삭제 후 true 반환, 없으면 false
bool RedBlackTree::remove(int key) {
	// 먼저 삭제 대상 노드 z 찾기 (BST 검색)
	Node* z = root;
	while (z != NIL && z->Data != key) {
		if (key < z->Data) z = z->left;
		else z = z->right;
	}
	if (z == NIL) return false; // 존재하지 않음

	Node* y = z;
	Color y_original_color = y->color;
	Node* x = nullptr;

	// z의 자식이 하나 이하인지, 두 개인지 처리
	if (z->left == NIL) {
		x = z->right;
		transplant(z, z->right);
	}
	else if (z->right == NIL) {
		x = z->left;
		transplant(z, z->left);
	}
	else {
		// z의 자식이 둘일 때: z의 오른쪽 서브트리의 최소를 찾아 y로 삼음
		y = treeMinimum(z->right);
		y_original_color = y->color;
		x = y->right;
		if (y->parent == z) {
			// MODIFIED: x가 NIL일 수 있으므로 NIL의 parent도 정확히 설정
			x->parent = y;
		}
		else {
			transplant(y, y->right);
			y->right = z->right;
			y->right->parent = y;
		}
		transplant(z, y);
		y->left = z->left;
		y->left->parent = y;
		y->color = z->color;
	}

	delete z;

	if (y_original_color == BLACK) {
		// 삭제로 블랙 균형이 깨졌으면 보정
		deleteFixup(x);
	}
	return true;
}

// 검색
Node* RedBlackTree::search(int key) {
	Node* x = root;
	std::cout << "\n" << "검색 경로: ";
	while (x != NIL) {
		std::cout << x->Data << " ";
		if (key == x->Data) {
			std::cout << "\n";
			return x;
		}
		if (key < x->Data) x = x->left;
		else x = x->right;
	}
	std::cout << "\n";
	return nullptr;
}


// 중위 순회 (키 출력)
void RedBlackTree::inorder(Node* x) {
	if (x == NIL) return;
	inorder(x->left);
	std::cout << x->Data << (x->color == RED ? "R " : "B ");
	inorder(x->right);
}

void RedBlackTree::printInorder() {
	inorder(root);
	std::cout << std::endl;
}

void RedBlackTree::draw(HDC hdc) {
	RECT rect;
	GetClientRect(WindowFromDC(hdc), &rect); // 현재 클라이언트 영역 크기 가져오기
	int startX = (rect.right - rect.left) / 2; // 가로 중앙
	int startY = 50;                           // 위에서 조금 내려온 위치
	int xOffset = startX / 2;                  // 좌우 퍼지는 간격

	drawTree(hdc, root, startX, startY, xOffset);
}

static void DrawNode(HDC hdc, Node* node, int x, int y, int r, COLORREF color) {
	HBRUSH brush = CreateSolidBrush(color);
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

	// 원 그리기
	Ellipse(hdc, x - r, y - r, x + r, y + r);
	SelectObject(hdc, oldBrush);
	DeleteObject(brush);

	// 1. 텍스트 배경을 투명하게 설정
	SetBkMode(hdc, TRANSPARENT);

	// 2. 텍스트 색상을 흰색으로 설정 (가독성 향상)
	SetTextColor(hdc, RGB(255, 255, 255));

	TCHAR buf[16];
	wsprintf(buf, TEXT("%d"), node->Data);
	TextOut(hdc, x-10, y - 8, buf, lstrlen(buf));
}

// 재귀적으로 트리 그리기
void RedBlackTree::drawTree(HDC hdc, Node* node, int x, int y, int xOffset) {
	if (node == NIL) return;

	COLORREF color = (node->color == RED) ? RGB(255, 0, 0) : RGB(0, 0, 0);
	DrawNode(hdc, node, x, y, 20, color);

	if (node->left != NIL) {
		MoveToEx(hdc, x, y, NULL);
		LineTo(hdc, x - xOffset, y + 80);
		drawTree(hdc, node->left, x - xOffset, y + 80, xOffset / 2);
	}

	if (node->right != NIL) {
		MoveToEx(hdc, x, y, NULL);
		LineTo(hdc, x + xOffset, y + 80);
		drawTree(hdc, node->right, x + xOffset, y + 80, xOffset / 2);
	}
}