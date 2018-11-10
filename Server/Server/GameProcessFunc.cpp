#include "GameProcessFunc.h"

////////////////////////////////////////  Game & Network Valiable  ////////////////////////////////////////
																										 //
unsigned int CurClientNum = 0;																			 //
																										 //
/*																										 //
CommunicationBuffer[index] 설명																		     //
	- index 0 ~ MAX_CLIENT*2-1까지는 Player 오브젝트에 대한 정보										 //
	- index MAX_CLIENT*2는 Boss 오브젝트에 대한 정보													 //
	- index MAX_CLIENT*2+1 ~ MAX_OBJECT-1까지는 BULLET 오브젝트에 대한 정보								 //
	- 게임 오브젝트와 통신용 오브젝트는 항상 1:1 로 매칭이 이루어진다.								     //
*/																										 //
CommunicationData CommunicationBuffer[MAX_OBJECT];														 //
																										 //
/*																										 //
GameObject 설명																						     //
	- 게임 오브젝트와 통신용 오브젝트는 항상 1:1 로 매칭이 이루어진다.								     //
*/																										 //
Player * PlayerBuffer[MAX_CLIENT];																		 //
Boss   * BossObj = NULL;																				 //
Bullet * BulletBuffer[MAX_OBJECT - MAX_CLIENT - 1]; // Bullet Count: Max_Object - Max_Client - Max_Boss	 //
																										 //
///////////////////////////////////////////////////////////////////////////////////////////////////////////


void GameProcessFunc::InitGameObject()
{
}

bool GameProcessFunc::CreateNewBoss()
{
	if (BossObj != NULL) return false;

	// Boss 초기 속성값 지정
	Point newPoint;
	POINT newPos_InTexture = { 0, 0 };
	BossObj = new Boss();
	BossObj->SetHP(BOSS_INIT_HP);
	BossObj->SetPosition(newPoint);
	BossObj->SetPos_InTexture(newPos_InTexture);

	return true;
}

int GameProcessFunc::CreateNewPlayer()
{
	// MAX_CLIENT보다 많은 수의 클라이언트를 수용하지 않음
	if (CurClientNum == MAX_CLIENT)
		return -1;

	// 새로운 플레이어 생성
	int ClientID = -1;
	// index 0 ~ MAX_CLIENT-1 중 PlayerBuffer[index] == NULL인 index를 ClientID로 사용
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PlayerBuffer[i] == NULL)
		{
			ClientID = i;
			break;
		}
	}
	if (ClientID != -1)
	{
		// Player 초기 속성값 지정
		u_int newHP = PLAYER_INIT_HP;
		Point newHeadPoint;
		Point newBodyPoint;
		POINT newPos_InHeadTexture = { 1,1 };
		POINT newPos_InBodyTexture = { 1,1 };

		PlayerBuffer[ClientID] = new Player();
		PlayerBuffer[ClientID]->SetHP(newHP);
		PlayerBuffer[ClientID]->SetHeadPosition(newHeadPoint);
		PlayerBuffer[ClientID]->SetBodyPosition(newBodyPoint);
		PlayerBuffer[ClientID]->SetPos_InHeadTexture(newPos_InHeadTexture);
		PlayerBuffer[ClientID]->SetPos_InBodyTexture(newPos_InBodyTexture);

		// 접속 클라이언트 수 증가
		++CurClientNum;
	}

	return ClientID;
}

bool GameProcessFunc::RecvInput(SOCKET sock, int ClientID)
{
	int retval;
	// 키 값을 받아옴
	retval = recvn(sock, (char*)PlayerBuffer[ClientID]->GetKeyBuffer(), sizeof(bool) * 256, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display((char*)"recv()");
		return false;
	}
	else if (retval == 0) // 받은 데이터가 없을 때
		return false;

	// 키 값을 받아옴
	retval = recvn(sock, (char*)PlayerBuffer[ClientID]->GetSpecialKeyBuffer(), sizeof(bool) * 246, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display((char*)"recv()");
		return false;
	}
	else if (retval == 0) // 받은 데이터가 없을 때
		return false;

	return true;
}

bool GameProcessFunc::SendCommunicationData(SOCKET sock)
{
	/*
		CommunicationBuffer를 MAX_OBJECT씩이나 보내는 이유:
		오브젝트의 현재 개수만큼만 보내야 할 경우 다음과 같은 이유로 추가처리비용이 들어간다.
		  - Obj_Type == NULL인 원소가 CommunicationBuffer 내에 여러군데 존재하면
			CommunicationBuffer 중간에 Obj_Type이 NULL인 원소가 없도록 정렬을 해줘야 한다.
			(CommunicationBuffer 시작 포인터부터 CommunicationData*CurrentObjectNum까지만 전송하는 방식이기에.)

		이때, (Obj_Type == NULL인 원소가 존재할 경우 정렬 비용) + (오브젝트의 현재 개수만큼 보낼 때 걸리는 비용) 인 경우에는
		오브젝트의 현재 개수가 적을 때 효율적이다.
		반대로 오브젝트의 개수가 많아지면, 오브젝트의 삭제 발생빈도가 적어야 효율적이다.
		그러나 오브젝트의 삭제 발생빈도는 정확히 측정할 수가 없기 때문에,
		통신속도가 1/60초 안에 CommunicatioBuffer를 MAX_OBJECT까지 보낼 수 있을 정도의 속도라면
		CommunicatioBuffer를 MAX_OBJECT까지 보내는 것이 더 바람직하다.
	*/
	int retval = send(sock, (char*)&CommunicationBuffer, sizeof(CommunicationData)*MAX_OBJECT, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display((char*)"send()");
		return false;
	}
	return true;
}

void GameProcessFunc::ProcessInput(float ElapsedTime)
{
	bool* KeyState = NULL;
	bool* SpecialKeyState = NULL;
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PlayerBuffer[i] != NULL)
		{
			KeyState = PlayerBuffer[i]->GetKeyBuffer();
			SpecialKeyState = PlayerBuffer[i]->GetSpecialKeyBuffer();

			Vector newForce = { 0.0f, 0.0f };
			if (KeyState['w'] == true)
			{
				newForce.j += PLAYER_SPEED;
				// Animation Frame Set(UpView-Sequance)
				// ...
			}
			if (KeyState['a'] == true)
			{
				newForce.i -= PLAYER_SPEED;
				// Animation Frame Set(LeftView-Sequance)
				//...
			}
			if (KeyState['s'] == true)
			{
				newForce.j -= PLAYER_SPEED;
				// Animation Frame Set(DownView-Sequance)
				//...
			}
			if (KeyState['d'] == true)
			{
				newForce.i += PLAYER_SPEED;
				// Animation Frame Set(RightView-Sequance)
				// ...
			}

			// 힘의 크기가 0이 아니라면 오브젝트에 힘을 가한다.
			if (!equal(newForce.magnitude(), 0.0f))
			{
				normalize(newForce);
				PlayerBuffer[i]->ApplyForce(newForce, ElapsedTime);
			}

			if (SpecialKeyState[0x0065] == true) // 0x0065 : GLUT_KEY_UP
			{
				// Animation Frame Set(UpView-Sequance)
				// ...

				BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_UP);
			}
			if (SpecialKeyState[0x0064] == true) // 0x0064 : GLUT_KEY_LEFT
			{
				// Animation Frame Set(LeftView-Sequance)
				//...

				BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_LEFT);
			}
			if (SpecialKeyState[0x0067] == true) // 0x0067 : GLUT_KEY_DOWN
			{
				// Animation Frame Set(DownView-Sequance)
				//...

				BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_DOWN);
			}
			if (SpecialKeyState[0x0066] == true) // 0x0066 : GLUT_KEY_RIGHT
			{
				// Animation Frame Set(RightView-Sequance)
				// ...

				BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_RIGHT);
			}
		}
	}
}

void GameProcessFunc::ProcessPhisics(float ElapsedTime)
{
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if(PlayerBuffer[i] != NULL)
			PlayerBuffer[i]->Update(ElapsedTime);
	}

	if (BossObj != NULL)
		BossObj->Update(ElapsedTime);

	for (int i = 0; i < MAX_OBJECT - (MAX_CLIENT * 2 + 1); ++i)
	{
		if (BulletBuffer[i] != NULL)
			BulletBuffer[i]->Update(ElapsedTime);
	}
}

void GameProcessFunc::ProcessCollision(float ElapsedTime)
{
	Point Pos;
	fSIZE Size;
	float WorldWidth = WND_WIDTH / RENDER_TRANSLATION_SCALE;
	float WorldHeight = WND_HEIGHT / RENDER_TRANSLATION_SCALE;
	for (int i = 0; i < MAX_OBJECT - (MAX_CLIENT * 2 + 1); ++i)
	{
		if (BulletBuffer[i] != NULL)
		{
			Pos = BulletBuffer[i]->GetPosition();
			Size.Width = BULLET_WIDTH;
			Size.Height = BULLET_HEIGHT;
			//if (Pos.x + Size.Width / 2 < -WorldWidth / 2 // left
			//	|| Pos.x - Size.Width / 2 > WorldWidth / 2 // right
			//	|| Pos.y + Size.Height / 2 < -WorldHeight / 2 // up
			//	|| Pos.y - Size.Height / 2 > WorldHeight / 2) // down
			//	DeleteObject(i);

			if (Pos.x - Size.Width / 2 < -WorldWidth / 2 // left
				|| Pos.x + Size.Width / 2 > WorldWidth / 2 // right
				|| Pos.y - Size.Height / 2 < -WorldHeight / 2 // down
				|| Pos.y + Size.Height / 2 > WorldHeight / 2) // up
			{
				Vector VerticalOfLine; // 직선에 수직인 벡터
				if (Pos.x - Size.Width / 2 < -WorldWidth / 2) // Collision left-side
				{
					Pos.x = Size.Width / 2 - WorldWidth / 2;
					VerticalOfLine.i = 1;
				}
				else if (Pos.x + Size.Width / 2 > WorldWidth / 2) // Collision right-side
				{
					Pos.x = WorldWidth / 2 - Size.Width / 2;
					VerticalOfLine.i = -1;
				}
				else if (Pos.y - Size.Height / 2 < -WorldHeight / 2) // Collision down-side
				{
					Pos.y = Size.Height / 2 - WorldHeight / 2;
					VerticalOfLine.j = 1;
				}
				else if (Pos.y + Size.Height / 2 > WorldHeight / 2) // Collision up-side
				{
					Pos.y = WorldHeight / 2 - Size.Height / 2;
					VerticalOfLine.j = -1;
				}
				Vector Velocity = BulletBuffer[i]->GetVelocity();
				Vector ReflectionVelocity = Velocity.reflectionAbout(VerticalOfLine);
				BulletBuffer[i]->SetPosition(Pos);
				BulletBuffer[i]->SetVelocity(ReflectionVelocity);
			}
		}
	}
}

void GameProcessFunc::BulletShoot(bool Possesion, Point Pos, Vector Velocity, unsigned int shootID)
{
	if (shootID == SHOOT_NONE || shootID == SHOOT_PATTERN_NONE)
		return;

	Point newPoint = Pos;
	Vector newVelocity = Velocity;
	float mass;
	float frictionFactor;
	u_int Damage;

	if (Possesion == true) // Player가 발사한 Bullet
	{
		mass = BULLET_MASS;
		frictionFactor = BULLET_FRICTION_FACTOR;
		Damage = PLAYER_BULLET_DAMAGE;

		switch (shootID)
		{
		case SHOOT_LEFT:
			newVelocity.i -= BULLET_SPEED;
			break;
		case SHOOT_RIGHT:
			newVelocity.i += BULLET_SPEED;
			break;
		case SHOOT_UP:
			newVelocity.j += BULLET_SPEED;
			break;
		case SHOOT_DOWN:
			newVelocity.j -= BULLET_SPEED;
			break;
		}
		for (int i = 0; i < MAX_OBJECT - (MAX_CLIENT * 2 + 1); ++i)
		{
			if (BulletBuffer[i] == NULL)
			{
				BulletBuffer[i] = new Bullet();
				BulletBuffer[i]->SetPossesion(true);
				BulletBuffer[i]->SetPosition(newPoint);
				BulletBuffer[i]->SetVelocity(newVelocity);
				BulletBuffer[i]->SetMass(mass);
				BulletBuffer[i]->SetFrictionFactor(frictionFactor);
				BulletBuffer[i]->SetDamage(Damage);
				break;
			}
		}
	}
	else // Boss가 발사한 Bullet
	{
		newVelocity *= rand() / (float)RAND_MAX;
		mass = BULLET_MASS * (rand() / (float)RAND_MAX);
		frictionFactor = BULLET_FRICTION_FACTOR * (rand() / (float)RAND_MAX);
		Damage = BOSS_BULLET_DAMAGE;

		switch (shootID)
		{
		case SHOOT_PATTERN_1:
			break;
		case SHOOT_PATTERN_2:
			break;
		case SHOOT_PATTERN_3:
			break;
		}
	}
}

void GameProcessFunc::ResetCommunicationBuffer()
{
	ZeroMemory(CommunicationBuffer, MAX_OBJECT);
	/*
	CommunicationBuffer[index] 설명
		- index 0 ~ MAX_CLIENT*2-1까지는 Player 오브젝트에 대한 정보
		- index MAX_CLIENT*2는 Boss 오브젝트에 대한 정보
		- index MAX_CLIENT*2+1 ~ MAX_OBJECT-1까지는 BULLET 오브젝트에 대한 정보
		- 게임 오브젝트와 통신용 오브젝트는 항상 1:1 로 매칭이 이루어진다.
	*/
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PlayerBuffer[i] != NULL)
		{
			CommunicationBuffer[i * 2].Obj_Type = KIND_PLAYER_HEAD;
			CommunicationBuffer[i * 2].Obj_Pos = PlayerBuffer[i]->GetHeadPosition();
			CommunicationBuffer[i * 2].Obj_Pos_InTexture = PlayerBuffer[i]->GetPos_InHeadTexture();
			CommunicationBuffer[i * 2 + 1].Obj_Type = KIND_PLAYER_BODY;
			CommunicationBuffer[i * 2 + 1].Obj_Pos = PlayerBuffer[i]->GetBodyPosition();
			CommunicationBuffer[i * 2 + 1].Obj_Pos_InTexture = PlayerBuffer[i]->GetPos_InBodyTexture();
		}
		else
		{
			CommunicationBuffer[i * 2].Obj_Type = KIND_NULL;
			CommunicationBuffer[i * 2 + 1].Obj_Type = KIND_NULL;
		}
	}
	if (BossObj != NULL)
	{
		CommunicationBuffer[MAX_CLIENT * 2].Obj_Type = KIND_BOSS;
		CommunicationBuffer[MAX_CLIENT * 2].Obj_Pos = BossObj->GetPosition();
		CommunicationBuffer[MAX_CLIENT * 2].Obj_Pos_InTexture = BossObj->GetPos_InTexture();
	}
	else
		CommunicationBuffer[MAX_CLIENT * 2].Obj_Type = KIND_NULL;

	for (int i = 0; i < MAX_OBJECT - (MAX_CLIENT * 2 + 1); ++i)
	{
		if (BulletBuffer[i] != NULL)
		{
			CommunicationBuffer[MAX_CLIENT * 2 + 1 + i].Obj_Type = BulletBuffer[i]->GetPossesion() ? KIND_BULLET_1 : KIND_BULLET_2;
			CommunicationBuffer[MAX_CLIENT * 2 + 1 + i].Obj_Pos = BulletBuffer[i]->GetPosition();
			CommunicationBuffer[MAX_CLIENT * 2 + 1 + i].Obj_Pos_InTexture = { 1,1 };
		}
		else
			CommunicationBuffer[MAX_CLIENT * 2 + 1 + i].Obj_Type = KIND_NULL;
	}
}
