#include <stdio.h>
#include <memory.h>
#include <Windows.h>
#include "Console.h"
#include "GameObject.h"
#include "CParse.h"
#include "Stage.h"
#include <time.h>
#pragma comment(lib, "Winmm.lib")

#define OBJECT_MAX 100

char szScreenBuffer[dfSCREEN_HEIGHTUI][dfSCREEN_WIDTH];
void Buffer_Flip(void);
void Buffer_Clear(void);
void Sprite_Draw(int iX, int iY, char chSprite);
void Int_Draw(int iX, int iY, int number);
extern Title t;
extern Result result;

// Clear or Over 분기
bool Gameclear = false;	

Object p1;
Object Monster[OBJECT_MAX];
Bullet bullet[OBJECT_MAX];
GameScene scene = TITLE;

bool finish = true;
int useCount = 0;
StageInfo stageinfo;
int stageNumber = 0;
Stage stage;


int score = 0;

void checkHaveBullet(Object* Owner)
{

	if (Owner->HaveBullet != useCount)
	{
		if (bullet[useCount].isVisible == false)
		{
			bullet[useCount].isVisible = true;
			bullet[useCount].bx = Owner->px;
			bullet[useCount].by = Owner->py - 1;
			bullet[useCount].fxy[0] = Owner->px;
			bullet[useCount].fxy[1] = Owner->py;
		}
		useCount++;
	}
	else if (p1.hp > 0)
	{

		bullet[p1.hp].isVisible = true;
		bullet[p1.hp].Design = '*';
		bullet[p1.hp].damage = 99;
		bullet[p1.hp].bx = Owner->px;
		bullet[p1.hp].by = Owner->py -1;
		bullet[p1.hp].fxy[0] = Owner->px;
		bullet[p1.hp].fxy[1] = Owner->py -1;
		p1.hp--;
		if (p1.hp <= 0)
			p1.hp = 0;
	}
}

void UpdateTitle()
{
	static int count[2] = { 1,1 };
	static bool isLoaded = false;
	static bool textFinish = false;
	if (!isLoaded)
	{
		GetTitleFile();
		isLoaded = true;
	}

	int x = (dfSCREEN_WIDTH / 2) - (t.FLength /2);
	int y = (dfSCREEN_HEIGHT / 2);

	for (int key = 0x01; key <= 0xFE; key++)
	{
		if (GetAsyncKeyState(key) && textFinish)
		{
			scene = SHOWSTAGENAME;
			stageNumber = 0;
			isLoaded = false;
			textFinish = false;
			for (int i = 0; i < 2; i++)
				count[i] = 1;
			return;
		}
	}
	Buffer_Clear();
	for (int i = 0; i < count[0]; i++)
	{
		if (count[0] > t.FLength)
			count[0] = t.FLength;
		if (t.FirstLine[i] == '@')
		{
			x += 2;
		}
		else
		{
			Sprite_Draw(x, y, t.FirstLine[i]);
			x++;
		}
	}
	count[0]++;
	if (count[0] >= t.FLength)
	{
		x = (dfSCREEN_WIDTH / 2) - (t.SLength /2);
		y += 3;
		for (int j = 0; j < count[1]; j++)
		{
			if (count[1] > t.SLength)
				count[1] = t.SLength;

			if (t.SecondLine[j] == '@')
			{
				x += 2;
			}
			else
			{
				Sprite_Draw(x, y, t.SecondLine[j]);
				x++;
			}
		}
		count[1]++;
	}
	if (count[1] > t.SLength)
	{
		textFinish = true;
	}
	Buffer_Flip();
	Sleep(50);
}

void ShowName()
{
	static char StageText[20];
	static char MonsterText[20];
	static bool ChangeSetting = true;
	static int count = 1;
	static bool TextFinish = false;
	int length = 0;
	//현재 GameStage 세팅 체크
	SettingInfoStage("StageInfo.txt", &stageinfo);
	length = sizeof(stageinfo.Nowstage[stageNumber]);


	if (TextFinish == true)
	{
		scene = GAME;
		TextFinish = false;
		ChangeSetting = true;
		count = 1;
		return;
	}

	if (ChangeSetting)
	{
		if (stageNumber < stageinfo.Index)
		{
			memcpy(StageText, stageinfo.Nowstage[stageNumber], length);
			StageText[length] = '\0';


			SettingStage(StageText, &stage);
			for (int i = 0; i < stage.MonsterCount; i++)
			{
				memcpy(MonsterText, stage.MonsterPreset[i], sizeof(stage.MonsterPreset[i]));
				MonsterText[sizeof(stage.MonsterPreset[i]) - 1] = '\0';
				SettingInfoMonster(MonsterText, &Monster[i]);
				Monster[i].px = stage.sx[i];
				Monster[i].py = stage.sy[i];
				Monster[i].fxy[0] = (double)stage.sx[i];
				Monster[i].fxy[1] = (double)stage.sy[i];
			}

			SettingInfoPlayer("player.txt", &p1);
			for (int i = 0; i < p1.HaveBullet; i++)
			{
				SettingInfoBullet("bullet1.txt", &p1, &bullet[i]);
			}
			p1.fxy[0] = p1.px;
			p1.fxy[1] = p1.py;
			stageNumber++;
			useCount = 0;
			ChangeSetting = !ChangeSetting;
		}
		else
		{
			Gameclear = true;
			scene = RESULT;
			return;
		}
	}
	else
	{
		int x = (dfSCREEN_WIDTH/2) - (length / 2);
		int y = dfSCREEN_HEIGHT / 2;

		Buffer_Clear();
		for (int i = 0; i < count; i++)
		{
			// stage.txt 에서 .만날시 출력 종료후 GAME으로 이동 (해당 스테이지 단계를 보여주기 위함이라서 뒤 .txt는 필요하지 않음)
			if (StageText[i] == '.')
			{
				TextFinish = true;
			}
			else
			{
				Sprite_Draw(x, y, StageText[i]);
				x++;
			}
		}
		count++;
		Buffer_Flip();
		Sleep(100);
	}
}


void UpdateGame()
{
	bool checkbullet = false;
	bool isliveMonster = false;
	bool stageName = false;
	bool renderPass = false;
	static int unVisibleCount = 0;
	////////////
	static LARGE_INTEGER freq = { 0 };
	static LARGE_INTEGER lastFpsTime = { 0 };
	static LARGE_INTEGER lastFrameTime;
	LARGE_INTEGER now;
	static int frameCount = 0;
	double elapsedMS;
	float delta;
	double lateTime = 0;

	if (freq.QuadPart == 0)
	{
		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&lastFrameTime);
		lastFpsTime = lastFrameTime;
		return;
	}

	QueryPerformanceCounter(&now);
	elapsedMS = (double)(now.QuadPart - lastFrameTime.QuadPart) * 1000.0 / freq.QuadPart;


	if (elapsedMS >= 20.0)
	{
		lateTime += elapsedMS - 20.0;
	}

	if (lateTime >= 20.0)
	{
		renderPass = true;
	}

	while (elapsedMS < 20.0) {
		QueryPerformanceCounter(&now);
		elapsedMS = (double)(now.QuadPart - lastFrameTime.QuadPart) * 1000.0 / freq.QuadPart;
	}
	lastFrameTime = now;
	frameCount++;

	if ((now.QuadPart - lastFpsTime.QuadPart) * 1000.0 / freq.QuadPart >= 1000.0)
	{
		printf("FPS: %d       \n", frameCount);
		frameCount = 0;
		lastFpsTime = now;
	}

	// 초 단위로 변환
	delta = (float)(elapsedMS / 1000.0);

	//키입력

#pragma region KeyInput
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		p1.fxy[0] -= p1.speed * delta;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		p1.fxy[0]+= p1.speed * delta;
	}
	if (GetAsyncKeyState(VK_UP) & 0x8000)
	{
		p1.fxy[1] -= p1.speed * delta;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
	{
		p1.fxy[1] += p1.speed * delta;
	}
	if (GetAsyncKeyState('Q') & 0x0001)
	{
		checkHaveBullet(&p1);
	}
	if (GetAsyncKeyState('A'))
	{

		Sleep(100);
	}

	////CHEAT
	if (GetAsyncKeyState('O'))
	{
		scene = RESULT;
		Gameclear = true;
		return;
	}
	if (GetAsyncKeyState('P'))
	{
		scene = RESULT;
		Gameclear = false;
		return;
	}
	////

	if (p1.fxy[0] < 0) p1.fxy[0] = 0;
	if (p1.fxy[0] > dfSCREEN_WIDTH - 1) p1.fxy[0] = dfSCREEN_WIDTH - 1;
	if (p1.fxy[1] < 0) p1.fxy[1] = 0;
	if (p1.fxy[1] > dfSCREEN_HEIGHT - 1) p1.fxy[1] = dfSCREEN_HEIGHT - 1;
	p1.px = (int)(p1.fxy[0] + 0.5f);
	p1.py = (int)(p1.fxy[1] + 0.5f);
#pragma endregion

	//게임 패배 체크
	if (p1.hp <= 0)
	{
		// 현재 날라가는 총알이 있는지 체크
		for (int i = 0; i < p1.HaveBullet; i++)
		{
			if (bullet[i].isVisible == true)
				checkbullet = true;

		}
		// 현재 살아있는 몬스터가 있는지 체크
		for (int i = 0; i < stage.MonsterCount; i++)
		{
			if (Monster[i].isVisible == true)
				isliveMonster = true;
		}
		if (!checkbullet && isliveMonster)
		{
			scene = RESULT;
			unVisibleCount = 0;
			return;
		}
	}


	// 모든 몬스터가 죽었다면
	if (unVisibleCount == stage.MonsterCount)
	{

		unVisibleCount = 0;
		scene = SHOWSTAGENAME;
		ShowName();

		return;
	}

	// 몬스터 AI행동
	for (int i = 0; i < stage.MonsterCount; i++)
	{
		if (!Monster[i].isVisible || Monster[i].speed == 0)
			continue;

		switch (Monster[i].movePhase)
		{
		case 0:
			if (Monster[i].px > stage.sx[i] - 5)
				Monster[i].fxy[0] -= (Monster[i].speed * delta);
			else
				Monster[i].movePhase = 1;
			break;

		case 1:
			if (Monster[i].px < stage.sx[i] + 5)
				Monster[i].fxy[0] += (Monster[i].speed * delta);
			else
				Monster[i].movePhase = 2;
			break;
		case 2:
			if (Monster[i].px > stage.sx[i])
				Monster[i].fxy[0] -= (Monster[i].speed * delta);
			else if (Monster[i].px < stage.sx[i])
				Monster[i].fxy[0] += (Monster[i].speed * delta);
			else
				Monster[i].movePhase = 0;
			break;
		}

		Monster[i].px = (int)(Monster[i].fxy[0] + 0.5f);
	}

	// 모든 불릿 이동
	for (int i = 0; i < p1.HaveBullet; i++)
	{
		if (bullet[i].isVisible == true)
		{
			bullet[i].fxy[1] -= (bullet[i].speed * delta);
			bullet[i].by = (int)(bullet[i].fxy[1] + 0.5f);
			if (bullet[i].by <= 0)
			{
				bullet[i].isVisible = false;
			}
		}
	}

	// 모든 몬스터와 불릿 순회
	for (int i = 0; i < stage.MonsterCount; i++)
	{
		for (int j = 0; j < p1.HaveBullet; j++)
		{
			if (bullet[j].isVisible == true && Monster[i].isVisible == true)
			{
				if (Monster[i].px == bullet[j].bx && Monster[i].py -1 == bullet[j].by)
				{
					Monster[i].hp -= bullet[j].damage;

					if (Monster[i].hp <= 0)
					{
						Monster[i].isVisible = false;
						score += Monster[i].score;
						unVisibleCount++;
					}
					bullet[j].isVisible = false;
				}
			}
		}
	}

	if (renderPass)
	{
		renderPass = false;
		lateTime = 0;
	}
	else
	{
		//랜더링
#pragma region Render

		Buffer_Clear();

		for (int i = 0; i < p1.HaveBullet; i++)
		{
			if (bullet[i].isVisible == true)
			{
				Sprite_Draw(bullet[i].bx, bullet[i].by, bullet[i].Design);
			}
		}

		for (int i = 0; i < stage.MonsterCount; i++)
		{
			if (Monster[i].isVisible == true)
			{
				Sprite_Draw(Monster[i].px, Monster[i].py, Monster[i].Design);
			}
		}
		Sprite_Draw(p1.px, p1.py, p1.Design);

		//HP
		Sprite_Draw(0, dfSCREEN_HEIGHTUI - 1, 'H');
		Sprite_Draw(1, dfSCREEN_HEIGHTUI - 1, 'P');
		Sprite_Draw(2, dfSCREEN_HEIGHTUI - 1, ':');
		Int_Draw(3, dfSCREEN_HEIGHTUI - 1, p1.hp);

		//BULLET
		Sprite_Draw(10, dfSCREEN_HEIGHTUI - 1, 'B');
		Sprite_Draw(11, dfSCREEN_HEIGHTUI - 1, 'L');
		Sprite_Draw(12, dfSCREEN_HEIGHTUI - 1, ':');
		Int_Draw(13, dfSCREEN_HEIGHTUI - 1, p1.HaveBullet - useCount);
		//SCORE
		Sprite_Draw(20, dfSCREEN_HEIGHTUI - 1, 'S');
		Sprite_Draw(21, dfSCREEN_HEIGHTUI - 1, 'C');
		Sprite_Draw(22, dfSCREEN_HEIGHTUI - 1, 'O');
		Sprite_Draw(23, dfSCREEN_HEIGHTUI - 1, 'R');
		Sprite_Draw(24, dfSCREEN_HEIGHTUI - 1, 'E');
		Sprite_Draw(25, dfSCREEN_HEIGHTUI - 1, ':');
		Int_Draw(26, dfSCREEN_HEIGHTUI - 1, score);
		//
		Buffer_Flip();

#pragma endregion
	}
}

void UpdateResult(bool Clear)
{
	static int count[5] = { 1,1,1,1,1 };
	static bool ResultIsFirstLoaded = false;
	static bool textFinish = false;


	int x = (dfSCREEN_WIDTH / 2) - (result.Length1 / 2);
	int y = dfSCREEN_HEIGHT / 2;

	if (!ResultIsFirstLoaded)
	{
		if (Clear)
			GetResultFile("Clear.txt");
		else
			GetResultFile("Over.txt");
		ResultIsFirstLoaded = true;
	}
	if (GetAsyncKeyState('R') && textFinish)
	{
		scene = TITLE;
		stageNumber = 0;
		ResultIsFirstLoaded = false;
		textFinish = false;
		for (int i = 0; i < 5; i++)
			count[i] = 1;
		return;
	}
	else if (GetAsyncKeyState('X') && textFinish)
	{
		finish = false;
		ResultIsFirstLoaded = false;
		textFinish = false;
		for (int i = 0; i < 5; i++)
			count[i] = 1;
		return;
	}

	Buffer_Clear();
	for (int i = 0; i < count[0]; i++)
	{
		if (count[0] > result.Length1)
			count[0] = result.Length1;
		if (result.FirstLine[i] == '@')
		{
			x += 2;
		}
		else
		{
			Sprite_Draw(x, y, result.FirstLine[i]);
			x++;
		}

	}
	count[0]++;
	if (count[0] >= result.Length1)
	{
		x = (dfSCREEN_WIDTH / 2) - (result.Length2 / 2);
		y += 1;
		for (int j = 0; j < count[1]; j++)
		{
			if (count[1] > result.Length2)
				count[1] = result.Length2;
			if (result.SecondLine[j] == '@')
			{
				x += 2;
			}
			else
			{
				Sprite_Draw(x, y, result.SecondLine[j]);
				x++;
			}
		}
		count[1]++;
	}

	if (count[1] >= result.Length2)
	{
		x = (dfSCREEN_WIDTH / 2) - (result.Length3 / 2);
		y += 1;
		for (int j = 0; j < count[2]; j++)
		{
			if (count[2] > result.Length3)
				count[2] = result.Length3;
			if (result.ThirdLine[j] == '@')
			{
				x += 2;
			}
			else
			{
				Sprite_Draw(x, y, result.ThirdLine[j]);
				x++;
			}
		}
		count[2]++;
	}

	if (count[2] >= result.Length3)
	{
		x = (dfSCREEN_WIDTH / 2) - (result.Length4 / 2);
		y += 1;
		for (int j = 0; j < count[3]; j++)
		{
			if (count[3] > result.Length4)
				count[3] = result.Length4;
			if (result.FourthLine[j] == '@')
			{
				x += 2;
			}
			else
			{
				Sprite_Draw(x, y, result.FourthLine[j]);
				x++;
			}
		}
		count[3]++;
	}

	if (count[3] >= result.Length4)
	{
		x = (dfSCREEN_WIDTH / 2) - (result.Length5 / 2);
		y += 1;
		for (int j = 0; j < count[4]; j++)
		{
			if (count[4] > result.Length5)
				count[4] = result.Length5;
			if (result.FifthLine[j] == '@')
			{
				x += 2;
			}
			else
			{
				Sprite_Draw(x, y, result.FifthLine[j]);
				x++;
			}
		}
		count[4]++;
	}
	if (count[4] > result.Length5)
	{
		textFinish = true;
	}
	Buffer_Flip();

	printf("Last Score: %d", score);
	Sleep(50);
}


int main(void)
{
	srand((unsigned int)time(NULL));

	cs_Initial();
	timeBeginPeriod(1);
	//--------------------------------------------------------------------
	// 게임의 메인 루프
	// 이 루프가  1번 돌면 1프레임 이다.
	//--------------------------------------------------------------------

	while (finish)
	{
		switch (scene)
		{
		case TITLE:
			UpdateTitle();
			break;

		case SHOWSTAGENAME:
			ShowName();
			break;

		case GAME:
			UpdateGame();
			break;

		case RESULT:
			UpdateResult(Gameclear);
			break;
		}

		// 하단은 게임씬의 로직 예시이며 
		// 이 부분에는 씬 표현을 위한 분기가 들어가시면 됩니다.
		// 
		// 
		// GameUpdate() 내부 예시
		// 
		// 1. 키보드 입력부
		// 2. 로직부 
		// 3. 랜더부
			/*  예시
				// 스크린 버퍼를 지움
				Buffer_Clear();
				// 스크린 버퍼에 객체들 출력
				Sprite_Draw(iX, 10, 'A');
				// 스크린 버퍼를 화면으로 출력
				Buffer_Flip();
			*/

		// 프레임 맞추기용 대기 Sleep(X)
	}
	timeEndPeriod(0);
	return 0;
}



//--------------------------------------------------------------------
// 버퍼의 내용을 화면으로 찍어주는 함수.
//
// 적군,아군,총알 등을 szScreenBuffer 에 넣어주고, 
// 1 프레임이 끝나는 마지막에 본 함수를 호출하여 버퍼 -> 화면 으로 그린다.
//--------------------------------------------------------------------
void Buffer_Flip(void)
{
	int iCnt;
	for (iCnt = 0; iCnt < dfSCREEN_HEIGHTUI; iCnt++)
	{
		cs_MoveCursor(0, iCnt);
		printf(szScreenBuffer[iCnt]);
	}
}


//--------------------------------------------------------------------
// 화면 버퍼를 지워주는 함수
//
// 매 프레임 그림을 그리기 직전에 버퍼를 지워 준다. 
// 안그러면 이전 프레임의 잔상이 남으니까
//--------------------------------------------------------------------
void Buffer_Clear(void)
{
	int iCnt;
	memset(szScreenBuffer, ' ', dfSCREEN_WIDTH * dfSCREEN_HEIGHTUI);

	for (iCnt = 0; iCnt < dfSCREEN_HEIGHTUI; iCnt++)
	{
		szScreenBuffer[iCnt][dfSCREEN_WIDTH - 1] = '\0';
	}

}

//--------------------------------------------------------------------
// 버퍼의 특정 위치에 원하는 문자를 출력.
//
// 입력 받은 X,Y 좌표에 아스키코드 하나를 출력한다. (버퍼에 그림)
//--------------------------------------------------------------------
void Sprite_Draw(int iX, int iY, char chSprite)
{
	if (iX < 0 || iY < 0 || iX >= dfSCREEN_WIDTH - 1 || iY >= dfSCREEN_HEIGHTUI)
		return;

	szScreenBuffer[iY][iX] = chSprite;
}

void Int_Draw(int iX, int iY, int number)
{
	int first;
	int second;
	if (iX < 0 || iY < 0 || iX >= dfSCREEN_WIDTH - 1 || iY >= dfSCREEN_HEIGHTUI)
		return;

	if(number < 10)
		szScreenBuffer[iY][iX] = number + 48;
	else
	{
		first = number / 10;
		second = number % 10;
		szScreenBuffer[iY][iX] = first + 48;
		szScreenBuffer[iY][iX+1] = second + 48;
	}
}




