#pragma once
#include "GameProcessFunc.h"

// ������ ���� �̺�Ʈ ����/����
void CreateThreadControlEvents();
void DeleteThreadControlEvents();


// Ŭ���̾�Ʈ ������ ��� ó��
DWORD WINAPI ProcessClient(LPVOID arg);

// ���� Update ó��
DWORD WINAPI ProcessGameUpdate(LPVOID arg);