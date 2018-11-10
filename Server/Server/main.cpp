#include "ProcessThreads.h"

int main(int argc, char argv[])
{
	// ���� ������Ʈ �ʱ�ȭ
	GameProcessFunc::InitGameObject();

	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// ��� ���� ����
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit((char*)"socket()");

	// ��� ������ ��� ����(Protocol, IPv4, PortNum)
	{
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));

		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		/*
		INADDR_ANY :
		������ ip�ּҸ� ������ ���� ��� Ȥ�� �ֱ������� ip�ּҰ� �ٲ� ��츦 ����ؼ�
		������ �ش��ϴ� ��� ip�ּҷ� ������ �����ϵ��� ����ϱ� ����.
		*/
		serveraddr.sin_port = htons(PORT_NUM);
		retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR) err_quit((char*)"bind()");
	}

	// Ŭ���̾�Ʈ�κ��� ��û ���(��� ������ ����.)
	retval = listen(listen_sock, SOMAXCONN);
	/*
	backlog : ���� ���� �� ť�� ��Ƴ��� �� �ִ� �ִ� ���ġ
	�������̸� SOMAXCONN���� �Ѵ�.
	Why?
	������ �ϵ��� �ٲ� ��츦 ����ؼ�.
	*/
	if (retval == SOCKET_ERROR) err_quit((char*)"listen()");

	// ������ ���� �̺�Ʈ ����
	CreateThreadControlEvents();

	// ���� Update ������ ����
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, ProcessGameUpdate, NULL, 0, NULL);
	if (hThread == NULL)
		err_quit((char*)"CreateThread()");
	else CloseHandle(hThread);

	cout << "[TCP ����] ���� ��� �غ� �Ϸ�\nŬ���̾�Ʈ ��û �����...";
	// ������ ��ſ� ����� ����
	SOCKET client_sock = NULL;
	SOCKADDR_IN clientaddr;
	int addrlen;
	while (true)
	{
		// ������ ������ Ŭ���̾�Ʈ Ȯ�� (��� ������ ���� Ȯ��.)
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		/*
		Ŭ���̾�Ʈ�κ��� ���� ��û�� ������
		��Ʒ� ��� �����
		(accept()�Լ� ���ο��� ���� �ݺ��ؼ� Ŭ���̾�Ʈ�� ������ ����Ѵ�.)
		*/
		if (client_sock == INVALID_SOCKET)
		{
			err_display((char*)"accept()");
			break;
		}
		cout << "[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(clientaddr.sin_addr) << ", ��Ʈ��ȣ=" << ntohs(clientaddr.sin_port) << endl;

		// Ŭ���̾�Ʈ���� ������ ���
		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock, 0, NULL);
		if (hThread == NULL)
		{
			// ���� �޸� ����(Ŭ���̾�Ʈ ���Ϻ���.)
			closesocket(client_sock);
			cout << "[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(clientaddr.sin_addr) << ", ��Ʈ��ȣ=" << ntohs(clientaddr.sin_port) << endl;
		}
		else CloseHandle(hThread);
	}

	// ���� �޸� ����(������ ������)
	closesocket(listen_sock);

	// ������ ���� �̺�Ʈ ����
	DeleteThreadControlEvents();

	// ���� ����
	WSACleanup();
}