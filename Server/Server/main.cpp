#include "ProcessThreads.h"

int main(int argc, char argv[])
{
	// 게임 오브젝트 초기화
	GameProcessFunc::InitGameObject();

	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 대기 소켓 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit((char*)"socket()");

	// 대기 소켓의 통신 설정(Protocol, IPv4, PortNum)
	{
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));

		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		/*
		INADDR_ANY :
		서버가 ip주소를 여러개 갖을 경우 혹은 주기적으로 ip주소가 바뀔 경우를 대비해서
		서버에 해당하는 모든 ip주소로 접근이 가능하도록 허용하기 위함.
		*/
		serveraddr.sin_port = htons(PORT_NUM);
		retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR) err_quit((char*)"bind()");
	}

	// 클라이언트로부터 요청 대기(대기 소켓을 통해.)
	retval = listen(listen_sock, SOMAXCONN);
	/*
	backlog : 동시 접속 시 큐에 담아넣을 수 있는 최대 허용치
	가급적이면 SOMAXCONN으로 한다.
	Why?
	서버의 하드웨어가 바뀔 경우를 대비해서.
	*/
	if (retval == SOCKET_ERROR) err_quit((char*)"listen()");

	// 스레드 제어 이벤트 생성
	CreateThreadControlEvents();

	// 게임 Update 스레드 생성
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, ProcessGameUpdate, NULL, 0, NULL);
	if (hThread == NULL)
		err_quit((char*)"CreateThread()");
	else CloseHandle(hThread);

	cout << "[TCP 서버] 소켓 통신 준비 완료\n클라이언트 요청 대기중...";
	// 데이터 통신에 사용할 변수
	SOCKET client_sock = NULL;
	SOCKADDR_IN clientaddr;
	int addrlen;
	while (true)
	{
		// 서버에 접속한 클라이언트 확인 (대기 소켓을 통해 확인.)
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		/*
		클라이언트로부터 받은 요청이 있으면
		↓아래 명령 실행↓
		(accept()함수 내부에서 무한 반복해서 클라이언트의 접속을 대기한다.)
		*/
		if (client_sock == INVALID_SOCKET)
		{
			err_display((char*)"accept()");
			break;
		}
		cout << "[TCP 서버] 클라이언트 접속: IP 주소=" << inet_ntoa(clientaddr.sin_addr) << ", 포트번호=" << ntohs(clientaddr.sin_port) << endl;

		// 클라이언트와의 데이터 통신
		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock, 0, NULL);
		if (hThread == NULL)
		{
			// 소켓 메모리 해제(클라이언트 소켓부터.)
			closesocket(client_sock);
			cout << "[TCP 서버] 클라이언트 종료: IP 주소=" << inet_ntoa(clientaddr.sin_addr) << ", 포트번호=" << ntohs(clientaddr.sin_port) << endl;
		}
		else CloseHandle(hThread);
	}

	// 소켓 메모리 해제(서버의 대기소켓)
	closesocket(listen_sock);

	// 스레드 제어 이벤트 삭제
	DeleteThreadControlEvents();

	// 윈속 종료
	WSACleanup();
}