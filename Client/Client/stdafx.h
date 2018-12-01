#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include "Geometric.h"
#include "Global.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>

// 윈속 함수 에러 출력
void err_quit(char* msg);
void err_display(char* msg);

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char* buf, int len, int flags);

// 사용자 정의 데이터 송신 함수
int sendn(SOCKET s, char* buf, int len, int flags);
