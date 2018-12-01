#include "CGameObject.h"
#include <time.h>

Player::Player()
{
	m_HP = 0;
	m_SeqBody = { 0,0 };
	m_SeqHead = { 0,0 };

	ZeroMemory(m_KeyState, 256);
	ZeroMemory(m_SpecialKeyState, 246);

	m_HitDealay = 0;
	m_StartHitDealay = timeGetTime();

	m_BulletShootDealay = 0;
	m_StartBulletShootDealay = timeGetTime();
}

void Player::InitHitDealay()
{
	m_StartHitDealay = timeGetTime();
}

bool Player::CheckHitDealayComplete()
{
	if (timeGetTime() - m_StartHitDealay >= m_HitDealay) return true;
	return false;
}

void Player::InitBulletShootDealay()
{
	m_StartBulletShootDealay = timeGetTime();
}

bool Player::CheckBulletShootDealayComplete()
{
	int ArrowKey = NULL;
	if (m_ArrowKeyStack.size() > 0) ArrowKey = m_ArrowKeyStack.back();
	if (timeGetTime() - m_StartBulletShootDealay >= m_BulletShootDealay)
	{
		// Animation Frame Update
		if (ArrowKey == 0x0065) // 0x0065 : GLUT_KEY_UP
		{
			if (m_SeqHead.x == 5) m_SeqHead.x = 4;
			else m_SeqHead.x = 5;
		}
		if (ArrowKey == 0x0064) // 0x0064 : GLUT_KEY_LEFT
		{
			if (m_SeqHead.x == 7) m_SeqHead.x = 6;
			else m_SeqHead.x = 7;
		}
		if (ArrowKey == 0x0067) // 0x0067 : GLUT_KEY_DOWN
		{
			if (m_SeqHead.x == 1) m_SeqHead.x = 0;
			else m_SeqHead.x = 1;
		}
		if (ArrowKey == 0x0066) // 0x0066 : GLUT_KEY_RIGHT
		{
			if (m_SeqHead.x == 3) m_SeqHead.x = 2;
			else m_SeqHead.x = 3;
		}
		return true;
	}
	return false;
}

void Player::Update(float ElapsedTime)
{
	if (!equal(m_Velocity.magnitude(), 0.0f))
	{
		// �߷�(Gravity) = mg (g: �߷°��ӵ�(9.8m/s��)
		Vector Gravity = Vector(1.0f, 1.0f, 1.0f) * PLAYER_MASS * GravityAccelarationFactor;

		// ������(Friction) = uN (u: �������, N: �����׷�(-mg))
		Vector NormalForce = -Gravity;
		Vector Friction = PLAYER_FRICTION_FACTOR * NormalForce;
		Vector Direction = unit(m_Velocity);
		Friction.i *= Direction.i;
		Friction.j *= Direction.j;
		Friction.k *= Direction.k;

		// �ܷ�(ExternalForce)
		Vector ExternalForce = /*Gravity + */Friction;

		//Player::ApplyForce(ExternalForce, ElapsedTime);

		 // Calculate Acceleration
		Vector Acceleration = ExternalForce / PLAYER_MASS;
		Vector AfterVelocity = m_Velocity + Acceleration * ElapsedTime;

		if (cosine(AfterVelocity, m_Velocity) < 0.0f) // �� ���� ���հ��� �а��� ��� = ������ ����(x,y,z)�� ��ȣ�� �ٸ� ������ 1�� �̻� ����.
			m_Velocity = { 0.0f, 0.0f, 0.0f };
		else
			m_Velocity = AfterVelocity;
	}

	// Calculation Position
	// ���ο� ��ġ = ���� ��ġ + �ӵ� * �ð�
	//m_HeadPosition += m_Velocity * ElapsedTime;
	m_BodyPosition += m_Velocity * ElapsedTime;
	Point amount = { PLAYER_HEAD_OFFSET_X, PLAYER_HEAD_OFFSET_Y, 0 };
	m_HeadPosition = m_BodyPosition + amount;

	// Animation Frame Update
	// Sprite Sequence ��� (�ӵ��� ��� / Sprite �� ĳ���� ������ �ݿ��ؼ� Sequence ��ȯ)
	Vector MoveDistance = m_Velocity * ElapsedTime;
	static float FootStep = 0.0f;

	if (abs(m_Velocity.i) < 0.5f) // Move Up/Down Or Stop
	{
		if (m_SeqBody.y != 0)
		{
			m_SeqBody.y = 0;
			m_SeqBody.x = 0;
		}
		if(abs(MoveDistance.j) > 0.0f)
			FootStep += abs(MoveDistance.j) / (PLAYER_FOOTSTEP_DIST_ONE_SEQUENCE);
	}
	else if (m_Velocity.i < 0.0f) // Move Left
	{
		if (m_SeqBody.y != 1)
		{
			m_SeqBody.y = 1;
			m_SeqBody.x = 0;
		}
		FootStep += abs(MoveDistance.i) / (PLAYER_FOOTSTEP_DIST_ONE_SEQUENCE);
	}
	else if (m_Velocity.i > 0.0f) // Move Right
	{
		if (m_SeqBody.y != 2)
		{
			m_SeqBody.y = 2;
			m_SeqBody.x = 0;
		}
		FootStep += abs(MoveDistance.i) / (PLAYER_FOOTSTEP_DIST_ONE_SEQUENCE);
	}
	// MoveDist = �̵� �Ÿ� : �̵� �ӵ� * ���� ����ð�
	// FootStep = 1���� : Isaac_Body.png�󿡼��� �� �� (10 Sequence)
	// FootStepDist = 1���� ���� : 17pixel(0.17m)
	// FootStepDist_OneSeq = 1Sequence �� ���� ���� : 17pixel / 10 Sequence = 1.7Pixel(0.017m)
	// MoveDist > FootStepDist_OneSeq �� ���
	// ���� SequenceNumber = ���� SequenceNumber + (MoveDist / FootStepDist_OneSeq)
	while (FootStep - 1.0f > 0.0f)
	{
		FootStep = (FootStep - 1.0f < 0.0f) ? 0.0f : FootStep - 1.0f;
		m_SeqBody.x = (m_Velocity.magnitude() > 0.5f) ? (m_SeqBody.x + 1) % MAX_PLAYER_BODY_ANIMATION_SEQUENCE_X : 0;
	}
}

void Player::ApplyForce(Vector Force, float ElapsedTime)
{
	// Calculation Acceleration
	// ���ӵ� = ��(�ܷ�)/����
	// a = F/m (m/s��)
	m_Acceleration = Force / PLAYER_MASS;
	if (equal(m_Acceleration.magnitude(), 0.0f)) m_Acceleration = { 0.0f, 0.0f, 0.0f };

	// Calculation Velocity
	// ���ο� �ӵ� = ���� �ӵ� + ���ӵ� * �ð�
	// v = a * s = (m/s��) * s
	m_Velocity += m_Acceleration * ElapsedTime;
	if (equal(m_Velocity.magnitude(), 0.0f)) m_Velocity = { 0.0f, 0.0f, 0.0f };
}

Boss::Boss()
{
	m_HP = 0;
	m_Pos_InTexture = { 0,0 };
}

void Boss::Update(float ElapsedTime)
{
	if (!equal(m_Velocity.magnitude(), 0.0f))
	{
		// �߷�(Gravity) = mg (g: �߷°��ӵ�(9.8m/s��)
		Vector Gravity = Vector(1.0f, 1.0f, 1.0f) * BOSS_MASS * GravityAccelarationFactor;

		// ������(Friction) = uN (u: �������, N: �����׷�(-mg))
		Vector NormalForce = -Gravity;
		Vector Friction = BOSS_FRICTION_FACTOR * NormalForce;
		Vector Direction = unit(m_Velocity);
		Friction.i *= Direction.i;
		Friction.j *= Direction.j;
		Friction.k *= Direction.k;

		// �ܷ�(ExternalForce)
		Vector ExternalForce = /*Gravity + */Friction;

		//CGameObject::ApplyForce(ExternalForce, elapsedTime);

		 // Calculate Acceleration
		Vector Acceleration = ExternalForce / BOSS_MASS;
		Vector AfterVelocity = m_Velocity + Acceleration * ElapsedTime;

		if (cosine(AfterVelocity, m_Velocity) < 0.0f) // �� ���� ���հ��� �а��� ��� = ������ ����(x,y,z)�� ��ȣ�� �ٸ� ������ 1�� �̻� ����.
			m_Velocity = { 0.0f, 0.0f, 0.0f };
		else
			m_Velocity = AfterVelocity;
	}

	// Calculation Position
	// ���ο� ��ġ = ���� ��ġ + �ӵ� * �ð�
	m_Position += m_Velocity * ElapsedTime;

	// Update Sequence in Texture
	// ...
}

void Boss::ApplyForce(Vector Force, float ElapsedTime)
{
	// Calculation Acceleration
	// ���ӵ� = ��(�ܷ�)/����
	// a = F/m (m/s��)
	m_Acceleration = Force / BOSS_MASS;
	if (equal(m_Acceleration.magnitude(), 0.0f)) m_Acceleration = { 0.0f, 0.0f, 0.0f };

	// Calculation Velocity
	// ���ο� �ӵ� = ���� �ӵ� + ���ӵ� * �ð�
	// v = a * s = (m/s��) * s
	m_Velocity += m_Acceleration * ElapsedTime;
	if (equal(m_Velocity.magnitude(), 0.0f)) m_Velocity = { 0.0f, 0.0f, 0.0f };
}

Bullet::Bullet()
{
	m_Possesion = FALSE;
	m_Mass = 0.0f;
	m_FrictionFactor = 0.0f;
	m_Damage = 0;
	m_ReflectCnt = 0;
}

void Bullet::Update(float ElapsedTime)
{
	if (!equal(m_Velocity.magnitude(), 0.0f))
	{
		// �߷�(Gravity) = mg (g: �߷°��ӵ�(9.8m/s��)
		Vector Gravity = Vector(1.0f, 1.0f, 1.0f) * BULLET_MASS * GravityAccelarationFactor;

		// ������(Friction) = uN (u: �������, N: �����׷�(-mg))
		Vector NormalForce = -Gravity;
		Vector Friction = BULLET_FRICTION_FACTOR * NormalForce;
		Vector Direction = unit(m_Velocity);
		Friction.i *= Direction.i;
		Friction.j *= Direction.j;
		Friction.k *= Direction.k;

		// �ܷ�(ExternalForce)
		Vector ExternalForce = /*Gravity + */Friction;

		//CGameObject::ApplyForce(ExternalForce, elapsedTime);

		 // Calculate Acceleration
		Vector Acceleration = ExternalForce / BULLET_MASS;
		Vector AfterVelocity = m_Velocity + Acceleration * ElapsedTime;

		if (cosine(AfterVelocity, m_Velocity) < 0.0f) // �� ���� ���հ��� �а��� ��� = ������ ����(x,y,z)�� ��ȣ�� �ٸ� ������ 1�� �̻� ����.
			m_Velocity = { 0.0f, 0.0f, 0.0f };
		else
			m_Velocity = AfterVelocity;
	}

	// Calculation Position
	// ���ο� ��ġ = ���� ��ġ + �ӵ� * �ð�
	m_Position += m_Velocity * ElapsedTime;
}
