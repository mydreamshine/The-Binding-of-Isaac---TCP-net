#include "ProcessThreads.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

HANDLE hRecvAllEvent[MAX_CLIENT];
HANDLE hUpdateEvent[MAX_CLIENT];
HANDLE hNewClient;


unsigned int CurClientNum = 0;
bool GameStart = false;

void CreateThreadControlEvents()
{
	// �ڵ� ���� �̺�Ʈ 1+MAX_CLIENT�� ����(���� ��ȣ, ���ȣ)
	// hUpdateEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		hUpdateEvent[i] = CreateEvent(NULL, FALSE, TRUE, NULL);
		hRecvAllEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	// ���ο� Ŭ���̾�Ʈ�� ������ ���� �̺�Ʈ(���ȣ)
	hNewClient = CreateEvent(NULL, FALSE, FALSE, NULL);
}

void DeleteThreadControlEvents()
{
	// �̺�Ʈ ����
	// CloseHandle(hUpdateEvent);
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		CloseHandle(hUpdateEvent[i]);
		CloseHandle(hRecvAllEvent[i]);
	}

	CloseHandle(hNewClient);
}

// Ŭ���̾�Ʈ ������ ��� ó��
DWORD WINAPI ProcessClient(LPVOID arg)
{
	SOCKET client_sock = (SOCKET)arg;
	SOCKADDR_IN clientaddr;
	int addrlen;
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

	// MAX_CLIENT���� ���� ���� Ŭ���̾�Ʈ�� �������� ����
	int ClientID = GameProcessFunc::CreateNewPlayer();
	if (ClientID == -1)
	{
		closesocket(client_sock);
		cout << "[TCP ����] Ŭ���̾�Ʈ ��������(�ο��ʰ�): IP �ּ�=" << inet_ntoa(clientaddr.sin_addr) << ", ��Ʈ��ȣ=" << ntohs(clientaddr.sin_port) << endl;
		SetEvent(hNewClient);
		return 0;
	}
	else
	{
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
	SetEvent(hNewClient);
	// ������ ��� �� ���� ������Ʈ ó��
	while (true)
	{
		// Ű ���� �޾ƿ�
		if (!GameProcessFunc::RecvInput(client_sock, ClientID))
		{
			--CurClientNum;
			SetEvent(hRecvAllEvent[ClientID]);
			break;
		}

		// ���ú긦 �޾Ҵٴ� ���� �˸�
		SetEvent(hRecvAllEvent[ClientID]);

		// ��� ������Ʈ�� ������Ʈ�� �����⸦ ��ٸ�
		WaitForSingleObject(hUpdateEvent[ClientID], INFINITE);

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
	MSG msg;
	while (true)
	{
		// ���ο� Ŭ���̾�Ʈ �����尡 ������ ��쿡�� hNewClient �̺�Ʈ�� ��ȣ���°� �� ������ ��ٸ���.
		// ��ٸ��� �ʰ� ��� �ݺ����� ��ȸ�ϸ�,
		// PlayerBuffer�� CurClientNum�� ���� ���ŵǾ
		// Update ������� �װ��� ���� �� �� ���� �ִ�. (���� �ݿ� X)
		// �ֳ��ϸ� ProcessGameUpdate�� ProcessClient �������
		// ���� ������ �ݺ��������� ������ ������� �ϰ� �ֱ� �����̴�.
		// ���Ӱ� ������ ProcessClient �����尡 �ݺ����� �����ϱ� ����
		// �����尣 �����ڿ��� PlayerBuffer�� CurClientNum�� �����Ϸ��� �õ��ϱ⿡
		// �� ������ ���ؼ��� ������ ������ �����ؾ� �Ѵ�.
		// ��, �̷��� �̺�Ʈ�� ����ؼ� �ֱ������� �߻��ϴ� �̺�Ʈ�� �ƴϱ� ������
		// ProcessClient �����尡 ���Ӱ� ������ ����
		// ProcessGameUpdate �����忡�� ���ο� Ŭ���̾�Ʈ�� �����Ǿ��ٰ� �޼����� ������.
		// ProcessGameUpdate�� �Ź� �ݺ����� ��ȸ�� ������ �޼��� ť�� üũ�ϰ�
		// �޼��� ť�� ���ο� Ŭ���̾�Ʈ�� �����Ǿ��ٴ� �޼����� Ȯ�εǸ�
		// hNewClient �̺�Ʈ�� ��ȣ���°� �� ������ ��ٸ���.
		// hNewClient�� ���ο� PorcessClient �����忡�� �ݺ����� �����ϱ� ����
		// PlayerBuffer�� CurClientNum�� ���� �����ϰ� ������ ��ȣ���·� �ٲ�Ƿ�,
		// ��������δ� ���ο� Ŭ���̾�Ʈ�� ������ ������ ���� ��� GameUpdate�� ������Ű�� �ȴ�.
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			if (msg.message == WM_USER)
				WaitForSingleObject(hNewClient, INFINITE);
		}
		if (CurClientNum == 0)
		{
			// ������ Ŭ���̾�Ʈ�� ���(���� Ŭ���̾�Ʈ ��: 0) Update�� ���ư��� �Ѵ�.
			// ��, Update�Ǿ�� �� ������Ʈ�� �������� �ʰ� �Ѵ�.
			if (GameStart)
			{
				GameProcessFunc::InitGameObject();
				//GameProcessFunc::CreateNewBoss();
				GameStart = false;
			}
			for (int i = 0; i < MAX_CLIENT; ++i)
				SetEvent(hRecvAllEvent[i]);
		}
		else
		{
			if (GameProcessFunc::CheckBossRaidStart() && GameStart == false)
			{
				GameProcessFunc::CreateNewBoss();
				GameStart = true;
			}

			// ���� ������ ������ Ŭ���̾�Ʈ�� �ش�Ǵ� hRecvAllEvent�� (hRecvAllEvent[ClientID])
			// ������ ������ hRecvAllEvent�� ���ؼ� ��ȣ���·� �ٲ��ش�.
			// (hUpdateEvent�� �����ϱ� ����.)
			int PlayerNullindexs[MAX_CLIENT];
			int PlayerNum = GameProcessFunc::FindNullPlayerIndex(PlayerNullindexs);
			for (int i = 0; i < PlayerNum; ++i)
				SetEvent(hRecvAllEvent[PlayerNullindexs[i]]);

			// ��� Ŭ���̾�Ʈ�κ��� Recv()�� Ȯ�� �� ������ ���
			WaitForMultipleObjects(MAX_CLIENT, hRecvAllEvent, TRUE, INFINITE);

			// ElapsedTime ���
			static DWORD g_PrevRenderTime = 0;
			static DWORD cur_Time = 0;
			static float eTime = 0.0f;

			if (g_PrevRenderTime == 0)
				g_PrevRenderTime = timeGetTime();
			cur_Time = timeGetTime();

			eTime = (float)(cur_Time - g_PrevRenderTime) / 1000.0f;
			g_PrevRenderTime = cur_Time;

			// ���� ������Ʈ Update
			GameProcessFunc::ProcessInput(eTime);
			GameProcessFunc::ProcessPhisics(eTime);
			GameProcessFunc::BossPattern(eTime);
			GameProcessFunc::ProcessCollision(eTime);

			// ���� ������Ʈ�� ������ ��� ������Ʈ�� �ݿ�
			GameProcessFunc::ResetCommunicationBuffer();
		}
		// ��� ������Ʈ�� ������Ʈ�� ������ �˸�
		// hUpdateEvent�� Ŭ���̾�Ʈ ������ŭ ��ȣ���·� �� ����:
		// hUpdateEvent�� �� ProcessClient���� ���ȣ ���·� �ٲܷ��� �ϸ�
		// Ư�� ProcessClient�� ���ȣ ������ hUpdateEvent�� ����ؼ� ��ȣ���°� �ɶ����� ��ٸ��� �ȴ�.
		// �̴� �� ProcessGameUpdate �����忡�Ե� ������ �ְ� �Ǿ�
		// hRecvAllEvent�� ���� ��� Ŭ���̾�Ʈ�� Recv�� �̷���������� Ȯ������ ���ϰ� �ȴ�.
		// �ֳ��ϸ� Ư�� ProcessClient�� hUpdateEvent�� ��ȣ���°� �� ������ ����ؼ� ��ٸ��� �־
		// hRecvAllEvent���� �ش� Ŭ���̾�Ʈ Event�� hRecvAllEvent[ClientID] ���� ����ؼ� ���ȣ ���·� �ӹ����� �ȴ�.
		// ProcessGameUpdate �����忡���� hRecvAllEvent�� ���� ��� Ŭ���̾�Ʈ�� Recv������
		// Ȯ���ؾ߸� ��μ� hUpdateEvent�� ��ȣ���°� �ȴ�.
		// �������ڸ�, ProcessClient������ hUpdateEvent�� ��ȣ���°� �Ǿ��
		// ���� �ݺ������� ���� Cycle�� hRecvAllEvent[ClientID]�� ��ȣ���·� �ٲ� �� �ִ�.
		// �׷��� ProcessGameUpdate�� hRecvAllEvent�� ���� ��� Ŭ���̾�Ʈ�� Recv�� �̷�������� Ȯ���ؾ߸�
		// hUpdateEvent�� ��ȣ���·� �ٲٰ� �ȴ�.
		// Ŭ���̾�Ʈ�� 1���� ������ �� ��Ȳ�� ������ ������
		// Ŭ���̾�Ʈ�� 2�� �̻���� ������ �߻��ϰ� �ȴ�.
		// �ֳ��ϸ� ProcessGameUpdate������ hUpdateEvent��� �̺�Ʈ ������ 1���ۿ� ���� ��Ȳ����
		// �ش� hUpdateEvent�� ��ȣ���·� �ٲٰ� �Ǵµ�
		// �̶�, ���� ProcessClient �����尡 �� �� 1���� hUpdateEvent�� ���ؼ� WaitForSingleObject()�� ����
		// ���ȣ ���·� �ٲٷ��� �õ��� �ϱ� �����̴�.
		// �̴� �� � ProcessClient �����尡 hUpdateEvent�� ���ȣ ���·� �ٲٰ� �Ǹ�
		// �ٸ� ProcessClient �����忡�� �̹� hUpdateEvent�� ���ȣ �����̱� ������
		// WaitForSingleObject()�� ���� ��� ����ϰ� �Ǵ� �������� ������ �ȴ�.
		// �̸� �ذ��ϱ� ���ؼ��� �� ProcessClient �����尡 hUpdateEvent�� �� �� ����������
		// ���ȣ���·� �ٲ� �� �ְ� ���ָ� �ȴ�.
		// ��, hUpdateEvent�� Ŭ���̾�Ʈ ����ŭ ����� �ȴ�.
		for (int i = 0; i < MAX_CLIENT; ++i)
			SetEvent(hUpdateEvent[i]);
	}
	return 0;
}