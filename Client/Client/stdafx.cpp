#include "stdafx.h"
#include <WinSock2.h>

// ���� �Լ� ���� ���
void err_quit(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, // ���� �޼����� �ý����� �˾Ƽ� ����
		NULL, // ���� �޼��� ���� ��ü(�ɼ��� ���� ���� �����ϸ� NULL��.)
		WSAGetLastError(), // ������ �����ڵ�
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // ���� �޼��� ��� ��� ����
		(LPTSTR)&lpMsgBuf, 0, NULL);

	// ���� �޼��� ���
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);

	// ���� �޼��� �޸� ����
	LocalFree(lpMsgBuf);

	exit(1);
}

void err_display(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, // ���� �޼����� �ý����� �˾Ƽ� ����
		NULL, // ���� �޼��� ���� ��ü(�ɼ��� ���� ���� �����ϸ� NULL��.)
		WSAGetLastError(), // ������ �����ڵ�
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // ���� �޼��� ��� ��� ����
		(LPTSTR)&lpMsgBuf, 0, NULL);

	// ���� �޼��� ���
	printf_s("[%s] %s", msg, (LPCTSTR)lpMsgBuf);

	// ���� �޼��� �޸� ����
	LocalFree(lpMsgBuf);
}

// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char* buf, int len, int flags)
{
	int received; // ���Ź��� ������ ũ��
	char* ptr = buf; // ���� �׼��� ��ġ
	int left = len; // ���� ���� ũ��

	/*
	recv() : ����� �������κ��� �����͸� �����ϴ� �Լ�
	Return
	0 �̻� : ���������� �����͸� �����Ͽ�����, ������ ���� �������� ũ�⸦ return
	-1 : ���� �߻�

	�Ʒ� �ڵ�� ��ü ������ ���ۿ��� ���� ������ ũ�⸦ �˱� �����̴�.
	Why?...
	�� �������� ���� ������ ũ�⸸ŭ ����� �о�Դ����� �˾ƺ��� ����.
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
	int sended; // �۽��� ������ ũ��
	char* ptr = buf; // ���� �׼��� ��ġ
	int left = len; // ���� ���� ũ��

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