#include <SDL/vkb.h>

#define HARM_SWIPE_SENS "harm_swipesens"
#define HARM_SWIPE_SENS_DEFAULT 1.0
#define HARM_SWIPE_SENS_DEFAULT_P "1.0"

static cvar_t *harm_swipeSens;

static int vkb_SwipeSens(int x)
{
	float sens = harm_swipeSens->value;
	if(sens <= 0.0)
	{
		sens = HARM_SWIPE_SENS_DEFAULT;
		Cvar_Set(HARM_SWIPE_SENS, HARM_SWIPE_SENS_DEFAULT_P);
	}
	return (int)((float)x * sens);
}


static unsigned vkb_HandleVKBAction(int action, unsigned pressed, int dx, int dy)
{
#define		MAXCMDLINE	256
#define		MAXCMDLENGTH 1024
	static int _keys[MAXCMDLINE];
	static char _cmd[MAXCMDLENGTH];
	unsigned int key_count = 0;

	int r = vkb_GetActionData(action, _keys, MAXCMDLINE, &key_count, _cmd, MAXCMDLENGTH);
	if(r == Cmd_Data)
	{
		if(vkb_AddCommand)
		{
			unsigned int time = Sys_Milliseconds();
			char cmd[MAXCMDLENGTH];
			if(pressed)
			{
				if (_cmd[0] == '+')
				{	// button commands add keynum and time as a parm
					Com_sprintf (cmd, sizeof(cmd), "%s %i %i\n", _cmd, _keys[0], time);
					vkb_AddCommand(cmd);
				}
				else
				{
					vkb_AddCommand(_cmd);
					vkb_AddCommand("\n");
				}
			}
			else
			{
				if (_cmd[0] == '+')
				{
					Com_sprintf (cmd, sizeof(cmd), "-%s %i %i\n", _cmd+1, _keys[0], time);
					vkb_AddCommand(cmd);
				}
			}
			return 1;
		}
	}
	else if(r == Key_Data)
	{
		// in_state_t *in_state = getState();
		// if (in_state && in_state->Key_Event_fp)
		// {
		// 	int i = 0;
		for(int i = 0; i < key_count; i++)
		{
			Key_Event(_keys[i], pressed == btrue ? true : false);
			// printf("%c ", k[i]);
		}
		return 1;
		// }
		// printf("Key Event!");
	}
	else if(r == Button_Data)
	{
		if(pressed)
		{
			int fdx = vkb_SwipeSens(dx);
			int fdy = vkb_SwipeSens(dy);
			// if(fdx != 0 || fdy != 0)
			// {
			// 	mx += fdx;
			// 	my -= fdy;
			// 	mouse_buttonstate = 0;
			// }
		}
	}

	return 0;
#undef MAXCMDLINE
#undef MAXCMDLENGTH
}