//
// Utility function
//

#define EGLAssert() if (eglGetError() != EGL_SUCCESS) { Print("EGL ERROR Line %d", __LINE__); __builtin_trap(); }

//
// Types
//

typedef u32		EGLint;
typedef u32		EGLBoolean;
typedef u32		EGLenum;

typedef void *	EGLConfig;
typedef void *	EGLContext;
typedef void *	EGLDisplay;
typedef void *	EGLSurface;
typedef void *	EGLNativeDisplayType;
typedef void *	EGLNativeWindowType;


#define EGL_DEFAULT_DISPLAY		((EGLNativeDisplayType)0)
#define EGL_NO_CONTEXT			((EGLContext)0)
#define EGL_NO_DISPLAY			((EGLDisplay)0)
#define EGL_NO_SURFACE			((EGLSurface)0)
#define EGL_DONT_CARE			((EGLint)-1)
#define EGL_UNKNOWN				((EGLint)-1)

#define EGL_NATIVE_VISUAL_ID		0x302E

#define EGL_CONTEXT_CLIENT_VERSION	0x3098


#define EGL_ALPHA_SIZE			0x3021
#define EGL_BLUE_SIZE			0x3022
#define EGL_GREEN_SIZE			0x3023
#define EGL_RED_SIZE			0x3024
#define EGL_SURFACE_TYPE		0x3033
#define EGL_RENDERABLE_TYPE		0x3040
#define EGL_NONE				0x3038

#define EGL_NO_TEXTURE			0x305C
#define EGL_TEXTURE_RGB			0x305D
#define EGL_TEXTURE_RGBA		0x305E
#define EGL_TEXTURE_2D			0x305F

// EGL_SURFACE_TYPE
#define EGL_WINDOW_BIT			0x0004

// EGL_RENDERABLE_TYPE
#define EGL_OPENGL_ES_BIT		0x0001
#define EGL_OPENVG_BIT			0x0002
#define EGL_OPENGL_ES2_BIT		0x0004
#define EGL_OPENGL_BIT			0x0008

// EglQuerySurface and others
#define EGL_HEIGHT				0x3056
#define EGL_WIDTH				0x3057

// Errors
#define EGL_SUCCESS				0x3000
#define EGL_NOT_INITIALIZED		0x3001
#define EGL_BAD_CONTEXT			0x3006
#define EGL_BAD_DISPLAY			0x3008
#define EGL_BAD_MATCH			0x3009
#define EGL_BAD_SURFACE			0x300D
#define EGL_CONTEXT_LOST		0x300E	// EGL 1.1 - IMG_power_management

#define EGL_FALSE				0
#define EGL_TRUE				1

#define EGLAPI		__attribute__((visibility("default")))
#define EGLAPIENTRY

#define GL_API		__attribute__((visibility("default")))
#define GL_APIENTRY

extern "C"
{
	EGLAPI EGLint		EGLAPIENTRY eglGetError();
	EGLAPI char *		EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLint name);
	
	EGLAPI EGLDisplay	EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id);
	EGLAPI EGLBoolean	EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor);
	
	EGLAPI EGLBoolean	EGLAPIENTRY eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
	EGLAPI EGLBoolean	EGLAPIENTRY eglChooseConfig(EGLDisplay dpy, EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
	EGLAPI EGLBoolean	EGLAPIENTRY eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
	
	EGLAPI EGLSurface	EGLAPIENTRY eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, EGLint *attrib_list);
	EGLAPI EGLBoolean	EGLAPIENTRY eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);
	EGLAPI EGLBoolean	EGLAPIENTRY eglDestroySurface(EGLDisplay dpy, EGLSurface surface);
	
	EGLAPI EGLBoolean	EGLAPIENTRY eglSwapInterval(EGLDisplay dpy, EGLint interval);
	EGLAPI EGLBoolean	EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy, EGLSurface surface);
	
	EGLAPI EGLContext	EGLAPIENTRY eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, EGLint *attrib_list);
	EGLAPI EGLBoolean	EGLAPIENTRY eglDestroyContext(EGLDisplay dpy, EGLContext ctx);
	EGLAPI EGLBoolean	EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
}
