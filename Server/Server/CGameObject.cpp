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
		// 중력(Gravity) = mg (g: 중력가속도(9.8m/s²)
		Vector Gravity = Vector(1.0f, 1.0f, 1.0f) * PLAYER_MASS * GravityAccelarationFactor;

		// 마찰력(Friction) = uN (u: 마찰계수, N: 수직항력(-mg))
		Vector NormalForce = -Gravity;
		Vector Friction = PLAYER_FRICTION_FACTOR * NormalForce;
		Vector Direction = unit(m_Velocity);
		Friction.i *= Direction.i;
		Friction.j *= Direction.j;
		Friction.k *= Direction.k;

		// 외력(ExternalForce)
		Vector ExternalForce = /*Gravity + */Friction;

		//CGameObject::ApplyForce(ExternalForce, elapsedTime);

		 // Calculate Acceleration
		Vector Acceleration = ExternalForce / PLAYER_MASS;
		Vector AfterVelocity = m_Velocity + Acceleration * ElapsedTime;

		if (cosine(AfterVelocity, m_Velocity) < 0.0f) // 두 벡터 사잇각이 둔각일 경우 = 벡터의 성분(x,y,z)중 부호가 다른 성분이 1개 이상 존재.
			m_Velocity = { 0.0f, 0.0f, 0.0f };
		else
			m_Velocity = AfterVelocity;
	}

	// Calculation Position
	// 새로운 위치 = 이전 위치 + 속도 * 시간
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
	// 가속도 = 힘(외력)/질량
	// a = F/m (m/s²)
	m_Acceleration = Force / PLAYER_MASS;
	if (equal(m_Acceleration.magnitude(), 0.0f)) m_Acceleration = { 0.0f, 0.0f, 0.0f };

	// Calculation Velocity
	// 새로운 속도 = 이전 속도 + 가속도 * 시간
	// v = a * s = (m/s²) * s
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
		// 중력(Gravity) = mg (g: 중력가속도(9.8m/s²)
		Vector Gravity = Vector(1.0f, 1.0f, 1.0f) * BOSS_MASS * GravityAccelarationFactor;

		// 마찰력(Friction) = uN (u: 마찰계수, N: 수직항력(-mg))
		Vector NormalForce = -Gravity;
		Vector Friction = BOSS_FRICTION_FACTOR * NormalForce;
		Vector Direction = unit(m_Velocity);
		Friction.i *= Direction.i;
		Friction.j *= Direction.j;
		Friction.k *= Direction.k;

		// 외력(ExternalForce)
		Vector ExternalForce = /*Gravity + */Friction;

		//CGameObject::ApplyForce(ExternalForce, elapsedTime);

		 // Calculate Acceleration
		Vector Acceleration = ExternalForce / BOSS_MASS;
		Vector AfterVelocity = m_Velocity + Acceleration * ElapsedTime;

		if (cosine(AfterVelocity, m_Velocity) < 0.0f) // 두 벡터 사잇각이 둔각일 경우 = 벡터의 성분(x,y,z)중 부호가 다른 성분이 1개 이상 존재.
			m_Velocity = { 0.0f, 0.0f, 0.0f };
		else
			m_Velocity = AfterVelocity;
	}

	// Calculation Position
	// 새로운 위치 = 이전 위치 + 속도 * 시간
	m_Position += m_Velocity * ElapsedTime;

	// Update Sequence in Texture
	// ...
}

void Boss::ApplyForce(Vector Force, float ElapsedTime)
{
	// Calculation Acceleration
	// 가속도 = 힘(외력)/질량
	// a = F/m (m/s²)
	m_Acceleration = Force / BOSS_MASS;
	if (equal(m_Acceleration.magnitude(), 0.0f)) m_Acceleration = { 0.0f, 0.0f, 0.0f };

	// Calculation Velocity
	// 새로운 속도 = 이전 속도 + 가속도 * 시간
	// v = a * s = (m/s²) * s
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
		// 중력(Gravity) = mg (g: 중력가속도(9.8m/s²)
		Vector Gravity = Vector(1.0f, 1.0f, 1.0f) * BULLET_MASS * GravityAccelarationFactor;

		// 마찰력(Friction) = uN (u: 마찰계수, N: 수직항력(-mg))
		Vector NormalForce = -Gravity;
		Vector Friction = BULLET_FRICTION_FACTOR * NormalForce;
		Vector Direction = unit(m_Velocity);
		Friction.i *= Direction.i;
		Friction.j *= Direction.j;
		Friction.k *= Direction.k;

		// 외력(ExternalForce)
		Vector ExternalForce = /*Gravity + */Friction;

		//CGameObject::ApplyForce(ExternalForce, elapsedTime);

		 // Calculate Acceleration
		Vector Acceleration = ExternalForce / BULLET_MASS;
		Vector AfterVelocity = m_Velocity + Acceleration * ElapsedTime;

		if (cosine(AfterVelocity, m_Velocity) < 0.0f) // 두 벡터 사잇각이 둔각일 경우 = 벡터의 성분(x,y,z)중 부호가 다른 성분이 1개 이상 존재.
			m_Velocity = { 0.0f, 0.0f, 0.0f };
		else
			m_Velocity = AfterVelocity;
	}

	// Calculation Position
	// 새로운 위치 = 이전 위치 + 속도 * 시간
	m_Position += m_Velocity * ElapsedTime;
}
