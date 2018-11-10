#include "ProcessThreads.h"

HANDLE hRecvAllEvent[MAX_CLIENT];
HANDLE hUpdateEvent;


void CreateThreadControlEvents()
{
	// 자동 리셋 이벤트 1+MAX_CLIENT개 생성(각각 신호, 비신호)
	hUpdateEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	for (int i = 0; i < MAX_CLIENT; ++i)
		hRecvAllEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
}

void DeleteThreadControlEvents()
{
	// 이벤트 제거
	CloseHandle(hUpdateEvent);
	for (int i = 0; i < MAX_CLIENT; ++i)
		CloseHandle(hRecvAllEvent[i]);
}

// 클라이언트 데이터 통신 처리
DWORD WINAPI ProcessClient(LPVOID arg)
{
	SOCKET client_sock = (SOCKET)arg;
	SOCKADDR_IN clientaddr;
	int addrlen;
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

	// 게임 오브젝트가 Update가 완료 될 때까지 대기
	WaitForSingleObject(hUpdateEvent, INFINITE);

	int ClientID = GameProcessFunc::CreateNewPlayer();
	if (ClientID == -1)
	{
		closesocket(client_sock);
		cout << "[TCP 서버] 클라이언트 접속제한(인원초과): IP 주소=" << inet_ntoa(clientaddr.sin_addr) << ", 포트번호=" << ntohs(clientaddr.sin_port) << endl;
		return 0;
	}
	else
	{
		// 게임 오브젝트의 정보를 통신 오브젝트에 반영
		GameProcessFunc::ResetCommunicationBuffer();
		// 최초 게임 입장할 때, 처음 렌더링하기 위한 모든 오브젝트의 정보를 보내줌.
		if (!GameProcessFunc::SendCommunicationData(client_sock))
		{
			closesocket(client_sock);
			cout << "[TCP 서버] 클라이언트 접속제한(인원초과): IP 주소=" << inet_ntoa(clientaddr.sin_addr) << ", 포트번호=" << ntohs(clientaddr.sin_port) << endl;
			return 0;
		}
	}
		
	cout << "[TCP 서버] 클라이언트(ID: " << ClientID << ") "
		<< "게임중: IP 주소=" << inet_ntoa(clientaddr.sin_addr) << ", 포트번호=" << ntohs(clientaddr.sin_port) << endl;
	// 데이터 통신 및 게임 오브젝트 처리
	while (true)
	{
		// 키 값을 받아옴
		if (!GameProcessFunc::RecvInput(client_sock, ClientID))
			break;

		// 리시브를 받았다는 것을 알림
		//SetEvent(hRecvAllEvent[ClientID]);

		////////// Server : Client = 1 : 1 일 때 TEST용 //////////
		for (int i = 0; i < MAX_CLIENT; ++i)					//
			SetEvent(hRecvAllEvent[i]);							//
		////////// TEST가 아닐 경우에는 주석 처리 할 것! /////////

		// 모든 오브젝트의 업데이트가 끝나기를 기다림
		WaitForSingleObject(hUpdateEvent, INFINITE);

		// 업데이트 된 정보를 클라이언트에게 보냄
		if (!GameProcessFunc::SendCommunicationData(client_sock))
			break;
	}

	// 소켓 메모리 해제(클라이언트 소켓부터.)
	closesocket(client_sock);
	cout << "[TCP 서버] 클라이언트 종료: IP 주소=" << inet_ntoa(clientaddr.sin_addr) << ", 포트번호=" << ntohs(clientaddr.sin_port) << endl;
	return 0;
}


// 게임 Update 처리
DWORD WINAPI ProcessGameUpdate(LPVOID arg)
{
	while (true)
	{
		// 모든 클라이언트로부터 Recv()가 확인 될 때까지 대기
		WaitForMultipleObjects(MAX_CLIENT, hRecvAllEvent, TRUE, INFINITE);

		// ElapsedTime 계산(계산 수정요)
		float ElapsedTime = 1.0f / 60;

		// 게임 오브젝트 Update
		GameProcessFunc::ProcessInput(ElapsedTime);
		GameProcessFunc::ProcessPhisics(ElapsedTime);
		GameProcessFunc::ProcessCollision(ElapsedTime);

		// 게임 오브젝트의 정보를 통신 오브젝트에 반영
		GameProcessFunc::ResetCommunicationBuffer();

		// 모든 오브젝트의 업데이트가 끝남을 알림
		SetEvent(hUpdateEvent);
	}
	return 0;
}