// Because static has multiple meanings depending on context
#define internal		static // marking a function as internal to the compilation unit
#define global			static // marking a variable as global

//
// Utility functions
//

// General
#define CompilerAssert(Expression, Name) typedef char assert_##Name [(Expression) ? 1 : -1 ];
#define GetArrayCount(Array)			(sizeof(Array) / sizeof((Array)[0]))
#define offsetof(Type, Member)			(__builtin_offsetof(Type, Member))
#define floatsof(Type, Member)			(sizeof(((Type *)0)->Member) / sizeof(f32))

// Math
#define Square(A)						((A) * (A))
#define Lerp(A, B, F)					((A) + (F) * ((B) - (A)))
#define Sign(A)							((A) >= 0 ? 1 : -1)
#define Abs(A)							((A) >= 0 ? (A) : -(A))
#define Min(A, B)						((A) < (B) ? (A) : (B))
#define Max(A, B)						((A) > (B) ? (A) : (B))
#define Clamp(A, MinValue, MaxValue)	(Max((MinValue), Min((A), (MaxValue))))
#define Clamp01(A)						(Max((0), Min((A), (1))))

// String
#define StringAndLength(String)			(String ""), (GetArrayCount(String) - 1)
#define StringAndSize(String)			(String ""), (sizeof(String))

// Integer
#define IPv4(B1, B2, B3, B4)			((B1 << 0) | (B2 << 8) | (B3 << 16) | (B4 << 24))
#define RoundUp(A, To)					(((A) + ((To) - 1)) & ~((To) - 1))
#define RoundDown(A, Multiple)			((A) - (A) % (Multiple))
#define NormalizeRGB(R, G, B)			(R) * (1.0f / 255.0f), (G) * (1.0f / 255.0f), (B) * (1.0f / 255.0f) 
#define BitMask(Type, OnesCount)		(((Type)(-((OnesCount) != 0))) & ((Type)-1 >> ((sizeof(Type) * 8) - (OnesCount))))
#define KB(Value)						((Value) * 1024)
#define MB(Value)						((Value) * 1024 * 1024)

// Big Endian to Host Endiannes macros
// On Big Endian machiens it can be defined as a dummy macro
#define BE16(A)\
	((((A) >> 8) & 0XFF) | \
	 (((A) & 0xFF) << 8))

#define BE32(A)\
	((((A) & 0xFF000000) >> 24) | \
	 (((A) & 0x00FF0000) >>  8) | \
	 (((A) & 0x0000FF00) <<  8) | \
	 (((A) & 0x000000FF) << 24))

#define BE64(A)\
	((((A) & 0xFF00000000000000ULL) >> 56) | \
	 (((A) & 0x00FF000000000000ULL) >> 40) | \
	 (((A) & 0x0000FF0000000000ULL) >> 24) | \
	 (((A) & 0x000000FF00000000ULL) >>  8) | \
	 (((A) & 0x00000000FF000000ULL) <<  8) | \
	 (((A) & 0x0000000000FF0000ULL) << 24) | \
	 (((A) & 0x000000000000FF00ULL) << 40) | \
	 (((A) & 0x00000000000000FFULL) << 56))

// Constants
#define MAX_U16					((u16)(~((u16)0)))
#define SECONDS_TO_NANOSECONDS	(1000000000)

//
// Colors
//

#define COL_WHITE	/* NO COLOR		*/ (v4{ NormalizeRGB(255, 255, 255), 1.0f })
#define COL_DEBUG	/* DEBUG		*/ (v4{ NormalizeRGB(255,   0, 255), 0.1f })
#define COL_BACK	/* BACKGROUND	*/ (v4{ NormalizeRGB( 10,  10,  10), 1.0f })
#define COL_FORE	/* FOREGROUND	*/ (v4{ NormalizeRGB(204, 204, 204), 1.0f })
#define COL_BUTN	/* BUTTON		*/ (v4{ NormalizeRGB( 30,  30,  30), 1.0f })
#define COL_ACNT	/* ACCENT		*/ (v4{ NormalizeRGB(255,  77,   0), 1.0f })

//
// Data Types
//

typedef signed char				s8;
typedef unsigned char			u8;
typedef signed short			s16;
typedef unsigned short			u16;
typedef signed int				s32;
typedef unsigned int			u32;
typedef signed long long		s64;
typedef unsigned long long		u64;
typedef float					f32;
typedef double					f64;

typedef char					b8;
typedef short					b16;
typedef int						b32;
typedef long long				b64;

#if defined(__LP64__)
	typedef unsigned long long	umm;
	typedef signed long long	smm;
	typedef long long			bmm;
#else
	typedef unsigned int		umm;
	typedef signed int			smm;
	typedef int					bmm;
#endif

//
// VarArg
//

#if defined(__clang__)
	typedef __builtin_va_list	va_list;
	#define va_start(ap, param)	__builtin_va_start(ap, param)
	#define va_end(ap)			__builtin_va_end(ap)
	#define va_arg(ap, type)	__builtin_va_arg(ap, type)
#else
	#error Compiler VarArg not implemented
#endif

union v2
{
	struct
	{
		f32 X;
		f32 Y;
	};
	
	f32 E[2];
};

union v4
{
	struct
	{
		f32 X;
		f32 Y;
		f32 Z;
		f32 W;
	};
	
	struct
	{
		f32 Left;
		f32 Bottom;
		f32 Right;
		f32 Top;
	};
	
	struct
	{
		f32 X1;
		f32 Y1;
		f32 X2;
		f32 Y2;
	};
	
	struct
	{
		v2 BottomLeft;
		v2 TopRight;
	};
	
	struct
	{
		f32 Red;
		f32 Green;
		f32 Blue;
		f32 Alpha;
	};
};

struct memory_arena
{
	u8 *Data;
	s32 Size;
	s32 Taken;
};

struct string
{
	char *Text;
	smm  Length;
};

struct vertex
{
	v2 Position;
	v2 Texture;
	v4 Color;
};
