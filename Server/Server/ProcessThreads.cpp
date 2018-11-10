#include "ProcessThreads.h"

HANDLE hRecvAllEvent[MAX_CLIENT];
HANDLE hUpdateEvent;

unsigned int CurClientNum = 0;
bool GameStart = false;

void CreateThreadControlEvents()
{
	// �ڵ� ���� �̺�Ʈ 1+MAX_CLIENT�� ����(���� ��ȣ, ���ȣ)
	hUpdateEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	for (int i = 0; i < MAX_CLIENT; ++i)
		hRecvAllEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
}

void DeleteThreadControlEvents()
{
	// �̺�Ʈ ����
	CloseHandle(hUpdateEvent);
	for (int i = 0; i < MAX_CLIENT; ++i)
		CloseHandle(hRecvAllEvent[i]);
}

// Ŭ���̾�Ʈ ������ ��� ó��
DWORD WINAPI ProcessClient(LPVOID arg)
{
	SOCKET client_sock = (SOCKET)arg;
	SOCKADDR_IN clientaddr;
	int addrlen;
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

	// ���� ������Ʈ�� Update�� �Ϸ� �� ������ ���
	WaitForSingleObject(hUpdateEvent, INFINITE);

	// MAX_CLIENT���� ���� ���� Ŭ���̾�Ʈ�� �������� ����
	if (CurClientNum == MAX_CLIENT)
	{
		closesocket(client_sock);
		cout << "[TCP ����] Ŭ���̾�Ʈ ��������(�ο��ʰ�): IP �ּ�=" << inet_ntoa(clientaddr.sin_addr) << ", ��Ʈ��ȣ=" << ntohs(clientaddr.sin_port) << endl;
		return 0;
	}

	int ClientID = GameProcessFunc::CreateNewPlayer();
	if (ClientID == -1)
	{
		closesocket(client_sock);
		cout << "[TCP ����] Ŭ���̾�Ʈ ��������(�ο��ʰ�): IP �ּ�=" << inet_ntoa(clientaddr.sin_addr) << ", ��Ʈ��ȣ=" << ntohs(clientaddr.sin_port) << endl;
		return 0;
	}
	else
	{
		GameStart = true;
		// ���� Ŭ���̾�Ʈ �� ����
		++CurClientNum;
		// ���� ������Ʈ�� ������ ��� ������Ʈ�� �ݿ�
		GameProcessFunc::ResetCommunicationBuffer();
		// ���� ���� ������ ��, ó�� �������ϱ� ���� ��� ������Ʈ�� ������ ������.
		if (!GameProcessFunc::SendCommunicationData(client_sock, ClientID))
		{
			closesocket(client_sock);
			cout << "[TCP ����] Ŭ���̾�Ʈ ��������(�ο��ʰ�): IP �ּ�=" << inet_ntoa(clientaddr.sin_addr) << ", ��Ʈ��ȣ=" << ntohs(clientaddr.sin_port) << endl;
			return 0;
		}
	}
		
	cout << "[TCP ����] Ŭ���̾�Ʈ(ID: " << ClientID << ") "
		<< "������: IP �ּ�=" << inet_ntoa(clientaddr.sin_addr) << ", ��Ʈ��ȣ=" << ntohs(clientaddr.sin_port) << endl;
	// ������ ��� �� ���� ������Ʈ ó��
	while (true)
	{
		// Ű ���� �޾ƿ�
		if (!GameProcessFunc::RecvInput(client_sock, ClientID))
		{
			--CurClientNum;
			for (int i = 0; i < MAX_CLIENT; ++i) SetEvent(hRecvAllEvent[i]);
			break;
		}

		// ���ú긦 �޾Ҵٴ� ���� �˸�
		//SetEvent(hRecvAllEvent[ClientID]);

		////////// Server : Client = 1 : 1 �� �� TEST�� //////////
		for (int i = 0; i < MAX_CLIENT; ++i)					//
			SetEvent(hRecvAllEvent[i]);							//
		////////// TEST�� �ƴ� ��쿡�� �ּ� ó�� �� ��! /////////

		// ��� ������Ʈ�� ������Ʈ�� �����⸦ ��ٸ�
		WaitForSingleObject(hUpdateEvent, INFINITE);

		// ������Ʈ �� ������ Ŭ���̾�Ʈ���� ����
		if (!GameProcessFunc::SendCommunicationData(client_sock, ClientID))
		{
			--CurClientNum;
			break;
		}
	}

	// ���� �޸� ����(Ŭ���̾�Ʈ ���Ϻ���.)
	closesocket(client_sock);
	cout << "[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=" << inet_ntoa(clientaddr.sin_addr) << ", ��Ʈ��ȣ=" << ntohs(clientaddr.sin_port) << endl;
	return 0;
}


// ���� Update ó��
DWORD WINAPI ProcessGameUpdate(LPVOID arg)
{
	while (true)
	{
		if (CurClientNum == 0)
		{
			// ������ Ŭ���̾�Ʈ�� ���(���� Ŭ���̾�Ʈ ��: 0) Update�� ���ư��� �Ѵ�.
			// ��, Update�Ǿ�� �� ������Ʈ�� �������� �ʰ� �Ѵ�.
			if (GameStart)
			{
				GameProcessFunc::InitGameObject();
				GameStart = false;
			}
			for (int i = 0; i < MAX_CLIENT; ++i)
				SetEvent(hRecvAllEvent[i]);
		}
		// ��� Ŭ���̾�Ʈ�κ��� Recv()�� Ȯ�� �� ������ ���
		WaitForMultipleObjects(MAX_CLIENT, hRecvAllEvent, TRUE, INFINITE);

		// ElapsedTime ���(��� ������)
		float ElapsedTime = 1.0f / 60;

		// ���� ������Ʈ Update
		GameProcessFunc::ProcessInput(ElapsedTime);
		GameProcessFunc::ProcessPhisics(ElapsedTime);
		GameProcessFunc::ProcessCollision(ElapsedTime);

		// ���� ������Ʈ�� ������ ��� ������Ʈ�� �ݿ�
		GameProcessFunc::ResetCommunicationBuffer();

		// ��� ������Ʈ�� ������Ʈ�� ������ �˸�
		SetEvent(hUpdateEvent);
	}
	return 0;
}