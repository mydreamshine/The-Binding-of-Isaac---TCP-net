#include "stdafx.h"
#include <WinSock2.h>

// 윈속 함수 에러 출력
void err_quit(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, // 오류 메세지를 시스템이 알아서 생성
		NULL, // 오류 메세지 생성 주체(옵션을 위와 같인 설정하면 NULL로.)
		WSAGetLastError(), // 마지막 오류코드
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // 오류 메세지 출력 언어 종류
		(LPTSTR)&lpMsgBuf, 0, NULL);

	// 오류 메세지 출력
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);

	// 오류 메세지 메모리 해제
	LocalFree(lpMsgBuf);

	exit(1);
}

void err_display(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, // 오류 메세지를 시스템이 알아서 생성
		NULL, // 오류 메세지 생성 주체(옵션을 위와 같인 설정하면 NULL로.)
		WSAGetLastError(), // 마지막 오류코드
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // 오류 메세지 출력 언어 종류
		(LPTSTR)&lpMsgBuf, 0, NULL);

	// 오류 메세지 출력
	printf_s("[%s] %s", msg, (LPCTSTR)lpMsgBuf);

	// 오류 메세지 메모리 해제
	LocalFree(lpMsgBuf);
}

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char* buf, int len, int flags)
{
	int received; // 수신받은 데이터 크기
	char* ptr = buf; // 버퍼 액세스 위치
	int left = len; // 남은 버퍼 크기

	/*
	recv() : 연결된 소켓으로부터 데이터를 수신하는 함수
	Return
	0 이상 : 정상적으로 데이터를 수신하였으며, 실제로 읽은 데이터의 크기를 return
	-1 : 오류 발생

	아래 코드는 전체 데이터 버퍼에서 남은 데이터 크기를 알기 위함이다.
	Why?...
	★ 서버에게 보낸 데이터 크기만큼 제대로 읽어왔는지를 알아보기 위함.
	*/
	while (left > 0)
	{
		received = recv(s, buf, len, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

int sendn(SOCKET s, char* buf, int len, int flags)
{
	int sended; // 송신한 데이터 크기
	char* ptr = buf; // 버퍼 액세스 위치
	int left = len; // 남은 버퍼 크기

	while (left > 0)
	{
		sended = send(s, buf, len, flags);
		if (sended == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (sended == 0)
			break;
		left -= sended;
		ptr += sended;
	}

	return (len - left);
}