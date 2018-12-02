#include "CGameObject.h"


Player::Player()
{
	m_HP = 0;
	m_SeqBody = { 0,0 };
	m_SeqHead = { 0,0 };

	ZeroMemory(m_KeyState, 256);
	ZeroMemory(m_SpecialKeyState, 246);
}

void Player::Update(float ElapsedTime)
{
	if (m_ShotCooltime > -FLT_EPSILON)
		m_ShotCooltime -= ElapsedTime;

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

		//CGameObject::ApplyForce(ExternalForce, elapsedTime);

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
	Point amount = { 0.041f, 0.209f, 0 };
	m_HeadPosition = m_BodyPosition + amount;
	// Animation Frame Update
	// ...
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

bool Player::CanShot()
{
	if (m_ShotCooltime < FLT_EPSILON)
		return true;

	return false;
}

void Player::ResetShotCooltime()
{
	m_ShotCooltime = m_InitShotCooltime;
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
