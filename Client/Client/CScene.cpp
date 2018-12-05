#include "stdafx.h"
#include "CScene.h"
#include <thread>
#include <time.h>
#include <timeapi.h>

using namespace std;

bool   bProgramExit = false;
bool   bRecvComplete = false;
HANDLE hCopyStart;
HANDLE hCopyComplete;
CRITICAL_SECTION cs;

CPlayScene::~CPlayScene()
{
	for (u_int i = 0; i < MAX_OBJECT_KIND; ++i)
		m_pRenderer->DeleteTexture(m_TextureIDs[i]);
	if (m_pRenderer) delete m_pRenderer;

	CloseHandle(hCopyStart);
	CloseHandle(hCopyComplete);
	DeleteCriticalSection(&cs);
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
	m_TextureIDs[KIND_PRESSURE_PLATE] = m_pRenderer->CreatePngTexture("./Resource/Graphic/PressurePlate.png");
	m_TextureIDs[KIND_PLAYER_HEART] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Heart.png");
	m_TextureIDs[KIND_BOSS_NAME] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Boss_Name.png");
	m_TextureIDs[KIND_GAME_FAIL] = m_pRenderer->CreatePngTexture("./Resource/Graphic/FAIL.png");
	m_TextureIDs[KIND_GAME_CLEAR] = m_pRenderer->CreatePngTexture("./Resource/Graphic/CLEAR.png");
	m_TextureIDs[KIND_PLAYER_DEAD] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Isaac_Dead.png");

	return m_pRenderer->IsInitialized();
}

bool CPlayScene::InitialObjects()
{
	m_GameInfo.Player_ClientID = -1;
	m_GameInfo.Boss_HP = -1;
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		m_GameInfo.Player_Index[i] = -1;
		m_GameInfo.Player_Hited[i] = false;
		m_GameInfo.Player_HP[i] = -1;
	}
	hCopyStart = CreateEvent(NULL, FALSE, FALSE, NULL);
	hCopyComplete = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&cs);
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
	// �� ������Ʈ���� Recv�� �Ϸ�� �������� Ȯ��
	EnterCriticalSection(&cs);
	bool RecvComplete = bRecvComplete;
	LeaveCriticalSection(&cs);
	if (RecvComplete) {
		static DWORD PrevComTime = 0;
		static DWORD Cur_ComTime = 0;

		SetEvent(hCopyStart);
		WaitForSingleObject(hCopyComplete, INFINITE);
		if (PrevComTime == 0)
			PrevComTime = timeGetTime();
		Cur_ComTime = timeGetTime();
		*com_elapsedTime = (float)(Cur_ComTime - PrevComTime) / 1000.0f;
		PrevComTime = Cur_ComTime;
	}
	else {
		//����� ���� ���� ��� Ŭ�� ��ü���� �������� ���� �����͸� �����
		for (u_int i = 0; i < MAX_OBJECT; ++i)
		{
			if (m_RenderObjects[i].Obj_Type != KIND_NULL)
			{
				if (!equal(m_RenderObjects[i].Obj_Velocity.magnitude(), 0.0f))
				{
					// �߷�(Gravity) = mg (g: �߷°��ӵ�(9.8m/s��)
					Vector Gravity = Vector(1.0f, 1.0f, 1.0f) * PLAYER_MASS * GravityAccelarationFactor;

					// ������(Friction) = uN (u: �������, N: �����׷�(-mg))
					Vector NormalForce = -Gravity;
					Vector Friction = PLAYER_FRICTION_FACTOR * NormalForce;
					Vector Direction = unit(m_RenderObjects[i].Obj_Velocity);
					Friction.i *= Direction.i;
					Friction.j *= Direction.j;
					Friction.k *= Direction.k;

					// �ܷ�(ExternalForce)
					Vector ExternalForce = /*Gravity + */Friction;

					//CGameObject::ApplyForce(ExternalForce, elapsedTime);

					// Calculate Acceleration
					Vector Acceleration = ExternalForce / PLAYER_MASS;
					Vector AfterVelocity = m_RenderObjects[i].Obj_Velocity + Acceleration * elapsedTime;

					if (cosine(AfterVelocity, m_RenderObjects[i].Obj_Velocity) < 0.0f) // �� ���� ���հ��� �а��� ��� = ������ ����(x,y,z)�� ��ȣ�� �ٸ� ������ 1�� �̻� ����.
						m_RenderObjects[i].Obj_Velocity = { 0.0f, 0.0f, 0.0f };
					else
						m_RenderObjects[i].Obj_Velocity = AfterVelocity;
				}

				// Calculation Position
				// ���ο� ��ġ = ���� ��ġ + �ӵ� * �ð�
				//m_HeadPosition += m_Velocity * ElapsedTime;
				m_RenderObjects[i].Obj_Pos += m_RenderObjects[i].Obj_Velocity * elapsedTime;


				// Player Object�� ���ؼ��� Sprite Sequence ��� (�ӵ��� ��� / Sprite �� ĳ���� ������ �ݿ��ؼ� Sequence ��ȯ)
				for (int j = 0; j < MAX_CLIENT; ++j)
				{
					if (m_GameInfo.Player_Index[j] == i)
					{
						Vector MoveDistance = m_RenderObjects[i].Obj_Velocity * elapsedTime;
						static float FootStep[MAX_CLIENT] = { 0.0f, 0.0f, 0.0f, 0.0f};
						if (abs(m_RenderObjects[i].Obj_Velocity.i) < 0.5f) // Move Up/Down Or Stop
						{
							if (m_RenderObjects[i].Obj_Pos_InTexture.y != 0)
							{
								m_RenderObjects[i].Obj_Pos_InTexture.y = 0;
								m_RenderObjects[i].Obj_Pos_InTexture.x = 0;
							}
							if (abs(MoveDistance.j) > 0.0f)
								FootStep[j] += abs(MoveDistance.j) / (PLAYER_FOOTSTEP_DIST_ONE_SEQUENCE);
						}
						else if (m_RenderObjects[i].Obj_Velocity.i < 0.0f) // Move Left
						{
							if (m_RenderObjects[i].Obj_Pos_InTexture.y != 1)
							{
								m_RenderObjects[i].Obj_Pos_InTexture.y = 1;
								m_RenderObjects[i].Obj_Pos_InTexture.x = 0;
							}
							FootStep[j] += abs(MoveDistance.i) / (PLAYER_FOOTSTEP_DIST_ONE_SEQUENCE);
						}
						else if (m_RenderObjects[i].Obj_Velocity.i > 0.0f) // Move Right
						{
							if (m_RenderObjects[i].Obj_Pos_InTexture.y != 2)
							{
								m_RenderObjects[i].Obj_Pos_InTexture.y = 2;
								m_RenderObjects[i].Obj_Pos_InTexture.x = 0;
							}
							FootStep[j] += abs(MoveDistance.i) / (PLAYER_FOOTSTEP_DIST_ONE_SEQUENCE);
						}
						// MoveDist = �̵� �Ÿ� : �̵� �ӵ� * ���� ����ð�
						// FootStep = 1���� : Isaac_Body.png�󿡼��� �� �� (10 Sequence)
						// FootStepDist = 1���� ���� : 17pixel(0.17m)
						// FootStepDist_OneSeq = 1Sequence �� ���� ���� : 17pixel / 10 Sequence = 1.7Pixel(0.017m)
						// MoveDist > FootStepDist_OneSeq �� ���
						// ���� SequenceNumber = ���� SequenceNumber + (MoveDist / FootStepDist_OneSeq)
						while (FootStep[j] - 1.0f > 0.0f)
						{
							FootStep[j] = (FootStep[j] - 1.0f < 0.0f) ? 0.0f : FootStep[j] - 1.0f;
							m_RenderObjects[i].Obj_Pos_InTexture.x = (m_RenderObjects[i].Obj_Velocity.magnitude() > 0.5f) ? (m_RenderObjects[i].Obj_Pos_InTexture.x + 1) % MAX_PLAYER_BODY_ANIMATION_SEQUENCE_X : 0;
						}
					}
				}
			}
		}
	}
}

void CPlayScene::RendrScene()
{
	// Draw Background
	m_pRenderer->DrawTextureRect(0.0f, 0.0f, 0.0f, WND_WIDTH, WND_HEIGHT, 1.0f, 1.0f, 1.0f, 1.0f, m_TextureIDs[KIND_BACKGROND]);
	if (m_GameInfo.Player_ClientID >= 0)
	{
		if(m_GameInfo.Boss_HP == 0)
			m_pRenderer->DrawTextureRect(0.0f, 0.0f, 0.0f, 599, 259, 1.0f, 1.0f, 1.0f, 1.0f, m_TextureIDs[KIND_GAME_FAIL]);
		else if (m_GameInfo.Player_HP[m_GameInfo.Player_ClientID] == 0)
			m_pRenderer->DrawTextureRect(0.0f, 0.0f, 0.0f, 545, 253, 1.0f, 1.0f, 1.0f, 1.0f, m_TextureIDs[KIND_GAME_FAIL]);
	}

	Point Pos;
	POINT Pos_InTexture = { 0,0 };
	fSIZE Size = { 0.0f, 0.0f, 0.0f };
	COLOR Color = {1.0f, 1.0f, 1.0f, 1.0f};
	int TextureID = 0;
	int Animation_Sequence_X = 0;
	int Animation_Sequence_Y = 0;
	
	float PlayerHitColor[MAX_CLIENT];
	static bool sign[MAX_CLIENT] = { false,false,false,false};
	static LONG PlayerDeadSeq_X[MAX_CLIENT] = { 0,0,0,0 };
	bool ColorChanged = false;
	bool PlayerDeaded = false;
	static int framecnt = 0;
	framecnt = (framecnt + 1) % 4;
	static int PlayerDeadFrame[MAX_CLIENT] = { 0, };

	for (int i = 0; i < MAX_OBJECT; ++i)
	{
		if (m_RenderObjects[i].Obj_Type != KIND_NULL)
		{
			ColorChanged = false;
			PlayerDeaded = false;
			for (int j = 0; j < MAX_CLIENT; ++j)
			{
				// Player Object �ǰݻ��¿� ���� ���� ����
				if (m_GameInfo.Player_Index[j] == i && m_GameInfo.Player_Hited[j] && framecnt == 3)
				{
					PlayerHitColor[j] = (sign[j]) ? 1.0f : 0.2f;
					Color = { PlayerHitColor[j], PlayerHitColor[j], PlayerHitColor[j], 1.0f };
					sign[j] = !sign[j];
					ColorChanged = true;
				}
				else if (m_GameInfo.Player_Index[j] + 1 == i)
					ColorChanged = true;

				// Player Object �������¿� ���� ���� ����
				if (m_GameInfo.Player_Index[j] == i && m_GameInfo.Player_HP[j] == 0)
				{
					PlayerDeadFrame[j] = (PlayerDeadFrame[j] + 1) % (FPS / MAX_PLAYER_DEAD_ANIMATION_SEQUENCE_X);
					Pos = m_RenderObjects[i].Obj_Pos;
					if(PlayerDeadFrame[j] == FPS / MAX_PLAYER_DEAD_ANIMATION_SEQUENCE_X - 1
						&& PlayerDeadSeq_X[j] < MAX_PLAYER_DEAD_ANIMATION_SEQUENCE_X - 1)
						PlayerDeadSeq_X[j]++;
					Pos_InTexture.x = PlayerDeadSeq_X[j];
					Size.Width = PLAYER_WIDTH / 2;
					Size.Height = PLAYER_HEIGHT / 2;
					TextureID = m_TextureIDs[KIND_PLAYER_DEAD];
					Animation_Sequence_X = MAX_PLAYER_DEAD_ANIMATION_SEQUENCE_X;
					Animation_Sequence_Y = MAX_PLAYER_DEAD_ANIMATION_SEQUENCE_Y;
					if (m_GameInfo.Player_Index[j] + 1 == i + 1) i++;
					PlayerDeaded = true;
				}
			}
			if(ColorChanged == false)
				Color = { 1.0f, 1.0f, 1.0f, 1.0f };


			// Obj_Type�� ���� ������
			if (PlayerDeaded == false)
			{
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
					Pos.z = 0.5f / RENDER_TRANSLATION_SCALE;
					break;
				case KIND_BOSS:
					Size.Width = BOSS_WIDTH / 2;
					Size.Height = BOSS_HEIGHT / 2;
					TextureID = m_TextureIDs[KIND_BOSS];
					Animation_Sequence_X = MAX_BOSS_ANIMATION_SEQUENCE_X;
					Animation_Sequence_Y = MAX_BOSS_ANIMATION_SEQUENCE_Y;
					break;
				case KIND_PRESSURE_PLATE:
					Size.Width = PRESSURE_PLATE_WIDTH;
					Size.Height = PRESSURE_PLATE_HEIGHT;
					TextureID = m_TextureIDs[KIND_PRESSURE_PLATE];
					Animation_Sequence_X = MAX_PRESSURE_PLATE_ANIMATION_SEQUENCE_X;
					Animation_Sequence_Y = MAX_PRESSURE_PLATE_ANIMATION_SEQUENCE_Y;
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
				}
			}

			// ���� ��ǥ��� ��ȯ
			Pos *= m_TranslationScale;
			Size.Width *= m_TranslationScale;
			Size.Height *= m_TranslationScale;

			m_pRenderer->DrawTextureRectHeightSeqXY
			(
				Pos.x, Pos.y, Pos.z, // Position In World Coordination
				Size.Width, Size.Height, // Size In World Coordination
				Color.a, Color.g, Color.b, Color.a, // Render Color
				TextureID, // Texture ID
				Pos_InTexture.x, Pos_InTexture.y, // Position In Texture Coordination
				Animation_Sequence_X, Animation_Sequence_Y // Max Sequence Count In Texture(Row, Column)
			);
		}
	}
	CPlayScene::RenderUI();
}

void CPlayScene::RenderUI()
{
	Point Pos;
	fSIZE Size = { 0.0f,0.0f,0.0f };
	POINT Pos_InTexture = { 0,0 };

	// Draw BossHP
	if (m_GameInfo.Boss_HP > 0)
	{
		// Background HP Rect
		Pos.x = UI_BOSS_HP_RECT_POS_X;
		Pos.y = UI_BOSS_HP_RECT_POS_Y;
		Size.Width = UI_BOSS_HP_RECT_WIDTH;
		Size.Height = UI_BOSS_HP_RECT_HEIGHT;
		m_pRenderer->DrawSolidRect(
			Pos.x, Pos.y, 0.0f,
			Size.Width, Size.Height,
			69.0f / 255, 56.0f / 255, 83.0f / 255, 1.0f);

		Pos.x -= (UI_BOSS_HP_RECT_WIDTH / 2)* ((float)(BOSS_INIT_HP - m_GameInfo.Boss_HP) / BOSS_INIT_HP);
		Size.Width = UI_BOSS_HP_RECT_WIDTH * ((float)m_GameInfo.Boss_HP / BOSS_INIT_HP);

		static Point destPos = { UI_BOSS_HP_RECT_POS_X, UI_BOSS_HP_RECT_POS_Y };
		static fSIZE destSize = { UI_BOSS_HP_RECT_WIDTH, UI_BOSS_HP_RECT_HEIGHT, 0.0f};
		destPos.x += (Pos.x - destPos.x) * 0.01f;
		destSize.Width += (Size.Width - destSize.Width) * 0.01f;

		// Decrease HP Rect
		m_pRenderer->DrawSolidRect(
			destPos.x, destPos.y, 0.0f,
			destSize.Width, destSize.Height,
			236.0f / 255, 180.0f / 255, 84.0f / 255, 1.0f);

		// ForeGround HP Rect
		m_pRenderer->DrawSolidRect(
			Pos.x, Pos.y, 0.0f,
			Size.Width, Size.Height,
			241.0f / 255, 99.0f / 255, 83.0f / 255, 1.0f);

		// Boss Name
		m_pRenderer->DrawTextureRect(
			UI_BOSS_HP_RECT_POS_X, UI_BOSS_HP_RECT_POS_Y, 0.0f,
			UI_BOSS_HP_RECT_WIDTH * 0.3f, UI_BOSS_HP_RECT_HEIGHT,
			1.0f, 1.0f, 1.0f, 1.0f, m_TextureIDs[KIND_BOSS_NAME]);
	}

	// Draw PlayerHP
	float PlayerHP_Cnt = (float)m_GameInfo.Player_HP[m_GameInfo.Player_ClientID] / 2;
	for (int i = 0; i < (int)(PLAYER_INIT_HP / 2.0f + 0.5f); ++i)
	{
		Pos.x = UI_PLAYER_HEART_POS_X + (i * UI_PLAYER_HEART_WIDTH * m_TranslationScale);
		Pos.y = UI_PLAYER_HEART_POS_Y;
		Pos.z = 0;

		if(PlayerHP_Cnt > 0.5f)
			Pos_InTexture.x = 0;
		else if (equal(PlayerHP_Cnt, 0.5f))
			Pos_InTexture.x = 1;
		else
			Pos_InTexture.x = 2;
		--PlayerHP_Cnt;

		m_pRenderer->DrawTextureRectHeightSeqXY
		(
			Pos.x, Pos.y, Pos.z,
			UI_PLAYER_HEART_WIDTH * m_TranslationScale, UI_PLAYER_HEART_HEIGHT * m_TranslationScale, // Size In World Coordination
			1.0f, 1.0f, 1.0f, 1.0f, // Render Color
			m_TextureIDs[KIND_PLAYER_HEART], // Texture ID
			Pos_InTexture.x, 0, // Position In Texture Coordination
			MAX_PLAYER_HEART_ANIMATION_SEQUENCE_X, MAX_PLAYER_HEART_ANIMATION_SEQUENCE_Y // Max Sequence Count In Texture(Row, Column)
		);
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

	static CommunicationData CommunicationBffuer[MAX_OBJECT];
	static CommunicationData2 CommunicationData_Sub;
	// recv(m_RenderObjects)
	retval = recvn(server_sock, (char*)CommunicationBffuer, sizeof(CommunicationData)*MAX_OBJECT, 0);
	if (retval == SOCKET_ERROR || retval == 0)
	{
		closesocket(server_sock);
		cout
			<< "[TCP Ŭ���̾�Ʈ] ���� ����: IP �ּ�=" << inet_ntoa(serveraddr.sin_addr)
			<< ", ��Ʈ��ȣ=" << ntohs(serveraddr.sin_port)
			<< endl;
		err_display((char*)"recv()");
	}
	// recv(m_GameInfo)
	retval = recvn(server_sock, (char*)&CommunicationData_Sub, sizeof(CommunicationData2), 0);
	if (retval == SOCKET_ERROR || retval == 0)
	{
		closesocket(server_sock);
		cout
			<< "[TCP Ŭ���̾�Ʈ] ���� ����: IP �ּ�=" << inet_ntoa(serveraddr.sin_addr)
			<< ", ��Ʈ��ȣ=" << ntohs(serveraddr.sin_port)
			<< endl;
		err_display((char*)"recv()");
	}
	EnterCriticalSection(&cs);
	bRecvComplete = true;
	LeaveCriticalSection(&cs);
	WaitForSingleObject(hCopyStart, INFINITE);
	memcpy_s(m_RenderObjects, sizeof(CommunicationData)*MAX_OBJECT, CommunicationBffuer, sizeof(CommunicationData)*MAX_OBJECT);
	memcpy_s(&m_GameInfo, sizeof(CommunicationData2), &CommunicationData_Sub, sizeof(CommunicationData2));
	SetEvent(hCopyComplete);
	EnterCriticalSection(&cs);
	bRecvComplete = false;
	LeaveCriticalSection(&cs);

	bool ShowGamestate = false;
	while (true)
	{
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
		retval = recvn(server_sock, (char*)CommunicationBffuer, sizeof(CommunicationData)*MAX_OBJECT, 0);
		if (retval == SOCKET_ERROR || retval == 0)
		{
			err_display((char*)"recv()");
			break;
		}
		// recv(m_GameInfo)
		retval = recvn(server_sock, (char*)&CommunicationData_Sub, sizeof(CommunicationData2), 0);
		if (retval == SOCKET_ERROR || retval == 0)
		{
			err_display((char*)"recv()");
			break;
		}

		EnterCriticalSection(&cs);
		bRecvComplete = true;
		LeaveCriticalSection(&cs);
		WaitForSingleObject(hCopyStart, INFINITE);
		//Point Sub_Pos = CommunicationBffuer[PLAYER_BODY_ID1].Obj_Pos - m_RenderObjects[PLAYER_BODY_ID1].Obj_Pos;
		//Vector Sub_Vel = CommunicationBffuer[PLAYER_BODY_ID1].Obj_Velocity - m_RenderObjects[PLAYER_BODY_ID1].Obj_Velocity;
		//std::cout << "SubPos:(" << Sub_Pos.x << "," << Sub_Pos.y << "," << Sub_Pos.z << ")" << std::endl;
		//std::cout << "SubVel:(" << Sub_Vel.i << "," << Sub_Vel.j << "," << Sub_Vel.k << ")" << std::endl;
		for (int i = 0; i < MAX_OBJECT; ++i)
		{
			m_RenderObjects[i].Obj_Type = CommunicationBffuer[i].Obj_Type;
			m_RenderObjects[i].Obj_Pos += (CommunicationBffuer[i].Obj_Pos - m_RenderObjects[i].Obj_Pos) * 0.5f;
			//m_RenderObjects[i].Obj_Velocity += (CommunicationBffuer[i].Obj_Velocity - m_RenderObjects[i].Obj_Velocity) * 0.5f;
			m_RenderObjects[i].Obj_Velocity = CommunicationBffuer[i].Obj_Velocity;
			m_RenderObjects[i].Obj_Pos_InTexture = CommunicationBffuer[i].Obj_Pos_InTexture;
			//m_RenderObjects[i].Obj_Pos_InTexture.x = (CommunicationBffuer[i].Obj_Pos_InTexture.x != 0) ? (int)((CommunicationBffuer[i].Obj_Pos_InTexture.x - m_RenderObjects[i].Obj_Pos_InTexture.x) * 0.5f + 0.5f) : 0;
			//m_RenderObjects[i].Obj_Pos_InTexture.y = CommunicationBffuer[i].Obj_Pos_InTexture.y;
		}
		//memcpy_s(m_RenderObjects, sizeof(CommunicationData)*MAX_OBJECT, CommunicationBffuer, sizeof(CommunicationData)*MAX_OBJECT);
		memcpy_s(&m_GameInfo, sizeof(CommunicationData2), &CommunicationData_Sub, sizeof(CommunicationData2));
		SetEvent(hCopyComplete);
		EnterCriticalSection(&cs);
		bRecvComplete = false;
		LeaveCriticalSection(&cs);

		this_thread::sleep_for(chrono::milliseconds(1000 / CPS));
	}

	cout
		<< "[TCP Ŭ���̾�Ʈ] ���� ����: IP �ּ�=" << inet_ntoa(serveraddr.sin_addr)
		<< ", ��Ʈ��ȣ=" << ntohs(serveraddr.sin_port)
		<< endl;
	closesocket(server_sock);
	exit(1);
}
