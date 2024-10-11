#pragma clang diagnostic ignored "-Wmissing-braces"

//
// Includes
//

#include "common.h"
#include "android_platform.h"
#include "android_opengl.h"
#include "opengl.h"
#include "math.h"

//
// Defines
//

#define MAX_VERTICES_COUNT  2048
#define TEXTURE_SIZE        2048

#define TAB_MENU_COUNT      3
#define TAB_MENU_HEIGHT     100.0f

// For solid color quads
#define TEXPOS_SOLID v2{\
    (TEXTURE_SIZE - 1.0f + 0.5f) / TEXTURE_SIZE, \
    (TEXTURE_SIZE - 1.0f + 0.5f) / TEXTURE_SIZE  \
}

#define PERMANENT_ARENA_SIZE MB(1)
#define TRANSIENT_ARENA_SIZE MB(10)

#define ACTIVE_RESUME       (1 << 0)
#define ACTIVE_FOCUS        (1 << 1)
#define ACTIVE_SURFACE      (1 << 2)
#define ACTIVE_INTERACTIVE  (ACTIVE_RESUME | ACTIVE_FOCUS | ACTIVE_SURFACE)

struct render_context
{
    // EGL Stuff
    EGLint Width;  // In Pixels
    EGLint Height; // In Pixels
    
    EGLDisplay  Display;
    EGLConfig   Config;
    EGLSurface  Surface;
    EGLContext  Context;
    
    ANativeWindow *Window;
    EGLint Format; // Format of the native window
    
    // Shader stuff GL 2.0
    GLuint MainShader;
    GLint  MetersToNDCLocation;
};

struct loaded_glyph
{
    u8 Width;
    u8 Height;
    s8 OffsetX;
    s8 OffsetY;
    u8 Advance; // Advance X when drawing next glyph
};

struct font // TTF Parsed
{
    memory_arena Arena;
    
    u8 *FileData;
    s32 FileSize;
    
    u16 FontSize;
    u16 GlyphCount;
    f32 ScaleFactor; // Design units to Pixels
    
    //
    // We are storing the valeus of the CMAP table parsed
    // so we can skip the conversion from BigEndian to LittleEndian
    //
    
    u16 HMetricCount;
    u16 CMAP_SegmentCount;
    u16 *CMAP_EndCodes;
    u16 *CMAP_StartCodes;
    s16 *CMAP_IDDeltas;
    u16 *CMAP_IDRangeOffsets;
    
    s32 IndexToLocFormat;
    s32 GlyfOffset;
    s32 HmtxOffset;
    s32 LocaOffset;
    
    //
    // Glyph Used Atlas
    //
    
    u8 LoadedGlyphMask;  // Used to map from index to X pos
    u8 LoadedGlyphShift; // Used to map from index to Y pos
    
    u16 ReservedGlyphCount;
    u16 LoadedGlyphCount;
    u16 MaxLoadedGlyphCount;
    
    u32 *LoadedGlyphsCodepoint; // Used for simple lookup, @Todo Switch to custom Hash table
    u16 *LoadedGlyphsUsedFrame;
    loaded_glyph *LoadedGlyphsData;
};

struct main_state
{
    render_context RenderContext;
    
    f32 PixelsToMeters;
    f32 MetersToPixels;
    f32 Width;              // In Meters
    f32 Height;             // In Meters
    f32 StatusBarHeight;    // In Meters
    
    s32 TabIndex;
    
    memory_arena    PermanentArena;
    memory_arena    TransientArena;
    
    font Font;
    
    timespec BeginClockTime;
    f32 SecondsPerFrame;
    f32 FramesPerSecond;
    s32 Frame;
    
    s32 VertexCount;
    vertex VertexBuffer[MAX_VERTICES_COUNT];
};

//
// Globals
//

global ANativeActivity  *GLOBAL_Activity;
global main_state       *GLOBAL_MainState;

// @Note These are written to by the Android thread
global ANativeWindow    *GLOBAL_PendingWindow;
global AInputQueue      *GLOBAL_InputQueue;
global s32              GLOBAL_ActiveFlags;

global s32 GLOBAL_GestureType;
global s32 GLOBAL_GestureStartTime;
global bmm GLOBAL_GestureCanScroll;
global v2  GLOBAL_GestureStart;
global v2  GLOBAL_GestureFocus;
global f32 GLOBAL_GestureMovedSq;

global f32 GLOBAL_ScrollYSpeed;
global f32 GLOBAL_ScrollPrevAmount;
global s32 GLOBAL_ScrollDir; // 0 is X, 1 is Y
global v2  GLOBAL_Scroll;

enum gesture
{
    GESTURE_NONE, 
    GESTURE_TOUCH, 
    GESTURE_SCROLL, 
    GESTURE_UNKNOWN, 
};

// Month daycount lookup INTERNAL, Use GetDaysInMonth() function
global u8 DaysInMonthInternal[] = // 0-11
{
    /* January      */ 31, 
    /* February     */ 28, // 29 in every leap year, handled specially
    /* March        */ 31, 
    /* April        */ 30, 
    /* May          */ 31, 
    /* June         */ 30, 
    /* July         */ 31, 
    /* August       */ 31, 
    /* September    */ 30, 
    /* October      */ 31, 
    /* November     */ 30, 
    /* December     */ 31 
};

// Month name lookup
global string MonthNames[] = // 0-11
{
    string{ StringAndLength("January")  }, 
    string{ StringAndLength("February") }, 
    string{ StringAndLength("March")    }, 
    string{ StringAndLength("April")    }, 
    string{ StringAndLength("May")      }, 
    string{ StringAndLength("June")     }, 
    string{ StringAndLength("July")     }, 
    string{ StringAndLength("August")   }, 
    string{ StringAndLength("September")}, 
    string{ StringAndLength("October")  }, 
    string{ StringAndLength("November") }, 
    string{ StringAndLength("December") } 
};

//
// Functions
//

#if DEBUG
    #define Assert(Expression) if (!(Expression)) { Print("ASSERT Line %d", __LINE__); __builtin_trap(); }
    
    #define Print(Message, ...) PrintInternal(Message, ##__VA_ARGS__)
    internal void PrintInternal(char *Message, ...)
    {
        //
        // Print the debug message
        //
        
        va_list List;
        va_start(List, Message);
        
        __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG, Message, List);
        
        va_end(List);
    }
    
    #define Error(Message, ...) ErrorInternal(Message, ##__VA_ARGS__)
    internal void ErrorInternal(char *Message, ...)
    {
        //
        // Print the error message
        //
        
        va_list List;
        va_start(List, Message);
        
        __android_log_vprint(ANDROID_LOG_ERROR, LOG_TAG, Message, List);
        Print("ERROR: errno %d", errno);
        
        va_end(List);
        
        // End the activity
        ANativeActivity_finish(GLOBAL_Activity);
    }
#else
    #define Assert(Expression)
    #define Print(Message ...)
    #define Error(Message ...)
#endif

internal inline u8 *ArenaAllocEx(memory_arena *Arena, smm Count, smm ElementSize)
{
    Assert(Arena != nullptr);
    Assert(Count >= 1);
    Assert(ElementSize >= 1);
    
    // Allocation
    smm Size = Count * ElementSize;
    u8 *Result = Arena->Data + Arena->Taken;
    Arena->Taken += Size;
    
    // Because stores/loads across cache lines are very slow we need to align
    // aligned 1 cycle VS unaligned 12 cycles, this isn't always like this
    smm Alignment = Min(ElementSize, 64);
    Assert((Alignment % ElementSize) == 0); // Do not allow elements crossing cache line boundary
    
    // Alignment
    smm Address = (smm)Result; // Cannout RoundUp pointer* types
    smm Padding = RoundUp(Address, Alignment) - Address;
    Result       += Padding;
    Arena->Taken += Padding;
    
    // Range check
    Assert(Arena->Taken <= Arena->Size);
    
    return Result;
}

internal inline memory_arena ArenaCreate(memory_arena *Arena, smm Size)
{
    memory_arena Result;
    Result.Data  = ArenaAllocEx(Arena, Size, sizeof(u8));
    Result.Size  = Size;
    Result.Taken = 0;
    
    return Result;
}

#define ArenaAlloc(Arena, Type, Count)\
((Type *)ArenaAllocEx((Arena), (Count), sizeof(Type)))

internal inline smm GetDaysInMonth(smm Month, smm IsLeapYear)
{
    smm Result = DaysInMonthInternal[Month];
    
    // Handle February leap year, month is 0-11, so 1 means February
    Result += (Month == 1) * IsLeapYear;
    
    return Result;
}

internal inline void MiliSleep(smm Miliseconds)
{
    Assert(Miliseconds < 1000);
    
    timespec Time;
    Time.tv_sec  = 0;
    Time.tv_nsec = Miliseconds * 1000000;
    
    if (nanosleep(&Time, nullptr) == -1)
    {
        Print("WARN: ------ nanosleep failed");
    }
}

internal inline void SecSleep(smm Seconds)
{
    timespec Time;
    Time.tv_sec  = Seconds;
    Time.tv_nsec = 0;
    
    if (nanosleep(&Time, nullptr) == -1)
    {
        Print("WARN: ------ nanosleep failed");
    }
}

internal inline smm GetNanosecondsElapsed(timespec BeginClockTime, timespec *EndClockTime)
{
    if (clock_gettime(CLOCK_MONOTONIC_RAW, EndClockTime) == -1)
    {
        Error("clock_gettime failed");
    }
    
    smm Result = ((EndClockTime->tv_sec  - BeginClockTime.tv_sec) * SECONDS_TO_NANOSECONDS + 
                  (EndClockTime->tv_nsec - BeginClockTime.tv_nsec));
    
    return Result;
}

//
// JNI Functions
//

internal f32 JNIGetXDPI()
{
    JNIEnv *Env = GLOBAL_Activity->env;
    (*GLOBAL_Activity->vm)->AttachCurrentThread(GLOBAL_Activity->vm, &Env, nullptr);
    
    jclass ActivityClass = (*Env)->FindClass(Env, "android/app/NativeActivity");
    JNIAssert(ActivityClass);
    
    jmethodID GetWindowManager = (*Env)->GetMethodID(Env, ActivityClass, "getWindowManager", "()Landroid/view/WindowManager;"); 
    JNIAssert(GetWindowManager);
    
    jobject WindowManager = (*Env)->CallObjectMethod(Env, GLOBAL_Activity->clazz, GetWindowManager);
    JNIAssert(WindowManager);
    
    jclass WindowManagerClass = (*Env)->FindClass(Env, "android/view/WindowManager");
    JNIAssert(WindowManagerClass);
    
    jmethodID GetDefaultDisplay = (*Env)->GetMethodID(Env, WindowManagerClass, "getDefaultDisplay", "()Landroid/view/Display;");
    JNIAssert(GetDefaultDisplay);
    
    jobject Display = (*Env)->CallObjectMethod(Env, WindowManager, GetDefaultDisplay);
    JNIAssert(Display);
    
    jclass DisplayClass = (*Env)->FindClass(Env, "android/view/Display");
    JNIAssert(DisplayClass);
    
    jclass DisplayMetricsClass = (*Env)->FindClass(Env, "android/util/DisplayMetrics");
    JNIAssert(DisplayMetricsClass);
    
    jmethodID DisplayMetricsConstructor = (*Env)->GetMethodID(Env, DisplayMetricsClass, "<init>", "()V");
    JNIAssert(DisplayMetricsConstructor);
    
    jobject DisplayMetrics = (*Env)->NewObject(Env, DisplayMetricsClass, DisplayMetricsConstructor);
    JNIAssert(DisplayMetrics);
    
    jmethodID GetMetrics = (*Env)->GetMethodID(Env, DisplayClass, "getMetrics", "(Landroid/util/DisplayMetrics;)V");
    JNIAssert(GetMetrics);
    
    (*Env)->CallVoidMethod(Env, Display, GetMetrics, DisplayMetrics);
    
    jfieldID FieldXDPI = (*Env)->GetFieldID(Env, DisplayMetricsClass, "xdpi", "F");
    JNIAssert(FieldXDPI);
    
    // This is measured in Pixels Per Inch
    f32 XDPI = (*Env)->GetFloatField(Env, DisplayMetrics, FieldXDPI);
    Print("XDPI: %f", XDPI);
    
    (*GLOBAL_Activity->vm)->DetachCurrentThread(GLOBAL_Activity->vm);
    
    return XDPI;
}

//
// Utility
//

internal inline smm BitScanForwardU32(smm Value)
{
    #if defined(__clang__)
        smm Result = __builtin_clz(__builtin_arm_rbit(Value));
    #else
        #error BitScanForwardU32
    #endif
    
    return Result;
}

internal inline smm GetNLZ(smm X) // Number Of Leading Zeroes
{
    smm Y, M, N;
    Y = -(X >> 16);     // If left half of X is 0,
    M = (Y >> 16) & 16; // set N = 16. If left half
    N = 16 - M;         // is nonzero, set N = 0 and
    X = X >> M;         // shift X right 16.
                        // Now X Is of the form 0000xxxx.
    Y = X - 0x100;      // If positions 8-15 are 0,
    M = (Y >> 16) & 8;  // add 8 to N and shift X left 8.
    N = N + M;
    X = X << M;
    Y = X - 0x1000;     // If positions 12-15 are 0,
    M = (Y >> 16) & 4;  // add 4 to N and shift X left 4.
    N = N + M;
    X = X << M;
    Y = X - 0x4000;     // If positions 14-15 are 0,
    M = (Y >> 16) & 2;  // add 2 to N and shift X left 2.
    N = N + M;
    X = X << M;
    Y = X >> 14;        // Set Y = 0, 1, 2, or 3.
    M = Y & ~(Y >> 1);  // Set M = 0, 1, 2, or 2 resp.
    
    smm Result = N + 2 - M;
    
    return Result;
}

internal inline s32 RoundUpToPowerOfTwo(s32 Value)
{
    return 1 << (32 - GetNLZ(Value - 1));
}

internal inline s32 RoundDownToPowerOfTwo(s32 Value)
{
    return 1 << (31 - GetNLZ(Value));
}

//
// Glyph Rasterization & TTF parsing
//

#include "truetype.h"

//
// OpenGL stuff
//

internal GLuint OGLCompileShader(GLenum ShaderType, char *ShaderSource, GLint ShaderSize)
{
    GLuint Shader = glCreateShader(ShaderType);
    if (Shader == 0)
    {
        Error("glCreateShader failed");
    }
    
    glShaderSource(Shader, 1, (GLchar **)&ShaderSource, &ShaderSize);
    GLAssert();
    
    glCompileShader(Shader);
    
    GLint CompileStatus;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &CompileStatus);
    if (CompileStatus == GL_FALSE)
    {
        GLsizei ErrorLength;
        glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &ErrorLength);
        
        // ErrorLength is 0 if there is no shader log
        if (ErrorLength > 0)
        {
            char ErrorBuffer[4 * 1024];
            Assert(ErrorLength < sizeof(ErrorBuffer));
            
            glGetShaderInfoLog(Shader, ErrorLength, &ErrorLength, (GLchar *)ErrorBuffer); GLAssert();
            Print("%.*s", ErrorLength, ErrorBuffer);
        }
        
        Error("glCompileShader failed");
    }
    
    return Shader;
}

internal void CommonInitOpenGL()
{
    //
    // Texture
    //
    
    glBindTexture(GL_TEXTURE_2D, TEXNAME_MAIN);                                 GLAssert();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL,   0);                 GLAssert();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,    0);                 GLAssert();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,   GL_LINEAR);         GLAssert();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,   GL_LINEAR);         GLAssert();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,       GL_CLAMP_TO_EDGE);  GLAssert();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,       GL_CLAMP_TO_EDGE);  GLAssert();
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 
        TEXTURE_SIZE, TEXTURE_SIZE, 0, 
        PLATFORM_OPENGL_UPLOAD_FORMAT, GL_UNSIGNED_BYTE, nullptr
    );  GLAssert();
    
    // White pixel for solid Color texturing
    u32 WhitePixel = 0xFFFFFFFF;
    glTexSubImage2D(
        GL_TEXTURE_2D, 0, 
        TEXTURE_SIZE - 1, TEXTURE_SIZE - 1, 1, 1,
        PLATFORM_OPENGL_UPLOAD_FORMAT, GL_UNSIGNED_BYTE, &WhitePixel
    );
    
    //
    // Compile the main vertex and fragment shaders
    //
    
    GLuint MainVertexShader = OGLCompileShader(GL_VERTEX_SHADER, StringAndLength(
R"RAWSTR(#version 100
precision mediump float;

attribute vec2 vPosition;
attribute vec2 vTexture;
attribute vec4 vColor;
varying vec2 Texture;
varying vec4 Color;
uniform vec2 MetersToNDC;

void main()
{
    gl_Position = vec4(vPosition * MetersToNDC - 1.0, 0.0, 1.0);
    Texture     = vTexture;
    Color      = vColor;
}

)RAWSTR"));
    
    GLuint MainFragmentShader = OGLCompileShader(GL_FRAGMENT_SHADER, StringAndLength(
R"RAWSTR(#version 100
precision mediump float;

varying vec2 Texture;
varying vec4 Color;
uniform sampler2D Sampler;

void main()
{
    gl_FragColor = texture2D(Sampler, Texture.st);
    gl_FragColor *= Color;
}

)RAWSTR"));
    
    GLuint MainShader = glCreateProgram();              GLAssert();
    glAttachShader(MainShader, MainVertexShader);       GLAssert();
    glAttachShader(MainShader, MainFragmentShader);     GLAssert();
    glBindAttribLocation(MainShader, 0, "vPosition");   GLAssert();
    glBindAttribLocation(MainShader, 1, "vTexture");    GLAssert();
    glBindAttribLocation(MainShader, 2, "vColor");      GLAssert();
    glLinkProgram(MainShader);                          GLAssert();
    
    GLint LinkStatus;
    glGetProgramiv(MainShader, GL_LINK_STATUS, &LinkStatus);
    if (LinkStatus == GL_FALSE)
    {
        Error("glLinkProgram");
    }
    
    glValidateProgram(MainShader);
    GLint ValidationStatus;
    glGetProgramiv(MainShader, GL_VALIDATE_STATUS, &ValidationStatus);
    if (ValidationStatus == GL_FALSE)
    {
        Error("glValidateProgram");
    }
    
    glUseProgram(MainShader); GLAssert();
    
    GLint MetersToNDCLocation = glGetUniformLocation(MainShader, "MetersToNDC");
    if (MetersToNDCLocation == -1)
    {
        Error("glGetUniformLocation(MetersToNDC)");
    }
    
    // The sampler will not be changed, always 0
    GLint SamplerLocation = glGetUniformLocation(MainShader, "Sampler");
    if (SamplerLocation == -1)
    {
        Error("glGetUniformLocation(Sampler)");
    }
    glUniform1i(SamplerLocation, 0);
    
    render_context *RenderContext = &GLOBAL_MainState->RenderContext;
    
    f32 MetersToPixels = 1.0f;
    v2 MetersToNDC = MetersToPixels * 2.0f / v2{ (f32)RenderContext->Width, (f32)RenderContext->Height };
    glUniform2f(MetersToNDCLocation, MetersToNDC.X, MetersToNDC.Y);
    
    // Store the shader information for later
    RenderContext->MainShader           = MainShader;
    RenderContext->MetersToNDCLocation  = MetersToNDCLocation;
    
    Print("MainShader %d", GLOBAL_MainState->RenderContext.MainShader);
    Print("Location   %d", GLOBAL_MainState->RenderContext.MetersToNDCLocation);
    
    //
    // Vertex buffer
    //
    
    glBindBuffer(GL_ARRAY_BUFFER, BUFNAME_VERTEX);                                                  GLAssert();
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES_COUNT * sizeof(vertex), nullptr, GL_DYNAMIC_DRAW);   GLAssert();
    
    glVertexAttribPointer(0, floatsof(vertex, Position), GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, Position));   GLAssert();
    glVertexAttribPointer(1, floatsof(vertex, Texture),  GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, Texture));    GLAssert();
    glVertexAttribPointer(2, floatsof(vertex, Color),    GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, Color));      GLAssert();
    glEnableVertexAttribArray(0);   GLAssert();
    glEnableVertexAttribArray(1);   GLAssert();
    glEnableVertexAttribArray(2);   GLAssert();
    
    //
    // Index buffer
    //
    
    u16 Indices[(MAX_VERTICES_COUNT / 4) * 6];
    u16 IndexBase = 0;
    for (size_t Index = 0; Index < GetArrayCount(Indices); Index += 6)
    {
        Indices[Index + 0] = IndexBase + 0;
        Indices[Index + 1] = IndexBase + 1;
        Indices[Index + 2] = IndexBase + 2;
        
        Indices[Index + 3] = IndexBase + 3;
        Indices[Index + 4] = IndexBase + 2;
        Indices[Index + 5] = IndexBase + 1;
        
        IndexBase += 4;
    }
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BUFNAME_INDEX);                                   GLAssert();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);        GLAssert();
    
    //
    // Blend
    //
    
    glEnable(GL_BLEND); GLAssert();
    // @Note Premultiplied alpha
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);    GLAssert();
    
    // Clear
    v4 ClearColor = COL_BACK;
    glClearColor(ClearColor.Red, ClearColor.Green, ClearColor.Blue, ClearColor.Alpha);      GLAssert();
}

internal void AndroidUpdateWindowSurface()
{
    render_context *RenderContext = &GLOBAL_MainState->RenderContext;
    
    if (ANativeWindow_setBuffersGeometry(RenderContext->Window, 0, 0, RenderContext->Format) < 0)
    {
        Error("ANativeWindow_setBuffersGeometry failed");
    }
    
    // Surface
    Print("Creating surface!\n");
    RenderContext->Surface = eglCreateWindowSurface(RenderContext->Display, RenderContext->Config, RenderContext->Window, nullptr);
    if (RenderContext->Surface == EGL_NO_SURFACE)
    {
        Error("eglCreateWindowSurface failed");
    }
    
    if (eglQuerySurface(RenderContext->Display, RenderContext->Surface, EGL_WIDTH, &RenderContext->Width) == EGL_FALSE)
    {
        Error("eglQuerySurface Width failed");
    }
    if (eglQuerySurface(RenderContext->Display, RenderContext->Surface, EGL_HEIGHT, &RenderContext->Height) == EGL_FALSE)
    {
        Error("eglQuerySurface Height failed");
    }
    
    Print("Width %d, Height %d", RenderContext->Width, RenderContext->Height);
    
    //
    // Make current
    //
    
    if (eglMakeCurrent(RenderContext->Display, 
        RenderContext->Surface, RenderContext->Surface, 
        RenderContext->Context) == EGL_FALSE)
    {
        s32 ErrorNumber = eglGetError();
        Print("eglMakeCurrent failed %d", ErrorNumber);
    }
        
    //
    // Set the viewport to the new dimensions
    //
    
    glViewport(0, 0, RenderContext->Width, RenderContext->Height);  GLAssert();
    
    //
    // Set values
    //
    
    GLOBAL_MainState->MetersToPixels = 1.0f;
    GLOBAL_MainState->PixelsToMeters = 1.0f;
    GLOBAL_MainState->Width  = GLOBAL_MainState->PixelsToMeters * RenderContext->Width;
    GLOBAL_MainState->Height = GLOBAL_MainState->PixelsToMeters * RenderContext->Height;
}

internal void AndroidInitOpenGL()
{
    Print("Initializing EGL state...\n");
    render_context *RenderContext = &GLOBAL_MainState->RenderContext;
    
    //
    // Display
    //
    
    RenderContext->Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (RenderContext->Display == EGL_NO_DISPLAY)
    {
        Error("eglGetDisplay failed");
    }
    
    if (eglInitialize(RenderContext->Display, 0, 0) == EGL_FALSE)
    {
        Error("eglInitialize failed");
    }
    
    //
    // Config
    //
    
    EGLint Attributes[] = 
    {
        EGL_SURFACE_TYPE,       EGL_WINDOW_BIT, 
        EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT, 
        EGL_ALPHA_SIZE,         8, 
        EGL_BLUE_SIZE,          8, 
        EGL_GREEN_SIZE,         8, 
        EGL_RED_SIZE,           8, 
        EGL_NONE, 
    };
    
    EGLint ConfigCount;
    if (eglChooseConfig(RenderContext->Display, Attributes, &RenderContext->Config, 1, &ConfigCount) == EGL_FALSE)
    {
        Error("eglChooseConfig failed");
    }
    
    //
    // Context
    //
    
    EGLint ContextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    RenderContext->Context = eglCreateContext(RenderContext->Display, RenderContext->Config, nullptr, ContextAttributes);
    if (RenderContext->Context == EGL_FALSE)
    {
        Error("eglCreateContext failed");
    }
    
    //
    // Set format of window buffer
    //
    
    if (eglGetConfigAttrib(RenderContext->Display, RenderContext->Config, 
        EGL_NATIVE_VISUAL_ID, &RenderContext->Format) == EGL_FALSE)
    {
        Error("eglGetConfigAttrib failed");
    }
    
    // Wait for us to Android to give us the Window
    while (GLOBAL_PendingWindow == nullptr)
    {
        Print("Waiting for window!");
        MiliSleep(5);
    }
    Print("Got window %d, previously was %d", GLOBAL_PendingWindow, RenderContext->Window);
    RenderContext->Window = GLOBAL_PendingWindow;
    
    // Initialize the window surface
    AndroidUpdateWindowSurface();
}

internal inline void AndroidReadFontFile()
{
    Print("opening Roboto-Regular.ttf");
    
    // Open the file
    smm FontFile = open("/system/fonts/Roboto-Regular.ttf", O_RDONLY);
    if (FontFile == -1)
    {
        Print("open font file failed, error %d", errno);
    }
    
    // Get the file size
    stat FontStat;
    if (fstat(FontFile, &FontStat) == -1)
    {
        Print("fstat font file failed, error %d", errno);
    }
    Print("File size is %lld KB", FontStat.st_size / 1024);
    
    font *Font = &GLOBAL_MainState->Font;
    Font->FileSize = FontStat.st_size;
    
    // Allocate the memory for the file
    Font->FileData = ArenaAlloc(&Font->Arena, u8, Font->FileSize);
    
    // Read the entire file
    ssize_t BytesRead = read(FontFile, Font->FileData, Font->FileSize);
    if (BytesRead == -1)
    {
        Print("read font file failed, error %d", errno);
        Error("read failed");
    }
    if (BytesRead != Font->FileSize)
    {
        Print("WARN: read font file missing bytes, %d / %d", BytesRead, Font->FileSize);
    }
}

internal inline void DrawQuad(v2 PosMin, v2 PosMax, v2 TexMin, v2 TexMax, v4 Color)
{
    vertex *VertexBuffer = GLOBAL_MainState->VertexBuffer;
    smm VertexCount  = GLOBAL_MainState->VertexCount;
    
    Assert(VertexCount + 4 <= MAX_VERTICES_COUNT);
    
    // Top Left
    VertexBuffer[VertexCount + 0] = 
        vertex{ v2{ PosMin.X, PosMin.Y }, v2{ TexMin.X, TexMax.Y }, Color };
    // Bottom Left
    VertexBuffer[VertexCount + 1] = 
        vertex{ v2{ PosMin.X, PosMax.Y }, v2{ TexMin.X, TexMin.Y }, Color };
    // Top Right
    VertexBuffer[VertexCount + 2] = 
        vertex{ v2{ PosMax.X, PosMin.Y }, v2{ TexMax.X, TexMax.Y }, Color };
    // Bottom Right
    VertexBuffer[VertexCount + 3] = 
        vertex{ v2{ PosMax.X, PosMax.Y }, v2{ TexMax.X, TexMin.Y }, Color };
    
    GLOBAL_MainState->VertexCount += 4;
}

internal inline void DrawGlyph(v2 Position, smm LoadedIndex, v4 Color)
{
    font *Font = &GLOBAL_MainState->Font;
    f32 TextureScale = (1.0f / TEXTURE_SIZE);
    
    v2 TexMin = v2{
        ((f32)((LoadedIndex &  Font->LoadedGlyphMask)  * FONT_CELL_WIDTH)  + 0.5f) * TextureScale, 
        ((f32)((LoadedIndex >> Font->LoadedGlyphShift) * FONT_CELL_HEIGHT) + 0.5f) * TextureScale, 
    };
    
    v2 Dimensions = v2{
        (f32)Font->LoadedGlyphsData[LoadedIndex].Width, 
        (f32)Font->LoadedGlyphsData[LoadedIndex].Height, 
    };
    
    v2 PosMin = v2{ 
        Position.X + (f32)Font->LoadedGlyphsData[LoadedIndex].OffsetX, 
        Position.Y + (f32)Font->LoadedGlyphsData[LoadedIndex].OffsetY, 
    };
    
    DrawQuad(
        PosMin, PosMin + (GLOBAL_MainState->PixelsToMeters  * Dimensions), 
        TexMin, TexMin + (TextureScale                      * (Dimensions - v2{ 1.0f, 1.0f })), 
        Color
    );
}

internal v2 DrawText(v2 Position, char *Text, smm Size, v4 Color)
{
    font *Font = &GLOBAL_MainState->Font;
    
    f32 LineStartX = Position.X;
    for (smm ByteIndex = 0; ByteIndex < Size;)
    {
        smm LoadedIndex = 0; // Default to null glyph
        umm CharacterUTF32 = UTF8ToCodepoint((u8 *)Text, &ByteIndex);
        if (CharacterUTF32 == '\n')
        {
            Position.X = LineStartX;
            Position.Y += Font->FontSize * GLOBAL_MainState->PixelsToMeters;
            continue;
        }
        
        if (CharacterUTF32 != ' ')
        {
            smm GlyphID = UnicodeToGlyphID(CharacterUTF32);
            // Print("U+%04X, GlyphID: %d", (u32)CharacterUTF32, (u32)GlyphID);
            
            LoadedIndex = RasterizeGlyph(GlyphID);
            DrawGlyph(Position, LoadedIndex, Color);
        }
        
        Position.X += Font->LoadedGlyphsData[LoadedIndex].Advance * GLOBAL_MainState->PixelsToMeters;
    }
    
    return Position;
}

internal void *AndroidMain(void *Param)
{
    Print("Thread start Param %d", Param);
    #if DEBUG
    SecSleep(2);
    #endif
    
    //
    // Arena creation
    //
    
    memory_arena *PermanentArena = &GLOBAL_MainState->PermanentArena;
    PermanentArena->Data  = (u8 *)GLOBAL_MainState + sizeof(main_state);
    PermanentArena->Size  = PERMANENT_ARENA_SIZE;
    PermanentArena->Taken = 0;
    
    memory_arena *TransientArena = &GLOBAL_MainState->TransientArena;
    TransientArena->Data  = PermanentArena->Data + PermanentArena->Size;
    TransientArena->Size  = TRANSIENT_ARENA_SIZE;
    TransientArena->Taken = 0;
    
    GLOBAL_MainState->Font.Arena = ArenaCreate(PermanentArena, MB(1));
    
    //
    // Read the font file
    //
    
    AndroidReadFontFile();
    ParseTTF();
    font *Font = &GLOBAL_MainState->Font;
    
    //
    // XDPI
    //
    
    f32 XDPI            = JNIGetXDPI();
    f32 PixelsPerInch   = XDPI;
    f32 PixelsPerMm     = PixelsPerInch * 25.4f;
    f32 MmPerPixel      = (1.0f / PixelsPerInch) * 25.4f;
    
    //
    // Get the day and month
    //
    
    tm LocalTime = {};
    time_t Time = time(nullptr);
    if (localtime_r(&Time, &LocalTime) == nullptr)
    {
        Print("localtime_r failed");
    }
    
    // @Todo Update this when changing years
    smm IsLeapYear = !((LocalTime.tm_year % 100) ? (LocalTime.tm_year % 4) : (LocalTime.tm_year % 16));
    
    //
    // OpenGL
    //
    
    AndroidInitOpenGL();
    CommonInitOpenGL();
    
    GLOBAL_MainState->MetersToPixels = 1.0f;
    GLOBAL_MainState->PixelsToMeters = 1.0f;
    GLOBAL_MainState->Width  = GLOBAL_MainState->RenderContext.Width  * GLOBAL_MainState->PixelsToMeters;
    GLOBAL_MainState->Height = GLOBAL_MainState->RenderContext.Height * GLOBAL_MainState->PixelsToMeters;
    GLOBAL_MainState->FramesPerSecond = 60.0f; // @Todo Calculate the FPS Target
    GLOBAL_MainState->SecondsPerFrame = 1.0f / GLOBAL_MainState->FramesPerSecond;
    
    // Status bar and navigation bar
    GLOBAL_MainState->StatusBarHeight = 48.0f * GLOBAL_MainState->PixelsToMeters;
    
    // @Todo One big glTexImage2D call to update all the initial state
    //
    // Maybe keep a local copy on the CPU in case a big chunk needs to be updated
    // and in that case the whole texture can be uploaded instead of subregion for a
    // unknown/unmeasured performance gain. this is probably a microoptimization
    //
    // This would also allow RasterizeGlyph to be called before OpenGL initialization
    // In that case it could be API agnostic, because texture upload can be deffered
    
    RasterizeGlyph(0); // ntdef glyph
    RasterizeGlyph(UnicodeToGlyphID('0'));
    RasterizeGlyph(UnicodeToGlyphID('1'));
    RasterizeGlyph(UnicodeToGlyphID('2'));
    RasterizeGlyph(UnicodeToGlyphID('3'));
    RasterizeGlyph(UnicodeToGlyphID('4'));
    RasterizeGlyph(UnicodeToGlyphID('5'));
    RasterizeGlyph(UnicodeToGlyphID('6'));
    RasterizeGlyph(UnicodeToGlyphID('7'));
    RasterizeGlyph(UnicodeToGlyphID('8'));
    RasterizeGlyph(UnicodeToGlyphID('9'));
    
    // Begin clock time
    timespec BeginClockTime;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &BeginClockTime) == -1)
    {
        Error("clock_gettime BeginClockTime failed");
    }
    
    for (;;)
    {
        while (GLOBAL_ActiveFlags != ACTIVE_INTERACTIVE)
        {
            MiliSleep(50);
        }
        
        //
        // Input
        //
        
        bmm PointerClicked = false;
        
        // Scroll
        f32 ScrollXTarget = -GLOBAL_MainState->TabIndex * GLOBAL_MainState->Width;
        GLOBAL_Scroll.X = Lerp(GLOBAL_Scroll.X, ScrollXTarget, 0.1f * (GLOBAL_GestureType != GESTURE_SCROLL));
        GLOBAL_Scroll.Y += GLOBAL_ScrollYSpeed;
        GLOBAL_ScrollYSpeed = Lerp(GLOBAL_ScrollYSpeed, 0.0f, 0.08f);
        
        //
        // Message Loop
        //
        
        f32 ClientStartY = TAB_MENU_HEIGHT;
        f32 ClientEndY   = GLOBAL_MainState->Height - GLOBAL_MainState->StatusBarHeight;
        
        AInputQueue *InputQueue = GLOBAL_InputQueue;
        if (InputQueue != nullptr)
        {
            AInputEvent *Event;
            while (AInputQueue_getEvent(InputQueue, &Event) >= 0)
            {
                if (AInputQueue_preDispatchEvent(InputQueue, Event))
                {
                    continue;
                }
                
                smm Type = AInputEvent_getType(Event);
                switch (Type)
                {
                    case AINPUT_EVENT_TYPE_MOTION:
                    {
                        smm ActionField = AMotionEvent_getAction(Event);
                        smm Action          = ActionField & AMOTION_EVENT_ACTION_MASK;
                        smm PointerIndex    = 
                            (ActionField & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> 
                            AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
                        
                        #if 0
                        char *ActionNames[] = 
                        {
                            "DOWN",
                            "UP", 
                            "MOVE", 
                            "CANCEL", 
                            "OUTSIDE", 
                            "POINTER_DOWN", 
                            "POINTER_UP", 
                            "HOVER_MOVE", 
                            "SCROLL", 
                            "HOVER_ENTER", 
                            "HOVER_EXIT", 
                            "BUTTON_PRESS", 
                            "BUTTON_RELEASE", 
                        };
                        
                        Print("%d: Action (%d) %s", PointerIndex, Action, ActionNames[Action]);
                        #endif
                        
                        v2 PointerPoint;
                        PointerPoint.X = AMotionEvent_getX(Event, PointerIndex);
                        PointerPoint.Y = AMotionEvent_getY(Event, PointerIndex);
                        
                        // @Note Change Pointer Y from Top-Left (0, 0) to Bottom-Left (0, 0)
                        
                        // @Todo is EGL Width Height here screen position or client position for cursor
                        // Fix it if it's client
                        PointerPoint.Y = GLOBAL_MainState->RenderContext.Height - PointerPoint.Y;
                        
                        f32 TouchSlopSq = Square(10.0f); // @Todo Do not hardcode the touch slop
                        
                        switch (Action)
                        {
                            case AMOTION_EVENT_ACTION_DOWN:
                            {
                                GLOBAL_GestureType = GESTURE_TOUCH;
                                GLOBAL_GestureStartTime = GLOBAL_MainState->Frame;
                                GLOBAL_GestureStart = PointerPoint;
                                GLOBAL_GestureFocus = PointerPoint;
                                
                                GLOBAL_GestureCanScroll = 
                                    PointInRange(GLOBAL_GestureStart.Y, ClientStartY, ClientEndY);
                            }
                            break;
                            
                            case AMOTION_EVENT_ACTION_MOVE:
                            {
                                GLOBAL_GestureMovedSq += DistanceBetweenTwoPointsSq(GLOBAL_GestureFocus, GLOBAL_GestureStart);
                                v2 ScrollAmount = PointerPoint - GLOBAL_GestureFocus;
                                
                                if ((GLOBAL_GestureType == GESTURE_TOUCH) && 
                                    (GLOBAL_GestureMovedSq > TouchSlopSq) &&
                                    GLOBAL_GestureCanScroll)
                                {
                                    GLOBAL_GestureType      = GESTURE_SCROLL;
                                    GLOBAL_ScrollDir        = (Abs(ScrollAmount.Y) > Abs(ScrollAmount.X));
                                    GLOBAL_ScrollYSpeed     = 0.0f;
                                }
                                
                                if (GLOBAL_GestureType == GESTURE_SCROLL)
                                {
                                    // Velocity
                                    f32 DirScroll = ScrollAmount.E[GLOBAL_ScrollDir];
                                    GLOBAL_Scroll.E[GLOBAL_ScrollDir] += DirScroll;
                                    GLOBAL_ScrollPrevAmount = DirScroll;
                                }
                                
                                GLOBAL_GestureFocus = PointerPoint;
                            }
                            break;
                            
                            case AMOTION_EVENT_ACTION_UP:
                            {
                                if (GLOBAL_GestureType == GESTURE_SCROLL)
                                {
                                    // Scroll velocity
                                    smm ElapsedFrames = GLOBAL_MainState->Frame - GLOBAL_GestureStartTime;
                                    f32 ElapsedElapsed = ElapsedFrames * GLOBAL_MainState->SecondsPerFrame;
                                    if ((GLOBAL_ScrollDir == 1) &&
                                        (ElapsedElapsed > (30.0f / 1000.0f))) // miliseconds
                                    {
                                        // f32 MaxVelocity = 50.0f; // pixels per frame
                                        GLOBAL_ScrollYSpeed += GLOBAL_ScrollPrevAmount;
                                    }
                                    else if (GLOBAL_ScrollDir == 0)
                                    {
                                        f32 ScrollAmount = GLOBAL_GestureStart.X - PointerPoint.X;
                                        f32 SlideLeewayPercent = 0.2f;
                                        if (Abs(ScrollAmount) > (0.5f - SlideLeewayPercent) * GLOBAL_MainState->Width)
                                        {
                                            smm DirX = Sign(ScrollAmount);
                                            Print("Start %f, Amount %f, Abs %f, Dir %d", GLOBAL_GestureStart.X, ScrollAmount, Abs(ScrollAmount), DirX);
                                            GLOBAL_MainState->TabIndex = 
                                                Clamp(GLOBAL_MainState->TabIndex + DirX, 0, (TAB_MENU_COUNT - 1));
                                        }
                                    }
                                }
                                else if (GLOBAL_GestureType == GESTURE_TOUCH)
                                {
                                    // Touch
                                    PointerClicked = true;
                                }
                                
                                GLOBAL_GestureType      = GESTURE_NONE;
                                GLOBAL_GestureStartTime = 0;
                                GLOBAL_GestureCanScroll = false;
                                GLOBAL_GestureMovedSq   = 0;
                                GLOBAL_GestureFocus     = v2{ 0.0f, 0.0f };
                                
                                GLOBAL_ScrollPrevAmount = 0.0f;
                            }
                            break;
                        }
                    }
                    break;
                    
                    case AINPUT_EVENT_TYPE_KEY:
                    {
                        Print("Key");
                    }
                    break;
                }
                
                smm Handled = false;
                AInputQueue_finishEvent(InputQueue, Event, Handled);
            }
        }
        
        bmm PointerPressed = (GLOBAL_GestureType != GESTURE_NONE);
        
        //
        // Logic
        //
        
        //
        // Draw Calendar
        //
        
        f32 HorMargin   = 16.0f * GLOBAL_MainState->PixelsToMeters;
        f32 BoxPadding  = 08.0f * GLOBAL_MainState->PixelsToMeters;
        
        v2 Position = GLOBAL_Scroll;
        Position.Y += GLOBAL_MainState->Height - GLOBAL_MainState->StatusBarHeight;
        
        // Boxes size
        v2 BoxDim;
        BoxDim.X = (GLOBAL_MainState->Width - (BoxPadding * (7 - 1)) - (HorMargin * 2)) / 7;
        BoxDim.Y = BoxDim.X;
        
        // Unix yeardays are [0, 365]
        smm DaysToFirstOfMonth = LocalTime.tm_yday;
        // smm DaysToFirstOfMonth = LocalTime.tm_mday - 1;
        
        f32 StartX = Position.X + HorMargin;
        for (smm MonthIndex = 0; MonthIndex < GetArrayCount(MonthNames); ++MonthIndex)
        {
            // Starting day calculation
            smm WeekDayOfFirst = (LocalTime.tm_wday - DaysToFirstOfMonth) % 7;
            if (WeekDayOfFirst < 0) { WeekDayOfFirst += 7; }
            
            // Convert from UNIX weekday (1, 2, 3, 4, 5, 6, 0) to (1, 2, 3, 4, 5, 6, 7)
            smm DayInMonth = 2 - ((WeekDayOfFirst != 0) ? WeekDayOfFirst : 7);
            
            // @Note This works ALL THE TIME because January and December
            // both have 31 days in the month and we don't need to underflow
            smm PrevDays = GetDaysInMonth(Max(MonthIndex - 1, 0),   IsLeapYear);
            smm CurrDays = GetDaysInMonth(MonthIndex,               IsLeapYear);
            
            // Titlebar
            f32 TitleBarMargin = GLOBAL_MainState->PixelsToMeters * 16.0f;
            Position.X = StartX;
            Position.Y -= TitleBarMargin + GLOBAL_MainState->Font.FontSize;
            
            string *MonthName = &MonthNames[MonthIndex];
            if (LineInRange(
                Position.Y, Position.Y + Font->FontSize * GLOBAL_MainState->PixelsToMeters,
                ClientStartY, ClientEndY))
            {
                DrawText(Position, MonthName->Text, MonthName->Length, COL_FORE);
            }
            
            Position.Y -= TitleBarMargin;
            for (smm VerIndex = 0; VerIndex < 6; ++VerIndex)
            {
                Position.X = StartX;
                Position.Y -= BoxDim.Y + BoxPadding;
                
                bmm RowVisible = LineInRange(
                    Position.Y, Position.Y + BoxDim.Y, 
                    ClientStartY, ClientEndY);
                
                for (smm HorIndex = 0; HorIndex < 7; ++HorIndex)
                {
                    v4 Color = COL_BUTN;
                    if (PointerPressed && PointInRectangle(GLOBAL_GestureFocus, 
                        v4{ .BottomLeft = Position, .TopRight = Position + BoxDim }))
                    {
                        Color = COL_ACNT;
                    }
                    
                    // Box Drawing
                    if (RowVisible)
                    {
                        DrawQuad(
                            Position, Position + BoxDim, 
                            TEXPOS_SOLID, TEXPOS_SOLID, 
                            Color
                        );
                    }
                    
                    // Get the number
                    smm Number = DayInMonth;
                    if (Number <= 0)
                    {
                        Number += PrevDays;
                    }
                    else if (Number > CurrDays)
                    {
                        Number -= CurrDays;
                    }
                    ++DayInMonth;
                    
                    // Number drawing
                    smm Length = 1 + (Number >= 10);
                    
                    smm Digit1 = Number / 10;
                    smm Digit2 = Number % 10;
                    
                    // @Todo We are multiplying by PixelsToMeters in DrawText as well
                    // Consider maybe changing the values in LoadedGlyphsData to Meters?
                    
                    // @Note Add 1 to skip the ntdef symbol
                    loaded_glyph *GlyphDigits = Font->LoadedGlyphsData + 1;
                    f32 DigitWidth  = 
                        ((Number < 10) ? 
                            GlyphDigits[Digit2].Width : 
                            GlyphDigits[Digit1].Advance + GlyphDigits[Digit2].Width)
                    * GLOBAL_MainState->PixelsToMeters;
                    
                    f32 DigitHeight = Font->LoadedGlyphsData[1 + Digit2].Height * GLOBAL_MainState->PixelsToMeters;
                    v2 CenterOffset = 0.5f * v2{ BoxDim.X - DigitWidth, BoxDim.Y - DigitHeight};
                    
                    if (RowVisible)
                    {
                        char Text[2] = { (char)('0' + Digit1), (char)('0' + Digit2) };
                        DrawText(Position + CenterOffset, Text + (Number < 10), Length, COL_FORE);
                    }
                    
                    Position.X += BoxDim.X + BoxPadding;
                }
            }
            
            // Calculate DaysToFirstOfMonth for the next month
            DaysToFirstOfMonth += CurrDays + 1;
        }
        
        // Tab Menu Background
        DrawQuad(
            v2{ 0.0f, 0.0f }, 
            v2{ GLOBAL_MainState->Width, TAB_MENU_HEIGHT }, 
            TEXPOS_SOLID, TEXPOS_SOLID, 
            COL_BACK
        );
        
        // Tab Menu Separators
        f32 TabWidth = GLOBAL_MainState->Width * (1.0f / TAB_MENU_COUNT);
        for (smm TabIndex = 0; TabIndex < (TAB_MENU_COUNT - 1); ++TabIndex)
        {
            f32 LineX = (TabIndex + 1) * TabWidth;
            f32 HalfThickness = 5.0f * 0.5f;
            
            DrawQuad(
                v2{ LineX - HalfThickness, 0.0f }, 
                v2{ LineX + HalfThickness, TAB_MENU_HEIGHT }, 
                TEXPOS_SOLID, TEXPOS_SOLID, 
                COL_BUTN
            );
        }
        
        // Tab Menu Icons
        v2 TabPos = v2{ 0.0f, 0.0f };
        for (smm TabIndex = 0; TabIndex < TAB_MENU_COUNT; ++TabIndex)
        {
            // Center the icon
            v2 IconDim = v2{ 70.0f, 70.0f };
            v2 TabDim  = v2{ TabWidth, TAB_MENU_HEIGHT };
            v2 IconPos = TabPos + 0.5f * (TabDim - IconDim);
            
            DrawQuad(
                IconPos, IconPos + IconDim, 
                TEXPOS_SOLID, TEXPOS_SOLID, 
                (GLOBAL_MainState->TabIndex == TabIndex) ? COL_ACNT : COL_BUTN
            );
            
            TabPos.X += TabWidth;
        }
        
        // Status Bar cover
        DrawQuad(
            v2{ 0.0f, GLOBAL_MainState->Height - GLOBAL_MainState->StatusBarHeight  }, 
            v2{ GLOBAL_MainState->Width, GLOBAL_MainState->Height },
            TEXPOS_SOLID, TEXPOS_SOLID,
            COL_BACK
        );
        
        // Texture Atlas
        if (0)
        {
            DrawQuad(
                v2{ 50.0f, 50.0f }, v2{ 550.0f, 550.0f }, 
                v2{ 0.0f, 0.0f }, v2{ 1.0f, 1.0f }, 
                COL_WHITE
            );
        }
        
        //
        // Surface update
        //
        
        render_context *RenderContext = &GLOBAL_MainState->RenderContext;
        if (RenderContext->Window != GLOBAL_PendingWindow)
        {
            // Destroy previous surface
            if (eglMakeCurrent(RenderContext->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE)
            {
                Error("eglMakeCurrent failed");
            }
            
            if (eglDestroySurface(RenderContext->Display, RenderContext->Surface) == EGL_FALSE)
            {
                Error("eglDestroySurface failed");
            }
            
            // Update native window
            RenderContext->Window = GLOBAL_PendingWindow;
            
            // Create new surface
            if (RenderContext->Window != nullptr)
            {
                AndroidUpdateWindowSurface();
            }
        }
        
        //
        // Rendering
        //
        
        if (RenderContext->Window != nullptr)
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            GLsizeiptr VertexBufferSize = GLOBAL_MainState->VertexCount * sizeof(vertex);
            glBufferData(GL_ARRAY_BUFFER, VertexBufferSize, GLOBAL_MainState->VertexBuffer, GL_DYNAMIC_DRAW);
            GLenum VertexResult = glGetError();
            if (VertexResult != GL_NO_ERROR)
            {
                Error("glBufferData");
            }
            
            GLsizeiptr IndexCount = (GLOBAL_MainState->VertexCount / 4) * 6;
            glDrawElements(GL_TRIANGLES, IndexCount, GL_UNSIGNED_SHORT, nullptr);
            
            if (eglSwapBuffers(RenderContext->Display, RenderContext->Surface) == EGL_FALSE)
            {
                EGLint ErrorCode = eglGetError();
                if (ErrorCode == EGL_CONTEXT_LOST)
                {
                    Print("EGL Context has been lost");
                }
                else if (ErrorCode == EGL_BAD_SURFACE)
                {
                    Print("EGL Bad Surface");
                }
                else
                {
                    Print("eglSwapBuffers failed %d", ErrorCode);
                }
            }
        }
        
        //
        // Sleep
        //
        
        timespec BeginClockTime = GLOBAL_MainState->BeginClockTime;
        timespec EndClockTime;
        
        smm NanosecondsElapsed = GetNanosecondsElapsed(BeginClockTime, &EndClockTime);
        
        bmm VSyncEnabled = false;
        if (!VSyncEnabled)
        {
            smm TargetNanoseconds = SECONDS_TO_NANOSECONDS / 60;
            if (NanosecondsElapsed < TargetNanoseconds)
            {
                //
                // Sleep until the next frame
                //
                
                timespec TimeToSleep;
                TimeToSleep.tv_sec  = 0;
                TimeToSleep.tv_nsec = TargetNanoseconds - NanosecondsElapsed;
                
                if (nanosleep(&TimeToSleep, nullptr) == -1)
                {
                    Print("WARN: ------ nanosleep failed");
                }
                
                //
                // Spinlock in case we woke up too early
                //
                
                do
                {
                    NanosecondsElapsed = GetNanosecondsElapsed(BeginClockTime, &EndClockTime);
                }
                while (NanosecondsElapsed < TargetNanoseconds);
            }
        }
        // Reset the begin clock of next frame
        GLOBAL_MainState->BeginClockTime = EndClockTime;
        
        // Reset the vertex buffer
        GLOBAL_MainState->VertexCount = 0;
        
        // Increment the frame counter
        GLOBAL_MainState->Frame += 1;
        
        // Print the FPS statistics
        // f32 SecondsElapsed  = (f32)NanosecondsElapsed * (1.0f / SECONDS_TO_NANOSECONDS);
        // f32 FramesPerSecond = 1.0f / SecondsElapsed;
        // Print("ns %d, fps %d", NanosecondsElapsed, (s32)FramesPerSecond);
    }
    
    Print("--------------- Exiting AndroidMain!");
    return nullptr;
}

internal void onStart(ANativeActivity *Activity)
{
    Print("-- onStart Activity %d", Activity);
}

internal void onResume(ANativeActivity *Activity)
{
    Print("-- onResume Activity %d", Activity);
    
    GLOBAL_ActiveFlags ^= ACTIVE_RESUME;
}

internal void *onSaveInstanceState(ANativeActivity *Activity, size_t *OutSize)
{
    Print("-- onSaveInstanceState Activity %d  OutSize %d", Activity, OutSize);
    
    return nullptr;
}

internal void onPause(ANativeActivity *Activity)
{
    Print("-- onPause Activity %d", Activity);
    
    GLOBAL_ActiveFlags ^= ACTIVE_RESUME;
}

internal void onStop(ANativeActivity *Activity)
{
    Print("-- onStop Activity %d", Activity);
}

internal void onDestroy(ANativeActivity *Activity)
{
    Print("-- onDestroy Activity %d", Activity);
}

internal void onWindowFocusChanged(ANativeActivity *Activity, s32 HasFocus)
{
    Print("-- onWindowFocusChanged Activity %d  HasFocus %d", Activity, HasFocus);
    
    GLOBAL_ActiveFlags ^= ACTIVE_FOCUS;
}

internal void onNativeWindowCreated(ANativeActivity *Activity, ANativeWindow *Window)
{
    Print("-- onNativeWindowCreated Activity %d  Window %d", Activity, Window);
    GLOBAL_PendingWindow = Window;
    
    GLOBAL_ActiveFlags ^= ACTIVE_SURFACE;
}

internal void onNativeWindowResized(ANativeActivity *Activity, ANativeWindow *Window)
{
    Print("-- onNativeWindowResized Activity %d  Window %d", Activity, Window);
}

internal void onNativeWindowRedrawNeeded(ANativeActivity *Activity, ANativeWindow *Window)
{
    Print("-- onNativeWindowRedrawNeeded Activity %d  Window %d", Activity, Window);
}

internal void onNativeWindowDestroyed(ANativeActivity *Activity, ANativeWindow *Window)
{
    Print("-- onNativeWindowDestroyed Activity %d  Window %d", Activity, Window);
    GLOBAL_PendingWindow = nullptr;
    
    GLOBAL_ActiveFlags ^= ACTIVE_SURFACE;
}

internal void onInputQueueCreated(ANativeActivity *Activity, AInputQueue *InputQueue)
{
    Print("-- onInputQueueCreated Activity %d  InputQueue %d", Activity, InputQueue);
    
    GLOBAL_InputQueue = InputQueue;
}

internal void onInputQueueDestroyed(ANativeActivity *Activity, AInputQueue *InputQueue)
{
    Print("-- onInputQueueDestroyed Activity %d  InputQueue %d", Activity, InputQueue);
    
    GLOBAL_InputQueue = nullptr;
}

internal void onContentRectChanged(ANativeActivity *Activity, ARect *Rect)
{
    Print("-- onContentRectChanged Activity %d  Rect %d { %dx%d  [%d, %d]x[%d, %d] }", 
        Activity, Rect, 
        (Rect->right  - Rect->left), // Width
        (Rect->bottom - Rect->top),  // Height
        Rect->left, Rect->top, Rect->right, Rect->bottom);
}

internal void onConfigurationChanged(ANativeActivity *Activity)
{
    Print("-- onConfigurationChanged Activity %d", Activity);
}

internal void onLowMemory(ANativeActivity *Activity)
{
    Print("-- onLowMemory Activity %d", Activity);
}

extern "C" JNIEXPORT void ANativeActivity_onCreate(ANativeActivity *Activity, void *SavedState, size_t SavedStateSize)
{
    Print("------------------------------- BUILD (%s) %s -------------------------------", __DATE__, __TIME__);
    Print("-- onCreate Activity %d  SavedState %d  SavedStateSize %d", Activity, SavedState, SavedStateSize);
    
    //
    // Callbacks
    //
    
    Activity->callbacks->onStart                    = onStart;
    Activity->callbacks->onResume                   = onResume;
    Activity->callbacks->onSaveInstanceState        = onSaveInstanceState;
    Activity->callbacks->onPause                    = onPause;
    Activity->callbacks->onStop                     = onStop;
    Activity->callbacks->onDestroy                  = onDestroy;
    Activity->callbacks->onWindowFocusChanged       = onWindowFocusChanged;
    Activity->callbacks->onNativeWindowCreated      = onNativeWindowCreated;
    Activity->callbacks->onNativeWindowResized      = onNativeWindowResized;
    Activity->callbacks->onNativeWindowRedrawNeeded = onNativeWindowRedrawNeeded;
    Activity->callbacks->onNativeWindowDestroyed    = onNativeWindowDestroyed;
    Activity->callbacks->onInputQueueCreated        = onInputQueueCreated;
    Activity->callbacks->onInputQueueDestroyed      = onInputQueueDestroyed;
    Activity->callbacks->onContentRectChanged       = onContentRectChanged;
    Activity->callbacks->onConfigurationChanged     = onConfigurationChanged;
    Activity->callbacks->onLowMemory                = onLowMemory;
    
    if (GLOBAL_Activity != nullptr)
    {
        GLOBAL_Activity = Activity;
        Print("EARLY Exiting onCreate!\n");
        return;
    }
    
    //
    // Memory
    //
    
    smm MainMemorySize = sizeof(main_state) + PERMANENT_ARENA_SIZE + TRANSIENT_ARENA_SIZE;
    void *MainMemory = mmap(nullptr, MainMemorySize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (MainMemory == (void *)-1)
    {
        Error("mmap MainMemory failed");
    }
    
    Print("Memory allocated! %d, %d bytes", MainMemory, sizeof(main_state));
    
    //
    // Globals
    //
    
    Print("MainState %d, Activity %d, PendingWindow %d, InputQueue %d", 
        GLOBAL_MainState, GLOBAL_Activity, GLOBAL_PendingWindow, GLOBAL_InputQueue);
    
    GLOBAL_MainState = (main_state *)MainMemory;
    GLOBAL_Activity  = Activity;
    
    //
    // Thread
    //
    
    pthread_attr_t Attributes;
    pthread_attr_init(&Attributes);
    pthread_attr_setdetachstate(&Attributes, PTHREAD_CREATE_DETACHED);
    
    pthread_t Thread;
    smm ThreadResult = pthread_create(&Thread, &Attributes, AndroidMain, nullptr);
    if (ThreadResult != 0)
    {
        Error("pthread_create failed");
    }
    
    pthread_attr_destroy(&Attributes);
    
    //
    // Exiting
    //
    
    Print("Exiting onCreate!\n");
}
