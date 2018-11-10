#pragma once
#include "GameProcessFunc.h"

// 스레드 제어 이벤트 생성/삭제
void CreateThreadControlEvents();
void DeleteThreadControlEvents();


// 클라이언트 데이터 통신 처리
DWORD WINAPI ProcessClient(LPVOID arg);

// 게임 Update 처리
DWORD WINAPI ProcessGameUpdate(LPVOID arg);