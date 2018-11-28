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
		Point newHeadPoint = { newBodyPoint.x + PLAYER_HEAD_OFFSET_X, newBodyPoint.y + PLAYER_HEAD_OFFSET_Y, newBodyPoint.z };
		POINT newSeqBody = { 0,0 };
		POINT newSeqHead = { 0,0 };

		PlayerBuffer[ClientID] = new Player();
		PlayerBuffer[ClientID]->SetHP(newHP);
		PlayerBuffer[ClientID]->SetBodyPosition(newBodyPoint);
		PlayerBuffer[ClientID]->SetHeadPosition(newHeadPoint);
		PlayerBuffer[ClientID]->SetSeqBody(newSeqBody);
		PlayerBuffer[ClientID]->SetSeqHead(newSeqHead);
		PlayerBuffer[ClientID]->SetHitDealay(PLAYER_HIT_DEALAY);
		PlayerBuffer[ClientID]->InitHitDealay();
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

			if (ArrowKey == 0x0065) // 0x0065 : GLUT_KEY_UP
			{
				// Animation Frame Set(UpView-Sequance)
				seq = PlayerBuffer[i]->GetSeqHead();
				if (seq.x == 5)	seq.x = 4;
				else seq.x = 5;
				PlayerBuffer[i]->SetSeqHead(seq);
				// ...

				BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_UP);
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
	static Point PlayerPos, BossPos, BulletPos;
	static fRECT PlayerBoundRect, BossBoundRect, BulletBoundRect, LandBoundRect, intersectRect;
	static const float WorldWidth = WND_WIDTH / RENDER_TRANSLATION_SCALE;
	static const float WorldHeight = WND_HEIGHT / RENDER_TRANSLATION_SCALE;
	static const float WorldLeftMargine = BACKGROUND_LEFT_MARGINE / RENDER_TRANSLATION_SCALE;
	static const float WorldRightMargine = BACKGROUND_RIGHT_MARGINE / RENDER_TRANSLATION_SCALE;
	static const float WorldUpMargine = BACKGROUND_UP_MARGINE / RENDER_TRANSLATION_SCALE;
	static const float WorldDownMargine = BACKGROUND_DOWN_MARGINE / RENDER_TRANSLATION_SCALE;

	// Set LandBoundary
	LandBoundRect.Left = -WorldWidth / 2 + WorldLeftMargine;
	LandBoundRect.Top = WorldHeight / 2 - WorldUpMargine;
	LandBoundRect.Right = WorldWidth / 2 - WorldRightMargine;
	LandBoundRect.Bottom = -WorldHeight / 2 + WorldDownMargine;

	if (BossObj != NULL)
	{
		// Set BossBoundingRect
		BossPos = BossObj->GetPosition();
		float UnderOrdered_Offset_Y = -((BOSS_HEIGHT / 2) - BOSS_BOUNDINGBOX_HEIGHT) * 0.5f;
		// UnderOrdered_Offset_Y: 2차원 평면에서의 깊이감을 나타내기 위해 BoundingRect를 실제 크기보다 작게 하고 이를
		// 실제 Bottom과 BoudingRect의 Bottom을 같게 아래쪽 맞춤으로 정렬함
		BossBoundRect.Left = BossPos.x - BOSS_BOUNDINGBOX_WIDTH / 2;
		BossBoundRect.Top = BossPos.y + BOSS_BOUNDINGBOX_HEIGHT / 2 + UnderOrdered_Offset_Y;
		BossBoundRect.Right = BossPos.x + BOSS_BOUNDINGBOX_WIDTH / 2;
		BossBoundRect.Bottom = BossPos.y - BOSS_BOUNDINGBOX_HEIGHT / 2 + UnderOrdered_Offset_Y;

		// ProcessCollision Wall-Boss
		if (CollisionFunc::CollideWndBoundary(BossBoundRect, LandBoundRect))
		{
			if (BossBoundRect.Left < LandBoundRect.Left) // Collision left-side
				BossPos.x += LandBoundRect.Left - BossBoundRect.Left;
			if (BossBoundRect.Right > LandBoundRect.Right) // Collision right-side
				BossPos.x += LandBoundRect.Right - BossBoundRect.Right;
			if (BossBoundRect.Bottom < LandBoundRect.Bottom) // Collision down-side
				BossPos.y += LandBoundRect.Bottom - BossBoundRect.Bottom;
			if (BossBoundRect.Top > LandBoundRect.Top) // Collision up-side
				BossPos.y += LandBoundRect.Top - BossBoundRect.Top;

			BossObj->SetPosition(BossPos);
		}
	}

	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PlayerBuffer[i] != NULL)
		{
			// Set PlayerBoundingRect
			PlayerPos = PlayerBuffer[i]->GetBodyPosition();
			PlayerBoundRect.Left = PlayerPos.x - PLAYER_BOUNDINGBOX_WIDTH / 2;
			PlayerBoundRect.Top = PlayerPos.y + PLAYER_BOUNDINGBOX_HEIGHT / 2;
			PlayerBoundRect.Right = PlayerPos.x + PLAYER_BOUNDINGBOX_WIDTH / 2;
			PlayerBoundRect.Bottom = PlayerPos.y - PLAYER_BOUNDINGBOX_HEIGHT / 2;

			// ProcessCollision Player-Wall
			if (CollisionFunc::CollideWndBoundary(PlayerBoundRect, LandBoundRect))
			{
				Vector Wall_NormalVector; // 벽의 법선벡터
				if (PlayerBoundRect.Left < LandBoundRect.Left) // Collision left-side
				{
					PlayerPos.x += LandBoundRect.Left - PlayerBoundRect.Left;
					Wall_NormalVector.i = 1;
				}
				if (PlayerBoundRect.Right > LandBoundRect.Right) // Collision right-side
				{
					PlayerPos.x += LandBoundRect.Right - PlayerBoundRect.Right;
					Wall_NormalVector.i = -1;
				}
				if (PlayerBoundRect.Bottom < LandBoundRect.Bottom) // Collision down-side
				{
					PlayerPos.y += LandBoundRect.Bottom - PlayerBoundRect.Bottom;
					Wall_NormalVector.j = 1;
				}
				if (PlayerBoundRect.Top > LandBoundRect.Top) // Collision up-side
				{
					PlayerPos.y += LandBoundRect.Top - PlayerBoundRect.Top;
					Wall_NormalVector.j = -1;
				}
				Vector Velocity = PlayerBuffer[i]->GetVelocity();
				normalize(Wall_NormalVector); // 충돌 면이 2개 이상일 경우 정규기저벡터를 만들어줘야 한다.
				Velocity.slidingAbout(Wall_NormalVector);
				PlayerBuffer[i]->SetBodyPosition(PlayerPos);
				PlayerBuffer[i]->SetVelocity(Velocity);
			}

			// ProcessCollision Player-Boss
			if (BossObj != NULL)
			{
				if (equal(BossPos.z, 0.0f))
				{
					if (CollisionFunc::IntersectRect(&intersectRect, PlayerBoundRect, BossBoundRect))
					{
						// 교차 영역 크기
						float nInterWidth = abs(intersectRect.Right - intersectRect.Left);
						float nInterHeight = abs(intersectRect.Top - intersectRect.Bottom);

						// Moved_BoundingRect: PlayerBoundRect
						// Hold_BoundingRect: BossBoundRect

						Vector PlayerVelocity = PlayerBuffer[i]->GetVelocity();

						// 교차 너비 > 교차 높이: 위/아래에서부터 충돌이 이루어짐
						if (nInterWidth > nInterHeight)
						{
							// Hold_BoundingRect의 윗부분 충돌
							if (intersectRect.Top == BossBoundRect.Top)
								PlayerPos.y += nInterHeight;
							// Hold_BoundingRect의 아랫부분 충돌
							else if (intersectRect.Bottom == BossBoundRect.Bottom)
								PlayerPos.y -= nInterHeight;

							// y축 방향 속도값 0으로 초기화
							PlayerVelocity.j = 0;
						}
						// 교차 너비 < 교차 높이: 좌/우로부터 충돌이 이루어짐
						else
						{
							// Hold_BoundingRect의 왼쪽부분 충돌
							if (intersectRect.Left == BossBoundRect.Left)
								PlayerPos.x -= nInterWidth;
							// Hold_BoundingRect의 오른쪽부분 충돌
							else if (intersectRect.Right == BossBoundRect.Right)
								PlayerPos.x += nInterWidth;

							// x축 방향 속도값 0으로 초기화
							PlayerVelocity.i = 0;
						}

						PlayerBuffer[i]->SetBodyPosition(PlayerPos);
						PlayerBuffer[i]->SetVelocity(PlayerVelocity);

						// 피격당하는 딜레이 확인 후 체력 감소
						if (PlayerBuffer[i]->CheckHitDealayComplete())
						{
							u_int PlayerHP = PlayerBuffer[i]->GetHP();
							u_int newHP = ((int)PlayerHP - BOSS_BULLET_DAMAGE > 0) ? PlayerHP - BOSS_BULLET_DAMAGE : 0;
							PlayerBuffer[i]->SetHP(newHP);
							PlayerBuffer[i]->InitHitDealay();
						}
					}
				}
			}
		}
	}

	bool DeleteBullet = false;
	for (int i = 0; i < MAX_OBJECT - (MAX_CLIENT * 2 + 1); ++i)
	{
		DeleteBullet = false;
		if (BulletBuffer[i] != NULL)
		{
			BulletPos = BulletBuffer[i]->GetPosition();
			BulletBoundRect.Left   = BulletPos.x - BULLET_BOUNDINGBOX_WIDTH / 2;
			BulletBoundRect.Top    = BulletPos.y + PLAYER_BOUNDINGBOX_HEIGHT / 2;
			BulletBoundRect.Right  = BulletPos.x + PLAYER_BOUNDINGBOX_WIDTH / 2;
			BulletBoundRect.Bottom = BulletPos.y - PLAYER_BOUNDINGBOX_HEIGHT / 2;

			// ProcessCollision Bullet-Wall
			if (CollisionFunc::CollideWndBoundary(BulletBoundRect, LandBoundRect))
				DeleteBullet = true;

			// ProcessCollision Bullet-Boss
			if (BossObj != NULL && BulletBuffer[i]->GetPossesion() == true)
			{
				if (equal(BossPos.z, 0.0f))
				{
					if (CollisionFunc::IntersectRect(&intersectRect, BulletBoundRect, BossBoundRect))
					{
						// 넉백 및 HP 감소
						BossPos += BulletBuffer[i]->GetVelocity() / RENDER_TRANSLATION_SCALE;
						u_int BossHP = BossObj->GetHP();
						u_int newHP = ((int)BossHP - PLAYER_BULLET_DAMAGE > 0) ? BossHP - PLAYER_BULLET_DAMAGE : 0;
						BossObj->SetPosition(BossPos);
						BossObj->SetHP(newHP);
						std::cout << "Boss HP: " << newHP << std::endl;
						DeleteBullet = true;
					}
				}
			}

			// ProcessCollision Bullet-Player
			if (BulletBuffer[i]->GetPossesion() == false)
			{
				for (int j = 0; j < MAX_CLIENT; ++j)
				{
					if (PlayerBuffer[j] != NULL)
					{
						// Set PlayerBoundingRect
						PlayerPos = PlayerBuffer[j]->GetBodyPosition();
						PlayerBoundRect.Left = PlayerPos.x - PLAYER_BOUNDINGBOX_WIDTH / 2;
						PlayerBoundRect.Top = PlayerPos.y + PLAYER_BOUNDINGBOX_HEIGHT / 2;
						PlayerBoundRect.Right = PlayerPos.x + PLAYER_BOUNDINGBOX_WIDTH / 2;
						PlayerBoundRect.Bottom = PlayerPos.y - PLAYER_BOUNDINGBOX_HEIGHT / 2;

						if (CollisionFunc::IntersectRect(&intersectRect, BulletBoundRect, PlayerBoundRect))
						{
							// 넉백
							PlayerPos += BulletBuffer[i]->GetVelocity() / RENDER_TRANSLATION_SCALE;
							PlayerBuffer[j]->SetBodyPosition(PlayerPos);
							// 피격당하는 딜레이 확인 후 체력 감소
							if (PlayerBuffer[j]->CheckHitDealayComplete())
							{
								u_int PlayerHP = PlayerBuffer[j]->GetHP();
								u_int newHP = ((int)PlayerHP - BOSS_BULLET_DAMAGE > 0) ? PlayerHP - BOSS_BULLET_DAMAGE : 0;
								PlayerBuffer[j]->SetHP(newHP);
								PlayerBuffer[j]->InitHitDealay();
								std::cout << "Player[" << j << "] HP: " << newHP << std::endl;
							}
							DeleteBullet = true;
						}
					}
				}
			}

			if (DeleteBullet == true)
			{
				delete BulletBuffer[i];
				BulletBuffer[i] = NULL;
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

Vector Direction;

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

	if (BossObj->GetPatternInit()) {
		Direction = BossGetDirectirion() * moveforce;
		BossObj->SetPatternInit(false);
		//std::cout << "INIT Velocity : " << "(" << Direction.i << "," << Direction.j << "," << Direction.k << ")" << std::endl;
	}

	if (total_eTime >= 0.0f && total_eTime < 1.0f) {
		BossObj->SetPos_InTextureX(7);
		if (Direction.i <= 0)
			BossObj->SetPos_InTextureY(0);
		else
			BossObj->SetPos_InTextureY(1);

		ps.z = -jumpforce * ((total_eTime * total_eTime) - total_eTime + 0.25f) + 2;
		if (ps.z >= 0) {
			Direction.k = -jumpforce * (2 * total_eTime - 1);
			BossObj->SetVelocity(Direction);
			ps.x = ps.x + (BossObj->GetVelocity().i * ElapsedTime * moveforce);
			ps.y = ps.y + (BossObj->GetVelocity().j * ElapsedTime * moveforce);

			//std::cout << "Etime : " << total_eTime << " / 보스 위치 : (" << ps.x << "," << ps.y << "," << ps.z << ")" << " / 보스 속도 : (" << Direction.i << "," << Direction.j << "," << Direction.k << ")" << std::endl;
		}
		else {
			BossObj->SetVelocity(Vector(0, 0, 0));
			ps.z = 0;
		}


	}
	else if (total_eTime >= 1.0f && total_eTime < 1.5f) {
		BossObj->SetVelocity(Vector(0, 0, 0));
		BossObj->SetPos_InTextureX(8);
		ps.z = 0;
	}
	else if (total_eTime >= 1.5f && total_eTime < 2.0f) {
		BossObj->SetPos_InTextureX(2);
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
		BossObj->SetPos_InTextureX(4);

		Direction = BossObj->GetVelocity();
		Direction.k = -jumpforce * (2 * total_eTime + 1.5f);

		BossObj->SetVelocity(Direction);
		ps.z = -jumpforce * (total_eTime * (total_eTime - 1.5f));

		if (ps.z < 0) {
			BossObj->SetVelocity(Vector(0, 0, 0));
			ps.z = 0;
		}

		BossObj->SetPatternInit(true);
	}
	else if (total_eTime >= 0.75f && total_eTime < 1.5f) {
		BossObj->SetPos_InTextureX(6);

		Direction = BossObj->GetVelocity();
		Direction.k = -jumpforce * (2 * total_eTime - 1.5f);

		BossObj->SetVelocity(Direction);
		ps.z = -jumpforce * (total_eTime * (total_eTime - 1.5f));

		if (BossObj->GetPatternInit()) {
			Point newPoint = BossGetPoint();
			if (newPoint.x < ps.x)
				BossObj->SetPos_InTextureY(0);
			else
				BossObj->SetPos_InTextureY(1);

			ps.x = newPoint.x;
			ps.y = newPoint.y;
			BossObj->SetPatternInit(false);
		}

		if (ps.z < 0) {
			BossObj->SetVelocity(Vector(0, 0, 0));
			ps.z = 0;
		}
	}
	else if (total_eTime >= 1.5f && total_eTime < 1.7f) {
		BossObj->SetPos_InTextureX(5);
		BossObj->SetVelocity(Vector(0, 0, 0));
		ps.z = 0;

		BossObj->SetPatternInit(true);
	}
	else if (total_eTime >= 1.7f && total_eTime < 2.2f) {
		BossObj->SetPos_InTextureX(8);
		ps.z = 0;

		if (BossObj->GetPatternInit()) {
			BulletShoot(false, BossObj->GetPosition(), Vector(0, 0, 0), SHOOT_PATTERN_3);
			BossObj->SetPatternInit(false);
		}
	}
	else if (total_eTime >= 2.2f && total_eTime < 2.7f) {
		BossObj->SetPos_InTextureX(2);
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

	Point newTarget = BossGetPoint();

	if (total_eTime >= 0.f && total_eTime < 0.5f) {
		BossObj->SetPos_InTextureX(1);
		if (BossObj->GetPosition().x > newTarget.x)
			BossObj->SetPos_InTextureY(0);
		else
			BossObj->SetPos_InTextureY(1);
		BossObj->SetPatternInit(true);
	}
	else if (total_eTime >= 0.5f && total_eTime < 1.5f) {
		BossObj->SetPos_InTextureX(3);

		if (total_eTime >= 0.6f) {
			if (BossObj->GetPatternInit()) {
				if (BossObj->GetPos_InTextureY() == 0)
					BulletShoot(false, BossObj->GetPosition(), Vector(0, 0, 0), SHOOT_PATTERN_2);
				if (BossObj->GetPos_InTextureY() == 1)
					BulletShoot(false, BossObj->GetPosition(), Vector(0, 0, 0), SHOOT_PATTERN_1);
				BossObj->SetPatternInit(false);
			}
		}

		// std::cout << "Attack!!!" << "(" << vX << "," << vY << ")" << std::endl;
	}
	else if (total_eTime >= 1.5f && total_eTime < 2.0f) {
		BossObj->SetPos_InTextureX(2);
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
	Vector min_direction = { 100, 100, 100 };

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