#pragma once
#include <stdio.h>
#include <iostream>
#include <list>
#include <tchar.h>
#include "Geometric.h"
#include "Global.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>

using namespace std;

// ���� �Լ� ���� ���
void err_quit(char* msg);
void err_display(char* msg);

// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char* buf, int len, int flags);

// ����� ���� ������ �۽� �Լ�
int sendn(SOCKET s, char* buf, int len, int flags);