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
	int    Obj_Type = KIND_NULL;
	Point  Obj_Pos;  // float x, y, z
	POINT  Obj_Pos_InTexture; // LONG x, y
	Vector Obj_Velocity;
};





class Player
{
private:
	u_int  m_HP;
	Point  m_HeadPosition;
	Point  m_BodyPosition;
	Vector m_Velocity;
	Vector m_Acceleration;

	POINT  m_SeqBody;
	POINT  m_SeqHead;

	bool      m_KeyState[256];
	bool      m_SpecialKeyState[246];
	list<int> m_ArrowKeyStack;

	long      m_HitDealay; //millisecond
	long      m_StartHitDealay;
public:
	Player();
	~Player() = default;

	void SetHP(u_int newHP)                   { m_HP = newHP; }
	void SetHeadPosition(Point newPoint)      { m_HeadPosition = newPoint; }
	void SetBodyPosition(Point newPoint)      { m_BodyPosition = newPoint; }
	void SetVelocity(Vector nweVelocity)      { m_Velocity = nweVelocity; }
	void SetSeqBody(POINT newPoint)           { m_SeqBody = newPoint; }
	void SetSeqHead(POINT newPoint)           { m_SeqHead = newPoint; }

	u_int  GetHP()                            { return m_HP; }
	Point  GetHeadPosition()                  { return m_HeadPosition; }
	Point  GetBodyPosition()                  { return m_BodyPosition; }
	Vector GetVelocity()                      { return m_Velocity; }

	POINT GetSeqBody()						  { return m_SeqBody; }
	POINT GetSeqHead()						  { return m_SeqHead; }

	bool * GetKeyBuffer()                     { return m_KeyState; }
	bool * GetSpecialKeyBuffer()              { return m_SpecialKeyState; }
	list<int>* GetArrowKeyStack()             { return &m_ArrowKeyStack; }

	void SetHitDealay(long newHitDealay)       { m_HitDealay = newHitDealay; }
	void InitHitDealay();
	bool CheckHitDealayComplete();

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

	short  m_Pattern;
	float  m_Etime;
	bool   m_PatternInit;

public:
	Boss();
	~Boss() = default;

	void SetHP(u_int newHP) { m_HP = newHP; }
	void SetPosition(Point newPoint) { m_Position = newPoint; }
	void SetVelocity(Vector nweVelocity) { m_Velocity = nweVelocity; }
	void SetPos_InTexture(POINT newPoint) { m_Pos_InTexture = newPoint; }
	void SetPattern(short pattern) { m_Pattern = pattern; }
	void SetEtime(float Etime) { m_Etime = Etime; }
	void SetPatternInit(bool PatternInit) { m_PatternInit = PatternInit; }

	u_int  GetHP() { return m_HP; }
	Point  GetPosition() { return m_Position; }
	Vector GetVelocity() { return m_Velocity; }
	POINT  GetPos_InTexture() { return m_Pos_InTexture; }
	short  GetPattern() { return m_Pattern; }
	float  GetEtime() { return m_Etime; }
	bool   GetPatternInit() { return m_PatternInit; }

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

	u_short m_ReflectCnt;

public:
	Bullet();
	~Bullet() = default;

	void SetPossesion(bool newPossesion)            { m_Possesion = newPossesion; }
	void SetPosition(Point newPoint)                { m_Position = newPoint; }
	void SetVelocity(Vector newVelocity)            { m_Velocity = newVelocity; }
	void SetMass(float newMass)                     { m_Mass = newMass; }
	void SetFrictionFactor(float newFrictionFactor) { m_FrictionFactor = newFrictionFactor; }
	void SetDamage(u_int newDamage)                 { m_Damage = newDamage; }
	void SetReflectCnt(u_short newReflectCnt)       { m_ReflectCnt = newReflectCnt; }

	bool    GetPossesion()                          { return m_Possesion; }
	Point   GetPosition()                           { return m_Position; }
	Vector  GetVelocity()                           { return m_Velocity; }
	float   GetMass()                               { return m_Mass; }
	float   GetFrictionFactor()                     { return m_FrictionFactor; }
	u_int   GetDamage()                             { return m_Damage; }
	u_short GetReflectCnt()							{ return m_ReflectCnt; }

	void Update(float ElapsedTime);
};