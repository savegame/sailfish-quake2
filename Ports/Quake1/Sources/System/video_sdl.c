#include "Client/client.h"
#include "Client/console.h"
#include "Client/input.h"
#include "Common/quakedef.h"
#include "Common/sys.h"
#include "Rendering/r_private.h"
#include "Rendering/r_video.h"

#include "SDL/SDLWrapper.h"
#include "OpenGLES/EGLWrapper.h"
#include "OpenGLES/OpenGLWrapper.h"

#include <stdlib.h>
#include <string.h>
   
viddef_t vid; // global video state

unsigned d_8to24table[256];

#define BASEWIDTH (320)
#define BASEHEIGHT (240)

void VID_SetPalette(unsigned char *palette)
{
	for (int i = 0; i < 256; ++i, palette+=3)
	{
        unsigned char *c = (unsigned char *)&d_8to24table[i];
        c[0] = palette[0];
        c[1] = palette[1];
        c[2] = palette[2];
        c[3] = (i == 255) ? 0x00 : 0xff;
	}
}

void VID_Init(unsigned char *palette)
{
  	extern bool IN_processEvent(SDL_Event *event);
	if (sdlwInitialize(IN_processEvent, SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER))
		goto on_error;
	sdlwEnableDefaultEventManagement(false);

	// Set up display mode (width and height)
	vid.width = BASEWIDTH;
	vid.height = BASEHEIGHT;
	int pnum;
	if ((pnum = COM_CheckParm("-winsize")))
	{
		if (pnum >= com_argc - 2)
			Sys_Error("VID: -winsize <width> <height>\n");
		vid.width = Q_atoi(com_argv[pnum + 1]);
		vid.height = Q_atoi(com_argv[pnum + 2]);
		if (!vid.width || !vid.height)
			Sys_Error("VID: Bad window width/height\n");
	}

	// Set video width, height and flags
	Uint32 flags = 0; // SDL_WINDOW_RESIZABLE
	if (COM_CheckParm("-fullscreen"))
		flags |= SDL_WINDOW_FULLSCREEN;

	if (sdlwCreateWindow(QUAKE_COMPLETE_NAME, vid.width, vid.height, flags))
		goto on_error;

	// now know everything we need to know about the buffer
	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0f / 240.0f);
	vid.numpages = 1;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));

	if (eglwInitialize(NULL, NULL, true))
		goto on_error;

	VID_SetPalette(palette);

	if (oglwCreate())
		goto on_error;

	gl_vendor = glGetString(GL_VENDOR);
	Con_Printf("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = glGetString(GL_RENDERER);
	Con_Printf("GL_RENDERER: %s\n", gl_renderer);

	gl_version = glGetString(GL_VERSION);
	Con_Printf("GL_VERSION: %s\n", gl_version);
	gl_extensions = glGetString(GL_EXTENSIONS);
	Con_Printf("GL_EXTENSIONS: %s\n", gl_extensions);

	r_stencilAvailable = (eglwContext->configInfo.stencilSize > 0);

	SDL_ShowCursor(0);
	return;

on_error:
	VID_Shutdown();
	return;
}

void VID_Shutdown()
{
    if (sdlwContext)
        IN_Grab(false);
	SDL_ShowCursor(1);

	oglwDestroy();
	eglwFinalize();
	sdlwFinalize();
}

void R_beginRendering(int *x, int *y, int *width, int *height)
{
	*x = *y = 0;
	*width = sdlwContext->windowWidth;
	*height = sdlwContext->windowHeight;
	oglwSetViewport(*x, *y, *width, *height);
}

void R_endRendering()
{
	eglwSwapBuffers();
}

void VID_HandlePause(qboolean pause)
{
}
