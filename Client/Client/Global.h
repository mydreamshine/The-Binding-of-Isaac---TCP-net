#pragma once
/*
열거형 상수, 매직넘버, 매크로 등은 한 곳에서만 정의하고 관리한다.
*/

// Window Option
#define WND_WIDTH 1280
#define WND_HEIGHT 800


// Object IDs
#define MAX_OBJECTS 1000
#define PLAYER_HEAD_ID1 0 // 0~1 : Player1
#define PLAYER_BODY_ID1 1
#define PLAYER_HEAD_ID2 2 // 2~3 : Player2
#define PLAYER_BODY_ID2 3
#define PLAYER_HEAD_ID3 4 // 4~5 : Player3
#define PLAYER_BODY_ID3 5
#define PLAYER_HEAD_ID4 6 // 6~7 : Player4
#define PLAYER_BODY_ID4 7
#define BOSS_ID         8


// Texture Kind
#define MAX_TEXTURE 6
#define KIND_NULL     -1
#define KIND_HERO_HEAD 0 // Down/Right/Up/Left Sequence Each Two-Frame (In Sprite of One-Row)
#define KIND_HERO_BODY 1 // Up-Down//Left-Right Sequence Each Ten-Frame (In Sprite of Three-Row)
#define KIND_BOSS      2
#define KIND_BULLET_1  3 // Normal Tear
#define KIND_BULLET_2  4 // Blood Tear
#define KIND_BACKGROND 5


// Texture Option
#define HERO_WIDTH    1.0f
#define HERO_HEIGHT   1.0f
#define BULLET_WIDTH  0.3f
#define BULLET_HEIGHT 0.3f


// Phisical Option
#define HERO_SPEED                1.0f
#define HERO_MASS                 0.1f
#define HERO_FRICTION_FACTOR      0.5f
#define HERO_BOUNDINGBOX_WIDTH    
#define HERO_BOUNDINGBOX_HEIGHT   
#define BOSS_SPEED                1.0f
#define BOSS_MASS                 0.1f
#define BOSS_FRICTION_FACTOR      0.5f
#define BOSS_BOUNDINGBOX_WIDTH    
#define BOSS_BOUNDINGBOX_HEIGHT   
#define BULLET_SPEED              10.0f
#define BULLET_MASS               0.01f
#define BULLET_FRICTION_FACTOR    0.01f
#define BULLET_BOUNDINGBOX_WIDTH
#define BULLET_BOUNDINGBOX_HEIGHT 


// Commands
#define SHOOT_LEFT  0
#define SHOOT_RIGHT 1
#define SHOOT_UP    2
#define SHOOT_DOWN  3