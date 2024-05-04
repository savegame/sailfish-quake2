#include "SDL/SDLWrapper.h"

#include <stdio.h>
#include <stdlib.h>

SdlwContext *sdlwContext = NULL;

#ifdef SAILFISHOS
#include <SDL_hints.h>
#include <SDL_events.h>
#include <SDL_video.h>
#include <SDL_syswm.h>
#include <wayland-client-protocol.h>
#endif
//SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER

static void sdlwLogOutputFunction(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    const char *categoryString = "";
    switch (category)
    {
    default:
        categoryString = "unknown";
        break;
    case SDL_LOG_CATEGORY_APPLICATION:
        categoryString = "application";
        break;
    case SDL_LOG_CATEGORY_ERROR:
        categoryString = "error";
        break;
    case SDL_LOG_CATEGORY_ASSERT:
        categoryString = "assert";
        break;
    case SDL_LOG_CATEGORY_SYSTEM:
        categoryString = "system";
        break;
    case SDL_LOG_CATEGORY_AUDIO:
        categoryString = "audio";
        break;
    case SDL_LOG_CATEGORY_VIDEO:
        categoryString = "video";
        break;
    case SDL_LOG_CATEGORY_RENDER:
        categoryString = "render";
        break;
    case SDL_LOG_CATEGORY_INPUT:
        categoryString = "input";
        break;
    case SDL_LOG_CATEGORY_TEST:
        categoryString = "test";
        break;
    }

    const char *priorityString = "unknown";
    switch (priority)
    {
    default:
        priorityString = "unknown";
        break;
    case SDL_LOG_PRIORITY_VERBOSE:
        priorityString = "verbose";
        break;
    case SDL_LOG_PRIORITY_DEBUG:
        priorityString = "debug";
        break;
    case SDL_LOG_PRIORITY_INFO:
        priorityString = "info";
        break;
    case SDL_LOG_PRIORITY_WARN:
        priorityString = "warn";
        break;
    case SDL_LOG_PRIORITY_ERROR:
        priorityString = "error";
        break;
    case SDL_LOG_PRIORITY_CRITICAL:
        priorityString = "critical";
        break;
    }
    
    printf("SDL - %s - %s - %s", categoryString, priorityString, message);
}

bool sdlwInitialize(SdlProcessEventFunction processEvent, Uint32 flags) {
    sdlwFinalize();
    
	SdlwContext *sdlw = malloc(sizeof(SdlwContext));
	if (sdlw == NULL) return true;
	sdlwContext = sdlw;
    sdlw->exitRequested = false;
    sdlw->defaultEventManagementEnabled = true;
	sdlw->processEvent = processEvent;
    sdlw->window = NULL;
    sdlw->windowWidth = 0;
    sdlw->windowHeight = 0;
#ifdef SAILFISHOS
    sdlw->orientation = SDL_ORIENTATION_LANDSCAPE;
#endif
    if (SDL_Init(flags) < 0) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        goto on_error;
    }
    
    SDL_LogSetOutputFunction(sdlwLogOutputFunction, NULL);
//    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Test\n");
    
//    if (SDL_NumJoysticks() > 0) SDL_JoystickOpen(0);
    
    return false;
on_error:
	sdlwFinalize();
    return true;
}

void sdlwFinalize() {
	SdlwContext *sdlw = sdlwContext;
	if (sdlw == NULL) return;
    
    SDL_Quit();
    
    free(sdlw);
    sdlwContext = NULL;
}

bool sdlwCreateWindow(const char *windowName, int windowWidth, int windowHeight, Uint32 flags)
{
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) 
        return true;
    #ifdef SAILFISHOS
    int r = SDL_GameControllerAddMappingsFromFile("/usr/share/ru.sashikknox.quake2/gamecontrollerdb.txt");
    printf("Load gamecontrollerdb : %i\n", r);
    #endif
    sdlwDestroyWindow();
#ifdef SAILFISHOS
    windowWidth = -1;
#endif
    if (windowWidth < 0 || windowHeight < 0)
    {
        SDL_DisplayMode dm;
        if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
        {
            printf("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
            goto on_error;
        }
        #if defined(__RASPBERRY_PI__) || defined(__GCW_ZERO__) || defined(SAILFISHOS)
        // Windowed mode does not work on these platforms. So use full screen.
        windowWidth = dm.w;
        windowHeight = dm.h;
        #else
        windowWidth = dm.w >> 1;
        windowHeight = dm.h >> 1;
        #endif
    }
    sdlw->windowWidth = windowWidth;
    sdlw->windowHeight = windowHeight;
    int windowPos = SDL_WINDOWPOS_CENTERED;
#ifdef SAILFISHOS
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    windowPos = SDL_WINDOWPOS_UNDEFINED;
    flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN;
#endif
   	if ((sdlw->window=SDL_CreateWindow(windowName, windowPos, windowPos, windowWidth, windowHeight, flags))==NULL) goto on_error;
#ifdef SAILFISHOS
    sdlwSetOrientation(SDL_ORIENTATION_LANDSCAPE); // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"landscape");
#endif
    return false;
on_error:
    return true;
}

void sdlwDestroyWindow() {
	SdlwContext *sdlw = sdlwContext;
	if (sdlw == NULL) return;
    if (sdlw->window != NULL) {
        SDL_DestroyWindow(sdlw->window);
        sdlw->window=NULL;
        sdlw->windowWidth = 0;
        sdlw->windowHeight = 0;
    }
}

bool sdlwIsExitRequested()
{
	SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return true;
    return sdlw->exitRequested;
}

void sdlwRequestExit(bool flag)
{
	SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return;
    sdlw->exitRequested = flag;
}

bool sdlwResize(int w, int h) {
	SdlwContext *sdlw = sdlwContext;
	sdlw->windowWidth = w;
	sdlw->windowHeight = h;
    return false;
}

void sdlwEnableDefaultEventManagement(bool flag)
{
	SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return;
    sdlw->defaultEventManagementEnabled = flag;
}

static void sdlwManageEvent(SdlwContext *sdlw, SDL_Event *event) {
    switch (event->type) {
    default:
        break;

    case SDL_QUIT:
        printf("Exit requested by the system.");
        sdlwRequestExit(true);
        break;

    case SDL_WINDOWEVENT:
        switch (event->window.event) {
        case SDL_WINDOWEVENT_CLOSE:
            printf("Exit requested by the user (by closing the window).");
            sdlwRequestExit(true);
            break;
        case SDL_WINDOWEVENT_RESIZED:
            sdlwResize(event->window.data1, event->window.data2);
            break;
        }
        break;
    case SDL_KEYDOWN:
        switch (event->key.keysym.sym) {
        default:
            break;
        case 27:
            printf("Exit requested by the user (with a key).");
            sdlwRequestExit(true);
            break;
        }
        break;
    }
}

void sdlwCheckEvents() {
	SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return;
    
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
        bool eventManaged = false;
		SdlProcessEventFunction processEvent = sdlw->processEvent;
		if (processEvent != NULL)
			eventManaged = processEvent(&event);
        if (!eventManaged && sdlw->defaultEventManagementEnabled)
            sdlwManageEvent(sdlw, &event);
	}
}

#ifdef SAILFISHOS
SDL_DisplayOrientation sdlwCurrentOrientation() {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return SDL_ORIENTATION_UNKNOWN;
    return sdlw->orientation;
}

SDL_DisplayOrientation sdlwGetRealOrientation() {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return SDL_ORIENTATION_UNKNOWN;
    return sdlw->real_orientation;
}

void sdlwSetOrientation(SDL_DisplayOrientation orientation) {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return;
    sdlw->orientation = orientation;

    struct SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(sdlwContext->window, &wmInfo)) {
        printf("Cannot get the window handle.\n");
        // goto on_error;
        switch (sdlw->orientation) {
            case SDL_ORIENTATION_LANDSCAPE:
                SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"landscape");
                break;
            case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
                SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"inverted-landscape");
                break;
            case SDL_ORIENTATION_PORTRAIT:
                SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"portrait");
                break;
            case SDL_ORIENTATION_PORTRAIT_FLIPPED:
                SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"inverted-portrait");
                break;
            default:
            case SDL_ORIENTATION_UNKNOWN:
                SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"landscape");
                // printf("SDL_DisplayOrientation is SDL_ORIENTATION_UNKNOWN\n");
                break;
        }
    }
    // nativeDisplay = wmInfo.info.wl.display;
    // wl_surface *sdl_wl_surface = wmInfo.info.wl.surface;

    // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"landscape");
    switch (sdlw->orientation) {
        case SDL_ORIENTATION_LANDSCAPE:
            // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"landscape");
            wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_270);
            break;
        case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
            // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"inverted-landscape");
            wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_90);
            break;
        case SDL_ORIENTATION_PORTRAIT:
            // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"portrait");
            wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_NORMAL);
            break;
        case SDL_ORIENTATION_PORTRAIT_FLIPPED:
            // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"inverted-portrait");
            wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_180);
            break;
        default:
        case SDL_ORIENTATION_UNKNOWN:
            // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"landscape");
            wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_90);
            // printf("SDL_DisplayOrientation is SDL_ORIENTATION_UNKNOWN\n");
            break;
    }
}

void sdlwSetRealOrientation(SDL_DisplayOrientation orientation) {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return;
    sdlw->real_orientation = orientation;
}
#endif

#ifdef SAILFISH_FBO
float sdlwGetFboScale() {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return SAILFISH_FBO_DEFAULT_SCALE;
    return sdlw->fbo_scale;
}

void sdlwSetFboScale(float scale) {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return;
    if( scale <= 0.0f )
    // TODO should calculate min width 320
        scale = 0.2f;
    else if( scale > 1.0f )
        scale = 1.0f;
    sdlw->fbo_scale = scale;
}

#endif

void sdlwGetWindowSize(int *w, int *h) {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return; 
#ifdef SAILFISHOS
    switch (sdlw->orientation) {
        case SDL_ORIENTATION_PORTRAIT:
        case SDL_ORIENTATION_PORTRAIT_FLIPPED:
            *h = sdlw->windowWidth;
            *w = sdlw->windowHeight;
            break;
        case SDL_ORIENTATION_LANDSCAPE:
        case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
        case SDL_ORIENTATION_UNKNOWN:
        default:
            *w = sdlw->windowWidth;
            *h = sdlw->windowHeight;
            break;
    }
#else
    *w = sdlw->windowWidth;
    *h = sdlw->windowHeight;
#endif
}
