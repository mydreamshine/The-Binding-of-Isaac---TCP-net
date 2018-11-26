#include "stdafx.h"
#include "CScene.h"

HANDLE hWriteEvent;
HANDLE hReadEvent;
bool specialKeycheck[4] = { false };

CPlayScene::~CPlayScene()
{
	for (u_int i = 0; i < MAX_OBJECT_KIND; ++i)
		m_pRenderer->DeleteTexture(m_TextureIDs[i]);
	if (m_pRenderer) delete m_pRenderer;

	CloseHandle(hWriteEvent);
	CloseHandle(hReadEvent);
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
	hWriteEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	hReadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
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
	for (int i = 0; i < 4; i++) {
		if (specialKeycheck[i] == true) return;
	}
	switch (key)
	{
	case 0x0065:	// up
		specialKeycheck[0] = true;
		break;
	case 0x0067:	// down
		specialKeycheck[1] = true;
		break;
	case 0x0066:	// right
		specialKeycheck[2] = true;
		break;
	case 0x0064:	// left
		specialKeycheck[3] = true;
		break;
	}
	m_SpecialKeyState[key] = true;
}

void CPlayScene::SpecialKeyUp(int key, int x, int y)
{
	switch (key)
	{
	case 0x0065:	// up
		specialKeycheck[0] = false;
		break;
	case 0x0067:	// down
		specialKeycheck[1] = false;
		break;
	case 0x0066:	// right
		specialKeycheck[2] = false;
		break;
	case 0x0064:	// left
		specialKeycheck[3] = false;
		break;
	}
	for (int i = 0; i < 4; i++) {
		if (specialKeycheck[i] == true) return;
	}
	m_SpecialKeyState[key] = false;
}

void CPlayScene::RendrScene()
{
	// Draw Background
	m_pRenderer->DrawTextureRect(0.0f, 0.0f, 0.0f, WND_WIDTH, WND_HEIGHT, 1.0f, 1.0f, 1.0f, 1.0f, m_TextureIDs[KIND_BACKGROND]);

	// RenderObject의 쓰기 이벤트를 대기
	WaitForSingleObject(hWriteEvent, INFINITE);
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
	SetEvent(hReadEvent);
}

void CPlayScene::CommunicationWithServer(LPVOID arg)
{
	int retval;
	SOCKET server_sock = (SOCKET)arg;
	SOCKADDR_IN serveraddr;
	int addrlen = sizeof(serveraddr);
	getpeername(server_sock, (SOCKADDR*)&serveraddr, &addrlen);

	WaitForSingleObject(hReadEvent, INFINITE);
	// recv(m_RenderObjects)
	retval = recv(server_sock, (char*)m_RenderObjects, sizeof(CommunicationData)*MAX_OBJECT, 0);
	if (retval == SOCKET_ERROR || retval == 0)
	{
		closesocket(server_sock);
		std::cout
			<< "[TCP 클라이언트] 서버 종료: IP 주소=" << inet_ntoa(serveraddr.sin_addr)
			<< ", 포트번호=" << ntohs(serveraddr.sin_port)
			<< std::endl;
		err_display((char*)"recv()");
	}
	SetEvent(hWriteEvent);

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

		WaitForSingleObject(hReadEvent, INFINITE);
		// recv(m_RenderObjects)
		retval = recv(server_sock, (char*)m_RenderObjects, sizeof(CommunicationData)*MAX_OBJECT, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display((char*)"recv()");
			break;
		}
		else if (retval == 0) // 받은 데이터가 없을 때
			break;
		SetEvent(hWriteEvent);
	}

	std::cout
		<< "[TCP 클라이언트] 서버 종료: IP 주소=" << inet_ntoa(serveraddr.sin_addr)
		<< ", 포트번호=" << ntohs(serveraddr.sin_port)
		<< std::endl;
	closesocket(server_sock);
	exit(1);
}
