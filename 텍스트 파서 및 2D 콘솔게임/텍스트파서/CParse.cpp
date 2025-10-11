#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <windows.h>
#include "CParse.h"
#include "Stage.h"
using namespace std;

CParse parser;

extern Object p1;
extern Object Monster[100];
extern Bullet bullet[100];

void ParseIntArray(const char* str, int* arr, int maxCount);

bool CParse::LoadFile(const char* FileName)
{
	file1 = fopen(FileName, "r");

	if (!(file1))
	{
		perror("fopen");
		return false;
	}

	fseek(file1, 0, SEEK_END);
	this->FileSize = ftell(file1);
	fseek(file1, 0, SEEK_SET);

	this->pFileBuffer = (char*)malloc(FileSize+1);
	if (!pFileBuffer)
	{
		fclose(file1);
		return false;
	}

	//자꾸 쓰레기값이 읽혀서 읽을수 있는 값만 읽도록 설정
	size_t byte = fread(this->pFileBuffer, 1, FileSize, file1);

	pFileBuffer[byte] = '\0';



	SkipNoneCommand();

	return true;

}

bool CParse::SkipNoneCommand()
{
	if (!this->pFileBuffer)
	{
		perror("SkipNoneCommand Failed");
		return false;
	}

	int writePos = 0;
	for (int readPos = 0; readPos < FileSize; ++readPos)
	{
		char ch = pFileBuffer[readPos];
		if (ch == '"' || ch == 0x20 || ch == 0x08 || ch == 0x09)	// ,과 .은 사용을 위해 지우지않음
		{
			continue;
		}
		pFileBuffer[writePos++] = ch;
	}
	//pFileBuffer[writePos] = '\0'; // 문자열 끝 표시
	this->FileSize = writePos;
	return true;
}

bool GetStringWord(char** chppBuffer, int* ipLength)
{

	if (**chppBuffer != '"') return FALSE;

	(*chppBuffer)++;  // 처음 따옴표 넘김
	char* start = *chppBuffer;

	while (**chppBuffer && **chppBuffer != '"') {
		(*chppBuffer)++;
	}

	if (**chppBuffer != '"') return FALSE;

	*ipLength = (int)(*chppBuffer - start);
	(*chppBuffer)++;  // 끝 따옴표 넘김

	return TRUE;
}

bool GetNextWord(char** chppBuffer, int* ipLength)
{

	char* start = *chppBuffer;
	if (!*start) return FALSE;

	char* wordStart = start;

	if (*start == '=')
	{
		start++;
		*ipLength = start - wordStart;
		*chppBuffer = start;

		return true;

	}

	if (*start == 0x0a)
	{
		start += 1;
		wordStart = start;
	}

	while (*start && *start != '=' && *start != 0x0a)
	{
		start++;
	}

	*ipLength = start - wordStart;
	*chppBuffer = start;

	return true;
}

bool CParse::GetValueInt(const char* szName, int* ipValue)
{
	char* p = pFileBuffer; // 시작 위치
	int iLength;
	char word[256] = {0,};
	while (GetNextWord(&p, &iLength)) {
		// 현재 단어 추출
		memset(word, 0, 256);
		memcpy(word, p - iLength, iLength);  // 단어 복사
		word[iLength] = '\0';
		// 키워드 비교
		if (strcmp(word, szName) == 0) {
			// 다음 단어: '='인지 확인
			if (GetNextWord(&p, &iLength))
			{
				memset(word, 0, 256);
				memcpy(word, p - iLength, iLength);
				if (0 == strcmp(word, "="))
				{
					if (GetNextWord(&p, &iLength))
					{
						memset(word, 0, 256);
						memcpy(word, p - iLength, iLength);
						*ipValue = atoi(word);
						return true;
					}
					return false;
				}
			}
			return false;
		}
	}

	return false; // 못 찾음
}

bool CParse::GetValueDouble(const char* szName, double* ipValue)
{
	char* p = pFileBuffer; // 시작 위치
	int iLength;
	char word[256] = { 0, };
	while (GetNextWord(&p, &iLength)) {
		// 현재 단어 추출
		memset(word, 0, 256);
		memcpy(word, p - iLength, iLength);  // 단어 복사
		word[iLength] = '\0';
		// 키워드 비교
		if (strcmp(word, szName) == 0) {
			// 다음 단어: '='인지 확인
			if (GetNextWord(&p, &iLength))
			{
				memset(word, 0, 256);
				memcpy(word, p - iLength, iLength);
				if (0 == strcmp(word, "="))
				{
					if (GetNextWord(&p, &iLength))
					{
						memset(word, 0, 256);
						memcpy(word, p - iLength, iLength);
						*ipValue = atof(word);
						return true;
					}
					return false;
				}
			}
			return false;
		}
	}

	return false; // 못 찾음
}


bool CParse::GetValueCharArray(const char* szName, char* ipValue)
{
	char* p = pFileBuffer; // 시작 위치
	int iLength;
	char word[256];

	while (GetNextWord(&p, &iLength)) {
		// 현재 단어 추출
		memset(word, 0, 256);
		memcpy(word, p - iLength, iLength);  // 단어 복사
		word[iLength] = '\0';
		// 키워드 비교
		if (strcmp(word, szName) == 0) {
			// 다음 단어: '='인지 확인
			if (GetNextWord(&p, &iLength))
			{
				memset(word, 0, 256);
				memcpy(word, p - iLength, iLength);
				if (0 == strcmp(word, "="))
				{
					if (GetNextWord(&p, &iLength))
					{
						memset(word, 0, 256);
						memcpy(word, p - iLength, iLength);
						memcpy(ipValue, word, iLength);
						ipValue[iLength] = '\0';
						return true;
					}
					return false;
				}
			}
			return false;
		}
	}

	return false; // 못 찾음
}
bool CParse::GetValueShort(const char* szName, short* ipValue)
{
	char* p = pFileBuffer; // 시작 위치
	int iLength;
	char word[256];

	while (GetNextWord(&p, &iLength)) {
		// 현재 단어 추출
		memset(word, 0, 256);
		memcpy(word, p - iLength, iLength);  // 단어 복사
		word[iLength] = '\0';

		// 키워드 비교
		if (strcmp(word, szName) == 0) {
			// 다음 단어: '='인지 확인
			if (GetNextWord(&p, &iLength))
			{
				memset(word, 0, 256);
				memcpy(word, p - iLength, iLength);
				word[iLength] = '\0';

				if (0 == strcmp(word, "="))
				{
					if (GetNextWord(&p, &iLength))
					{
						memset(word, 0, 256);
						memcpy(word, p - iLength, iLength);
						word[iLength] = '\0';

						*ipValue = (short)atoi(word);
						return true;
					}
					return false;
				}
			}
			return false;
		}
	}

	return false; // 못 찾음
}
bool CParse::GetValueBool(const char* szName, bool* ipValue)
{
	char* p = pFileBuffer; // 시작 위치
	int iLength;
	char word[256];

	while (GetNextWord(&p, &iLength)) {
		// 현재 단어 추출
		memset(word, 0, 256);
		memcpy(word, p - iLength, iLength);  // 단어 복사
		word[iLength] = '\0';
		// 키워드 비교
		if (strcmp(word, szName) == 0) {
			// 다음 단어: '='인지 확인
			if (GetNextWord(&p, &iLength))
			{
				memset(word, 0, 256);
				memcpy(word, p - iLength, iLength);
				word[iLength] = '\0';

				if (0 == strcmp(word, "="))
				{
					if (GetNextWord(&p, &iLength))
					{
						memset(word, 0, 256);
						memcpy(word, p - iLength, iLength);
						word[iLength] = '\0';

						if (strcmp(word, "true") == 0 || strcmp(word, "1") == 0)
							*ipValue = true;
						else
							*ipValue = false;

						return true;
					}
					return false;
				}
			}
			return false;
		}
	}

	return false; // 못 찾음
}

Title t;
Result result;
void GetTitleFile()
{
	parser.LoadFile("Title.txt");
	parser.GetValueCharArray("FirstLine", t.FirstLine);
	parser.GetValueCharArray("SecondLine", t.SecondLine);
	parser.GetValueInt("FLength", &t.FLength);
	parser.GetValueInt("SLength", &t.SLength);
	fclose(parser.file1);
}

void GetResultFile(const char* fileName)
{
	parser.LoadFile(fileName);
	parser.GetValueCharArray("FirstLine", result.FirstLine);
	parser.GetValueCharArray("SecondLine", result.SecondLine);
	parser.GetValueCharArray("ThirdLine", result.ThirdLine);
	parser.GetValueCharArray("FourthLine", result.FourthLine);
	parser.GetValueCharArray("FifthLine", result.FifthLine);
	parser.GetValueInt("Length1", &result.Length1);
	parser.GetValueInt("Length2", &result.Length2);
	parser.GetValueInt("Length3", &result.Length3);
	parser.GetValueInt("Length4", &result.Length4);
	parser.GetValueInt("Length5", &result.Length5);
	fclose(parser.file1);
}


void SettingInfoPlayer(const char* fileName, Object* p)
{
	parser.LoadFile(fileName);
	parser.GetValueBool("isVisible", &p->isVisible);
	parser.GetValueCharArray("Design", &p->Design);
	parser.GetValueShort("HaveBullet", &p->HaveBullet);
	parser.GetValueShort("x", &p->px);
	parser.GetValueShort("y", &p->py);
	parser.GetValueInt("hp", &p->hp);
	parser.GetValueInt("movePhase", &p->movePhase);
	parser.GetValueDouble("speed", &p->speed);
	fclose(parser.file1);
}

void SettingInfoMonster(const char* fileName, Object* m)
{
	parser.LoadFile(fileName);
	parser.GetValueBool("isVisible", &m->isVisible);
	parser.GetValueCharArray("Design", &m->Design);
	parser.GetValueInt("hp", &m->hp);
	parser.GetValueInt("movePhase", &m->movePhase);
	parser.GetValueDouble("speed", &m->speed);
	parser.GetValueInt("score", &m->score);
	fclose(parser.file1);
}

void SettingInfoBullet(const char* fileName, Object* Owner, Bullet* b)
{
	parser.LoadFile(fileName);
	parser.GetValueBool("isVisible", &b->isVisible);
	parser.GetValueCharArray("Design", &b->Design);
	b->bx = Owner->px;
	b->by = Owner->py - 1;
	parser.GetValueInt("damage", &b->damage);
	parser.GetValueDouble("speed", &b->speed);
	fclose(parser.file1);
}

void SettingInfoStage(const char* fileName, StageInfo *si)
{
	char key[10];
	parser.LoadFile(fileName);
	parser.GetValueInt("Index", &si->Index);
	for (int i = 0; i < si->Index; i++)
	{
		sprintf(key, "S%d", i);
		parser.GetValueCharArray(key, &si->Nowstage[i][0]);
	}
	fclose(parser.file1);
}

void SettingStage(const char* fileName, Stage *s)
{
	char key[20];
	parser.LoadFile(fileName);
	parser.GetValueBool("allow", &s->allow);
	parser.GetValueInt("MonsterCount", &s->MonsterCount);
	for (int i = 0; i < s->MonsterCount; i++)
	{
		sprintf(key, "M%d", i);
		
		parser.GetValueCharArray(key, &s->MonsterPreset[i][0]);
	}

	parser.GetValueCharArray("x", key);
	ParseIntArray(key, s->sx, s->MonsterCount);
	
	parser.GetValueCharArray("y", key);
	ParseIntArray(key, s->sy, s->MonsterCount);

	fclose(parser.file1);
}

void ParseIntArray(const char* str, int* arr, int maxCount) {
	int i = 0;
	const char* p = str;

	while (*p != '\0' && i < maxCount) {
		while (*p == ' ' || *p == ',') p++;         // 공백이나 ',' 건너뛰기
		if (*p == '\0') break;

		arr[i++] = atoi(p);                         // 정수 변환

		while (*p != '\0' && *p != ',') p++;         // 다음 ','까지 이동
	}
}