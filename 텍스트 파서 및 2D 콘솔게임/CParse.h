#pragma once
#include "GameObject.h"
#include "Stage.h"
#ifndef __CPARSE__
#define __CPARSE__

struct CParse
{
	FILE* file1;
	char* pFileBuffer;
	int FileSize;

	bool LoadFile(const char* FileName);
	bool GetValueInt(const char* szName, int* ipValue);
	bool GetValueShort(const char* szName, short* ipValue);
	bool GetValueCharArray(const char* szName, char* ipValue);
	bool GetValueBool(const char* szName, bool* ipValue);
	bool GetValueDouble(const char* szName, double* ipValue);
	bool SkipNoneCommand();
};

extern void SettingInfoPlayer(const char* fileName, Object* p);
extern void SettingInfoMonster(const char* fileName, Object* m);
extern void SettingInfoBullet(const char* fileName, Object* Owner, Bullet* b);
extern void SettingInfoStage(const char* fileName, StageInfo* si);
extern void SettingStage(const char* fileName, Stage* s);
extern void GetTitleFile();
extern void GetResultFile(const char* fileName);

#endif