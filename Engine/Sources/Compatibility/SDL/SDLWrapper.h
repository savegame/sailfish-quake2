#ifndef SDLWrapper_h
#define SDLWrapper_h

#include <SDL.h>

#include <stdbool.h>

# ifdef SAILFISHOS
#  include <SDL_video.h>
# endif

typedef bool (*SdlProcessEventFunction)(SDL_Event *event);

typedef struct {
    bool exitRequested;
    bool defaultEventManagementEnabled;
	SdlProcessEventFunction processEvent;
	SDL_Window *window;
	int windowWidth, windowHeight;
# ifdef SAILFISHOS
	SDL_DisplayOrientation orientation;
# endif
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
# endif

#endif
