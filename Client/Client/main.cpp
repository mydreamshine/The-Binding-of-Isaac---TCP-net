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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.3f, 0.3f, 1.0f);

	g_pScene->RendrScene();

	glutSwapBuffers();

	static char m_pszFrameRate[50] = "The Binding of Isaac - Client (";

	static DWORD g_PrevRenderTime = 0;
	static DWORD cur_Time = 0;
	static float eTime = 0.0f;

	if (g_PrevRenderTime == 0)
		g_PrevRenderTime = timeGetTime();
	cur_Time = timeGetTime();
	eTime = (float)(cur_Time - g_PrevRenderTime) / 1000.0f;
	g_PrevRenderTime = cur_Time;
	_itoa_s((int)(1.0f/ eTime), m_pszFrameRate + 31, 19, 10);
	strcat_s(m_pszFrameRate + 31, 19, " FPS)");
	glutSetWindowTitle(m_pszFrameRate);

	// frame : ���������� FPS �� ����� �ĺ��� ��������� �����Ӽ�
	// time - ������ �и���
	// timebase - ���������� FPS �� ������� ���� �и���
	//static int frame, time, timebase;
	/*frame++;
	time = glutGet(GLUT_ELAPSED_TIME);
	if (time - timebase > 1000)
	{
		_itoa_s((int)(frame*1000.0f / (time - timebase)), m_pszFrameRate + 31, 19, 10);
		strcat_s(m_pszFrameRate + 31, 19, " FPS)");
		glutSetWindowTitle(m_pszFrameRate);
		timebase = time;
		frame = 0;
	}*/
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

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 9000
DWORD WINAPI CommunicationWithServer(LPVOID arg)
{
	if(g_pScene) g_pScene->CommunicationWithServer(arg);
	return 0;
}


int main(int argc, char **argv)
{
	///////////////////////////// Initialize GL things  /////////////////////////////
	glutInit(&argc, argv);														   //
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);					   //
	glutInitWindowPosition(0, 0);												   //
	glutInitWindowSize(WND_WIDTH, WND_HEIGHT);									   //
	glutCreateWindow("The Binding of Isaac - Client");							   //
	srand(static_cast <unsigned> (time(0)));									   //
																				   //
	glewInit();																	   //
	if (glewIsSupported("GL_VERSION_3_0"))										   //
	{																			   //
		std::cout << " GLEW Version is 3.0\n ";									   //
	}																			   //
	else																		   //
	{																			   //
		std::cout << "GLEW 3.0 not supported\n ";								   //
	}																			   //
																				   //
	// Initialize Renderer (WndWidth, WndHeight, TranslationScale)				   //
	g_pScene = new CPlayScene();												   //
	if (!g_pScene->InitialRenderer(WND_WIDTH, WND_HEIGHT, 100.0f))				   //
	{																			   //
		std::cout << "Renderer could not be initialized.. \n";					   //
		delete g_pScene;														   //
		return 0;																   //
	}																			   //
	// Initialize Object														   //
	if (!g_pScene->InitialObjects())											   //
	{																			   //
		std::cout << "Objects could not be initialized.. \n";					   //
		delete g_pScene;														   //
		return 0;																   //
	}																			   //
																				   //
	// Setting GL Callback Function												   //
	glutDisplayFunc(RenderScene);												   //
	glutIdleFunc(Idle);															   //
																				   //
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);										   //
	glutKeyboardFunc(KeyPressed);												   //
	glutKeyboardUpFunc(KeyUp);													   //
	glutSpecialFunc(SpecialKeyPressed);											   //
	glutSpecialUpFunc(SpecialKeyUp);											   //
	glutMouseFunc(MouseInput);													   //
																				   //
	glutMainLoop();																   //
	/////////////////////////////////////////////////////////////////////////////////


	

	//////////////////////////////////////// Initialize Winsock  ////////////////////////////////////////
	int retval;																						   //
	WSADATA wsa;																					   //
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)														   //
	{																								   //
		delete g_pScene;																			   //
		return 1;																					   //
	}																								   //
																									   //
	// Create Socket																				   //
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);													   //
	if (sock == INVALID_SOCKET)																		   //
	{																								   //
		delete g_pScene;																			   //
		err_quit((char*)"socket()");																   //
	}																								   //
																									   //
	// Setting Socket(Protocol, IPv4, PortNum) <- Server Information								   //
	SOCKADDR_IN serveraddr;																			   //
	ZeroMemory(&serveraddr, sizeof(serveraddr));													   //
																									   //
	serveraddr.sin_family = AF_INET;																   //
	serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);											   //
	serveraddr.sin_port = htons(SERVER_PORT);														   //
																									   //
	retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));								   //
	if (retval == SOCKET_ERROR)																		   //
	{																								   //
		delete g_pScene;																			   //
		err_quit((char*)"connect()");																   //
	}																								   //
																									   //
	// Communication With Server																	   //
	HANDLE hThread = CreateThread(NULL, 0, CommunicationWithServer, (LPVOID)&sock, 0, NULL);		   //
	if (hThread == NULL)																			   //
	{																								   //
		closesocket(sock);																			   //
		std::cout																					   //
			<< "[TCP Ŭ���̾�Ʈ] ���� ����: IP �ּ�=" << inet_ntoa(serveraddr.sin_addr)				   //
			<< ", ��Ʈ��ȣ=" << ntohs(serveraddr.sin_port)											   //
			<< std::endl;																			   //
	}																								   //
	else CloseHandle(hThread);																		   //
	/////////////////////////////////////////////////////////////////////////////////////////////////////

	delete g_pScene;
    return 0;
}

