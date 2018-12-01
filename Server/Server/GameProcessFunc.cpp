#include "GameProcessFunc.h"

////////////////////////////////////////  Game & Network Valiable  ////////////////////////////////////////
																										 //
/*																										 //
CommunicationBuffer[index] ����																		     //
	- index 0 ~ MAX_CLIENT*2-1������ Player ������Ʈ�� ���� ����										 //
	- index MAX_CLIENT*2�� Boss ������Ʈ�� ���� ����													 //
	- index MAX_CLIENT*2+1 ~ MAX_OBJECT-1������ BULLET ������Ʈ�� ���� ����								 //
	- ���� ������Ʈ�� ��ſ� ������Ʈ�� �׻� 1:1 �� ��Ī�� �̷������.								     //
*/																										 //
CommunicationData CommunicationBuffer[MAX_OBJECT];														 //
																										 //
/*																										 //
GameObject ����																						     //
	- ���� ������Ʈ�� ��ſ� ������Ʈ�� �׻� 1:1 �� ��Ī�� �̷������.								     //
*/																										 //
Player * PlayerBuffer[MAX_CLIENT];																		 //
Boss   * BossObj = NULL;																				 //
PressurePlate * PressurePlateBuffer[MAX_CLIENT];														 //
Bullet * BulletBuffer[MAX_BULLET];																		 //
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
		if (PressurePlateBuffer[i] != NULL)
		{
			delete PressurePlateBuffer[i];
			PressurePlateBuffer[i] = NULL;
		}
	}

	if (BossObj != NULL)
	{
		delete BossObj;
		BossObj = NULL;
	}

	for (int i = 0; i < MAX_BULLET; ++i)
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

	// Boss �ʱ� �Ӽ��� ����
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
	// ���ο� �÷��̾� ����
	int ClientID = -1;
	// index 0 ~ MAX_CLIENT-1 �� PlayerBuffer[index] == NULL�� index�� ClientID�� ���
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
		// Player �ʱ� �Ӽ��� ����
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
		PlayerBuffer[ClientID]->SetBulletShootDealay(PLAYER_BULLET_SHOOT_DEALAY);
		PlayerBuffer[ClientID]->InitBulletShootDealay();

		if (!GameProcessFunc::CheckBossRaidStart())
		{
			Point newPoint;
			POINT newSeq = { 0, 0 };
			newPoint.x = (ClientID % 2 == 0) ? -3.2f : 3.2f;
			newPoint.y = (ClientID < 2) ? 1.5f : -1.5f;
			PressurePlateBuffer[ClientID] = new PressurePlate();
			PressurePlateBuffer[ClientID]->SetPosition(newPoint);
			PressurePlateBuffer[ClientID]->SetPos_InTexture(newSeq);
		}
	}

	return ClientID;
}

// Player_Body_Index (Player_Head_Index = Player_Body_Index + 1)
void GameProcessFunc::ComPlayerBodyIndex(int indexArray[])
{
	for (int i = 0; i < MAX_CLIENT; ++i)
		indexArray[i] = MAX_CLIENT + i*2;
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
	// Ű ���� �޾ƿ�
	retval = recvn(sock, (char*)PlayerBuffer[ClientID]->GetKeyBuffer(), sizeof(bool) * 256, 0);
	if (retval == SOCKET_ERROR || retval == 0)
	{
		delete PlayerBuffer[ClientID];
		PlayerBuffer[ClientID] = NULL;
		//delete PressurePlateBuffer[ClientID];
		//PressurePlateBuffer[ClientID] = NULL;
		err_display((char*)"recv()");
		return false;
	}

	// Ű ���� �޾ƿ�
	retval = recvn(sock, (char*)PlayerBuffer[ClientID]->GetSpecialKeyBuffer(), sizeof(bool) * 246, 0);
	if (retval == SOCKET_ERROR || retval == 0)
	{
		delete PlayerBuffer[ClientID];
		PlayerBuffer[ClientID] = NULL;
		//delete PressurePlateBuffer[ClientID];
		//PressurePlateBuffer[ClientID] = NULL;
		err_display((char*)"recv()");
		return false;
	}

	// ArrowKey�� ���ؼ��� Stack���� ����
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
		CommunicationBuffer�� MAX_OBJECT���̳� ������ ����:
		������Ʈ�� ���� ������ŭ�� ������ �� ��� ������ ���� ������ �߰�ó������� ����.
		  - Obj_Type == NULL�� ���Ұ� CommunicationBuffer ���� �������� �����ϸ�
			CommunicationBuffer �߰��� Obj_Type�� NULL�� ���Ұ� ������ ������ ����� �Ѵ�.
			(CommunicationBuffer ���� �����ͺ��� CommunicationData*CurrentObjectNum������ �����ϴ� ����̱⿡.)

		�̶�, (Obj_Type == NULL�� ���Ұ� ������ ��� ���� ���) + (������Ʈ�� ���� ������ŭ ���� �� �ɸ��� ���) �� ��쿡��
		������Ʈ�� ���� ������ ���� �� ȿ�����̴�.
		�ݴ�� ������Ʈ�� ������ ��������, ������Ʈ�� ���� �߻��󵵰� ����� ȿ�����̴�.
		�׷��� ������Ʈ�� ���� �߻��󵵴� ��Ȯ�� ������ ���� ���� ������,
		��żӵ��� 1/60�� �ȿ� CommunicatioBuffer�� MAX_OBJECT���� ���� �� ���� ������ �ӵ����
		CommunicatioBuffer�� MAX_OBJECT���� ������ ���� �� �ٶ����ϴ�.
	*/
	int retval = send(sock, (char*)&CommunicationBuffer, sizeof(CommunicationData)*MAX_OBJECT, 0);
	if (retval == SOCKET_ERROR)
	{
		delete PlayerBuffer[ClientID];
		PlayerBuffer[ClientID] = NULL;
		//delete PressurePlateBuffer[ClientID];
		//PressurePlateBuffer[ClientID] = NULL;
		err_display((char*)"send()");
		return false;
	}
	static CommunicationData2 CommunicationData_SubInfo;
	GameProcessFunc::ComPlayerBodyIndex(CommunicationData_SubInfo.Player_Index);
	CommunicationData_SubInfo.Player_HP = PlayerBuffer[ClientID]->GetHP();
	CommunicationData_SubInfo.Boss_HP = (BossObj != NULL) ? BossObj->GetHP() : 0;
	CommunicationData_SubInfo.GameFail = GameProcessFunc::CheckGameFail(ClientID);
	CommunicationData_SubInfo.GameClear = GameProcessFunc::CheckGameClear();
	for (int i = 0; i < MAX_CLIENT; ++i)
		CommunicationData_SubInfo.bPlayerHited[i] = (PlayerBuffer[i] != NULL) ? !PlayerBuffer[i]->CheckHitDealayComplete() : false;
	retval = send(sock, (char*)&CommunicationData_SubInfo, sizeof(CommunicationData2), 0);
	if (retval == SOCKET_ERROR)
	{
		delete PlayerBuffer[ClientID];
		PlayerBuffer[ClientID] = NULL;
		//delete PressurePlateBuffer[ClientID];
		//PressurePlateBuffer[ClientID] = NULL;
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
			if (GameProcessFunc::CheckGameFail(i) == false)
			{
				KeyState = PlayerBuffer[i]->GetKeyBuffer();
				ArrowKeyStack = PlayerBuffer[i]->GetArrowKeyStack();
				if (ArrowKeyStack->size() > 0) ArrowKey = ArrowKeyStack->back();
				else ArrowKey = -1;

				Vector newForce = { 0.0f, 0.0f };
				if (KeyState['w'] == true)
					newForce.j += PLAYER_SPEED;
				if (KeyState['a'] == true)
					newForce.i -= PLAYER_SPEED;
				if (KeyState['s'] == true)
					newForce.j -= PLAYER_SPEED;
				if (KeyState['d'] == true)
					newForce.i += PLAYER_SPEED;
				
				// ���� ũ�Ⱑ 0�� �ƴ϶�� ������Ʈ�� ���� ���Ѵ�.
				if (!equal(newForce.magnitude(), 0.0f))
				{
					normalize(newForce);
					PlayerBuffer[i]->ApplyForce(newForce, ElapsedTime);
				}

				if (ArrowKey == 0x0065) // 0x0065 : GLUT_KEY_UP
				{
					if (PlayerBuffer[i]->CheckBulletShootDealayComplete())
					{
						BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_UP);
						PlayerBuffer[i]->InitBulletShootDealay();
					}
				}
				if (ArrowKey == 0x0064) // 0x0064 : GLUT_KEY_LEFT
				{
					if (PlayerBuffer[i]->CheckBulletShootDealayComplete())
					{
						BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_LEFT);
						PlayerBuffer[i]->InitBulletShootDealay();
					}
				}
				if (ArrowKey == 0x0067) // 0x0067 : GLUT_KEY_DOWN
				{
					if (PlayerBuffer[i]->CheckBulletShootDealayComplete())
					{
						BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_DOWN);
						PlayerBuffer[i]->InitBulletShootDealay();
					}
				}
				if (ArrowKey == 0x0066) // 0x0066 : GLUT_KEY_RIGHT
				{
					if (PlayerBuffer[i]->CheckBulletShootDealayComplete())
					{
						BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_RIGHT);
						PlayerBuffer[i]->InitBulletShootDealay();
					}
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

	for (int i = 0; i < MAX_BULLET; ++i)
	{
		if (BulletBuffer[i] != NULL)
			BulletBuffer[i]->Update(ElapsedTime);
	}
}

void GameProcessFunc::ProcessCollision(float ElapsedTime)
{
	static Point PlayerPos, BossPos, PressurePlatePos, BulletPos;
	static fRECT PlayerBoundRect, BossBoundRect, PressurePlateBoundRect, BulletBoundRect, LandBoundRect, intersectRect;
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
		float UnderOrdered_Offset_Y = -((BOSS_HEIGHT * 0.5f) - BOSS_BOUNDINGBOX_HEIGHT) * 0.36f;
		// UnderOrdered_Offset_Y: 2���� ��鿡���� ���̰��� ��Ÿ���� ���� BoundingRect�� ���� ũ�⺸�� �۰� �ϰ� �̸�
		// ���� Bottom�� BoudingRect�� Bottom�� ���� �Ʒ��� �������� ������
		BossBoundRect.Left = BossPos.x - BOSS_BOUNDINGBOX_WIDTH / 2;
		BossBoundRect.Top = BossPos.y + BOSS_BOUNDINGBOX_HEIGHT / 2 + UnderOrdered_Offset_Y;
		BossBoundRect.Right = BossPos.x + BOSS_BOUNDINGBOX_WIDTH / 2;
		BossBoundRect.Bottom = BossPos.y - BOSS_BOUNDINGBOX_HEIGHT / 2 + UnderOrdered_Offset_Y;

		// ProcessCollision Wall-Boss
		if (equal(BossPos.z, 0.0f))
		{
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
	}

	static bool Hited_ForBoss[MAX_CLIENT] = { false, };
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		Hited_ForBoss[i] = false;
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
				Vector Velocity = PlayerBuffer[i]->GetVelocity();
				if (PlayerBoundRect.Left < LandBoundRect.Left) // Collision left-side
				{
					PlayerPos.x += LandBoundRect.Left - PlayerBoundRect.Left;
					Velocity.i = 0;
				}
				if (PlayerBoundRect.Right > LandBoundRect.Right) // Collision right-side
				{
					PlayerPos.x += LandBoundRect.Right - PlayerBoundRect.Right;
					Velocity.i = 0;
				}
				if (PlayerBoundRect.Bottom < LandBoundRect.Bottom) // Collision down-side
				{
					PlayerPos.y += LandBoundRect.Bottom - PlayerBoundRect.Bottom;
					Velocity.j= 0;
				}
				if (PlayerBoundRect.Top > LandBoundRect.Top) // Collision up-side
				{
					PlayerPos.y += LandBoundRect.Top - PlayerBoundRect.Top;
					Velocity.j = 0;
				}
				PlayerBuffer[i]->SetBodyPosition(PlayerPos);
				PlayerBuffer[i]->SetVelocity(Velocity);
			}

			// ProcessCollision Player-PressurePlate
			for (int j = 0; j < MAX_CLIENT; ++j)
			{
				if (PressurePlateBuffer[j] != NULL)
				{
					if (!PressurePlateBuffer[j]->CheckPressed())
					{
						// Set PressurePlateBoundingRect
						PressurePlatePos = PressurePlateBuffer[j]->GetPosition();
						PressurePlateBoundRect.Left = PressurePlatePos.x - PRESSURE_PLATE_BOUNDINGBOX_WIDTH / 2;
						PressurePlateBoundRect.Top = PressurePlatePos.y + PRESSURE_PLATE_BOUNDINGBOX_HEIGHT / 2;
						PressurePlateBoundRect.Right = PressurePlatePos.x + PRESSURE_PLATE_BOUNDINGBOX_WIDTH / 2;
						PressurePlateBoundRect.Bottom = PressurePlatePos.y - PRESSURE_PLATE_BOUNDINGBOX_HEIGHT / 2;

						if (CollisionFunc::IntersectRect(&intersectRect, PlayerBoundRect, PressurePlateBoundRect))
							PressurePlateBuffer[j]->SetPos_InTexture({ 1, 0 });
					}
				}
			}

			// ProcessCollision Player-Boss
			if (BossObj != NULL && GameProcessFunc::CheckGameClear() == false && GameProcessFunc::CheckGameFail(i) == false)
			{
				if (equal(BossPos.z, 0.0f))
				{
					if (CollisionFunc::IntersectRect(&intersectRect, PlayerBoundRect, BossBoundRect))
					{
						// ���� ���� ũ��
						float nInterWidth = abs(intersectRect.Right - intersectRect.Left);
						float nInterHeight = abs(intersectRect.Top - intersectRect.Bottom);

						// Moved_BoundingRect: PlayerBoundRect
						// Hold_BoundingRect: BossBoundRect

						Vector PlayerVelocity = PlayerBuffer[i]->GetVelocity();

						// ���� �ʺ� > ���� ����: ��/�Ʒ��������� �浹�� �̷����
						if (nInterWidth > nInterHeight)
						{
							// Hold_BoundingRect�� ���κ� �浹
							if (intersectRect.Top == BossBoundRect.Top)
								PlayerPos.y += nInterHeight;
							// Hold_BoundingRect�� �Ʒ��κ� �浹
							else if (intersectRect.Bottom == BossBoundRect.Bottom)
								PlayerPos.y -= nInterHeight;

							// y�� ���� �ӵ��� 0���� �ʱ�ȭ
							PlayerVelocity.j = 0;
						}
						// ���� �ʺ� < ���� ����: ��/��κ��� �浹�� �̷����
						else
						{
							// Hold_BoundingRect�� ���ʺκ� �浹
							if (intersectRect.Left == BossBoundRect.Left)
								PlayerPos.x -= nInterWidth;
							// Hold_BoundingRect�� �����ʺκ� �浹
							else if (intersectRect.Right == BossBoundRect.Right)
								PlayerPos.x += nInterWidth;

							// x�� ���� �ӵ��� 0���� �ʱ�ȭ
							PlayerVelocity.i = 0;
						}

						PlayerBuffer[i]->SetBodyPosition(PlayerPos);
						PlayerBuffer[i]->SetVelocity(PlayerVelocity);

						// �ǰݴ��ϴ� ������ Ȯ�� �� ü�� ����
						if (PlayerBuffer[i]->CheckHitDealayComplete())
						{
							u_int PlayerHP = PlayerBuffer[i]->GetHP();
							u_int newHP = ((int)PlayerHP - BOSS_BULLET_DAMAGE > 0) ? PlayerHP - BOSS_BULLET_DAMAGE : 0;
							PlayerBuffer[i]->SetHP(newHP);
							PlayerBuffer[i]->InitHitDealay();
							//std::cout << "Player[" << i << "] HP: " << newHP << std::endl;
							Hited_ForBoss[i] = true;
						}
					}
				}
			}
		}
	}

	bool DeleteBullet = false;
	for (int i = 0; i < MAX_BULLET; ++i)
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
						// �˹� �� HP ����
						BossPos += BulletBuffer[i]->GetVelocity() / RENDER_TRANSLATION_SCALE;
						u_int BossHP = BossObj->GetHP();
						u_int newHP = ((int)BossHP - PLAYER_BULLET_DAMAGE > 0) ? BossHP - PLAYER_BULLET_DAMAGE : 0;
						BossObj->SetPosition(BossPos);
						BossObj->SetHP(newHP);
						//std::cout << "Boss HP: " << newHP << std::endl;
						DeleteBullet = true;
					}
				}
			}

			// ProcessCollision Bullet-Player
			if (BulletBuffer[i]->GetPossesion() == false)
			{
				for (int j = 0; j < MAX_CLIENT; ++j)
				{
					if (PlayerBuffer[j] != NULL && GameProcessFunc::CheckGameClear() == false && GameProcessFunc::CheckGameFail(j) == false)
					{
						// Set PlayerBoundingRect
						PlayerPos = PlayerBuffer[j]->GetBodyPosition();
						PlayerBoundRect.Left = PlayerPos.x - PLAYER_BOUNDINGBOX_WIDTH / 2;
						PlayerBoundRect.Top = PlayerPos.y + PLAYER_BOUNDINGBOX_HEIGHT / 2;
						PlayerBoundRect.Right = PlayerPos.x + PLAYER_BOUNDINGBOX_WIDTH / 2;
						PlayerBoundRect.Bottom = PlayerPos.y - PLAYER_BOUNDINGBOX_HEIGHT / 2;

						if (CollisionFunc::IntersectRect(&intersectRect, BulletBoundRect, PlayerBoundRect))
						{
							// �˹�
							PlayerPos += (BulletBuffer[i]->GetVelocity() / RENDER_TRANSLATION_SCALE) * 6;
							PlayerBuffer[j]->SetBodyPosition(PlayerPos);
							// �ǰݴ��ϴ� ������ Ȯ�� �� ü�� ����
							if (Hited_ForBoss[i] == false && PlayerBuffer[j]->CheckHitDealayComplete())
							{
								u_int PlayerHP = PlayerBuffer[j]->GetHP();
								u_int newHP = ((int)PlayerHP - BOSS_BULLET_DAMAGE > 0) ? PlayerHP - BOSS_BULLET_DAMAGE : 0;
								PlayerBuffer[j]->SetHP(newHP);
								PlayerBuffer[j]->InitHitDealay();
								//std::cout << "Player[" << j << "] HP: " << newHP << std::endl;
								Hited_ForBoss[i] = true;
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

	if (Possesion == true) // Player�� �߻��� Bullet
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
		for (int i = 0; i < MAX_BULLET; ++i)
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
	else // Boss�� �߻��� Bullet
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
						newVelocity.i = cos(float(rand() % 45) / 180 * PI);

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
			bossShootNum = rand() % 4 + 7;
			currentShoot = 0;
			while (currentShoot < bossShootNum) {
				for (int i = 0; i < MAX_OBJECT - (MAX_CLIENT * 2 + 1); ++i)
				{
					if (BulletBuffer[i] == NULL)
					{
						newVelocity.i = -cos(float(rand() % 45) / 180 * PI);

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

		case SHOOT_PATTERN_3:
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
		}
	}
}

void GameProcessFunc::ResetCommunicationBuffer()
{
	ZeroMemory(CommunicationBuffer, MAX_OBJECT);
	/*
	CommunicationBuffer[index] ����
		- index 0 ~ MAX_CLIENT*2-1������ Player ������Ʈ�� ���� ����
		- index MAX_CLIENT*2�� Boss ������Ʈ�� ���� ����
		- index MAX_CLIENT*2+1 ~ MAX_OBJECT-1������ BULLET ������Ʈ�� ���� ����
		- ���� ������Ʈ�� ��ſ� ������Ʈ�� �׻� 1:1 �� ��Ī�� �̷������.
	*/


	// PlayerBuffer To CommunicationBuffer
	// And PressurePlateBuffer To CommunicationBuffer
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PlayerBuffer[i] != NULL)
		{
			CommunicationBuffer[MAX_CLIENT + i * 2].Obj_Type = KIND_PLAYER_BODY;
			CommunicationBuffer[MAX_CLIENT + i * 2].Obj_Pos = PlayerBuffer[i]->GetBodyPosition();
			CommunicationBuffer[MAX_CLIENT + i * 2].Obj_Pos_InTexture = PlayerBuffer[i]->GetSeqBody();
			CommunicationBuffer[MAX_CLIENT + i * 2].Obj_Velocity = PlayerBuffer[i]->GetVelocity();
			CommunicationBuffer[MAX_CLIENT + i * 2 + 1].Obj_Type = KIND_PLAYER_HEAD;
			CommunicationBuffer[MAX_CLIENT + i * 2 + 1].Obj_Pos = PlayerBuffer[i]->GetHeadPosition();
			CommunicationBuffer[MAX_CLIENT + i * 2 + 1].Obj_Pos_InTexture = PlayerBuffer[i]->GetSeqHead();
			CommunicationBuffer[MAX_CLIENT + i * 2 + 1].Obj_Velocity = PlayerBuffer[i]->GetVelocity();
		}
		else
		{
			CommunicationBuffer[MAX_CLIENT + i * 2].Obj_Type = KIND_NULL;
			CommunicationBuffer[MAX_CLIENT + i * 2 + 1].Obj_Type = KIND_NULL;
		}

		if (PressurePlateBuffer[i] != NULL)
		{
			CommunicationBuffer[i].Obj_Type = KIND_PRESSURE_PLATE;
			CommunicationBuffer[i].Obj_Pos = PressurePlateBuffer[i]->GetPosition();
			CommunicationBuffer[i].Obj_Pos_InTexture = PressurePlateBuffer[i]->GetPos_InTexture();
			CommunicationBuffer[i].Obj_Velocity = { 0.0f, 0.0f, 0.0f };
		}
		else
			CommunicationBuffer[i].Obj_Type = KIND_NULL;
	}

	// BossObj To CommunicationBuffer
	if (BossObj != NULL)
	{
		CommunicationBuffer[BOSS_ID].Obj_Type = KIND_BOSS;
		CommunicationBuffer[BOSS_ID].Obj_Pos = BossObj->GetPosition();
		CommunicationBuffer[BOSS_ID].Obj_Pos_InTexture = BossObj->GetPos_InTexture();
		CommunicationBuffer[BOSS_ID].Obj_Velocity = BossObj->GetVelocity();
	}
	else
		CommunicationBuffer[BOSS_ID].Obj_Type = KIND_NULL;

	// BulletBuffer To CommunicationBuffer
	for (int i = 0; i < MAX_BULLET; ++i)
	{
		if (BulletBuffer[i] != NULL)
		{
			CommunicationBuffer[MAX_CLIENT * 3 + 1 + i].Obj_Type = BulletBuffer[i]->GetPossesion() ? KIND_BULLET_1 : KIND_BULLET_2;
			CommunicationBuffer[MAX_CLIENT * 3 + 1 + i].Obj_Pos = BulletBuffer[i]->GetPosition();
			CommunicationBuffer[MAX_CLIENT * 3 + 1 + i].Obj_Velocity = BulletBuffer[i]->GetVelocity();
			CommunicationBuffer[MAX_CLIENT * 3 + 1 + i].Obj_Pos_InTexture = { 1,1 };
		}
		else
			CommunicationBuffer[MAX_CLIENT * 3 + 1 + i].Obj_Type = KIND_NULL;
	}
}

bool GameProcessFunc::CheckBossRaidStart()
{
	int NoPlayers = 0;
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PressurePlateBuffer[i] != NULL)
		{
			if (PressurePlateBuffer[i]->CheckPressed() != true)
				return false;
		}
		else ++NoPlayers;
	}
	if (NoPlayers == MAX_CLIENT) return false;
	else return true;
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

			//std::cout << "Etime : " << total_eTime << " / ���� ��ġ : (" << ps.x << "," << ps.y << "," << ps.z << ")" << " / ���� �ӵ� : (" << Direction.i << "," << Direction.j << "," << Direction.k << ")" << std::endl;
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
				//std::cout << i << "�� Ŭ�� : " << BossPosition.distanceTo(ClientPosition) << std::endl;
				min_direction = ClientPosition - BossPosition;
			}
		}
	}
	//std::cout << "���� ���� : " << "(" << unit(min_direction).i << "," << unit(min_direction).j << "," << unit(min_direction).k << ")" <<std::endl;
	return unit(min_direction);
}

Point GameProcessFunc::BossGetPoint()
{
	Point newPoint;
	int randomPlayer;

	int nullPlayerCnt = 0;
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PlayerBuffer[i] == NULL)
			nullPlayerCnt++;
	}
	if (nullPlayerCnt == MAX_CLIENT) return newPoint;

	while (1) {
		randomPlayer = rand() % MAX_CLIENT;
		if (PlayerBuffer[randomPlayer] != NULL) {
			newPoint = PlayerBuffer[randomPlayer]->GetHeadPosition();
			break;
		}
	}

	//std::cout << "�� ��ġ : " << "(" << newPoint.x << "," << newPoint.y << "," << newPoint.z << ")" << std::endl;

	return newPoint;
}

bool GameProcessFunc::CheckGameClear()
{
	if (BossObj != NULL)
	{
		if (BossObj->GetHP() > 0)
			return false;
		else return true;
	}
	else return false;
}

bool GameProcessFunc::CheckGameFail(int ClientID)
{
	if (PlayerBuffer[ClientID] != NULL)
	{
		if (PlayerBuffer[ClientID]->GetHP() > 0) return false;
		else return true;
	}
	else return false;
}
