#include "GameProcessFunc.h"

////////////////////////////////////////  Game & Network Valiable  ////////////////////////////////////////
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
Bullet * BulletBuffer[MAX_OBJECT - (MAX_CLIENT*2 + 1)]; // Bullet Count: Max_Object - Max_Client*2 - Max_Boss //
																										 //
///////////////////////////////////////////////////////////////////////////////////////////////////////////


void GameProcessFunc::InitGameObject()
{
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PlayerBuffer[i] != NULL)
		{
			delete PlayerBuffer[i];
			PlayerBuffer[i] = NULL;
		}
	}

	if (BossObj != NULL)
	{
		delete BossObj;
		BossObj = NULL;
	}

	for (int i = 0; i < MAX_OBJECT - (MAX_CLIENT * 2 + 1); ++i)
	{
		if (BulletBuffer[i] != NULL)
		{
			delete BulletBuffer[i];
			BulletBuffer[i] = NULL;
		}
	}
}

bool GameProcessFunc::CreateNewBoss()
{
	if (BossObj != NULL) return false;

	// Boss 초기 속성값 지정
	Point newPoint = { 3, 0, 0 };
	POINT newPos_InTexture = { 2, 0 };
	BossObj = new Boss();
	BossObj->SetHP(BOSS_INIT_HP);
	BossObj->SetPosition(newPoint);
	BossObj->SetPos_InTexture(newPos_InTexture);
	BossObj->SetPattern(1);
	BossObj->SetEtime(0.f);
	BossObj->SetPatternInit(true);

	return true;
}

int GameProcessFunc::CreateNewPlayer()
{
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
		Point newBodyPoint = { 0,0,0 };
		Point newHeadPoint = { newBodyPoint.x+0.041f, newBodyPoint.y+0.209f,newBodyPoint.z };
		POINT newSeqBody = { 0,0 };
		POINT newSeqHead = { 0,0 };

		PlayerBuffer[ClientID] = new Player();
		PlayerBuffer[ClientID]->SetHP(newHP);
		PlayerBuffer[ClientID]->SetBodyPosition(newBodyPoint);
		PlayerBuffer[ClientID]->SetHeadPosition(newHeadPoint);
		PlayerBuffer[ClientID]->SetSeqBody(newSeqBody);
		PlayerBuffer[ClientID]->SetSeqHead(newSeqHead);
	}

	return ClientID;
}

int GameProcessFunc::FindNullPlayerIndex(int indexArray[])
{
	int k = 0;
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PlayerBuffer[i] == NULL)
			indexArray[k++] = i;
	}
	return k;
}

bool GameProcessFunc::RecvInput(SOCKET sock, int ClientID)
{
	int retval;
	// 키 값을 받아옴
	retval = recvn(sock, (char*)PlayerBuffer[ClientID]->GetKeyBuffer(), sizeof(bool) * 256, 0);
	if (retval == SOCKET_ERROR || retval == 0)
	{
		delete PlayerBuffer[ClientID];
		PlayerBuffer[ClientID] = NULL;
		err_display((char*)"recv()");
		return false;
	}

	// 키 값을 받아옴
	retval = recvn(sock, (char*)PlayerBuffer[ClientID]->GetSpecialKeyBuffer(), sizeof(bool) * 246, 0);
	if (retval == SOCKET_ERROR || retval == 0)
	{
		delete PlayerBuffer[ClientID];
		PlayerBuffer[ClientID] = NULL;
		err_display((char*)"recv()");
		return false;
	}

	// ArrowKey에 대해서만 Stack으로 관리
	list<int>* ArrowkeyStack = PlayerBuffer[ClientID]->GetArrowKeyStack();
	bool* SpecialKey = PlayerBuffer[ClientID]->GetSpecialKeyBuffer();

	auto FindKey = [](list<int>* ArrowkeyStack, int SearchKey)
	{
		for (auto Arrowkey = ArrowkeyStack->begin(); Arrowkey != ArrowkeyStack->end(); ++Arrowkey)
		{
			if (*Arrowkey == SearchKey) return Arrowkey;
		}
		return ArrowkeyStack->end();
	};

	// 0x0064 ~ 0x0067: GLUT_KEY_LEFT ~ GLUT_KEY_DOWN
	for (int i = 0x0064; i <= 0x0067; ++i)
	{
		auto SearchKey = FindKey(ArrowkeyStack, i);
		if (SpecialKey[i] == true && SearchKey == ArrowkeyStack->end())
			ArrowkeyStack->emplace_back(i);
		else if(SpecialKey[i] != true && SearchKey != ArrowkeyStack->end())
			ArrowkeyStack->erase(SearchKey);
	}

	return true;
}

bool GameProcessFunc::SendCommunicationData(SOCKET sock, int ClientID)
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
		delete PlayerBuffer[ClientID];
		PlayerBuffer[ClientID] = NULL;
		err_display((char*)"send()");
		return false;
	}
	return true;
}

void GameProcessFunc::ProcessInput(float ElapsedTime)
{
	bool* KeyState = NULL;
	int ArrowKey = NULL;
	list<int>* ArrowKeyStack = NULL;
	POINT seq = { 0,0 };
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PlayerBuffer[i] != NULL)
		{
			KeyState = PlayerBuffer[i]->GetKeyBuffer();
			ArrowKeyStack = PlayerBuffer[i]->GetArrowKeyStack();
			if (ArrowKeyStack->size() > 0) ArrowKey = ArrowKeyStack->back();
			else ArrowKey = -1;

			Vector newForce = { 0.0f, 0.0f };
			if (KeyState['w'] == true)
			{
				// Animation Frame Set(UpView-Sequance)
				seq = PlayerBuffer[i]->GetSeqBody();
				seq.x++;
				if (seq.y != 0) {
					seq.y = 0;
					seq.x = 0;
				}
				if (seq.x > 9) seq.x = 0;
				newForce.j += PLAYER_SPEED;
				PlayerBuffer[i]->SetSeqBody(seq);
				// ...
			}
			if (KeyState['a'] == true)
			{
				// Animation Frame Set(LeftView-Sequance)
				seq = PlayerBuffer[i]->GetSeqBody();
				seq.x++;
				if (seq.y != 1) {
					seq.y = 1;
					seq.x = 0;
				}
				if (seq.x > 9) seq.x = 0;
				newForce.i -= PLAYER_SPEED;
				PlayerBuffer[i]->SetSeqBody(seq);
				//...
			}
			if (KeyState['s'] == true)
			{
				// Animation Frame Set(DownView-Sequance)
				seq = PlayerBuffer[i]->GetSeqBody();
				seq.x--;
				if (seq.y != 0) {
					seq.y = 0;
					seq.x = 9;
				}
				if (seq.x < 0) seq.x = 9;
				newForce.j -= PLAYER_SPEED;
				PlayerBuffer[i]->SetSeqBody(seq);
				//...
			}
			if (KeyState['d'] == true)
			{
				// Animation Frame Set(RightView-Sequance)
				seq = PlayerBuffer[i]->GetSeqBody();
				seq.x++;
				if (seq.y != 2) {
					seq.y = 2;
					seq.x = 0;
				}
				if (seq.x > 9) seq.x = 0;
				newForce.i += PLAYER_SPEED;
				PlayerBuffer[i]->SetSeqBody(seq);
				// ...
			}

			// 힘의 크기가 0이 아니라면 오브젝트에 힘을 가한다.
			if (!equal(newForce.magnitude(), 0.0f))
			{
				normalize(newForce);
				PlayerBuffer[i]->ApplyForce(newForce, ElapsedTime);
			}


			if (PlayerBuffer[i]->CanShot()) {
				if (ArrowKey == 0x0065) // 0x0065 : GLUT_KEY_UP
				{
					// Animation Frame Set(UpView-Sequance)
					seq = PlayerBuffer[i]->GetSeqHead();
					if (seq.x == 5)	seq.x = 4;
					else seq.x = 5;
					PlayerBuffer[i]->SetSeqHead(seq);
					// ...

					BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_UP);
					PlayerBuffer[i]->ResetShotCooltime();
				}
				if (ArrowKey == 0x0064) // 0x0064 : GLUT_KEY_LEFT
				{
					// Animation Frame Set(LeftView-Sequance)
					seq = PlayerBuffer[i]->GetSeqHead();
					if (seq.x == 7)	seq.x = 6;
					else seq.x = 7;
					PlayerBuffer[i]->SetSeqHead(seq);
					//...

					BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_LEFT);
					PlayerBuffer[i]->ResetShotCooltime();
				}
				if (ArrowKey == 0x0067) // 0x0067 : GLUT_KEY_DOWN
				{
					// Animation Frame Set(DownView-Sequance)
					seq = PlayerBuffer[i]->GetSeqHead();
					if (seq.x == 1)	seq.x = 0;
					else seq.x = 1;
					PlayerBuffer[i]->SetSeqHead(seq);
					//...

					BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_DOWN);
					PlayerBuffer[i]->ResetShotCooltime();
				}
				if (ArrowKey == 0x0066) // 0x0066 : GLUT_KEY_RIGHT
				{
					// Animation Frame Set(RightView-Sequance)
					seq = PlayerBuffer[i]->GetSeqHead();
					if (seq.x == 3)	seq.x = 2;
					else seq.x = 3;
					PlayerBuffer[i]->SetSeqHead(seq);
					// ...

					BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_RIGHT);
					PlayerBuffer[i]->ResetShotCooltime();
				}
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
			BossPattern(ElapsedTime);

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
	float WorldLeftMargine = BACKGROUND_LEFT_MARGINE / RENDER_TRANSLATION_SCALE;
	float WorldRightMargine = BACKGROUND_RIGHT_MARGINE / RENDER_TRANSLATION_SCALE;
	float WorldUpMargine = BACKGROUND_UP_MARGINE / RENDER_TRANSLATION_SCALE;
	float WorldDownMargine = BACKGROUND_DOWN_MARGINE / RENDER_TRANSLATION_SCALE;

	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PlayerBuffer[i] != NULL)
		{
			Pos = PlayerBuffer[i]->GetBodyPosition();
			Size.Width = PLAYER_WIDTH / 2;
			Size.Height = PLAYER_HEIGHT / 2;

			// ProcessCollision Player-Wall
			if (Pos.x - Size.Width / 2 < -WorldWidth / 2 + WorldLeftMargine// left
				|| Pos.x + Size.Width / 2 > WorldWidth / 2 - WorldRightMargine// right
				|| Pos.y - Size.Height / 2 < -WorldHeight / 2 + WorldDownMargine// down
				|| Pos.y + Size.Height / 2 > WorldHeight / 2 - WorldUpMargine)// up
			{
				Vector VerticalOfLine; // 직선에 수직인 벡터
				if (Pos.x - Size.Width / 2 < -WorldWidth / 2 + WorldLeftMargine) // Collision left-side
				{
					Pos.x = Size.Width / 2 - WorldWidth / 2 + WorldLeftMargine;
					VerticalOfLine.i = 1;
				}
				if (Pos.x + Size.Width / 2 > WorldWidth / 2 - WorldRightMargine) // Collision right-side
				{
					Pos.x = WorldWidth / 2 - Size.Width / 2 - WorldRightMargine;
					VerticalOfLine.i = -1;
				}
				if (Pos.y - Size.Height / 2 < -WorldHeight / 2 + WorldDownMargine) // Collision down-side
				{
					Pos.y = Size.Height / 2 - WorldHeight / 2 + WorldDownMargine;
					VerticalOfLine.j = 1;
				}
				if (Pos.y + Size.Height / 2 > WorldHeight / 2 - WorldUpMargine) // Collision up-side
				{
					Pos.y = WorldHeight / 2 - Size.Height / 2 - WorldUpMargine;
					VerticalOfLine.j = -1;
				}
				Vector Velocity = PlayerBuffer[i]->GetVelocity();
				Velocity.slidingAbout(VerticalOfLine);
				PlayerBuffer[i]->SetBodyPosition(Pos);
				PlayerBuffer[i]->SetVelocity(Velocity);
			}

		}
	}

	for (int i = 0; i < MAX_OBJECT - (MAX_CLIENT * 2 + 1); ++i)
	{
		if (BulletBuffer[i] != NULL)
		{
			Pos = BulletBuffer[i]->GetPosition();
			Size.Width = BULLET_WIDTH;
			Size.Height = BULLET_HEIGHT;

			// ProcessCollision Wall-Bullet
			if (Pos.x - Size.Width / 2 < -WorldWidth / 2 + WorldLeftMargine// left
				|| Pos.x + Size.Width / 2 > WorldWidth / 2 - WorldRightMargine// right
				|| Pos.y - Size.Height / 2 < -WorldHeight / 2 + WorldDownMargine// down
				|| Pos.y + Size.Height / 2 > WorldHeight / 2 - WorldUpMargine)// up
			{
				if (BulletBuffer[i]->GetPossesion() || BulletBuffer[i]->GetReflectCnt() == MAX_BULLET_REFLECT_COUNT)
				{
					delete BulletBuffer[i];
					BulletBuffer[i] = NULL;
				}
				else // 보스가 발사하는 눈물에 대해서는 MAX_BULLET_REFLECT_COUNT만큼 튕겨나갈 수 있음
				{
					Vector VerticalOfLine; // 직선에 수직인 벡터
					if (Pos.x - Size.Width / 2 < -WorldWidth / 2 + WorldLeftMargine) // Collision left-side
					{
						Pos.x = Size.Width / 2 - WorldWidth / 2 + WorldLeftMargine;
						VerticalOfLine.i = 1;
					}
					if (Pos.x + Size.Width / 2 > WorldWidth / 2 - WorldRightMargine) // Collision right-side
					{
						Pos.x = WorldWidth / 2 - Size.Width / 2 - WorldRightMargine;
						VerticalOfLine.i = -1;
					}
					if (Pos.y - Size.Height / 2 < -WorldHeight / 2 + WorldDownMargine) // Collision down-side
					{
						Pos.y = Size.Height / 2 - WorldHeight / 2 + WorldDownMargine;
						VerticalOfLine.j = 1;
					}
					if (Pos.y + Size.Height / 2 > WorldHeight / 2 - WorldUpMargine) // Collision up-side
					{
						Pos.y = WorldHeight / 2 - Size.Height / 2 - WorldUpMargine;
						VerticalOfLine.j = -1;
					}
					Vector Velocity = BulletBuffer[i]->GetVelocity();
					Vector ReflectionVelocity = Velocity.reflectionAbout(VerticalOfLine);
					u_short ReflectCnt = BulletBuffer[i]->GetReflectCnt();
					ReflectCnt++;
					BulletBuffer[i]->SetPosition(Pos);
					BulletBuffer[i]->SetVelocity(ReflectionVelocity);
					BulletBuffer[i]->SetReflectCnt(ReflectCnt);
				}
			}
		}
	}
}

void GameProcessFunc::BulletShoot(bool Possesion, Point Pos, Vector Velocity, unsigned int shootID)
{
	if (shootID == SHOOT_NONE)
		return;

	Point newPoint = Pos;
	Vector newVelocity = Velocity;
	float mass;
	float frictionFactor;
	u_int Damage;

	int bossShootNum;
	float currentShoot;

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
		mass = BULLET_MASS;
		frictionFactor = BULLET_FRICTION_FACTOR;
		Damage = BOSS_BULLET_DAMAGE;

		switch (shootID)
		{
		case SHOOT_PATTERN_1:
			bossShootNum = rand() % 4 + 7;
			currentShoot = 0;
			while (currentShoot < bossShootNum) {
				for (int i = 0; i < MAX_OBJECT - (MAX_CLIENT * 2 + 1); ++i)
				{
					if (BulletBuffer[i] == NULL)
					{
						Point target = BossGetPoint();

						newVelocity.i = cos(float(rand() % 45) / 180 * PI);

						if (target.x - BossObj->GetPosition().x < 0)
							newVelocity.i = -newVelocity.i;

						if (!(rand() % 2))
							newVelocity.j = sin(float(rand() % 45) / 180 * PI);
						else
							newVelocity.j = -sin(float(rand() % 45) / 180 * PI);

						BulletBuffer[i] = new Bullet();
						BulletBuffer[i]->SetPossesion(false);
						BulletBuffer[i]->SetPosition(newPoint);
						BulletBuffer[i]->SetVelocity(newVelocity * BOSS_BULLET_SPEED);
						BulletBuffer[i]->SetMass(mass);
						BulletBuffer[i]->SetFrictionFactor(frictionFactor);
						BulletBuffer[i]->SetDamage(Damage);
						break;
					}
				}
				currentShoot++;
			}
			break;

		case SHOOT_PATTERN_2:
			bossShootNum = 8;
			currentShoot = 0;
			while (currentShoot < bossShootNum) {
				for (int i = 0; i < MAX_OBJECT - (MAX_CLIENT * 2 + 1); ++i)
				{
					if (BulletBuffer[i] == NULL)
					{
						newVelocity.i = cos((currentShoot * 45 / 180) * PI);
						newVelocity.j = sin((currentShoot * 45 / 180) * PI);

						//std::cout << newVelocity.i << "," << newVelocity.j << std::endl;

						BulletBuffer[i] = new Bullet();
						BulletBuffer[i]->SetPossesion(false);
						BulletBuffer[i]->SetPosition(newPoint);
						BulletBuffer[i]->SetVelocity(newVelocity * BOSS_BULLET_SPEED);
						BulletBuffer[i]->SetMass(mass);
						BulletBuffer[i]->SetFrictionFactor(frictionFactor);
						BulletBuffer[i]->SetDamage(Damage);
						break;
					}
				}
				currentShoot++;
			}
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
			CommunicationBuffer[i * 2].Obj_Type = KIND_PLAYER_BODY;
			CommunicationBuffer[i * 2].Obj_Pos = PlayerBuffer[i]->GetBodyPosition();
			CommunicationBuffer[i * 2].Obj_Pos_InTexture = PlayerBuffer[i]->GetSeqBody();
			CommunicationBuffer[i * 2].Obj_Velocity = PlayerBuffer[i]->GetVelocity();
			CommunicationBuffer[i * 2 + 1].Obj_Type = KIND_PLAYER_HEAD;
			CommunicationBuffer[i * 2 + 1].Obj_Pos = PlayerBuffer[i]->GetHeadPosition();
			CommunicationBuffer[i * 2 + 1].Obj_Pos_InTexture = PlayerBuffer[i]->GetSeqHead();
			CommunicationBuffer[i * 2 + 1].Obj_Velocity = PlayerBuffer[i]->GetVelocity();
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
		CommunicationBuffer[MAX_CLIENT * 2].Obj_Velocity = BossObj->GetVelocity();
	}
	else
		CommunicationBuffer[MAX_CLIENT * 2].Obj_Type = KIND_NULL;

	for (int i = 0; i < MAX_OBJECT - (MAX_CLIENT * 2 + 1); ++i)
	{
		if (BulletBuffer[i] != NULL)
		{
			CommunicationBuffer[MAX_CLIENT * 2 + 1 + i].Obj_Type = BulletBuffer[i]->GetPossesion() ? KIND_BULLET_1 : KIND_BULLET_2;
			CommunicationBuffer[MAX_CLIENT * 2 + 1 + i].Obj_Pos = BulletBuffer[i]->GetPosition();
			CommunicationBuffer[MAX_CLIENT * 2 + 1 + i].Obj_Velocity = BulletBuffer[i]->GetVelocity();
			CommunicationBuffer[MAX_CLIENT * 2 + 1 + i].Obj_Pos_InTexture = { 1,1 };
		}
		else
			CommunicationBuffer[MAX_CLIENT * 2 + 1 + i].Obj_Type = KIND_NULL;
	}
}

void GameProcessFunc::BossPattern(float ElapsedTime)
{
	if (BossObj == NULL) return;
	switch (BossObj->GetPattern())
	{
	case 1:
		BossJump(ElapsedTime);
		break;
	case 2:
		BossHighJump(ElapsedTime);
		break;
	case 3:
		BossShoot(ElapsedTime);
		break;
	}
}

void GameProcessFunc::BossJump(float ElapsedTime)
{
	if (BossObj == NULL) return;
	Point ps;
	ps = BossObj->GetPosition();

	float total_eTime;
	total_eTime = BossObj->GetEtime();

	float jumpforce = 10.f;
	float moveforce = 1.5f;

	total_eTime += ElapsedTime;

	Vector Direction;

	if (BossObj->GetPatternInit()) {
		Direction = BossGetDirectirion();
		BossObj->SetPatternInit(false);
		BossObj->SetVelocity(Direction);
	}

	if (total_eTime >= 0.0f && total_eTime < 1.0f) {
		BossObj->SetPos_InTexture(POINT{ 7,0 });
		ps.z = -jumpforce * ((total_eTime * total_eTime) - total_eTime + 0.25f) + 2;
		if (ps.z >= 0) {
			ps.x = ps.x + (BossObj->GetVelocity().i * ElapsedTime * moveforce);
			ps.y = ps.y + (BossObj->GetVelocity().j * ElapsedTime * moveforce);
		}
		if (ps.z < 0) {
			ps.z = 0;
		}

		//std::cout << "Etime : " << total_eTime << " / 보스 위치 : (" << ps.x << "," << ps.y <<"," << ps.z << ")" << std::endl;
	}
	else if (total_eTime >= 1.0f && total_eTime < 1.5f) {
		BossObj->SetPos_InTexture(POINT{ 8,0 });
		ps.z = 0;
	}
	else if (total_eTime >= 1.5f && total_eTime < 2.0f) {
		BossObj->SetPos_InTexture(POINT{ 2,0 });
		ps.z = 0;
	}
	else {
		total_eTime = 0;
		BossObj->SetPatternInit(true);
		BossObj->SetPattern(rand() % 3 + 1);
	}

	BossObj->SetEtime(total_eTime);
	BossObj->SetPosition(ps);
}

void GameProcessFunc::BossHighJump(float ElapsedTime)
{
	if (BossObj == NULL) return;
	Point ps;
	ps = BossObj->GetPosition();

	float total_eTime;
	total_eTime = BossObj->GetEtime();

	float jumpforce = 25.f;

	total_eTime += ElapsedTime;

	if (total_eTime >= 0.f && total_eTime < 0.75f) {
		BossObj->SetPos_InTexture(POINT{ 4,0 });
		ps.z = -jumpforce * (total_eTime * (total_eTime - 1.5f));

		if (ps.z < 0)
			ps.z = 0;

		BossObj->SetPatternInit(true);
	}
	else if (total_eTime >= 0.75f && total_eTime < 1.5f) {
		BossObj->SetPos_InTexture(POINT{ 6,0 });
		ps.z = -jumpforce * (total_eTime * (total_eTime - 1.5f));
		
		if (BossObj->GetPatternInit()) {
			Point newPoint = BossGetPoint();
			ps.x = newPoint.x;
			ps.y = newPoint.y;
			BossObj->SetPatternInit(false);
		}

		if (ps.z < 0)
			ps.z = 0;
	}
	else if (total_eTime >= 1.5f && total_eTime < 1.7f) {
		BossObj->SetPos_InTexture(POINT{ 5,0 });
		ps.z = 0;

		BossObj->SetPatternInit(true);
	}
	else if (total_eTime >= 1.7f && total_eTime < 2.2f) {
		BossObj->SetPos_InTexture(POINT{ 8,0 });
		ps.z = 0;

		if (BossObj->GetPatternInit()) {
			BulletShoot(false, BossObj->GetPosition(), Vector(0, 0, 0), SHOOT_PATTERN_2);
			BossObj->SetPatternInit(false);
		}
	}
	else if (total_eTime >= 2.2f && total_eTime < 2.7f) {
		BossObj->SetPos_InTexture(POINT{ 2,0 });
		ps.z = 0;
	}
	else {
		total_eTime = 0;
		BossObj->SetPattern(rand() % 3 + 1);
	}

	BossObj->SetEtime(total_eTime);
	BossObj->SetPosition(ps);
}

void GameProcessFunc::BossShoot(float ElapsedTime)
{
	if (BossObj == NULL) return;
	Point ps;
	ps = BossObj->GetPosition();

	float total_eTime;
	total_eTime = BossObj->GetEtime();

	total_eTime += ElapsedTime;

	if (total_eTime >= 0.f && total_eTime < 0.5f) {
		BossObj->SetPos_InTexture(POINT{ 1,0 });
		BossObj->SetPatternInit(true);
	}
	else if (total_eTime >= 0.5f && total_eTime < 1.5f) {
		BossObj->SetPos_InTexture(POINT{ 3,0 });

		if (total_eTime >= 0.6f) {
			if (BossObj->GetPatternInit()) {
				BulletShoot(false, BossObj->GetPosition(), Vector(0, 0, 0), SHOOT_PATTERN_1);
				BossObj->SetPatternInit(false);
			}
		}

		// std::cout << "Attack!!!" << "(" << vX << "," << vY << ")" << std::endl;
	}
	else if (total_eTime >= 1.5f && total_eTime < 2.0f) {
		BossObj->SetPos_InTexture(POINT{ 2,0 });
		// std::cout << "Attack!!!" << "(" << vX << "," << vY << ")" << std::endl;
	}
	else {
		total_eTime = 0;
		BossObj->SetPattern(rand() % 3 + 1);
	}
	BossObj->SetEtime(total_eTime);
}

Vector GameProcessFunc::BossGetDirectirion()
{
	Vector min_direction = {100, 100, 100};

	Point BossPosition = BossObj->GetPosition();
	BossPosition.z = 0;

	for (int i = 0; i < MAX_CLIENT; i++)
	{
		if (PlayerBuffer[i] != NULL) {
			Point ClientPosition = PlayerBuffer[i]->GetHeadPosition();

			if (BossPosition.distanceTo(ClientPosition) < min_direction.magnitude()) {
				//std::cout << i << "번 클라 : " << BossPosition.distanceTo(ClientPosition) << std::endl;
				min_direction = ClientPosition - BossPosition;
			}
		}
	}
	//std::cout << "단위 벡터 : " << "(" << unit(min_direction).i << "," << unit(min_direction).j << "," << unit(min_direction).k << ")" <<std::endl;
	return unit(min_direction);
}

Point GameProcessFunc::BossGetPoint()
{
	Point newPoint;
	int randomPlayer;

	while (1) {
		randomPlayer = rand() % MAX_CLIENT;
		if (PlayerBuffer[randomPlayer] != NULL) {
			newPoint = PlayerBuffer[randomPlayer]->GetHeadPosition();
			break;
		}
	}

	//std::cout << "새 위치 : " << "(" << newPoint.x << "," << newPoint.y << "," << newPoint.z << ")" << std::endl;

	return newPoint;
}