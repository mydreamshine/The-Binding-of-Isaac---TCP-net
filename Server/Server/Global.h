#pragma once
/*
열거형 상수, 매직넘버, 매크로 등은 한 곳에서만 정의하고 관리한다.
*/

// Network Option
#define PORT_NUM 9000
#define MAX_CLIENT 4


// Window Option
#define WND_WIDTH 1280
#define WND_HEIGHT 800

// Game Elements
#define MAX_OBJECT 1000
#define PLAYER_INIT_HP 6
#define BOSS_INIT_HP 100
#define PLAYER_BULLET_DAMAGE 1
#define BOSS_BULLET_DAMAGE 1
#define MAX_BULLET_REFLECT_COUNT 3



// Object IDs
#define PLAYER_HEAD_ID1 1 // 0~1 : Player1
#define PLAYER_BODY_ID1 0
#define PLAYER_HEAD_ID2 3 // 2~3 : Player2
#define PLAYER_BODY_ID2 2
#define PLAYER_HEAD_ID3 5 // 4~5 : Player3
#define PLAYER_BODY_ID3 4
#define PLAYER_HEAD_ID4 7 // 6~7 : Player4
#define PLAYER_BODY_ID4 6
#define BOSS_ID         2 * MAX_CLIENT

 
// Object Kind
#define MAX_OBJECT_KIND 6
#define KIND_NULL       -1
#define KIND_PLAYER_HEAD 1 // Down/Right/Up/Left Sequence Each Two-Frame (In Sprite of One-Row)
#define KIND_PLAYER_BODY 0 // Up-Down//Left-Right Sequence Each Ten-Frame (In Sprite of Three-Row)
#define KIND_BOSS        2
#define KIND_BULLET_1    3 // Normal Tear
#define KIND_BULLET_2    4 // Blood Tear
#define KIND_BACKGROND   5


// Render Option
#define RENDER_TRANSLATION_SCALE 100.0f
#define MAX_PLAYER_HEAD_ANIMATION_SEQUENCE_X 8
#define MAX_PLAYER_HEAD_ANIMATION_SEQUENCE_Y 1
#define MAX_PLAYER_BODY_ANIMATION_SEQUENCE_X 10
#define MAX_PLAYER_BODY_ANIMATION_SEQUENCE_Y 3
#define MAX_BOSS_ANIMATION_SEQUENCE_X 9
#define MAX_BOSS_ANIMATION_SEQUENCE_Y 1
#define PLAYER_WIDTH  1.0f
#define PLAYER_HEIGHT 1.0f
#define BULLET_WIDTH  0.3f
#define BULLET_HEIGHT 0.3f
#define BOSS_WIDTH    2.0f
#define BOSS_HEIGHT   2.0f


// Phisical Option
#define GravityAccelarationFactor   9.80665f
#define BACKGROUND_UP_MARGINE       110.0f
#define BACKGROUND_DOWN_MARGINE     110.0f
#define BACKGROUND_LEFT_MARGINE     110.0f
#define BACKGROUND_RIGHT_MARGINE    110.0f
#define PLAYER_SPEED                1.0f
#define PLAYER_MASS                 0.1f
#define PLAYER_FRICTION_FACTOR      0.5f
#define PLAYER_BOUNDINGBOX_WIDTH    1.0f
#define PLAYER_BOUNDINGBOX_HEIGHT   1.0f
#define BOSS_SPEED                  1.0f
#define BOSS_MASS                   0.1f
#define BOSS_FRICTION_FACTOR        0.5f
#define BOSS_BULLET_SPEED			2.0f
#define BOSS_BOUNDINGBOX_WIDTH    
#define BOSS_BOUNDINGBOX_HEIGHT   
#define BULLET_SPEED                10.0f
#define BULLET_MASS                 0.01f
#define BULLET_FRICTION_FACTOR      0.01f
#define BULLET_BOUNDINGBOX_WIDTH
#define BULLET_BOUNDINGBOX_HEIGHT 


// Player Commands
#define SHOOT_NONE -1
#define SHOOT_LEFT  0
#define SHOOT_RIGHT 1
#define SHOOT_UP    2
#define SHOOT_DOWN  3


// Boss Commands
#define MOVE_NONE  -1
#define MOVE_LEFT   0
#define MOVE_RIGHT  1
#define MOVE_UP     2
#define MOVE_DOWN   3
#define SHOOT_PATTERN_NONE -1
#define SHOOT_PATTERN_1     0
#define SHOOT_PATTERN_2     1
#define SHOOT_PATTERN_3     2

//PI
#define PI 3.141592
