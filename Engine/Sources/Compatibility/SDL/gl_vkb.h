#ifndef KARIN_GL_VKB_H
#define KARIN_GL_VKB_H

#include "vkb.h"

void vkb_NewGLVKB(float x, float y, float z, float w, float h);
void vkb_DeleteGLVKB(void);
void vkb_RenderGLVKB(void);
unsigned vkb_GLVKBMouseMotionEvent(int b, int p, int x, int y, int dx, int dy, VKB_Key_Action_Function f);
unsigned vkb_GLVKBMouseEvent(int b, int p, int x, int y, VKB_Key_Action_Function f);
void vkb_ResizeGLVKB(float w, float h);

#endif
