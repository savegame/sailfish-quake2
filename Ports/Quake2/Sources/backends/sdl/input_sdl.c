#include "backends/input.h"
#include "client/client.h"
#include "client/keyboard.h"
#include "client/refresh/r_private.h"

#include <SDL/SDLWrapper.h>
#ifdef SAILFISH_FBO
#include <SDL/gl_vkb.h>
#include "input_sdl_private.c"
#endif
#include <SDL.h>

#define MOUSE_MAX 3000
#define MOUSE_MIN 40

static cvar_t *input_grab;
cvar_t *input_freelook;
cvar_t *input_lookstrafe;
cvar_t *input_lookspring;

cvar_t *stick_enabled;
cvar_t *stick_pitch_inverted;
cvar_t *stick_sensitivity;
cvar_t *stick_curve;
cvar_t *stick_deadzone;

cvar_t *mouse_windowed;
cvar_t *mouse_pitch_inverted;
cvar_t *mouse_filter;
cvar_t *mouse_exponential_sensitivity;
cvar_t *mouse_linear_sensitivity;
cvar_t *mouse_speed_forward;
cvar_t *mouse_speed_side;
cvar_t *mouse_speed_pitch;
cvar_t *mouse_speed_yaw;

static int l_mouseX, l_mouseY;
static int l_mouseOldX, l_mouseOldY;
static SDL_Joystick *l_joystick = NULL;
static SDL_GameController *l_controller = NULL;

#ifdef SAILFISHOS
struct _TouchFinger {
	SDL_TouchID  touch_id;
	SDL_FingerID finger_id;
	float press_x;
	float press_y;
	float x;
	float y;
	float dx;
	float dy;
	Uint32 timestamp;
	bool pressed;
	bool wait_double_tap;
};
typedef struct _TouchFinger TouchFinger;

struct _ScreenRect {
	int x, y;
	int w, h;
};
typedef struct _ScreenRect ScreenRect;

#define MAX_FINGER 5
#define DOUBLE_TAP_TIME 250
#define DOUBLE_TAP_SIZE 50.0f

static TouchFinger fingers[5] = {
	{0,0,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,false},
	{0,0,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,false},
	{0,0,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,false},
	{0,0,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,false},
	{0,0,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,false}
};

static ScreenRect sr_mouse_look = {
	0,0,0,0
};

static ScreenRect sr_joystick = {
	0,0,0,0
};

bool is_PointInRect( float x, float y, ScreenRect *sr) {
	return sr->x <= (int)x && sr->y <= (int)y && sr->x + sr->w >= (int)x && sr->y + sr->h >= (int)y;
}
// transform delta to FBO orientaton
void transformDelta( float *dx, float *dy ) {
	float tmp;
	switch( sdlwCurrentOrientation() ) {
	case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
		tmp = *dy;
		*dy = *dx;
		*dx = -tmp;
		break;
	default:
	case SDL_ORIENTATION_LANDSCAPE:
		tmp = *dy;
		*dy = -*dx;
		*dx = tmp;
		break;
	// do not use portrait orientations
	}
}
// transform touch point to FBO orientation 
void transformTouch( float *x, float *y ) {
	int w,h;
	float tmp;
	sdlwGetWindowSize(&w, &h);
	switch( sdlwCurrentOrientation() ) {
	case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
		tmp = *y;
		*y = *x;
		*x = w - tmp;
		break;
	default:
	case SDL_ORIENTATION_LANDSCAPE:
		tmp = *y;
		*y = h - *x;
		*x = tmp;
		break;
	// do not use portrait orientations
	}

	if(sr_mouse_look.w != 0)
		return;
	int half_width = w / 2;
	sr_mouse_look.x = half_width + 1;
	sr_mouse_look.y = 0;
	sr_mouse_look.w = half_width;
	sr_mouse_look.h = h;

	sr_joystick.x = 0;
	sr_joystick.y = 0;
	sr_joystick.w = half_width;
	sr_joystick.h = h;
}
#endif

// convert joy sholder to mouse button
static bool joy_shoulder_to_mouse_button[2] = { false, false };

static int IN_TranslateSDLtoQ2Key(Sint32 keysym)
{
	int key;

	switch (keysym)
	{
	default:
        key = (keysym >= SDLK_SPACE && keysym < SDLK_DELETE) ? keysym : 0;
		break;

		#if defined(__GCW_ZERO__)

	case SDLK_RETURN:
		key = K_GAMEPAD_START;
		break;
	case SDLK_ESCAPE:
		key = K_GAMEPAD_SELECT;
		break;

	case SDLK_LCTRL:
		key = K_GAMEPAD_A;
		break;
	case SDLK_LALT:
		key = K_GAMEPAD_B;
		break;
	case SDLK_LSHIFT:
		key = K_GAMEPAD_X;
		break;
	case SDLK_SPACE:
		key = K_GAMEPAD_Y;
		break;

	case SDLK_TAB:
		key = K_GAMEPAD_L;
		break;
	case SDLK_BACKSPACE:
		key = K_GAMEPAD_R;
		break;

	case SDLK_LEFT:
		key = K_GAMEPAD_LEFT;
		break;
	case SDLK_RIGHT:
		key = K_GAMEPAD_RIGHT;
		break;
	case SDLK_DOWN:
		key = K_GAMEPAD_DOWN;
		break;
	case SDLK_UP:
		key = K_GAMEPAD_UP;
		break;

	case SDLK_PAUSE:
		key = K_GAMEPAD_LOCK;
		break;
	case SDLK_HOME:
		key = K_GAMEPAD_POWER;
		break;

		#else

	case SDLK_PAGEUP:
		key = K_PGUP;
		break;
	case SDLK_KP_9:
		key = K_KP_PGUP;
		break;
	case SDLK_PAGEDOWN:
		key = K_PGDN;
		break;
	case SDLK_KP_3:
		key = K_KP_PGDN;
		break;
	case SDLK_KP_7:
		key = K_KP_HOME;
		break;
	case SDLK_HOME:
		key = K_HOME;
		break;
	case SDLK_KP_1:
		key = K_KP_END;
		break;
	case SDLK_END:
		key = K_END;
		break;
	case SDLK_KP_4:
		key = K_KP_LEFTARROW;
		break;
	case SDLK_LEFT:
		key = K_LEFTARROW;
		break;
	case SDLK_KP_6:
		key = K_KP_RIGHTARROW;
		break;
	case SDLK_RIGHT:
		key = K_RIGHTARROW;
		break;
	case SDLK_KP_2:
		key = K_KP_DOWNARROW;
		break;
	case SDLK_DOWN:
		key = K_DOWNARROW;
		break;
	case SDLK_KP_8:
		key = K_KP_UPARROW;
		break;
	case SDLK_UP:
		key = K_UPARROW;
		break;
	case SDLK_ESCAPE:
		key = K_ESCAPE;
		break;
	case SDLK_KP_ENTER:
		key = K_KP_ENTER;
		break;
	case SDLK_RETURN:
		key = K_ENTER;
		break;
	case SDLK_TAB:
		key = K_TAB;
		break;
	case SDLK_F1:
		key = K_F1;
		break;
	case SDLK_F2:
		key = K_F2;
		break;
	case SDLK_F3:
		key = K_F3;
		break;
	case SDLK_F4:
		key = K_F4;
		break;
	case SDLK_F5:
		key = K_F5;
		break;
	case SDLK_F6:
		key = K_F6;
		break;
	case SDLK_F7:
		key = K_F7;
		break;
	case SDLK_F8:
		key = K_F8;
		break;
	case SDLK_F9:
		key = K_F9;
		break;
	case SDLK_F10:
		key = K_F10;
		break;
	case SDLK_F11:
		key = K_F11;
		break;
	case SDLK_F12:
		key = K_F12;
		break;
	case SDLK_F13:
		key = K_F13;
		break;
	case SDLK_F14:
		key = K_F14;
		break;
	case SDLK_F15:
		key = K_F15;
		break;
	case SDLK_BACKSPACE:
		key = K_BACKSPACE;
		break;
	case SDLK_KP_PERIOD:
		key = K_KP_DEL;
		break;
	case SDLK_DELETE:
		key = K_DEL;
		break;
	case SDLK_PAUSE:
		key = K_PAUSE;
		break;
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		key = K_SHIFT;
		break;
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		key = K_CTRL;
		break;
	case SDLK_RGUI:
	case SDLK_LGUI:
		key = K_COMMAND;
		break;
	case SDLK_RALT:
	case SDLK_LALT:
		key = K_ALT;
		break;
	case SDLK_KP_5:
		key = K_KP_5;
		break;
	case SDLK_INSERT:
		key = K_INS;
		break;
	case SDLK_KP_0:
		key = K_KP_INS;
		break;
	case SDLK_KP_MULTIPLY:
		key = K_KP_STAR;
		break;
	case SDLK_KP_PLUS:
		key = K_KP_PLUS;
		break;
	case SDLK_KP_MINUS:
		key = K_KP_MINUS;
		break;
	case SDLK_KP_DIVIDE:
		key = K_KP_SLASH;
		break;
	case SDLK_MODE:
		key = K_MODE;
		break;
	case SDLK_APPLICATION:
		key = K_COMPOSE;
		break;
	case SDLK_HELP:
		key = K_HELP;
		break;
	case SDLK_PRINTSCREEN:
		key = K_PRINT;
		break;
	case SDLK_SYSREQ:
		key = K_SYSREQ;
		break;
	case SDLK_MENU:
		key = K_MENU;
		break;
	case SDLK_POWER:
		key = K_POWER;
		break;
	case SDLK_UNDO:
		key = K_UNDO;
		break;
	case SDLK_SCROLLLOCK:
		key = K_SCROLLOCK;
		break;
	case SDLK_NUMLOCKCLEAR:
		key = K_KP_NUMLOCK;
		break;
	case SDLK_CAPSLOCK:
		key = K_CAPSLOCK;
		break;

		#endif
	}

	return key;
}

bool IN_processEvent(SDL_Event *event)
{
	switch (event->type)
	{
	case SDL_QUIT:
		printf("Exit requested by the system.");
		sdlwRequestExit(true);
		break;

	case SDL_WINDOWEVENT:
		switch (event->window.event)
		{
		case SDL_WINDOWEVENT_CLOSE:
			printf("Exit requested by the user (by closing the window).");
			sdlwRequestExit(true);
			break;
		case SDL_WINDOWEVENT_RESIZED:
//        case SDL_WINDOWEVENT_SIZE_CHANGED:
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			cl_paused->value = 1;
			Key_MarkAllUp();
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			cl_paused->value = 0;
			break;
		}
		break;
#if defined(SAILFISHOS) && defined(SAILFISH_FBO)
    case SDL_DISPLAYEVENT:
        if( event->display.event == SDL_DISPLAYEVENT_ORIENTATION ) {
            /*switch (event->display.data1) {
                case SDL_ORIENTATION_LANDSCAPE:
                    printf("SDL_DisplayOrientation is SDL_ORIENTATION_LANDSCAPE\n");
                    break;
                case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
                    printf("SDL_DisplayOrientation is SDL_ORIENTATION_LANDSCAPE_FLIPPED\n");
                    break;
                case SDL_ORIENTATION_PORTRAIT:
                    printf("SDL_DisplayOrientation is SDL_ORIENTATION_PORTRAIT\n");
                    break;
                case SDL_ORIENTATION_PORTRAIT_FLIPPED:
                    printf("SDL_DisplayOrientation is SDL_ORIENTATION_PORTRAIT_FLIPPED\n");
                    break;
                case SDL_ORIENTATION_UNKNOWN:
                    printf("SDL_DisplayOrientation is SDL_ORIENTATION_UNKNOWN\n");
                    break;
            }*/
			// skip non landscape orinetations 
			if( event->display.data1 == SDL_ORIENTATION_LANDSCAPE 
			    || event->display.data1 == SDL_ORIENTATION_LANDSCAPE_FLIPPED ) 
			{
				sdlwSetRealOrientation((SDL_DisplayOrientation)event->display.data1);
				#ifdef SAILFISH_FBO
				if( (int)(r_rotaterender->value) == 1 ) {
					if( event->display.data1 == SDL_ORIENTATION_LANDSCAPE )
            			sdlwSetOrientation(SDL_ORIENTATION_LANDSCAPE_FLIPPED);
					else
						sdlwSetOrientation(SDL_ORIENTATION_LANDSCAPE);
				}
				else
				#endif
					sdlwSetOrientation((SDL_DisplayOrientation)event->display.data1);
			}
        }
        break;
#endif
	case SDL_MOUSEWHEEL:
		Key_Event((event->wheel.y > 0 ? K_MWHEELUP : K_MWHEELDOWN), true);
		Key_Event((event->wheel.y > 0 ? K_MWHEELUP : K_MWHEELDOWN), false);
		break;
#ifndef SAILFISHOS
	case SDL_MOUSEBUTTONDOWN:
	// fall-through
	case SDL_MOUSEBUTTONUP:
        {
            int key = -1;
            switch (event->button.button)
            {
            default:
                break;
            case SDL_BUTTON_LEFT:
                key = K_MOUSE1;
                break;
            case SDL_BUTTON_MIDDLE:
                key = K_MOUSE3;
                break;
            case SDL_BUTTON_RIGHT:
                key = K_MOUSE2;
                break;
            case SDL_BUTTON_X1:
                key = K_MOUSE4;
                break;
            case SDL_BUTTON_X2:
                key = K_MOUSE5;
                break;
            }
            if (key >= 0) {
                Key_Event(key, (event->type == SDL_MOUSEBUTTONDOWN));
#ifdef SAILFISH_FBO
				vkb_GLVKBMouseEvent(key, (event->type == SDL_MOUSEBUTTONDOWN) ? btrue : bfalse, event->button.x * sdlwGetFboScale(), event->button.y * sdlwGetFboScale(),  vkb_HandleVKBAction);
#endif
			}
        }
		break;
	case SDL_MOUSEMOTION:
		if (cls.key_dest == key_game && !cl_paused->value)
		{
			l_mouseX += event->motion.xrel;
			l_mouseY += event->motion.yrel;
		}
		break;
#endif
#ifdef SAILFISHOS
	case SDL_FINGERDOWN:
		{
			//int touch_count = 0;
			transformTouch(&event->tfinger.x, &event->tfinger.y);
			for(int i = 0; i < MAX_FINGER; i++) {
				if( fingers[i].finger_id == 0 ){
					fingers[i].press_x = event->tfinger.x;
					fingers[i].press_y = event->tfinger.y;
					fingers[i].x = event->tfinger.x;
					fingers[i].y = event->tfinger.y;
					fingers[i].dx = event->tfinger.dx;
					fingers[i].dy = event->tfinger.dy;
					fingers[i].touch_id = event->tfinger.touchId;
					fingers[i].finger_id = event->tfinger.fingerId;
					fingers[i].timestamp = event->tfinger.timestamp;
					fingers[i].pressed = true;
					fingers[i].wait_double_tap = false; 
					#ifdef SAILFISH_FBO
					float scale = sdlwGetFboScale();
					vkb_GLVKBMouseEvent(K_MOUSE1, btrue,event->tfinger.x * scale, event->tfinger.y * scale,  vkb_HandleVKBAction);
					#endif
					break;
				}
			}
		}
		break;
	case SDL_FINGERUP:
		{
			int touch_count = 0;
			transformTouch(&event->tfinger.x, &event->tfinger.y);

			for(int i = 0; i < MAX_FINGER; i++) {
				if( fingers[i].finger_id == event->tfinger.fingerId ){
					// if( is_PointInRect(fingers[i].press_x, fingers[i].press_y, &sr_joystick) ) {
					// }
					#ifdef SAILFISH_FBO
					float scale = sdlwGetFboScale();
					vkb_GLVKBMouseEvent(K_MOUSE1, bfalse, event->tfinger.x * scale, event->tfinger.y * scale,  vkb_HandleVKBAction);
					#endif
					fingers[i].x = 0;
					fingers[i].y = 0;
					fingers[i].press_x = 0;
					fingers[i].press_y = 0;
					fingers[i].dx = 0;
					fingers[i].dy = 0;
					fingers[i].touch_id = 0;
					fingers[i].finger_id = 0;
					fingers[i].timestamp = 0;
					fingers[i].pressed = false;
					fingers[i].wait_double_tap = false; 
				}
			}
        }
		break;
	case SDL_FINGERMOTION:
		transformTouch(&event->tfinger.x, &event->tfinger.y);
		transformDelta(&event->tfinger.dx, &event->tfinger.dy);
		if (cls.key_dest == key_game && !cl_paused->value)
		{
			for(int i = 0; i < MAX_FINGER; i++ ) {
				if( fingers[i].pressed && event->tfinger.fingerId == fingers[i].finger_id ) {
					fingers[i].x = event->tfinger.x;
					fingers[i].y = event->tfinger.y;
					if ( is_PointInRect(fingers[i].press_x, fingers[i].press_y, &sr_mouse_look) ) {
						l_mouseX += event->tfinger.dx * 2.0;
						l_mouseY += event->tfinger.dy * 2.0;
					}
					break;
				}
			}
		}
		break;
#endif // SAILFISHOS
#if !defined(__GCW_ZERO__)
	case SDL_TEXTINPUT:
		Char_Event(event->text.text[0], false);
		break;
#endif

	case SDL_KEYDOWN:
	case SDL_KEYUP:
	{
        int key = IN_TranslateSDLtoQ2Key(event->key.keysym.sym);
        if (key > 0)
        {
            bool down = (event->type == SDL_KEYDOWN);
            Key_Event(key, down);
        }
	}
	break;

	case SDL_JOYAXISMOTION:
	{
		float avalue = event->jaxis.value / 32767.0f;
		/* axis 
			4 - left sholder
			5 - right sholder
		*/
		int key = -1;
		bool down = avalue >= 0.25;
		char cmd[1024];
		// joy_shoulder_to_mouse_button
		switch( event->jaxis.axis ) {
			case 4: 
				// key = K_GAMEPAD_L; 
				if( joy_shoulder_to_mouse_button[1] == down )
					break;
				// key = K_MOUSE2;
				joy_shoulder_to_mouse_button[1] = down;
				if( down ) {
					Com_sprintf (cmd, sizeof(cmd), "+%s %i %i\n", "speed", K_LAST, Sys_Milliseconds());
				} else {
					Com_sprintf (cmd, sizeof(cmd), "-%s %i %i\n", "speed", K_LAST, Sys_Milliseconds());
				}
				vkb_AddCommand(cmd);
				break;
			case 5: 
				// key = K_GAMEPAD_R; 
				if( joy_shoulder_to_mouse_button[0] == down )
					break;
				key = K_MOUSE1;
				joy_shoulder_to_mouse_button[0] = down;
				break;
		}
		if( key != -1 )
			Key_Event(key, down);
		break;
	}
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
	{
		/*
			11 - up
			12 - down
			13 - left
			14 - right

			key axis
			3 - up/triangle	
			0 - down/cross
			2 - left/square
			1 - right/circle

			9 - left trigger
			10 - right trigger

			7 - left joy toggle
			8 - right joy toggle

			4 - select (share on DS4)
			6 - start (option) 				
		*/
		bool down = (event->type == SDL_JOYBUTTONDOWN);
		int key = K_JOY1 + event->jbutton.button;
		// int vkb_state = vkb_GetClientState();
		// int in_game = vkb_state & VKB_In_Game;
		char cmd[1024];
		cmd[0] = '\0';

		switch( event->jbutton.button ) {
			case 11: // UP
				if( vkb_GetClientState() == Client_In_Game ) {
					if( down )
						Com_sprintf (cmd, sizeof(cmd), "%s %i %i\n", "invdrop", K_LAST, Sys_Milliseconds());
				} else
					key = K_GAMEPAD_UP; 
				break;
			case 12: // DOWN
				if( vkb_GetClientState() == Client_In_Game ) {
					if( down )
						Com_sprintf (cmd, sizeof(cmd), "%s %i %i\n", "invuse", K_LAST, Sys_Milliseconds());
				} else
					key = K_GAMEPAD_DOWN; 
				break;
			case 13: // LEFT
				if( vkb_GetClientState() == Client_In_Game ) {
					if( down )
						Com_sprintf (cmd, sizeof(cmd), "%s %i %i\n", "invprev", K_LAST, Sys_Milliseconds());
				} else 
					key = K_GAMEPAD_LEFT;
				break;
			case 14: // RIGHT
				if( vkb_GetClientState() == Client_In_Game ) {
					if( down )
						Com_sprintf (cmd, sizeof(cmd), "%s %i %i\n", "invnext", K_LAST, Sys_Milliseconds());
				} else 
					key = K_GAMEPAD_RIGHT;
				break;
			case 6: {// start
				key = K_GAMEPAD_SELECT;
				// if( down )
					// Com_sprintf (cmd, sizeof(cmd), "%s %i %i\n", "invnext", K_LAST, Sys_Milliseconds());
				break;
			}
			case 4: {// select
 				// key = K_GAMEPAD_START; 
				if( down )
					Com_sprintf (cmd, sizeof(cmd), "%s %i %i\n", "cmd help", K_LAST, Sys_Milliseconds());
				break;
			}
			case 0: { // cross
				if( vkb_GetClientState() == Client_In_Game ) {
					if( down ) {
						Com_sprintf (cmd, sizeof(cmd), "+%s %i %i\n", "moveup", K_LAST, Sys_Milliseconds());
					} else {
						Com_sprintf (cmd, sizeof(cmd), "-%s %i %i\n", "moveup", K_LAST, Sys_Milliseconds());
					}
				}
				break;
			}
			case 2: // square
			case 8: { // right stick press
				if( down ) {
					Com_sprintf (cmd, sizeof(cmd), "+%s %i %i\n", "movedown", K_LAST, Sys_Milliseconds());
				} else {
					Com_sprintf (cmd, sizeof(cmd), "-%s %i %i\n", "movedown", K_LAST, Sys_Milliseconds());
				}
				break;
			}
			case 9: {
				if( down )
					Com_sprintf (cmd, sizeof(cmd), "%s %i %i\n", "weapprev", K_LAST, Sys_Milliseconds());
				break;
			}
			case 10: {
				if( down )
					Com_sprintf (cmd, sizeof(cmd), "%s %i %i\n", "weapnext", K_LAST, Sys_Milliseconds());
				break;
			}
		}
		if( cmd[0] == '\0' )
			Key_Event(key, down);
		else
			vkb_AddCommand(cmd);
	}
	break;
	case SDL_JOYHATMOTION:
	{
		int v = event->jhat.value;
		bool left = (v == SDL_HAT_LEFTDOWN || v == SDL_HAT_LEFT || v == SDL_HAT_LEFTUP);
		bool right = (v == SDL_HAT_RIGHTDOWN || v == SDL_HAT_RIGHT || v == SDL_HAT_RIGHTUP);
		bool down = (v == SDL_HAT_LEFTDOWN || v == SDL_HAT_DOWN || v == SDL_HAT_RIGHTDOWN);
		bool up = (v == SDL_HAT_LEFTUP || v == SDL_HAT_UP || v == SDL_HAT_RIGHTUP);
		Key_Event(K_GAMEPAD_LEFT, left);
		Key_Event(K_GAMEPAD_RIGHT, right);
		Key_Event(K_GAMEPAD_DOWN, down);
		Key_Event(K_GAMEPAD_UP, up);
	}
	break;

	case SDL_CONTROLLERAXISMOTION:
		break;
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
	{
		bool down = (event->type == SDL_CONTROLLERBUTTONDOWN);
		int key;
		switch (event->cbutton.button)
		{
		default: key = -1; break;
		case SDL_CONTROLLER_BUTTON_A: key = K_GAMEPAD_A; break;
		case SDL_CONTROLLER_BUTTON_B: key = K_GAMEPAD_B; break;
		case SDL_CONTROLLER_BUTTON_X: key = K_GAMEPAD_X; break;
		case SDL_CONTROLLER_BUTTON_Y: key = K_GAMEPAD_Y; break;
		//case SDL_CONTROLLER_BUTTON_BACK: key = K_GAMEPAD_; break;
		case SDL_CONTROLLER_BUTTON_GUIDE: key = K_GAMEPAD_SELECT; break;
		case SDL_CONTROLLER_BUTTON_START: key = K_GAMEPAD_START; break;
		//case SDL_CONTROLLER_BUTTON_LEFTSTICK: key = K_GAMEPAD_; break;
		//case SDL_CONTROLLER_BUTTON_RIGHTSTICK: key = K_GAMEPAD_; break;
		case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: key = K_GAMEPAD_L; break;
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: key = K_GAMEPAD_R; break;
		case SDL_CONTROLLER_BUTTON_DPAD_UP: key = K_GAMEPAD_UP; break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN: key = K_GAMEPAD_DOWN; break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT: key = K_GAMEPAD_LEFT; break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: key = K_GAMEPAD_RIGHT; break;
		}
		if (key >= 0)
			Key_Event(key, down);
	}
	break;
	}
	return true;
}

/*
 * Updates the input queue state. Called every
 * frame by the client and does nearly all the
 * input magic.
 */
void IN_Update()
{
	sdlwCheckEvents();

	// Grab and ungrab the mouse if the  console or the menu is opened.
	bool want_grab = (r_fullscreen->value || input_grab->value == 1 || (input_grab->value == 2 && mouse_windowed->value));
	In_Grab(want_grab);
}

static float ComputeStickValue(float stickValue)
{
	float deadzone = stick_deadzone->value * (1.0f / 2.0f);
	if (stickValue < 0.0f)
	{
		if (stickValue > -deadzone)
			stickValue = 0.0f;
		else
			stickValue = (stickValue + deadzone) / (1.0f - deadzone); // Normalize.
	}
	else
	{
		if (stickValue < deadzone)
			stickValue = 0.0f;
		else
			stickValue = (stickValue - deadzone) / (1.0f - deadzone); // Normalize.
	}

	// for stick_curve, 0.5 gives quick response, 1.0 is linear, 2.0 gives slow responses.
	if (stickValue < 0.0f)
		stickValue = -powf(-stickValue, stick_curve->value);
	else
		stickValue = +powf(+stickValue, stick_curve->value);

	return stickValue * stick_sensitivity->value;
}

/*
 * Move handling
 */
void IN_Move(usercmd_t *cmd)
{
    int mouseX = l_mouseX, mouseY = l_mouseY;
	l_mouseX = l_mouseY = 0;

	if (mouse_filter->value)
	{
		if ((mouseX > 1) || (mouseX < -1))
			mouseX = (mouseX + l_mouseOldX) * 0.5f;

		if ((mouseY > 1) || (mouseY < -1))
			mouseY = (mouseY + l_mouseOldY) * 0.5f;
	}
	l_mouseOldX = mouseX;
	l_mouseOldY = mouseY;

    float exponential_sensitivity = mouse_exponential_sensitivity->value;
    if (!exponential_sensitivity)
    {
        float linear_sensitivity = mouse_linear_sensitivity->value;
        mouseX *= linear_sensitivity;
        mouseY *= linear_sensitivity;
    }
    else
    {
        if (mouseX > MOUSE_MIN || mouseY > MOUSE_MIN || mouseX < -MOUSE_MIN || mouseY < -MOUSE_MIN)
        {
            mouseX = exponential_sensitivity * (mouseX * mouseX * mouseX) / 4;
            mouseY = exponential_sensitivity * (mouseY * mouseY * mouseY) / 4;

            if (mouseX > MOUSE_MAX)
                mouseX = MOUSE_MAX;
            else if (mouseX < -MOUSE_MAX)
                mouseX = -MOUSE_MAX;

            if (mouseY > MOUSE_MAX)
                mouseY = MOUSE_MAX;
            else if (mouseY < -MOUSE_MAX)
                mouseY = -MOUSE_MAX;
        }
    }

    float speed;
    if (in_speed.state & 1)
        speed = cls.frametime * cl_anglespeedkey->value;
    else
        speed = cls.frametime;

    float running;
    if ((in_speed.state & 1) ^ (int)(cl_run->value))
        running = 2.0f;
    else
        running = 1.0f;

    float joyXFloat = 0.0f, joyYFloat = 0.0f;

    if (l_joystick != NULL && stick_enabled->value)
    {
        float joyX, joyY;
        joyX = (float)SDL_JoystickGetAxis(l_joystick, 0) / 32768.0f;
        joyY = (float)SDL_JoystickGetAxis(l_joystick, 1) / 32768.0f;
        joyXFloat = ComputeStickValue(joyX);
        joyYFloat = ComputeStickValue(joyY);
    }

    if (l_controller != NULL)
    {
        float joyX, joyY;
		// mouselook
        joyX = (float)SDL_GameControllerGetAxis(l_controller, SDL_CONTROLLER_AXIS_RIGHTX) / 32768.0f;;
        joyY = (float)SDL_GameControllerGetAxis(l_controller, SDL_CONTROLLER_AXIS_RIGHTY) / 32768.0f;;
        joyXFloat += ComputeStickValue(joyX);
        joyYFloat += ComputeStickValue(joyY);
		// movement
        joyX = (float)SDL_GameControllerGetAxis(l_controller, SDL_CONTROLLER_AXIS_LEFTX) / 32768.0f;;
        joyY = (float)SDL_GameControllerGetAxis(l_controller, SDL_CONTROLLER_AXIS_LEFTY) / -32768.0f;;
        joyX = ComputeStickValue(joyX);
        joyY = ComputeStickValue(joyY);
        cmd->sidemove += cl_speed_side->value * joyX * running;
        cmd->forwardmove += cl_speed_forward->value * joyY * running;
    }

#ifdef SAILFISHOS
	for(int i = 0; i < MAX_FINGER; i++ ) {
		if( fingers[i].pressed ) {
			if ( is_PointInRect(fingers[i].press_x, fingers[i].press_y, &sr_joystick) ) {
				// here we should compute it as joystick
				float joyX, joyY;
				int w,h;
				sdlwGetWindowSize(&w, &h);
				float joySize = (float)h * 0.15;
				joyX = (float)(fingers[i].x - fingers[i].press_x);
				joyY = (float)(fingers[i].y - fingers[i].press_y);
				float angle = atan2f(joyX, joyY);
				float vec_len = sqrtf(joyX*joyX + joyY*joyY)/joySize;
				if( vec_len > 1.0 ) 
					vec_len = 1.0;
				joyX = sin(angle) * vec_len;
				joyY = cos(angle) * vec_len;
				// joyXFloat = ComputeStickValue(joyX);
				// joyYFloat = ComputeStickValue(joyY);

				cmd->sidemove    += cl_speed_side->value    *  joyX * running;
        		cmd->forwardmove += cl_speed_forward->value * -joyY * running;
				break;
			}
		}
	}
#else
#endif
    /* add mouse X/Y movement to cmd */
    if ((in_strafe.state & 1) || (input_lookstrafe->value && (in_mlook.state & 1)))
    {
        float sideDelta;
        sideDelta  = mouse_speed_side->value * mouseX;
        sideDelta += cl_speed_side->value * joyXFloat * running;
        cmd->sidemove += sideDelta;
    }
    else
    {
        float yawDelta;
        yawDelta  = mouse_speed_yaw->value * mouseX;
        yawDelta += speed * cl_speed_yaw->value * joyXFloat;
        cl.viewangles[YAW] -= yawDelta;
    }

    if (((in_mlook.state & 1) || input_freelook->value) && !(in_strafe.state & 1))
    {
        float pitchDeltaMouse = mouse_speed_pitch->value * mouseY;
        if (mouse_pitch_inverted->value)
            pitchDeltaMouse = -pitchDeltaMouse;
        float pitchDeltaStick = speed * cl_speed_pitch->value * joyYFloat;
        if (stick_pitch_inverted->value)
            pitchDeltaStick = -pitchDeltaStick;
        cl.viewangles[PITCH] += pitchDeltaMouse + pitchDeltaStick;
    }
    else
    {
        float forwardDelta;
        forwardDelta  = -mouse_speed_forward->value * mouseY;
        forwardDelta += cl_speed_forward->value * joyYFloat * running;
        cmd->forwardmove += forwardDelta;
    }
}

void IN_Init()
{
	Com_Printf("------- input initialization -------\n");

	l_mouseX = l_mouseY = 0;

	input_grab = Cvar_Get("input_grab", "2", CVAR_ARCHIVE);
	input_freelook = Cvar_Get("input_freelook", "1", CVAR_ARCHIVE);
	input_lookstrafe = Cvar_Get("input_lookstrafe", "0", CVAR_ARCHIVE);
	input_lookspring = Cvar_Get("input_lookspring", "0", CVAR_ARCHIVE);
    
	mouse_windowed = Cvar_Get("mouse_windowed", GL_WINDOWED_MOUSE_DEFAULT_STRING, CVAR_USERINFO | CVAR_ARCHIVE);
	mouse_pitch_inverted = Cvar_Get("mouse_pitch_inverted", "0", CVAR_ARCHIVE);
	mouse_filter = Cvar_Get("mouse_filter", "0", CVAR_ARCHIVE);
	mouse_exponential_sensitivity = Cvar_Get("mouse_exponential_sensitivity", "0", CVAR_ARCHIVE);
	mouse_linear_sensitivity = Cvar_Get("mouse_linear_sensitivity", "3", CVAR_ARCHIVE);
	mouse_speed_forward = Cvar_Get("mouse_speed_forward", "1", CVAR_ARCHIVE);
	mouse_speed_pitch = Cvar_Get("mouse_speed_pitch", "0.022", CVAR_ARCHIVE);
	mouse_speed_side = Cvar_Get("mouse_speed_side", "0.8", CVAR_ARCHIVE);
	mouse_speed_yaw = Cvar_Get("mouse_speed_yaw", "0.022", CVAR_ARCHIVE);
    
	#if defined(__unix__) && !defined(__GCW_ZERO__)
	// There is some issues with the l_joystick under Linux. So disable it by default (it works for GCW Zero and it is needed for this platform).
	stick_enabled = Cvar_Get("stick_enabled", "0", CVAR_ARCHIVE);
	#else
	stick_enabled = Cvar_Get("stick_enabled", "1", CVAR_ARCHIVE);
	#endif
	stick_pitch_inverted = Cvar_Get("stick_pitch_inverted", "0", CVAR_ARCHIVE);
	stick_sensitivity = Cvar_Get("stick_sensitivity", "1", CVAR_ARCHIVE);
	stick_curve = Cvar_Get("stick_curve", "1", CVAR_ARCHIVE);
	stick_deadzone = Cvar_Get("stick_deadzone", "0.2", CVAR_ARCHIVE);

	r_fullscreen = Cvar_Get("r_fullscreen", GL_FULLSCREEN_DEFAULT_STRING, CVAR_ARCHIVE);
	#ifdef SAILFISH_FBO
	r_rotaterender = Cvar_Get("r_rotaterender", "0", CVAR_ARCHIVE);
	r_sizerender = Cvar_Get("r_sizerender", "0.5", CVAR_ARCHIVE);
	#endif

	SDL_StartTextInput();

	if (!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		if (SDL_Init(SDL_INIT_JOYSTICK) == -1)
		{
			R_printf(PRINT_ALL, "Couldn't init SDL l_joystick: %s.\n", SDL_GetError());
		}
		else
		{
			if (SDL_NumJoysticks() > 0)
			{
				int n = SDL_NumJoysticks();
				for (int i = 0; i < n; i++)
				{
					if (SDL_IsGameController(i))
					{
						if (!l_controller)
						{
							l_controller = SDL_GameControllerOpen(i);
							if (!l_controller)
							{
								R_printf(PRINT_ALL, "Could not open gamecontroller %i: %s\n", i, SDL_GetError());
							}
						}
					}
					else
					{
						if (!l_joystick)
						{
							l_joystick = SDL_JoystickOpen(i);
							if (!l_joystick)
							{
								R_printf(PRINT_ALL, "Could not open l_joystick %i: %s\n", i, SDL_GetError());
							}
						}
					}
				}
			}
		}
	}

	Com_Printf("------------------------------------\n\n");
}

void IN_Shutdown()
{
	Cmd_RemoveCommand("force_centerview");
	Cmd_RemoveCommand("+mlook");
	Cmd_RemoveCommand("-mlook");

	if (SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		if (SDL_JoystickGetAttached(l_joystick))
		{
			SDL_JoystickClose(l_joystick);
			l_joystick = NULL;
		}
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}

	Com_Printf("Shutting down input.\n");
}

void In_Grab(bool grab)
{
	if (sdlwContext->window != NULL)
	{
		SDL_SetWindowGrab(sdlwContext->window, grab ? SDL_TRUE : SDL_FALSE);
	}
	if (SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE) < 0)
	{
		R_printf(PRINT_ALL, "Setting relative mouse mode failed, reason: %s\nYou should probably update to SDL 2.0.3 or newer\n", SDL_GetError());
	}
}
