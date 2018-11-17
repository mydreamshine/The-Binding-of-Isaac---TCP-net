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

	// Boss �ʱ� �Ӽ��� ����
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
		Point newHeadPoint = { newBodyPoint.x + 0.041f, newBodyPoint.y + 0.209f,newBodyPoint.z };
		POINT newPos_InHeadTexture = { 0,0 };
		POINT newPos_InBodyTexture = { 0,0 };

		PlayerBuffer[ClientID] = new Player();
		PlayerBuffer[ClientID]->SetHP(newHP);
		PlayerBuffer[ClientID]->SetBodyPosition(newBodyPoint);
		PlayerBuffer[ClientID]->SetHeadPosition(newHeadPoint);
		PlayerBuffer[ClientID]->SetPos_InHeadTexture(newPos_InHeadTexture);
		PlayerBuffer[ClientID]->SetPos_InBodyTexture(newPos_InBodyTexture);
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
	// Ű ���� �޾ƿ�
	retval = recvn(sock, (char*)PlayerBuffer[ClientID]->GetKeyBuffer(), sizeof(bool) * 256, 0);
	if (retval == SOCKET_ERROR || retval == 0)
	{
		delete PlayerBuffer[ClientID];
		PlayerBuffer[ClientID] = NULL;
		err_display((char*)"recv()");
		return false;
	}

	// Ű ���� �޾ƿ�
	retval = recvn(sock, (char*)PlayerBuffer[ClientID]->GetSpecialKeyBuffer(), sizeof(bool) * 246, 0);
	if (retval == SOCKET_ERROR || retval == 0)
	{
		delete PlayerBuffer[ClientID];
		PlayerBuffer[ClientID] = NULL;
		err_display((char*)"recv()");
		return false;
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
		err_display((char*)"send()");
		return false;
	}
	return true;
}

void GameProcessFunc::ProcessInput(float ElapsedTime)
{
	bool* KeyState = NULL;
	bool* SpecialKeyState = NULL;
	POINT seq = { 0,0 };
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PlayerBuffer[i] != NULL)
		{
			KeyState = PlayerBuffer[i]->GetKeyBuffer();
			SpecialKeyState = PlayerBuffer[i]->GetSpecialKeyBuffer();

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
				PlayerBuffer[i]->SetSeqBody(seq);
				newForce.j += PLAYER_SPEED;
				PlayerBuffer[i]->SetPos_InBodyTexture(seq);
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
				PlayerBuffer[i]->SetSeqBody(seq);
				newForce.i -= PLAYER_SPEED;
				PlayerBuffer[i]->SetPos_InBodyTexture(seq);
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
				PlayerBuffer[i]->SetSeqBody(seq);
				newForce.j -= PLAYER_SPEED;
				PlayerBuffer[i]->SetPos_InBodyTexture(seq);
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
				PlayerBuffer[i]->SetSeqBody(seq);
				newForce.i += PLAYER_SPEED;
				PlayerBuffer[i]->SetPos_InBodyTexture(seq);
				// ...
			}

			// ���� ũ�Ⱑ 0�� �ƴ϶�� ������Ʈ�� ���� ���Ѵ�.
			if (!equal(newForce.magnitude(), 0.0f))
			{
				normalize(newForce);
				PlayerBuffer[i]->ApplyForce(newForce, ElapsedTime);
			}

			if (SpecialKeyState[0x0065] == true) // 0x0065 : GLUT_KEY_UP
			{
				// Animation Frame Set(UpView-Sequance)
				seq = PlayerBuffer[i]->GetSeqHead();
				if (seq.x == 5)	seq.x = 4;
				else seq.x = 5;
				PlayerBuffer[i]->SetPos_InHeadTexture(seq);
				// ...

				BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_UP);
			}
			if (SpecialKeyState[0x0064] == true) // 0x0064 : GLUT_KEY_LEFT
			{
				// Animation Frame Set(LeftView-Sequance)
				seq = PlayerBuffer[i]->GetSeqHead();
				if (seq.x == 7)	seq.x = 6;
				else seq.x = 7;
				PlayerBuffer[i]->SetPos_InHeadTexture(seq);
				//...

				BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_LEFT);
			}
			if (SpecialKeyState[0x0067] == true) // 0x0067 : GLUT_KEY_DOWN
			{
				// Animation Frame Set(DownView-Sequance)
				seq = PlayerBuffer[i]->GetSeqHead();
				if (seq.x == 1)	seq.x = 0;
				else seq.x = 1;
				PlayerBuffer[i]->SetPos_InHeadTexture(seq);
				//...

				BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_DOWN);
			}
			if (SpecialKeyState[0x0066] == true) // 0x0066 : GLUT_KEY_RIGHT
			{
				// Animation Frame Set(RightView-Sequance)
				seq = PlayerBuffer[i]->GetSeqHead();
				if (seq.x == 3)	seq.x = 2;
				else seq.x = 3;
				PlayerBuffer[i]->SetPos_InHeadTexture(seq);
				// ...

				BulletShoot(true, PlayerBuffer[i]->GetHeadPosition(), PlayerBuffer[i]->GetVelocity(), SHOOT_RIGHT);
			}
		}
		/* �߰� ��, ���� �߻�
		// Ű ���� ������ �⺻ �������� ���ư�
		if (PlayerBuffer[i] == NULL)
		{
			seq = PlayerBuffer[i]->GetSeq();
			if (seq.y = 0) seq.x = 0;
			if (seq.y = 1) seq.x = 9;
			if (seq.y = 2) seq.x = 0;
			PlayerBuffer[i]->SetSeq(seq);
			PlayerBuffer[i]->SetPos_InBodyTexture(seq);
		}*/
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
				Vector VerticalOfLine; // ������ ������ ����
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
	else // Boss�� �߻��� Bullet
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
	CommunicationBuffer[index] ����
		- index 0 ~ MAX_CLIENT*2-1������ Player ������Ʈ�� ���� ����
		- index MAX_CLIENT*2�� Boss ������Ʈ�� ���� ����
		- index MAX_CLIENT*2+1 ~ MAX_OBJECT-1������ BULLET ������Ʈ�� ���� ����
		- ���� ������Ʈ�� ��ſ� ������Ʈ�� �׻� 1:1 �� ��Ī�� �̷������.
	*/
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		if (PlayerBuffer[i] != NULL)
		{
			CommunicationBuffer[i * 2].Obj_Type = KIND_PLAYER_BODY;
			CommunicationBuffer[i * 2].Obj_Pos = PlayerBuffer[i]->GetBodyPosition();
			CommunicationBuffer[i * 2].Obj_Pos_InTexture = PlayerBuffer[i]->GetPos_InBodyTexture();
			CommunicationBuffer[i * 2 + 1].Obj_Type = KIND_PLAYER_HEAD;
			CommunicationBuffer[i * 2 + 1].Obj_Pos = PlayerBuffer[i]->GetHeadPosition();
			CommunicationBuffer[i * 2 + 1].Obj_Pos_InTexture = PlayerBuffer[i]->GetPos_InHeadTexture();
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
