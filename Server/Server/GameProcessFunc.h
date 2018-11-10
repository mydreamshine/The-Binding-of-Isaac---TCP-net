#pragma once
#include "CGameObject.h"


/////////////////////////////////  Game Process Function  /////////////////////////////////
namespace GameProcessFunc
{
	void InitGameObject();

	bool CreateNewBoss();
	int CreateNewPlayer();
	void BulletShoot(bool Possesion, Point Pos, Vector Velocity, unsigned int shootID);

	void ProcessInput(float ElapsedTime);
	void ProcessPhisics(float ElapsedTime);
	void ProcessCollision(float ElapsedTime);

	bool RecvInput(SOCKET sock, int ClientID);
	bool SendCommunicationData(SOCKET sock, int ClientID);
	void ResetCommunicationBuffer();
}
///////////////////////////////////////////////////////////////////////////////////////////