//
// Handle names
//

#define TEXNAME_MAIN	1 // Identifier for the main opengl texture

#define BUFNAME_VERTEX	1 // Identifier for the main vertex buffer, array buffer
#define BUFNAME_INDEX	2 // Identifier for the main index buffer, element buffer

//
// Utility function
//

#define GLAssert() if (glGetError() != GL_NO_ERROR) { Print("GL ERROR Line %d", __LINE__); __builtin_trap(); }

//
// Constants
//

#define GL_ARRAY_BUFFER					0x8892
#define GL_ELEMENT_ARRAY_BUFFER			0x8893

#define GL_STATIC_DRAW					0x88E4
#define GL_DYNAMIC_DRAW					0x88E8

#define GL_SHADER_COMPILER				0x8DFA

#define GL_FRAGMENT_SHADER				0x8B30
#define GL_VERTEX_SHADER				0x8B31

#define GL_COMPILE_STATUS				0x8B81
#define GL_LINK_STATUS					0x8B82
#define GL_VALIDATE_STATUS				0x8B83

#define GL_INFO_LOG_LENGTH				0x8B84

#define GL_TEXTURE0						0x84C0

#define GL_MAX_TEXTURE_SIZE				0x0D33 // glGetIntegerv

// glClear
#define GL_COLOR_BUFFER_BIT				0x00004000
#define GL_STENCIL_BUFFER_BIT			0x00000400
#define GL_DEPTH_BUFFER_BIT				0x00004000

// glGetString
#define GL_VENDOR						0x1F00
#define GL_RENDERER						0x1F01
#define GL_VERSION						0x1F02
#define GL_EXTENSIONS					0x1F03
#define GL_SHADING_LANGUAGE_VERSION		0x8B8C

// Blending
#define GL_ZERO							0
#define GL_ONE							1
#define GL_SRC_COLOR					0x0300
#define GL_ONE_MINUS_SRC_COLOR			0x0301
#define GL_SRC_ALPHA					0x0302
#define GL_ONE_MINUS_SRC_ALPHA			0x0303
#define GL_DST_ALPHA					0x0304
#define GL_ONE_MINUS_DST_ALPHA			0x0305
#define GL_FUNC_ADD						0x8006

// glEnable
#define GL_BLEND						0x0BE2
#define GL_TEXTURE_2D					0x0DE1


//
// Texture formats
//

#define GL_RGBA8						0x8058

#define GL_RGBA							0x1908
#define GL_BGRA							0x80E1

#define GL_UNSIGNED_BYTE				0x1401
#define GL_UNSIGNED_SHORT				0x1403
#define GL_FLOAT						0x1406

// Texture Magnification/Minification filter
#define GL_TEXTURE_MAG_FILTER			0x2800
#define GL_TEXTURE_MIN_FILTER			0x2801

#define GL_NEAREST						0x2600
#define GL_LINEAR						0x2601
#define GL_NEAREST_MIPMAP_LINEAR		0x2702 // Default, will fail because we use only 1 mipmap but don't set the mipmap count

// Texture Wrap mode
#define GL_TEXTURE_BASE_LEVEL			0x813C
#define GL_TEXTURE_MAX_LEVEL			0x813D

#define GL_TEXTURE_WRAP_S				0x2802
#define GL_TEXTURE_WRAP_T				0x2803

#define GL_REPEAT						0x2901 // Default
#define GL_CLAMP_TO_EDGE				0x812F
#define GL_MIRRORED_REPEAT				0x8370

// SetPrimitiveTopology
#define GL_POINTS                       0x0000
#define GL_TRIANGLES					0x0004
#define GL_TRIANGLE_STRIP				0x0005

// Errors
#define GL_FALSE						0
#define GL_TRUE							1

#define GL_NO_ERROR						0

//
// Types
//

typedef void			GLvoid;
typedef char			GLchar;
typedef unsigned int	GLenum;
typedef float			GLfloat;
typedef s32				GLfixed;
typedef unsigned int	GLuint;
typedef int				GLsizei;
typedef size_t			GLsizeiptr;
typedef int *			GLintptr;
typedef unsigned int	GLbitfield;
typedef int				GLint;
typedef unsigned short	GLushort;
typedef short			GLshort;
typedef s8				GLbyte;
typedef u8				GLubyte;
typedef unsigned char	GLboolean;

//
// Functions
//

extern "C"
{
	GL_API void		GL_APIENTRY glEnable(GLenum cap);
	GL_API GLenum	GL_APIENTRY glGetError();
	GL_API GLubyte *GL_APIENTRY glGetString(GLenum name);
	GL_API void		GL_APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
	GL_API void		GL_APIENTRY glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	GL_API void		GL_APIENTRY glClear(GLbitfield mask);
	GL_API void		GL_APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, void *pixels);
	GL_API void		GL_APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
	GL_API void		GL_APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param);

	GL_API void		GL_APIENTRY glBindTexture(GLenum target, GLuint texture);
	GL_API void		GL_APIENTRY glDeleteTextures(GLsizei n, GLuint *textures);
	GL_API void		GL_APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, GLvoid *indices);
	GL_API void		GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count); // @Todo Use for point sprites
	GL_API void		GL_APIENTRY glGetIntegerv(GLenum pname, GLint *params);

	GL_API void		GL_APIENTRY glDeleteProgram(GLuint program);
	GL_API void		GL_APIENTRY glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
	GL_API void		GL_APIENTRY glBufferData(GLenum target, GLsizeiptr size, void *data, GLenum usage);
	GL_API void		GL_APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void *data);
	GL_API GLuint	GL_APIENTRY glCreateShader(GLenum type);
	GL_API void		GL_APIENTRY glShaderSource(GLuint shader, GLsizei count, GLchar **string, GLint *length);
	GL_API void		GL_APIENTRY glCompileShader(GLuint shader);
	GL_API void		GL_APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint *params);
	GL_API void		GL_APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
	GL_API GLuint	GL_APIENTRY glCreateProgram();
	GL_API void		GL_APIENTRY glAttachShader(GLuint program, GLuint shader);
	GL_API void		GL_APIENTRY glBindAttribLocation(GLuint program, GLuint index, GLchar *name);
	GL_API void		GL_APIENTRY glLinkProgram(GLuint program);
	GL_API void		GL_APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint *params);
	GL_API void		GL_APIENTRY glValidateProgram(GLuint program);
	GL_API void		GL_APIENTRY glUseProgram(GLuint program);
	GL_API void		GL_APIENTRY glDeleteShader(GLuint shader);
	GL_API GLint	GL_APIENTRY glGetUniformLocation(GLuint program, GLchar *name);
	GL_API void		GL_APIENTRY glUniform2f(GLint location, GLfloat v0, GLfloat v1);
	GL_API void		GL_APIENTRY glUniform1i(GLint location, GLint v0);
	GL_API void		GL_APIENTRY glActiveTexture(GLenum texture);
	GL_API void		GL_APIENTRY glBindBuffer(GLenum target, GLuint buffer);
	GL_API void		GL_APIENTRY glDeleteBuffers(GLsizei n, GLuint *buffers);
	GL_API void		GL_APIENTRY glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, void *pointer);
	GL_API void		GL_APIENTRY glEnableVertexAttribArray(GLuint index);
}