#ifndef KARIN_GL_VKB_H
#define KARIN_GL_VKB_H

#include "vkb.h"

void karinNewGLVKB(float x, float y, float z, float w, float h);
void karinDeleteGLVKB(void);
void karinRenderGLVKB(void);
unsigned karinGLVKBMouseMotionEvent(int b, int p, int x, int y, int dx, int dy, VKB_Key_Action_Function f);
unsigned karinGLVKBMouseEvent(int b, int p, int x, int y, VKB_Key_Action_Function f);
void karinResizeGLVKB(float w, float h);

#endif
