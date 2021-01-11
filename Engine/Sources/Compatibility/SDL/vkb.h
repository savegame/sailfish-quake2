#ifndef _KARIN_VKB_H
#define _KARIN_VKB_H

#define HARMATTAN_WIDTH 854
#define HARMATTAN_HEIGHT 480
#define N950_W 854
#define N950_H 480

#define NO_BINDING_KEY -1

// _ - . ? ! /
#define VKB_COUNT (16 + 4 + 1 + (26 + 10 + 5 + 9))// button style // Menu += y n enter ` // console += enter ` // all += escape
#define JOYSTICK_COUNT (1 + 1)// joystick style // menu += arrow, console += arrow
#define SWIPE_COUNT 1 // swipe style // 0
#define CURSOR_COUNT 2 // cursor style // 0
#define TOTAL_VKB_COUNT (VKB_COUNT + SWIPE_COUNT + JOYSTICK_COUNT + CURSOR_COUNT)

#define VKB_TEX_COUNT 5

#define VB_WIDTH 64
#define VB_HEIGHT 64
#define VB_SPACING 10

#define TEX_WIDTH 64
#define TEX_HEIGHT 64
#define TEX_FULL_WIDTH 512
#define TEX_FULL_HEIGHT 512

enum{
	VKB_In_Game = 1,
	VKB_In_Console = 1 << 2,
	VKB_In_Message = 1 << 3,
	VKB_In_Menu = 1 << 4,
};

typedef enum{
	bfalse = 0,
	btrue
} boolean;

enum {
	Position_Coord = 0,
	Texture_Coord,
	Total_Coord
};

typedef unsigned int uint;
typedef float clampf;
typedef unsigned uenum;
typedef int sizei;

typedef enum _vkb_type
{
	vkb_button_type = 0,
	vkb_joystick_type,
	vkb_swipe_type,
	vkb_cursor_type,
	vkb_total_type
} vkb_type;

typedef enum _circle_direction
{
	circle_center = 0,
	circle_top_direction,
	circle_righttop_direction,
	circle_right_direction,
	circle_rightbottom_direction,
	circle_bottom_direction,
	circle_leftbottom_direction,
	circle_left_direction,
	circle_lefttop_direction,
	circle_outside
} circle_direction;

typedef enum _coord_base
{
	opengl_s_base = 0,
	opengl_mf_base,
	opengl_mb_base,
	opengl_e_base
} coord_base;

typedef enum _mouse_motion_button_status
{
	all_out_range_status = 0,
	last_out_now_in_range_status = 1,
	last_in_now_out_range_status = 1 << 1,
	all_in_range_status = last_out_now_in_range_status | last_in_now_out_range_status
} mouse_motion_button_status;

enum{
	Invalid_Data = 0,
	Cmd_Data, // call vkb_AddCommand
	Key_Data, // call Key_Event
	Button_Data, // set mx, my, buttonstate
};

// sort by setting in Q2
typedef enum _Game_Act
{
	Attack_Action = 0,
	NextWeapon_Action,
	PrevWeapon_Action,
	Forward_Action,
	Backward_Action,
	TurnLeft_Action,
	TurnRight_Action,
	Run_Action, // 静步
	MoveLeft_Action,
	MoveRight_Action,
	SideStep_Action, // 固定视角
	TurnUp_Action,
	TurnDown_Action,
	ViewCenter_Action,
	MouseLook_Action, // ???
	KeyboardLook_Action, // ???
	Jump_Action,
	Crouch_Action,
	Invertory_Action, // 清单
	UseItem_Action, //
	DropItem_Action, //
	PrevItem_Action, //
	NextItem_Action, //
	CmdHelp_Action, // 帮助
	Total_Action, // invalid

	Up_Key = 100,
	Down_Key,
	Left_Key,
	Right_Key,
	Escape_Key,
	Console_Key,
	A_Key,
	B_Key,
	C_Key,
	D_Key,
	E_Key,
	F_Key,
	G_Key,
	H_Key,
	I_Key,
	J_Key,
	K_Key,
	L_Key,
	M_Key,
	N_Key,
	O_Key,
	P_Key,
	Q_Key,
	R_Key,
	S_Key,
	T_Key,
	U_Key,
	V_Key,
	W_Key,
	X_Key,
	Y_Key,
	Z_Key,
	Num0_Key,
	Num1_Key,
	Num2_Key,
	Num3_Key,
	Num4_Key,
	Num5_Key,
	Num6_Key,
	Num7_Key,
	Num8_Key,
	Num9_Key,
	Underscore_Key,
	Minus_Key,
	Period_Key,
	Splash_Key,
	Plus_Key,
	Equal_Key,
	Question_Key,
	Exclam_Key,
	Space_Key,
	Shift_Key,
	Backspace_Key,
	Enter_Key,
	PageUp_Key,
	PageDown_Key,
	Tab_Key,
	Mouse1_Button,
	Total_Key, // invalid

	God_Cmd = 200,
	GiveAll_Cmd,
	NoClip_Cmd,
	NoTarget_Cmd,
	InvDrop_Cmd,
	GiveAmmo_Cmd,
	GiveArmor_Cmd,
	GiveHealth_Cmd,
	Kill_Cmd,
	Version_Cmd,
	Total_Cmd, // invalid
} Game_Action;

enum {
	Client_In_Game, 
	Client_In_Console, 
	Client_In_Message, 
	Client_In_Menu, 
	Client_In_Invalid
};

struct vkb_cursor
{
	int x;
	int y;
	uint r;
	clampf eminr;
	clampf emaxr;
	boolean render;
	uint tex_index;
	float ignore_distance;
	int tx;
	int ty;
	int tw;
	float joy_r;
	int joy_tx;
	int joy_ty;
	int joy_tw;
	int joy_ptx;
	int joy_pty;
	int joy_ptw;
	uenum x_base;
	uenum y_base;
	boolean ava;
	int z;
	uint mask;
	Game_Action action;
	Game_Action action0;
	Game_Action action1;
	Game_Action action2;
	Game_Action action3;
};

struct vkb_swipe
{
	int x;
	int y;
	uint w;
	uint h;
	uint ex;
	uint ey;
	uint ew;
	uint eh;
	boolean render;
	int tx;
	int ty;
	int tw;
	int th;
	uenum x_base;
	uenum y_base;
	boolean ava;
	int z;
	uint mask;
	Game_Action action;
	Game_Action action0;
	Game_Action action1;
	Game_Action action2;
	Game_Action action3;
};

struct vkb_joystick
{
	int x;
	int y;
	uint r;
	clampf eminr;
	clampf emaxr;
	int tx;
	int ty;
	int tw;
	float joy_r;
	int joy_tx;
	int joy_ty;
	int joy_tw;
	uenum x_base;
	uenum y_base;
	boolean ava;
	int z;
	uint mask;
	Game_Action action0;
	Game_Action action1;
	Game_Action action2;
	Game_Action action3;
};

struct vkb_button
{
	int x;
	int y;
	uint w;
	uint h;
	uint ex;
	uint ey;
	uint ew;
	uint eh;
	int tx;
	int ty;
	int tw;
	int th;
	int ptx;
	int pty;
	int ptw;
	int pth;
	uenum x_base;
	uenum y_base;
	boolean ava;
	int z;
	uint mask;
	uint tex_index;
	Game_Action action;
};

typedef unsigned (* VKB_Key_Action_Function)(int, unsigned, int, int);
typedef void (* VKB_Add_Command_Function)(char *text);

extern unsigned client_state;
extern void (* vkb_AddCommand) (char *cmd);
extern const char *Tex_Files[VKB_TEX_COUNT];
extern struct vkb_button VKB_Button[VKB_COUNT];
extern struct vkb_joystick VKB_Joystick[JOYSTICK_COUNT];
extern struct vkb_swipe VKB_Swipe[SWIPE_COUNT];
extern struct vkb_cursor VKB_Cursor[CURSOR_COUNT];
extern unsigned VKB_States[Client_In_Invalid];
extern boolean render_lock;

float vkb_FormatAngle(float angle);
void vkb_SetKeyBindinds(char ** kbs);
void vkb_GetClientState(unsigned state);
void vkb_SetAddCommandFunction(void (* f)(char *));
int vkb_GetActionData(unsigned action, int *keys, unsigned int key_max, unsigned int *key_count, char *cmd, unsigned int cmd_max);

#endif
