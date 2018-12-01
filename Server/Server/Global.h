#pragma once
/*
열거형 상수, 매직넘버, 매크로 등은 한 곳에서만 정의하고 관리한다.
*/

// Network Option
#define PORT_NUM 9000
#define MAX_CLIENT 4
#define CPS 15 // Communication Per Second (1초당 통신횟수)


// Frame option
#define FPS 60 // Frame Per Second (1초당 렌더링횟수)


// Window Option
#define WND_WIDTH 1280
#define WND_HEIGHT 800


// Game Elements
#define MAX_OBJECT 200
#define MAX_BULLET MAX_OBJECT - (MAX_CLIENT * 3 + 1) // MAX_OBJET - (MAX_CLIENT + MAX_PRESSURE_PLATE + MAX_BOSS)
#define PLAYER_INIT_HP 6
#define BOSS_INIT_HP 100
#define PLAYER_BULLET_DAMAGE 1
#define BOSS_BULLET_DAMAGE 1
#define MAX_BULLET_REFLECT_COUNT 2
#define PLAYER_HIT_DEALAY 2000 // millisecond
#define PLAYER_BULLET_SHOOT_DEALAY 200 // millisecond



// Object IDs
#define PRESSURE_PLATE_ID1 0 
#define PRESSURE_PLATE_ID2 1 
#define PRESSURE_PLATE_ID3 2 
#define PRESSURE_PLATE_ID4 3 // 0~3 : Pressure_Plate1,2,3,4
#define PLAYER_HEAD_ID1    5 // 4~5 : Player1
#define PLAYER_BODY_ID1    4
#define PLAYER_HEAD_ID2    7 // 6~7 : Player2
#define PLAYER_BODY_ID2    6
#define PLAYER_HEAD_ID3    9 // 8~9 : Player3
#define PLAYER_BODY_ID3    8
#define PLAYER_HEAD_ID4    11 // 10~11 : Player4
#define PLAYER_BODY_ID4    10
#define BOSS_ID            3 * MAX_CLIENT // 12 : Boss


 
// Object Kind
#define MAX_OBJECT_KIND     9
#define KIND_NULL          -1
#define KIND_PLAYER_HEAD    1 // Down/Right/Up/Left Sequence Each Two-Frame (In Sprite of One-Row)
#define KIND_PLAYER_BODY    0 // Up-Down//Left-Right Sequence Each Ten-Frame (In Sprite of Three-Row)
#define KIND_BOSS           2
#define KIND_BULLET_1       3 // Normal Tear
#define KIND_BULLET_2       4 // Blood Tear
#define KIND_BACKGROND      5
#define KIND_PRESSURE_PLATE 6
#define KIND_PLAYER_HEART	7
#define KIND_BOSS_NAME      8


// Render Option
#define RENDER_TRANSLATION_SCALE 100.0f
#define MAX_PLAYER_HEAD_ANIMATION_SEQUENCE_X 8
#define MAX_PLAYER_HEAD_ANIMATION_SEQUENCE_Y 1
#define MAX_PLAYER_BODY_ANIMATION_SEQUENCE_X 10
#define MAX_PLAYER_BODY_ANIMATION_SEQUENCE_Y 3
#define MAX_BOSS_ANIMATION_SEQUENCE_X 9
#define MAX_BOSS_ANIMATION_SEQUENCE_Y 2
#define MAX_PRESSURE_PLATE_ANIMATION_SEQUENCE_X 2
#define MAX_PRESSURE_PLATE_ANIMATION_SEQUENCE_Y 1
#define MAX_PLAYER_HEART_ANIMATION_SEQUENCE_X 4
#define MAX_PLAYER_HEART_ANIMATION_SEQUENCE_Y 4
#define PLAYER_WIDTH  1.5f
#define PLAYER_HEIGHT 1.5f
#define BULLET_WIDTH  0.2f
#define BULLET_HEIGHT 0.2f
#define BOSS_WIDTH    2.0f
#define BOSS_HEIGHT   2.0f
#define PRESSURE_PLATE_WIDTH  1.0f
#define PRESSURE_PLATE_HEIGHT 1.0f
#define UI_PLAYER_HEART_WIDTH  0.5f	
#define UI_PLAYER_HEART_HEIGHT 0.5f
#define UI_PLAYER_HEART_POS_X  -(WND_WIDTH / 2.0f) + 80.f
#define UI_PLAYER_HEART_POS_Y  WND_HEIGHT / 2.0f - 90.f
#define UI_BOSS_HP_RECT_POS_X  0.0f
#define UI_BOSS_HP_RECT_POS_Y  WND_HEIGHT / 2.0f - 70.0f
#define UI_BOSS_HP_RECT_WIDTH  WND_WIDTH / 2.0f
#define UI_BOSS_HP_RECT_HEIGHT WND_HEIGHT / 20.0f


// Phisical Option
#define GravityAccelarationFactor   9.80665f
#define BACKGROUND_UP_MARGINE       110.0f
#define BACKGROUND_DOWN_MARGINE     110.0f
#define BACKGROUND_LEFT_MARGINE     110.0f
#define BACKGROUND_RIGHT_MARGINE    110.0f
#define PLAYER_SPEED                0.5f
#define PLAYER_MASS                 0.1f
#define PLAYER_FRICTION_FACTOR      0.5f
#define PLAYER_BOUNDINGBOX_WIDTH    PLAYER_WIDTH * 0.5f
#define PLAYER_BOUNDINGBOX_HEIGHT   PLAYER_HEIGHT * 0.3f
#define PLAYER_HEAD_OFFSET_X        0.071f
#define PLAYER_HEAD_OFFSET_Y        0.341f
#define PLAYER_FOOTSTEP_DISTANCE    68.0f / RENDER_TRANSLATION_SCALE // 실제 Pixel(17.0f)의 4배(값이 클 수록 Sequence 전환이 느려짐)
#define PLAYER_FOOTSTEP_DIST_ONE_SEQUENCE (PLAYER_FOOTSTEP_DISTANCE) / MAX_PLAYER_BODY_ANIMATION_SEQUENCE_X
#define BOSS_SPEED                  1.0f
#define BOSS_MASS                   0.1f
#define BOSS_FRICTION_FACTOR        0.5f
#define BOSS_BULLET_SPEED			2.0f
#define BOSS_BOUNDINGBOX_WIDTH      (BOSS_WIDTH * 0.5f) * 0.6f
#define BOSS_BOUNDINGBOX_HEIGHT     (BOSS_HEIGHT * 0.5f) * 0.5f
#define BULLET_SPEED                10.0f
#define BULLET_MASS                 0.01f
#define BULLET_FRICTION_FACTOR      0.001f
#define BULLET_BOUNDINGBOX_WIDTH    BULLET_WIDTH
#define BULLET_BOUNDINGBOX_HEIGHT   BULLET_HEIGHT
#define PRESSURE_PLATE_BOUNDINGBOX_WIDTH  PRESSURE_PLATE_WIDTH * 0.2f
#define PRESSURE_PLATE_BOUNDINGBOX_HEIGHT PRESSURE_PLATE_HEIGHT * 0.1f


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
