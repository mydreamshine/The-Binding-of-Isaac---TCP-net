#pragma once
#include "Renderer.h"
#include "Global.h"
#include <Windows.h>
#include <WinSock2.h>






// 통신에 쓰일 구조체
struct CommunicationData
{
	int    Obj_Type = KIND_NULL;
	Point  Obj_Pos;  // float x, y, z
	POINT  Obj_Pos_InTexture = { 0,0 }; // LONG x, y
	Vector Obj_Velocity;
};





// 응용프로그램에서 쓰일 Scene
class CScene;
class CPlayScene;













class CScene
{
protected:
	bool         m_KeyState[256] = { false, };
	bool         m_SpecialKeyState[246] = { false, };

	int          m_MouseButton;
	int          m_MouseState;
	int          m_MousePointX;
	int          m_MousePointY;

public:
	virtual bool InitialRenderer(int windowSizeX, int windowSizeY, float TranslationScale = 1.0f) { return false; }
	virtual bool InitialObjects() { return false; }

	virtual void MouseInput(int button, int state, int x, int y) {}
	virtual void MouseOperation(float elapsedTime) {}
	virtual void KeyPressed(unsigned char key, int x, int y) {}
	virtual void KeyUp(unsigned char key, int x, int y) {}
	virtual void SpecialKeyPressed(int key, int x, int y) {}
	virtual void SpecialKeyUp(int key, int x, int y) {}
	virtual void KeyOperation(float elapsedTime) {}
	virtual void SpecialKeyOperation(float elapsedTime) {}

	virtual void UpdateScene(float elapsedTime) {}
	virtual void RendrScene() {}

	virtual void CommunicationWithServer(LPVOID arg) {}
};












// 1pixel : 1cm, 게임상에서의 길이 최소단위는 1m
/*
m_TranslationScale = 100이면 게임상에서의 100pixel 즉, 1m의 스케일이다.
여기서 말하는 스케일이란 이동, 크기, 회전 등 물체의 모든 월드변환의 가중치를 의미한다.
*/

class CPlayScene : public CScene
{
private:
	float              m_TranslationScale = 1.0f;
	Renderer*          m_pRenderer = nullptr;
	CommunicationData  m_RenderObjects[MAX_OBJECT];
	u_int              m_CurrentObjectNum;
	u_int              m_TextureIDs[MAX_OBJECT_KIND];

public:
	CPlayScene() = default;
	~CPlayScene();

	virtual bool InitialRenderer(int windowSizeX, int windowSizeY, float TranslationScale = 1.0f);
	virtual bool InitialObjects();

	virtual void KeyPressed(unsigned char key, int x, int y);
	virtual void KeyUp(unsigned char key, int x, int y);
	virtual void SpecialKeyPressed(int key, int x, int y);
	virtual void SpecialKeyUp(int key, int x, int y);

	virtual void UpdateScene(float elapsedTime);
	virtual void RendrScene();

	virtual void CommunicationWithServer(LPVOID arg);
};