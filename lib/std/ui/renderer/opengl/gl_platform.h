// The OpenGL prototypes std.ui.renderer.opengl.gl binds, from where each
// platform declares them. macOS keeps the fixed-function entry points in
// <OpenGL/gl.h> and those of OpenGL 3.2 and later in <OpenGL/gl3.h>; the
// OpenGL framework exports both. Elsewhere <GL/gl.h> declares them all.
#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif
