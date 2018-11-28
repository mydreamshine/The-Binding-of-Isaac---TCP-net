#include "stdafx.h"
#include "CScene.h"
#include <thread>
#include <time.h>
#include <timeapi.h>

using namespace std;

HANDLE hCallCommunicationEvent;
HANDLE hCompleteCommunicaitionEvent;
bool   bRecvComplete = false;

CommunicationData CommuncationBuffer[MAX_OBJECT];

CPlayScene::~CPlayScene()
{
	for (u_int i = 0; i < MAX_OBJECT_KIND; ++i)
		m_pRenderer->DeleteTexture(m_TextureIDs[i]);
	if (m_pRenderer) delete m_pRenderer;

	CloseHandle(hCallCommunicationEvent);
	CloseHandle(hCompleteCommunicaitionEvent);
}

bool CPlayScene::InitialRenderer(int windowSizeX, int windowSizeY, float TranslationScale)
{
	if (m_pRenderer) delete m_pRenderer;
	m_pRenderer = new Renderer(windowSizeX, windowSizeY);

	m_TranslationScale = TranslationScale;
	m_TextureIDs[KIND_PLAYER_HEAD] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Isaac_Head.png");
	m_TextureIDs[KIND_PLAYER_BODY] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Isaac_Body.png");
	m_TextureIDs[KIND_BULLET_1] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Tear.png");
	m_TextureIDs[KIND_BULLET_2] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Blood_Tear.png");
	m_TextureIDs[KIND_BACKGROND] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Background.png");
	m_TextureIDs[KIND_BOSS] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Boss.png");

	return m_pRenderer->IsInitialized();
}

bool CPlayScene::InitialObjects()
{
	hCallCommunicationEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	hCompleteCommunicaitionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	return true;
}


void CPlayScene::KeyPressed(unsigned char key, int x, int y)
{
	m_KeyState[key] = true;
}

void CPlayScene::KeyUp(unsigned char key, int x, int y)
{
	m_KeyState[key] = false;
}

void CPlayScene::SpecialKeyPressed(int key, int x, int y)
{
	m_SpecialKeyState[key] = true;
}

void CPlayScene::SpecialKeyUp(int key, int x, int y)
{
	m_SpecialKeyState[key] = false;
}

void CPlayScene::UpdateScene(float elapsedTime, float* com_elapsedTime)
{
	//업데이트에서 일정 시간마다 서버와의 통신 스레드를 부르도록 설정

	static float communication_eTime = 0.0f;
	communication_eTime += elapsedTime;

	//일정 시간 마다 서버와의 통신을 함
	if (1.f / COMUNICATIONFRAME < (communication_eTime + 1.f / DRAWFRAME)) {
		//서버와의 통신 설정  //서버에서 그릴 것들을 가져옴
		SetEvent(hCallCommunicationEvent);

		//통신이 완료 될 때 까지 기다림
		WaitForSingleObject(hCompleteCommunicaitionEvent, INFINITE);

		static DWORD PrevComTime = 0;
		static DWORD Cur_ComTime = 0;
		if (PrevComTime == 0)
			PrevComTime = timeGetTime();
		Cur_ComTime = timeGetTime();
		*com_elapsedTime = (float)(Cur_ComTime - PrevComTime) / 1000.0f;
		PrevComTime = Cur_ComTime;

		//커뮤니케이션 시간이 지난 시간을 초기화
		communication_eTime = 0;
	}
	else {
		//통신을 하지 않을 경우 클라 자체에서 랜더링을 위한 데이터를 계산함
		for (u_int i = 0; i < MAX_OBJECT; ++i)
		{
			if (m_RenderObjects[i].Obj_Type != KIND_NULL)
			{
				if (!equal(m_RenderObjects[i].Obj_Velocity.magnitude(), 0.0f))
				{
					// 중력(Gravity) = mg (g: 중력가속도(9.8m/s²)
					Vector Gravity = Vector(1.0f, 1.0f, 1.0f) * PLAYER_MASS * GravityAccelarationFactor;

					// 마찰력(Friction) = uN (u: 마찰계수, N: 수직항력(-mg))
					Vector NormalForce = -Gravity;
					Vector Friction = PLAYER_FRICTION_FACTOR * NormalForce;
					Vector Direction = unit(m_RenderObjects[i].Obj_Velocity);
					Friction.i *= Direction.i;
					Friction.j *= Direction.j;
					Friction.k *= Direction.k;

					// 외력(ExternalForce)
					Vector ExternalForce = /*Gravity + */Friction;

					//CGameObject::ApplyForce(ExternalForce, elapsedTime);

					// Calculate Acceleration
					Vector Acceleration = ExternalForce / PLAYER_MASS;
					Vector AfterVelocity = m_RenderObjects[i].Obj_Velocity + Acceleration * elapsedTime;

					if (cosine(AfterVelocity, m_RenderObjects[i].Obj_Velocity) < 0.0f) // 두 벡터 사잇각이 둔각일 경우 = 벡터의 성분(x,y,z)중 부호가 다른 성분이 1개 이상 존재.
						m_RenderObjects[i].Obj_Velocity = { 0.0f, 0.0f, 0.0f };
					else
						m_RenderObjects[i].Obj_Velocity = AfterVelocity;
				}

				// Calculation Position
				// 새로운 위치 = 이전 위치 + 속도 * 시간
				//m_HeadPosition += m_Velocity * ElapsedTime;
				m_RenderObjects[i].Obj_Pos += m_RenderObjects[i].Obj_Velocity * elapsedTime;
			}
		}
	}
	
}

void CPlayScene::RendrScene()
{
	// Draw Background
	m_pRenderer->DrawTextureRect(0.0f, 0.0f, 0.0f, WND_WIDTH, WND_HEIGHT, 1.0f, 1.0f, 1.0f, 1.0f, m_TextureIDs[KIND_BACKGROND]);

	Point Pos;
	POINT Pos_InTexture;
	fSIZE Size;
	int TextureID = 0;
	int Animation_Sequence_X = 1;
	int Animation_Sequence_Y = 1;

	for (u_int i = 0; i < MAX_OBJECT; ++i)
	{
		if (m_RenderObjects[i].Obj_Type != KIND_NULL)
		{
			// Obj_Type에 따른 렌더링
			Pos = m_RenderObjects[i].Obj_Pos;
			Pos_InTexture = m_RenderObjects[i].Obj_Pos_InTexture;

			switch (m_RenderObjects[i].Obj_Type)
			{
			case KIND_PLAYER_HEAD:
				Size.Width = PLAYER_WIDTH / 2;
				Size.Height = PLAYER_HEIGHT / 2;
				TextureID = m_TextureIDs[KIND_PLAYER_HEAD];
				Animation_Sequence_X = MAX_PLAYER_HEAD_ANIMATION_SEQUENCE_X;
				Animation_Sequence_Y = MAX_PLAYER_HEAD_ANIMATION_SEQUENCE_Y;
				break;
			case KIND_PLAYER_BODY:
				Size.Width = PLAYER_WIDTH / 2;
				Size.Height = PLAYER_HEIGHT / 2;
				TextureID = m_TextureIDs[KIND_PLAYER_BODY];
				Animation_Sequence_X = MAX_PLAYER_BODY_ANIMATION_SEQUENCE_X;
				Animation_Sequence_Y = MAX_PLAYER_BODY_ANIMATION_SEQUENCE_Y;
				break;
			case KIND_BOSS:
				Size.Width = BOSS_WIDTH;
				Size.Height = BOSS_HEIGHT;
				TextureID = m_TextureIDs[KIND_BOSS];
				Animation_Sequence_X = MAX_BOSS_ANIMATION_SEQUENCE_X;
				Animation_Sequence_Y = MAX_BOSS_ANIMATION_SEQUENCE_Y;
				break;
			case KIND_BULLET_1:
				Size.Width = BULLET_WIDTH;
				Size.Height = BULLET_HEIGHT;
				TextureID = m_TextureIDs[KIND_BULLET_1];
				Animation_Sequence_X = 1;
				Animation_Sequence_Y = 1;
				break;
			case KIND_BULLET_2:
				Size.Width = BULLET_WIDTH;
				Size.Height = BULLET_HEIGHT;
				TextureID = m_TextureIDs[KIND_BULLET_2];
				Animation_Sequence_X = 1;
				Animation_Sequence_Y = 1;
				break;
			default:
				break;
			}

			// 월드 좌표계로 변환
			Pos *= m_TranslationScale;
			Size.Width *= m_TranslationScale;
			Size.Height *= m_TranslationScale;

			m_pRenderer->DrawTextureRectHeightSeqXY
			(
				Pos.x, Pos.y, Pos.z, // Position In World Coordination
				Size.Width, Size.Height, // Size In World Coordination
				1.0f, 1.0f, 1.0f, 1.0f, // Render Color
				TextureID, // Texture ID
				Pos_InTexture.x, Pos_InTexture.y, // Position In Texture Coordination
				Animation_Sequence_X, Animation_Sequence_Y // Max Sequence Count In Texture(Row, Column)
			);
		}
	}
}

void CPlayScene::CommunicationWithServer(LPVOID arg)
{
	//////////////////////////////////////// Initialize Winsock  ////////////////////////////////////////
	int retval;																						   //
	WSADATA wsa;																					   //
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)														   //
		return;																					       //
																									   //
	// Create Socket																				   //
	SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);											   //
	if (server_sock == INVALID_SOCKET)																   //
		err_quit((char*)"socket()");																   //
																									   //
	// Setting Socket(Protocol, IPv4, PortNum) <- Server Information								   //
	SOCKADDR_IN serveraddr;																			   //
	ZeroMemory(&serveraddr, sizeof(serveraddr));													   //
																									   //
	serveraddr.sin_family = AF_INET;																   //
	serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);											   //
	serveraddr.sin_port = htons(SERVER_PORT);														   //
																									   //
	retval = connect(server_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));						   //
	if (retval == SOCKET_ERROR)																		   //
		err_quit((char*)"connect()");																   //
	/////////////////////////////////////////////////////////////////////////////////////////////////////

	WaitForSingleObject(hCallCommunicationEvent, INFINITE);
	// recv(m_RenderObjects)
	retval = recv(server_sock, (char*)m_RenderObjects, sizeof(CommunicationData)*MAX_OBJECT, 0);
	if (retval == SOCKET_ERROR || retval == 0)
	{
		closesocket(server_sock);
		cout
			<< "[TCP 클라이언트] 서버 종료: IP 주소=" << inet_ntoa(serveraddr.sin_addr)
			<< ", 포트번호=" << ntohs(serveraddr.sin_port)
			<< endl;
		err_display((char*)"recv()");
	}
	SetEvent(hCompleteCommunicaitionEvent);

	while (true)
	{
		//서버 통신을 부르길 기다림
		WaitForSingleObject(hCallCommunicationEvent, INFINITE);
		// send(m_KeyState)
		retval = send(server_sock, (char*)m_KeyState, sizeof(bool) * 256, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display((char*)"send()");
			break;
		}
		// send(m_SpecialKeyState)
		retval = send(server_sock, (char*)m_SpecialKeyState, sizeof(bool) * 246, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display((char*)"send()");
			break;
		}

		// recv(m_RenderObjects)
		retval = recv(server_sock, (char*)CommuncationBuffer, sizeof(CommunicationData)*MAX_OBJECT, 0);
		if (retval == SOCKET_ERROR || retval == 0)
		{
			err_display((char*)"recv()");
			break;
		}

		// 데이터 보간
		for (int i = 0; i < MAX_OBJECT; ++i)
		{
			if (CommuncationBuffer[i].Obj_Type != KIND_NULL)
			{
				Point subPos = CommuncationBuffer[i].Obj_Pos - m_RenderObjects[i].Obj_Pos;
				//cout << "sub Pos: (" << subPos.x << "," << subPos.y << ")" << endl;
				m_RenderObjects[i].Obj_Type = CommuncationBuffer[i].Obj_Type;
				m_RenderObjects[i].Obj_Pos += (CommuncationBuffer[i].Obj_Pos - m_RenderObjects[i].Obj_Pos)*0.3f;
				//m_RenderObjects[i].Obj_Velocity += (CommuncationBuffer[i].Obj_Velocity - m_RenderObjects[i].Obj_Velocity)*0.5;
				m_RenderObjects[i].Obj_Pos_InTexture = CommuncationBuffer[i].Obj_Pos_InTexture;
			}
		}

		//ZeroMemory(m_RenderObjects, sizeof(CommunicationData)*MAX_OBJECT);
		memcpy_s(m_RenderObjects, sizeof(CommunicationData)*MAX_OBJECT, CommuncationBuffer, sizeof(CommunicationData)*MAX_OBJECT);
		
		//더이상 랜더신에서 서버와의 동기화를 고려하지 않음.
		bRecvComplete = true;

		//서버 통신이 완료 됬음을 알림
		SetEvent(hCompleteCommunicaitionEvent);

		//서버 스레드가 잠자도록 하지 않도록 변경
		//this_thread::sleep_for(50ms);
	}

	cout
		<< "[TCP 클라이언트] 서버 종료: IP 주소=" << inet_ntoa(serveraddr.sin_addr)
		<< ", 포트번호=" << ntohs(serveraddr.sin_port)
		<< endl;
	closesocket(server_sock);
	exit(1);
}
