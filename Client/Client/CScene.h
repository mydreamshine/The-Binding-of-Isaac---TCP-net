#pragma once
#include "Renderer.h"
#include "Global.h"
#include <Windows.h>
#include <WinSock2.h>






// ��ſ� ���� ����ü
struct CommunicationData
{
	int    Obj_Type = KIND_NULL;
	Point  Obj_Pos;  // float x, y, z
	POINT  Obj_Pos_InTexture = { 0,0 }; // LONG x, y
	Vector Obj_Velocity;
};





// �������α׷����� ���� Scene
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












// 1pixel : 1cm, ���ӻ󿡼��� ���� �ּҴ����� 1m
/*
m_TranslationScale = 100�̸� ���ӻ󿡼��� 100pixel ��, 1m�� �������̴�.
���⼭ ���ϴ� �������̶� �̵�, ũ��, ȸ�� �� ��ü�� ��� ���庯ȯ�� ����ġ�� �ǹ��Ѵ�.
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