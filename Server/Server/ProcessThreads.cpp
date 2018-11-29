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
	// 자동 리셋 이벤트 1+MAX_CLIENT개 생성(각각 신호, 비신호)
	// hUpdateEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		hUpdateEvent[i] = CreateEvent(NULL, FALSE, TRUE, NULL);
		hRecvAllEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	// 새로운 클라이언트가 접속할 때의 이벤트(비신호)
	hNewClient = CreateEvent(NULL, FALSE, FALSE, NULL);
}

void DeleteThreadControlEvents()
{
	// 이벤트 제거
	// CloseHandle(hUpdateEvent);
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		CloseHandle(hUpdateEvent[i]);
		CloseHandle(hRecvAllEvent[i]);
	}

	CloseHandle(hNewClient);
}

// 클라이언트 데이터 통신 처리
DWORD WINAPI ProcessClient(LPVOID arg)
{
	SOCKET client_sock = (SOCKET)arg;
	SOCKADDR_IN clientaddr;
	int addrlen;
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

	// MAX_CLIENT보다 많은 수의 클라이언트를 수용하지 않음
	int ClientID = GameProcessFunc::CreateNewPlayer();
	if (ClientID == -1)
	{
		closesocket(client_sock);
		cout << "[TCP 서버] 클라이언트 접속제한(인원초과): IP 주소=" << inet_ntoa(clientaddr.sin_addr) << ", 포트번호=" << ntohs(clientaddr.sin_port) << endl;
		SetEvent(hNewClient);
		return 0;
	}
	else
	{
		// 접속 클라이언트 수 증가
		++CurClientNum;
		// 게임 오브젝트의 정보를 통신 오브젝트에 반영
		GameProcessFunc::ResetCommunicationBuffer();
		// 최초 게임 입장할 때, 처음 렌더링하기 위한 모든 오브젝트의 정보를 보내줌.
		if (!GameProcessFunc::SendCommunicationData(client_sock, ClientID))
		{
			closesocket(client_sock);
			cout << "[TCP 서버] 클라이언트 접속제한(인원초과): IP 주소=" << inet_ntoa(clientaddr.sin_addr) << ", 포트번호=" << ntohs(clientaddr.sin_port) << endl;
			return 0;
		}
	}
		
	cout << "[TCP 서버] 클라이언트(ID: " << ClientID << ") "
		<< "게임중: IP 주소=" << inet_ntoa(clientaddr.sin_addr) << ", 포트번호=" << ntohs(clientaddr.sin_port) << endl;
	SetEvent(hNewClient);
	// 데이터 통신 및 게임 오브젝트 처리
	while (true)
	{
		// 키 값을 받아옴
		if (!GameProcessFunc::RecvInput(client_sock, ClientID))
		{
			--CurClientNum;
			SetEvent(hRecvAllEvent[ClientID]);
			break;
		}

		// 리시브를 받았다는 것을 알림
		SetEvent(hRecvAllEvent[ClientID]);

		// 모든 오브젝트의 업데이트가 끝나기를 기다림
		WaitForSingleObject(hUpdateEvent[ClientID], INFINITE);

		// 업데이트 된 정보를 클라이언트에게 보냄
		if (!GameProcessFunc::SendCommunicationData(client_sock, ClientID))
		{
			--CurClientNum;
			break;
		}
	}

	// 소켓 메모리 해제(클라이언트 소켓부터.)
	closesocket(client_sock);
	cout << "[TCP 서버] 클라이언트 종료: IP 주소=" << inet_ntoa(clientaddr.sin_addr) << ", 포트번호=" << ntohs(clientaddr.sin_port) << endl;
	return 0;
}


// 게임 Update 처리
DWORD WINAPI ProcessGameUpdate(LPVOID arg)
{
	MSG msg;
	while (true)
	{
		// 새로운 클라이언트 스레드가 생성될 경우에는 hNewClient 이벤트가 신호상태가 될 때까지 기다린다.
		// 기다리지 않고 계속 반복문을 순회하면,
		// PlayerBuffer나 CurClientNum의 값이 갱신되어도
		// Update 스레드는 그것을 알지 못 할 수도 있다. (갱신 반영 X)
		// 왜냐하면 ProcessGameUpdate와 ProcessClient 스레드는
		// 서로 별도의 반복문에서만 스레드 순서제어를 하고 있기 때문이다.
		// 새롭게 생성한 ProcessClient 스레드가 반복문에 진입하기 전에
		// 스레드간 공유자원인 PlayerBuffer나 CurClientNum을 변경하려고 시도하기에
		// 이 시점에 대해서도 스레드 순서를 제어해야 한다.
		// 단, 이러한 이벤트는 계속해서 주기적으로 발생하는 이벤트가 아니기 때문에
		// ProcessClient 스레드가 새롭게 생성될 때만
		// ProcessGameUpdate 스레드에게 새로운 클라이언트가 생성되었다고 메세지를 보낸다.
		// ProcessGameUpdate는 매번 반복문을 순회할 때마다 메세지 큐를 체크하고
		// 메세지 큐에 새로운 클라이언트가 생성되었다는 메세지가 확인되면
		// hNewClient 이벤트가 신호상태가 될 때까지 기다린다.
		// hNewClient는 새로운 PorcessClient 스레드에서 반복문에 진입하기 전에
		// PlayerBuffer나 CurClientNum의 값을 갱신하고 나서야 신호상태로 바뀌므로,
		// 결과적으로는 새로운 클라이언트가 접속할 때마다 아주 잠깐씩 GameUpdate를 정지시키게 된다.
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			if (msg.message == WM_USER)
				WaitForSingleObject(hNewClient, INFINITE);
		}
		if (CurClientNum == 0)
		{
			// 접속한 클라이언트가 없어도(현재 클라이언트 수: 0) Update는 돌아가야 한다.
			// 단, Update되어야 할 오브젝트는 존재하지 않게 한다.
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

			// 현재 서버에 접속한 클라이언트에 해당되는 hRecvAllEvent를 (hRecvAllEvent[ClientID])
			// 제외한 나머지 hRecvAllEvent에 대해서 신호상태로 바꿔준다.
			// (hUpdateEvent를 수행하기 위함.)
			int PlayerNullindexs[MAX_CLIENT];
			int PlayerNum = GameProcessFunc::FindNullPlayerIndex(PlayerNullindexs);
			for (int i = 0; i < PlayerNum; ++i)
				SetEvent(hRecvAllEvent[PlayerNullindexs[i]]);

			// 모든 클라이언트로부터 Recv()가 확인 될 때까지 대기
			WaitForMultipleObjects(MAX_CLIENT, hRecvAllEvent, TRUE, INFINITE);

			// ElapsedTime 계산
			static DWORD g_PrevRenderTime = 0;
			static DWORD cur_Time = 0;
			static float eTime = 0.0f;

			if (g_PrevRenderTime == 0)
				g_PrevRenderTime = timeGetTime();
			cur_Time = timeGetTime();

			eTime = (float)(cur_Time - g_PrevRenderTime) / 1000.0f;
			g_PrevRenderTime = cur_Time;

			// 게임 오브젝트 Update
			GameProcessFunc::ProcessInput(eTime);
			GameProcessFunc::ProcessPhisics(eTime);
			GameProcessFunc::BossPattern(eTime);
			GameProcessFunc::ProcessCollision(eTime);

			// 게임 오브젝트의 정보를 통신 오브젝트에 반영
			GameProcessFunc::ResetCommunicationBuffer();
		}
		// 모든 오브젝트의 업데이트가 끝남을 알림
		// hUpdateEvent를 클라이언트 개수만큼 신호상태로 한 이유:
		// hUpdateEvent를 각 ProcessClient마다 비신호 상태로 바꿀려고 하면
		// 특정 ProcessClient는 비신호 상태인 hUpdateEvent를 계속해서 신호상태가 될때까지 기다리게 된다.
		// 이는 곧 ProcessGameUpdate 스레드에게도 영향을 주게 되어
		// hRecvAllEvent를 통해 모든 클라이언트의 Recv가 이루어졌는지를 확인하지 못하게 된다.
		// 왜냐하면 특정 ProcessClient는 hUpdateEvent가 신호상태가 될 때까지 계속해서 기다리고 있어서
		// hRecvAllEvent에서 해당 클라이언트 Event인 hRecvAllEvent[ClientID] 또한 계속해서 비신호 상태로 머무르게 된다.
		// ProcessGameUpdate 스레드에서는 hRecvAllEvent를 통해 모든 클라이언트가 Recv했음을
		// 확인해야만 비로소 hUpdateEvent가 신호상태가 된다.
		// 정리하자면, ProcessClient에서는 hUpdateEvent가 신호상태가 되어야
		// 현재 반복문에서 다음 Cycle의 hRecvAllEvent[ClientID]를 신호상태로 바꿀 수 있다.
		// 그러나 ProcessGameUpdate는 hRecvAllEvent를 통해 모든 클라이언트의 Recv가 이루어졌는지 확인해야만
		// hUpdateEvent를 신호상태로 바꾸게 된다.
		// 클라이언트가 1명일 때에는 위 상황이 문제가 없지만
		// 클라이언트가 2명 이상부터 문제가 발생하게 된다.
		// 왜냐하면 ProcessGameUpdate에서는 hUpdateEvent라는 이벤트 변수가 1개밖에 없는 상황에서
		// 해당 hUpdateEvent를 신호상태로 바꾸게 되는데
		// 이때, 여러 ProcessClient 스레드가 각 각 1개의 hUpdateEvent에 대해서 WaitForSingleObject()를 통해
		// 비신호 상태로 바꾸려고 시도를 하기 때문이다.
		// 이는 곧 어떤 ProcessClient 스레드가 hUpdateEvent를 비신호 상태로 바꾸게 되면
		// 다른 ProcessClient 스레드에선 이미 hUpdateEvent가 비신호 상태이기 때문에
		// WaitForSingleObject()를 통해 계속 대기하게 되는 딜레마에 빠지게 된다.
		// 이를 해결하기 위해서는 각 ProcessClient 스레드가 hUpdateEvent를 각 각 독립적으로
		// 비신호상태로 바꿀 수 있게 해주면 된다.
		// 즉, hUpdateEvent를 클라이언트 수만큼 만들면 된다.
		for (int i = 0; i < MAX_CLIENT; ++i)
			SetEvent(hUpdateEvent[i]);
	}
	return 0;
}