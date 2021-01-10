/** functions for load shader programs */
#ifndef SHADER_GLES2_HEADER
#define SHADER_GLES2_HEADER

#include <GLES2/gl2.h>

/** @brief load/compile  vertex/fragment shader programm
 * @param  type    shader type {GL_VERTEX_SHADER or GL_FRAGMENT_SHADER}
 * @param  source  string with source code
 */
GLuint loadShader(GLenum type, const char *source);

/** @brief load shader programm 
 * @param vp             vertex programm string 
 * @param fp             fragment program string 
 * @param attributes     array of attributes names
 * @param attributeCount count of attributes
 */
GLuint loadProgram(const char *vp, const char *fp, const char *attributes[], int attributeCount);

#endif