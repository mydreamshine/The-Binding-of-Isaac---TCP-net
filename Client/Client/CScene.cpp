#include "stdafx.h"
#include "CScene.h"

CRITICAL_SECTION cs;

CPlayScene::~CPlayScene()
{
	for (u_int i = 0; i < MAX_TEXTURE; ++i)
		m_pRenderer->DeleteTexture(m_TextureIDs[i]);
	if (m_pRenderer) delete m_pRenderer;
}

bool CPlayScene::InitialRenderer(int windowSizeX, int windowSizeY, float TranslationScale)
{
	if (m_pRenderer) delete m_pRenderer;
	m_pRenderer = new Renderer(windowSizeX, windowSizeY);

	m_TranslationScale = TranslationScale;
	m_TextureIDs[KIND_HERO_HEAD] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Isaac_Head.png");
	m_TextureIDs[KIND_HERO_BODY] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Isaac_Body.png");
	m_TextureIDs[KIND_BULLET_1] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Tear.png");
	m_TextureIDs[KIND_BULLET_2] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Blood_Tear.png");
	m_TextureIDs[KIND_BACKGROND] = m_pRenderer->CreatePngTexture("./Resource/Graphic/Background.png");
	/*
	m_TextureIDs[KIND_BOSS] = m_pRenderer->CreatePngTexture("");
	*/

	return m_pRenderer->IsInitialized();
}

bool CPlayScene::InitialObjects()
{
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

void CPlayScene::RendrScene()
{
	// Draw Background
	m_pRenderer->DrawTextureRect(0.0f, 0.0f, 0.0f, WND_WIDTH, WND_HEIGHT, 1.0f, 1.0f, 1.0f, 1.0f, m_TextureIDs[KIND_BACKGROND]);

	EnterCriticalSection(&cs);
	for (u_int i = 0; i < MAX_OBJECTS; ++i)
	{
		if (m_RenderObjects[i].Obj_Type != KIND_NULL)
		{
			// Obj_Type에 따른 렌더링
			// m_pRenderer->DrawTextureRectSeqXY();
		}
	}
	LeaveCriticalSection(&cs);
}

void CPlayScene::CommunicationWithServer(LPVOID arg)
{
	SOCKET sock = (SOCKET)arg;
	SOCKADDR_IN serveraddr;
	int addrlen = sizeof(serveraddr);
	getpeername(sock, (SOCKADDR*)&serveraddr, &addrlen);

	while (true)
	{
		// send(m_KeyState)
		// send(m_SpecialKeyState)

		EnterCriticalSection(&cs);
		// recv(m_RenderObjects)
		LeaveCriticalSection(&cs);
	}
	return;
}

