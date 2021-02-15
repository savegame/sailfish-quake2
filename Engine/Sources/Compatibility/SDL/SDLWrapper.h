#ifndef SDLWrapper_h
#define SDLWrapper_h

#include <SDL.h>

#include <stdbool.h>

# ifdef SAILFISHOS
#  include <SDL_video.h>
# endif

#ifdef SAILFISH_FBO 
# define SAILFISH_FBO_DEFAULT_SCALE 0.5f
#endif

typedef bool (*SdlProcessEventFunction)(SDL_Event *event);

typedef struct {
    bool exitRequested;
    bool defaultEventManagementEnabled;
	SdlProcessEventFunction processEvent;
	SDL_Window *window;
	int windowWidth, windowHeight;
# ifdef SAILFISHOS
	SDL_DisplayOrientation orientation;
	SDL_DisplayOrientation real_orientation;
# endif
#ifdef SAILFISH_FBO
	float fbo_scale;
#endif
} SdlwContext;

extern SdlwContext *sdlwContext;

bool sdlwInitialize(SdlProcessEventFunction processEvent, Uint32 flags);
void sdlwFinalize();

bool sdlwCreateWindow(const char *windowName, int windowWidth, int windowHeight, Uint32 flags);
void sdlwDestroyWindow();

bool sdlwIsExitRequested();
void sdlwRequestExit(bool flag);
bool sdlwResize(int w, int h);

void sdlwEnableDefaultEventManagement(bool flag);
void sdlwCheckEvents();

# ifdef SAILFISHOS
SDL_DisplayOrientation sdlwCurrentOrientation();
void sdlwSetOrientation(SDL_DisplayOrientation orientation);

SDL_DisplayOrientation sdlwGetRealOrientation();
void sdlwSetRealOrientation(SDL_DisplayOrientation orientation);
# endif
#ifdef SAILFISH_FBO
float sdlwGetFboScale();
void sdlwSetFboScale(float scale);
#endif
void sdlwGetWindowSize(int *w, int *h);

#endif
