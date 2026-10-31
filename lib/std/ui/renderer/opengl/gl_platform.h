// The OpenGL prototypes std.ui.renderer.opengl.gl binds, from where each
// platform declares them. macOS keeps the fixed-function entry points in
// <OpenGL/gl.h> and those of OpenGL 3.2 and later in <OpenGL/gl3.h>; the
// OpenGL framework exports both. Windows' opengl32.dll exports OpenGL 1.1
// only (below). Elsewhere <GL/gl.h> declares them all.
#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#elif defined(_WIN32)
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <stdio.h>
#include <stdlib.h>

// Every entry point after OpenGL 1.1 comes from the driver through
// wglGetProcAddress, which answers once a context is current (the renderer
// calls none before). This header defines each one the binding uses, under
// its own name, as a forwarder that asks for it at its first call; a driver
// that has none of it ends the program naming the function. Only the
// sidecar of std.ui.renderer.opengl.gl includes it, so a program holds one
// definition of each, which the binding's calls (and the import library's
// lack of them) leave to it.
static inline PROC adm_gl_proc(const char* name) {
    PROC p = wglGetProcAddress(name);
    // Some drivers answer 1, 2, 3 or -1 for a name they do not have.
    if (p == 0 || p == (PROC)1 || p == (PROC)2 || p == (PROC)3 || p == (PROC)-1) {
        HMODULE gl = GetModuleHandleA("opengl32.dll");
        p = gl ? GetProcAddress(gl, name) : 0;
    }
    if (!p) {
        fprintf(stderr, "OpenGL: the driver has no %s (OpenGL 2.0 or newer is needed)\n", name);
        abort();
    }
    return p;
}

#define ADM_GL_VOID(name, params, args) \
    void name params { \
        static void (APIENTRY * fn) params; \
        if (!fn) fn = (void (APIENTRY *) params)(void (*)(void))adm_gl_proc(#name); \
        fn args; \
    }
#define ADM_GL_RET(ret, name, params, args) \
    ret name params { \
        static ret (APIENTRY * fn) params; \
        if (!fn) fn = (ret (APIENTRY *) params)(void (*)(void))adm_gl_proc(#name); \
        return fn args; \
    }

ADM_GL_VOID(glActiveTexture, (GLenum texture), (texture))
ADM_GL_VOID(glAttachShader, (GLuint program, GLuint shader), (program, shader))
ADM_GL_VOID(glBindFramebuffer, (GLenum target, GLuint framebuffer), (target, framebuffer))
ADM_GL_VOID(glBlendEquation, (GLenum mode), (mode))
ADM_GL_VOID(glBlendFuncSeparate, (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha), (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha))
ADM_GL_RET(GLenum, glCheckFramebufferStatus, (GLenum target), (target))
ADM_GL_VOID(glCompileShader, (GLuint shader), (shader))
ADM_GL_RET(GLuint, glCreateProgram, (void), ())
ADM_GL_RET(GLuint, glCreateShader, (GLenum type), (type))
ADM_GL_VOID(glDeleteFramebuffers, (GLsizei n, const GLuint *framebuffers), (n, framebuffers))
ADM_GL_VOID(glDeleteProgram, (GLuint program), (program))
ADM_GL_VOID(glDeleteShader, (GLuint shader), (shader))
ADM_GL_VOID(glDisableVertexAttribArray, (GLuint index), (index))
ADM_GL_VOID(glEnableVertexAttribArray, (GLuint index), (index))
ADM_GL_VOID(glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level), (target, attachment, textarget, texture, level))
ADM_GL_VOID(glGenFramebuffers, (GLsizei n, GLuint *framebuffers), (n, framebuffers))
ADM_GL_RET(GLint, glGetAttribLocation, (GLuint program, const GLchar *name), (program, name))
ADM_GL_VOID(glGetProgramInfoLog, (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog), (program, bufSize, length, infoLog))
ADM_GL_VOID(glGetProgramiv, (GLuint program, GLenum pname, GLint *params), (program, pname, params))
ADM_GL_VOID(glGetShaderInfoLog, (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog), (shader, bufSize, length, infoLog))
ADM_GL_VOID(glGetShaderiv, (GLuint shader, GLenum pname, GLint *params), (shader, pname, params))
ADM_GL_RET(GLint, glGetUniformLocation, (GLuint program, const GLchar *name), (program, name))
ADM_GL_VOID(glLinkProgram, (GLuint program), (program))
ADM_GL_VOID(glShaderSource, (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length), (shader, count, string, length))
ADM_GL_VOID(glUniform1dv, (GLint location, GLsizei count, const GLdouble *value), (location, count, value))
ADM_GL_VOID(glUniform1f, (GLint location, GLfloat v0), (location, v0))
ADM_GL_VOID(glUniform1i, (GLint location, GLint v0), (location, v0))
ADM_GL_VOID(glUniform1ui, (GLint location, GLuint v0), (location, v0))
ADM_GL_VOID(glUniform2f, (GLint location, GLfloat v0, GLfloat v1), (location, v0, v1))
ADM_GL_VOID(glUniform2i, (GLint location, GLint v0, GLint v1), (location, v0, v1))
ADM_GL_VOID(glUniform3f, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2), (location, v0, v1, v2))
ADM_GL_VOID(glUniform3ui, (GLint location, GLuint v0, GLuint v1, GLuint v2), (location, v0, v1, v2))
ADM_GL_VOID(glUniform4f, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3), (location, v0, v1, v2, v3))
ADM_GL_VOID(glUniform4i, (GLint location, GLint v0, GLint v1, GLint v2, GLint v3), (location, v0, v1, v2, v3))
ADM_GL_VOID(glUniform4ui, (GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3), (location, v0, v1, v2, v3))
ADM_GL_VOID(glUniform4uiv, (GLint location, GLsizei count, const GLuint *value), (location, count, value))
ADM_GL_VOID(glUseProgram, (GLuint program), (program))
ADM_GL_VOID(glVertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer), (index, size, type, normalized, stride, pointer))

#else
#include <GL/gl.h>
#endif

// adm_gl_texture_barrier_ready reports whether the current context has
// glTextureBarrier (OpenGL 4.5, ARB_texture_barrier, or NV_texture_barrier's
// glTextureBarrierNV), looking the entry point up for adm_gl_texture_barrier.
// The renderer asks once a context is current; a context without it, or a
// platform that never has it (macOS stops at OpenGL 4.1, the web at WebGL 2),
// answers 0 and the renderer copies the destination instead.
#if defined(__APPLE__) || defined(__EMSCRIPTEN__) || defined(__wasm__) || defined(__ANDROID__)
int adm_gl_texture_barrier_ready(void) { return 0; }
void adm_gl_texture_barrier(void) {}
#else
#include <string.h>
#if defined(_WIN32)
static void* adm_gl_lookup(const char* name) {
    PROC p = wglGetProcAddress(name);
    if (p == 0 || p == (PROC)1 || p == (PROC)2 || p == (PROC)3 || p == (PROC)-1) return 0;
    return (void*)p;
}
#else
#include <dlfcn.h>
// The program's own handle: the GL library it links answers for it (RTLD_DEFAULT
// needs _GNU_SOURCE, which this header does not set).
static void* adm_gl_lookup(const char* name) {
    static void* self;
    if (!self) self = dlopen(0, RTLD_LAZY);
    return self ? dlsym(self, name) : 0;
}
#endif
#ifndef APIENTRY
#define APIENTRY
#endif
static void (APIENTRY* adm_gl_barrier)(void);

static int adm_gl_has_extension(const char* want) {
    typedef const GLubyte* (APIENTRY* stringi_fn)(GLenum, GLuint);
    stringi_fn stringi = (stringi_fn)adm_gl_lookup("glGetStringi");
    if (!stringi) return 0;
    GLint n = 0;
    glGetIntegerv(0x821D /* GL_NUM_EXTENSIONS */, &n);
    for (GLint i = 0; i < n; i++) {
        const GLubyte* e = stringi(0x1F03 /* GL_EXTENSIONS */, (GLuint)i);
        if (e && strcmp((const char*)e, want) == 0) return 1;
    }
    return 0;
}

int adm_gl_texture_barrier_ready(void) {
    const char* version = (const char*)glGetString(0x1F02 /* GL_VERSION */);
    int major = 0, minor = 0;
    if (version) {
        const char* v = version;
        while (*v && (*v < '0' || *v > '9')) v++;
        while (*v >= '0' && *v <= '9') major = major * 10 + (*v++ - '0');
        if (*v == '.') {
            v++;
            while (*v >= '0' && *v <= '9') minor = minor * 10 + (*v++ - '0');
        }
    }
    const char* name = 0;
    if (major > 4 || (major == 4 && minor >= 5) || adm_gl_has_extension("GL_ARB_texture_barrier")) {
        name = "glTextureBarrier";
    } else if (adm_gl_has_extension("GL_NV_texture_barrier")) {
        name = "glTextureBarrierNV";
    }
    adm_gl_barrier = name ? (void (APIENTRY*)(void))adm_gl_lookup(name) : 0;
    return adm_gl_barrier != 0;
}

void adm_gl_texture_barrier(void) {
    if (adm_gl_barrier) adm_gl_barrier();
}
#endif
