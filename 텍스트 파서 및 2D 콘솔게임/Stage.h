#pragma once
struct Title
{
	char FirstLine[80];
	char SecondLine[80];
	int FLength;
	int SLength;
};

struct Game
{
	int MonsterCount;
	int MonsterPreset;

};

struct Result
{
	char FirstLine[80];
	char SecondLine[80];
	char ThirdLine[80];
	char FourthLine[80];
	char FifthLine[80];
	int Length1;
	int Length2;
	int Length3;
	int Length4;
	int Length5;
};

struct ChangeStage
{
	char FirstLine[80];
};

struct Stage
{
	bool allow;
	int MonsterCount;
	char MonsterPreset[10][20];
	int sx[10];
	int sy[10];
};

struct StageInfo
{
	int Index;
	char Nowstage[10][10];
};

enum GameScene
{
	TITLE = 0,
	SHOWSTAGENAME = 1,
	GAME = 2,
	RESULT = 3,
};
