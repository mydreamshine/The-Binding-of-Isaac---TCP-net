#pragma once
#include "stdafx.h"

// 통신에 쓰일 오브젝트
struct CommunicationData;

// 게임 플레이에 쓰일 오브젝트
class Player;
class Boss;
class Bullet;










struct CommunicationData
{
	int   Obj_Type = KIND_NULL;
	Point Obj_Pos;  // float x, y, z
	POINT Obj_Pos_InTexture; // LONG x, y
};





class Player
{
private:
	u_int  m_HP;
	Point  m_HeadPosition;
	Point  m_BodyPosition;
	Vector m_Velocity;
	Vector m_Acceleration;

	POINT  m_Pos_InHeadTexture;
	POINT  m_Pos_InBodyTexture;

	bool   m_KeyState[256];
	bool   m_SpecialKeyState[246];
public:
	Player();
	~Player() = default;

	void SetHP(u_int newHP)                   { m_HP = newHP; }
	void SetHeadPosition(Point newPoint)      { m_HeadPosition = newPoint; }
	void SetBodyPosition(Point newPoint)      { m_BodyPosition = newPoint; }
	void SetVelocity(Vector nweVelocity)      { m_Velocity = nweVelocity; }
	void SetPos_InHeadTexture(POINT newPoint) { m_Pos_InHeadTexture = newPoint; }
	void SetPos_InBodyTexture(POINT newPoint) { m_Pos_InBodyTexture = newPoint; }

	u_int  GetHP()                            { return m_HP; }
	Point  GetHeadPosition()                  { return m_HeadPosition; }
	Point  GetBodyPosition()                  { return m_BodyPosition; }
	Vector GetVelocity()                      { return m_Velocity; }
	POINT  GetPos_InHeadTexture()             { return m_Pos_InHeadTexture; }
	POINT  GetPos_InBodyTexture()             { return m_Pos_InBodyTexture; }

	bool * GetKeyBuffer()                     { return m_KeyState; }
	bool * GetSpecialKeyBuffer()              { return m_SpecialKeyState; }

	void Update(float ElapsedTime);
	void ApplyForce(Vector Force, float ElapsedTime);
};






class Boss
{
private:
	u_int  m_HP;
	Point  m_Position;
	Vector m_Velocity;
	Vector m_Acceleration;

	POINT  m_Pos_InTexture;

public:
	Boss();
	~Boss() = default;

	void SetHP(u_int newHP)               { m_HP = newHP; }
	void SetPosition(Point newPoint)      { m_Position = newPoint; }
	void SetVelocity(Vector nweVelocity)  { m_Velocity = nweVelocity; }
	void SetPos_InTexture(POINT newPoint) { m_Pos_InTexture = newPoint; }

	u_int  GetHP()                        { return m_HP; }
	Point  GetPosition()                  { return m_Position; }
	Vector GetVelocity()                  { return m_Velocity; }
	POINT  GetPos_InTexture()             { return m_Pos_InTexture; }

	void Update(float ElapsedTime);
	void ApplyForce(Vector Force, float ElapsedTime);
};







class Bullet
{
private:
	bool   m_Possesion; // TRUE이면 Player 소유, FALSE면 Boss 소유
	Point  m_Position;
	Vector m_Velocity;
	Vector m_Acceleration;
	float  m_Mass;
	float  m_FrictionFactor;
	u_int  m_Damage;

public:
	Bullet();
	~Bullet() = default;

	void SetPossesion(bool newPossesion)            { m_Possesion = newPossesion; }
	void SetPosition(Point newPoint)                { m_Position = newPoint; }
	void SetVelocity(Vector newVelocity)            { m_Velocity = newVelocity; }
	void SetMass(float newMass)                     { m_Mass = newMass; }
	void SetFrictionFactor(float newFrictionFactor) { m_FrictionFactor = newFrictionFactor; }
	void SetDamage(u_int newDamage)                 { m_Damage = newDamage; }

	bool   GetPossesion()                           { return m_Possesion; }
	Point  GetPosition()                            { return m_Position; }
	Vector GetVelocity()                            { return m_Velocity; }
	float  GetMass()                                { return m_Mass; }
	float  GetFrictionFactor()                      { return m_FrictionFactor; }
	u_int  GetDamage()                              { return m_Damage; }

	void Update(float ElapsedTime);
};