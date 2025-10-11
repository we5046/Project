

#pragma once
#define OBJECT_MAX 100
struct Object
{
	bool isVisible;
	char Design;
	short HaveBullet;
	short px;
	short py;
	int hp;
	int movePhase;
	double speed;
	int score;
	double fxy[2];
};

struct Bullet
{
	bool isVisible;
	char Design;
	short bx;
	short by;
	int damage;
	double speed;
	double fxy[2];
};