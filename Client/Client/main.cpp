#include "stdafx.h"
#include <iostream>
#include <time.h>
#include <timeapi.h>

#pragma comment(lib, "./Dependencies/freeglut")
#pragma comment(lib, "./Dependencies/glew32")
#pragma comment(lib, "./Dependencies/glew32s")
#include "Dependencies\glew.h"
#include "Dependencies\freeglut.h"

#include "CScene.h"

CScene* g_pScene = nullptr;

void RenderScene(void)
{

	static char FrameAndCommunicationRate[70] = "The Binding of Isaac - Client (";

	static DWORD g_PrevRenderTime = 0;
	static DWORD cur_Time = 0;
	static float eTime = 0.0f, Communcation_eTime = 0.0f;
	static size_t WndTitlelen;

	if (g_PrevRenderTime == 0)
		g_PrevRenderTime = timeGetTime();
	cur_Time = timeGetTime();
	eTime = (float)(cur_Time - g_PrevRenderTime) / 1000.0f;

	//일정 프레임이 나오도록 변경
	if (eTime > 1.f / FPS) {

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0f, 0.3f, 0.3f, 1.0f);

		g_PrevRenderTime = cur_Time;
		_itoa_s((int)(1.0f / eTime), FrameAndCommunicationRate + 31, 39, 10);
		strcat_s(FrameAndCommunicationRate + 31, 39, " FPS, ");

		g_pScene->UpdateScene(eTime, &Communcation_eTime);

		WndTitlelen = strlen(FrameAndCommunicationRate);
		_itoa_s((int)(1.0f / Communcation_eTime), FrameAndCommunicationRate + WndTitlelen, 70 - WndTitlelen, 10);
		strcat_s(FrameAndCommunicationRate + WndTitlelen, 70 - WndTitlelen, " CPS)");

		glutSetWindowTitle(FrameAndCommunicationRate);

		g_pScene->RendrScene();

		glutSwapBuffers();
	}
}

void Idle(void)
{
	RenderScene();
}

void MouseInput(int button, int state, int x, int y)
{
	g_pScene->MouseInput(button, state, x, y);
}

void KeyPressed(unsigned char key, int x, int y)
{
	g_pScene->KeyPressed(key, x, y);
}

void KeyUp(unsigned char key, int x, int y)
{
	g_pScene->KeyUp(key, x, y);
}

void SpecialKeyPressed(int key, int x, int y)
{
	g_pScene->SpecialKeyPressed(key, x, y);
}

void SpecialKeyUp(int key, int x, int y)
{
	g_pScene->SpecialKeyUp(key, x, y);
}

DWORD WINAPI CommunicationWithServer(LPVOID arg)
{
	if(g_pScene) g_pScene->CommunicationWithServer(arg);
	return 0;
}


int main(int argc, char **argv)
{
	////////////////////////////// Initialize GL things  //////////////////////////////
	glutInit(&argc, argv);														     //
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);					     //
	glutInitWindowPosition(0, 0);												     //
	glutInitWindowSize(WND_WIDTH, WND_HEIGHT);									     //
	glutCreateWindow("The Binding of Isaac - Client");							     //
	srand(static_cast <unsigned> (time(0)));									     //
																				     //
	glewInit();																	     //
	if (glewIsSupported("GL_VERSION_3_0"))										     //
	{																			     //
		std::cout << " GLEW Version is 3.0\n ";									     //
	}																			     //
	else																		     //
	{																			     //
		std::cout << "GLEW 3.0 not supported\n ";								     //
	}																			     //
																				     //
	// Initialize Renderer (WndWidth, WndHeight, TranslationScale)				     //
	g_pScene = new CPlayScene();												     //
	if (!g_pScene->InitialRenderer(WND_WIDTH, WND_HEIGHT, RENDER_TRANSLATION_SCALE)) //
	{																			     //
		std::cout << "Renderer could not be initialized.. \n";					     //
		delete g_pScene;														     //
		return 0;																     //
	}																			     //
	// Initialize Object														     //
	if (!g_pScene->InitialObjects())											     //
	{																			     //
		std::cout << "Objects could not be initialized.. \n";					     //
		delete g_pScene;														     //
		return 0;																     //
	}																			     //
																				     //
	// Setting GL Callback Function												     //
	glutDisplayFunc(RenderScene);												     //
	glutIdleFunc(Idle);															     //
																				     //
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);										     //
	glutKeyboardFunc(KeyPressed);												     //
	glutKeyboardUpFunc(KeyUp);													     //
	glutSpecialFunc(SpecialKeyPressed);											     //
	glutSpecialUpFunc(SpecialKeyUp);											     //
	glutMouseFunc(MouseInput);													     //
	///////////////////////////////////////////////////////////////////////////////////

	// Create Thread(CommunicationWithServer)
	HANDLE hThread = CreateThread(NULL, 0, CommunicationWithServer, NULL, 0, NULL);
	if (hThread != NULL) CloseHandle(hThread);

	glutMainLoop();

	delete g_pScene;
	g_pScene = NULL;
    return 0;
}

