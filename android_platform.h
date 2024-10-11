//
// Warnings
//

#pragma clang diagnostic ignored "-Wwritable-strings"
#pragma clang diagnostic ignored "-Wunused-function"

// @Note It's BGRA for Windows
#define PLATFORM_OPENGL_UPLOAD_FORMAT GL_RGBA

#define NEWLINE "\n"

#define NOINLINE __attribute__((noinline))

//
// Common types
//

typedef signed int		int32_t;
typedef unsigned int	uint32_t;

#if defined(__LP64__)
	typedef unsigned long long	size_t;
	typedef signed long long	ssize_t;
#else
	typedef unsigned int		size_t;
	typedef signed int			ssize_t;
#endif

typedef size_t off_t;

#define CLOCK_MONOTONIC_RAW 4
#define CLOCK_MONOTONIC 1
typedef long time_t; // @Todo @Warn, 64bit time_t sometimes?? !!
typedef int clockid_t;
struct timespec
{
	time_t tv_sec;
	long tv_nsec;
};

struct tm
{
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
	long int tm_gmtoff;
	const char* tm_zone;
};

//
// File I/O
//

#if defined(__aarch64__) || (defined(__mips__) && defined(__LP64__))
#define __STAT64_BODY \
	dev_t st_dev; \
	ino_t st_ino; \
	mode_t st_mode; \
	int st_nlink; \
	int st_uid; \
	int st_gid; \
	dev_t st_rdev; \
	unsigned long __pad1; \
	off_t st_size; \
	int st_blksize; \
	int __pad2; \
	long st_blocks; \
	struct timespec st_atim; \
	struct timespec st_mtim; \
	struct timespec st_ctim; \
	unsigned int __unused4; \
	unsigned int __unused5; \

#elif defined(__mips__) && !defined(__LP64__)
#define __STAT64_BODY \
	unsigned int st_dev; \
	unsigned int __pad0[3]; \
	unsigned long long st_ino; \
	mode_t st_mode; \
	int st_nlink; \
	int st_uid; \
	int st_gid; \
	unsigned int st_rdev; \
	unsigned int __pad1[3]; \
	long long st_size; \
	struct timespec st_atim; \
	struct timespec st_mtim; \
	struct timespec st_ctim; \
	unsigned int st_blksize; \
	unsigned int __pad2; \
	unsigned long long st_blocks; \

#elif defined(__x86_64__)
#define __STAT64_BODY \
	dev_t st_dev; \
	ino_t st_ino; \
	unsigned long st_nlink; \
	mode_t st_mode; \
	int st_uid; \
	int st_gid; \
	unsigned int __pad0; \
	dev_t st_rdev; \
	off_t st_size; \
	long st_blksize; \
	long st_blocks; \
	struct timespec st_atim; \
	struct timespec st_mtim; \
	struct timespec st_ctim; \
	long __pad3[3]; \

#else /* __arm__ || __i386__ */
#define __STAT64_BODY \
	unsigned long long st_dev; \
	unsigned char __pad0[4]; \
	unsigned long __st_ino; \
	unsigned int st_mode; \
	int st_nlink; \
	int st_uid; \
	int st_gid; \
	unsigned long long st_rdev; \
	unsigned char __pad3[4]; \
	long long st_size; \
	unsigned long st_blksize; \
	unsigned long long st_blocks; \
	struct timespec st_atim; \
	struct timespec st_mtim; \
	struct timespec st_ctim; \
	unsigned long long st_ino; \

#endif

struct stat		{ __STAT64_BODY };
struct stat64	{ __STAT64_BODY };

#define O_RDONLY	00000000
#define O_WRONLY	00000001
#define O_RDWR		00000002

//
// JNI
//

#define JNIAssert(Value)\
if (Value == nullptr) { Print("JNI Error on line %d %s", __LINE__, #Value); }

#define JNIEXPORT __attribute__ ((visibility ("default")))

typedef u8		jboolean;
typedef s8		jbyte;
typedef u16		jchar;
typedef s16		jshort;
typedef s32		jint;
typedef s64		jlong;
typedef f32		jfloat;
typedef f64		jdouble;
typedef s32		jsize;

// The implementation is only required to reserve slots for 16 local references
// this includes jobject, jclass, jstring, jarray, etc..
typedef void *	jobject;
typedef void *	jclass;
typedef void *	jstring;
typedef void *	jarray;
typedef void *	jobjectArray;
typedef void *	jbooleanArray;
typedef void *	jbyteArray;
typedef void *	jcharArray;
typedef void *	jshortArray;
typedef void *	jintArray;
typedef void *	jlongArray;
typedef void *	jfloatArray;
typedef void *	jdoubleArray;
typedef void *	jthrowable;
typedef void *	jweak;

typedef void *	jfieldID;
typedef void *	jmethodID;

union jvalue
{
	jboolean	z;
	jbyte		b;
	jchar		c;
	jshort		s;
	jint		i;
	jlong		j;
	jfloat		f;
	jdouble		d;
	jobject		l;
};

struct JNINativeMethod
{
	char* name;
	char* signature;
	void* fnPtr;
};

enum
{
	JNI_OK			=  0, // Success
	JNI_ERR			= -1, // Unknown error
	JNI_EDETACHED	= -2, // Thread detached from the VM
	JNI_EVERSION	= -3, // JNI version error
	JNI_ENOMEM		= -4, // Not enough memory
	JNI_EEXIST		= -5, // VM already created
	JNI_EINVAL		= -6, // Invalid arguments
};

enum
{
	JNI_FALSE		= 0, 
	JNI_TRUE		= 1, 
};

enum jobjectRefType
{
	JNIInvalidRefType		= 0, 
	JNILocalRefType			= 1, 
	JNIGlobalRefType		= 2, 
	JNIWeakGlobalRefType	= 3, 
};

struct JNINativeInterface;
struct JNIInvokeInterface;
typedef JNINativeInterface *JNIEnv;
typedef JNIInvokeInterface *JavaVM;

struct JNINativeInterface
{
	void*		reserved0;
	void*		reserved1;
	void*		reserved2;
	void*		reserved3;
	
	jint		(*GetVersion)(JNIEnv *);
	jclass		(*DefineClass)(JNIEnv*, char*, jobject, jbyte*, jsize);
	jclass		(*FindClass)(JNIEnv*, char*);
	jmethodID	(*FromReflectedMethod)(JNIEnv*, jobject);
	jfieldID	(*FromReflectedField)(JNIEnv*, jobject);
	jobject     (*ToReflectedMethod)(JNIEnv*, jclass, jmethodID, jboolean);
	jclass      (*GetSuperclass)(JNIEnv*, jclass);
	jboolean    (*IsAssignableFrom)(JNIEnv*, jclass, jclass);
	jobject     (*ToReflectedField)(JNIEnv*, jclass, jfieldID, jboolean);
	
	jint        (*Throw)(JNIEnv*, jthrowable);
	jint        (*ThrowNew)(JNIEnv *, jclass, char *);
	jthrowable  (*ExceptionOccurred)(JNIEnv*);
	void        (*ExceptionDescribe)(JNIEnv*);
	void        (*ExceptionClear)(JNIEnv*);
	void        (*FatalError)(JNIEnv*, char*);
	
	jint        (*PushLocalFrame)(JNIEnv*, jint);
	jobject     (*PopLocalFrame)(JNIEnv*, jobject);
	
	jobject     (*NewGlobalRef)(JNIEnv*, jobject);
	void        (*DeleteGlobalRef)(JNIEnv*, jobject);
	void        (*DeleteLocalRef)(JNIEnv*, jobject);
	jboolean    (*IsSameObject)(JNIEnv*, jobject, jobject);
	
	jobject     (*NewLocalRef)(JNIEnv*, jobject);
	jint        (*EnsureLocalCapacity)(JNIEnv*, jint);
	
	jobject     (*AllocObject)(JNIEnv*, jclass);
	jobject     (*NewObject)(JNIEnv*, jclass, jmethodID, ...);
	jobject     (*NewObjectV)(JNIEnv*, jclass, jmethodID, va_list);
	jobject     (*NewObjectA)(JNIEnv*, jclass, jmethodID, jvalue*);
	
	jclass      (*GetObjectClass)(JNIEnv*, jobject);
	jboolean    (*IsInstanceOf)(JNIEnv*, jobject, jclass);
	jmethodID   (*GetMethodID)(JNIEnv*, jclass, char*, char*);
	
	jobject     (*CallObjectMethod)(JNIEnv*, jobject, jmethodID, ...);
	jobject     (*CallObjectMethodV)(JNIEnv*, jobject, jmethodID, va_list);
	jobject     (*CallObjectMethodA)(JNIEnv*, jobject, jmethodID, jvalue*);
	jboolean    (*CallBooleanMethod)(JNIEnv*, jobject, jmethodID, ...);
	jboolean    (*CallBooleanMethodV)(JNIEnv*, jobject, jmethodID, va_list);
	jboolean    (*CallBooleanMethodA)(JNIEnv*, jobject, jmethodID, jvalue*);
	jbyte       (*CallByteMethod)(JNIEnv*, jobject, jmethodID, ...);
	jbyte       (*CallByteMethodV)(JNIEnv*, jobject, jmethodID, va_list);
	jbyte       (*CallByteMethodA)(JNIEnv*, jobject, jmethodID, jvalue*);
	jchar       (*CallCharMethod)(JNIEnv*, jobject, jmethodID, ...);
	jchar       (*CallCharMethodV)(JNIEnv*, jobject, jmethodID, va_list);
	jchar       (*CallCharMethodA)(JNIEnv*, jobject, jmethodID, jvalue*);
	jshort      (*CallShortMethod)(JNIEnv*, jobject, jmethodID, ...);
	jshort      (*CallShortMethodV)(JNIEnv*, jobject, jmethodID, va_list);
	jshort      (*CallShortMethodA)(JNIEnv*, jobject, jmethodID, jvalue*);
	jint        (*CallIntMethod)(JNIEnv*, jobject, jmethodID, ...);
	jint        (*CallIntMethodV)(JNIEnv*, jobject, jmethodID, va_list);
	jint        (*CallIntMethodA)(JNIEnv*, jobject, jmethodID, jvalue*);
	jlong       (*CallLongMethod)(JNIEnv*, jobject, jmethodID, ...);
	jlong       (*CallLongMethodV)(JNIEnv*, jobject, jmethodID, va_list);
	jlong       (*CallLongMethodA)(JNIEnv*, jobject, jmethodID, jvalue*);
	jfloat      (*CallFloatMethod)(JNIEnv*, jobject, jmethodID, ...);
	jfloat      (*CallFloatMethodV)(JNIEnv*, jobject, jmethodID, va_list);
	jfloat      (*CallFloatMethodA)(JNIEnv*, jobject, jmethodID, jvalue*);
	jdouble     (*CallDoubleMethod)(JNIEnv*, jobject, jmethodID, ...);
	jdouble     (*CallDoubleMethodV)(JNIEnv*, jobject, jmethodID, va_list);
	jdouble     (*CallDoubleMethodA)(JNIEnv*, jobject, jmethodID, jvalue*);
	void        (*CallVoidMethod)(JNIEnv*, jobject, jmethodID, ...);
	void        (*CallVoidMethodV)(JNIEnv*, jobject, jmethodID, va_list);
	void        (*CallVoidMethodA)(JNIEnv*, jobject, jmethodID, jvalue*);
	
	jobject     (*CallNonvirtualObjectMethod)(JNIEnv*, jobject, jclass, jmethodID, ...);
	jobject     (*CallNonvirtualObjectMethodV)(JNIEnv*, jobject, jclass, jmethodID, va_list);
	jobject     (*CallNonvirtualObjectMethodA)(JNIEnv*, jobject, jclass, jmethodID, jvalue*);
	jboolean    (*CallNonvirtualBooleanMethod)(JNIEnv*, jobject, jclass, jmethodID, ...);
	jboolean    (*CallNonvirtualBooleanMethodV)(JNIEnv*, jobject, jclass, jmethodID, va_list);
	jboolean    (*CallNonvirtualBooleanMethodA)(JNIEnv*, jobject, jclass, jmethodID, jvalue*);
	jbyte       (*CallNonvirtualByteMethod)(JNIEnv*, jobject, jclass, jmethodID, ...);
	jbyte       (*CallNonvirtualByteMethodV)(JNIEnv*, jobject, jclass, jmethodID, va_list);
	jbyte       (*CallNonvirtualByteMethodA)(JNIEnv*, jobject, jclass, jmethodID, jvalue*);
	jchar       (*CallNonvirtualCharMethod)(JNIEnv*, jobject, jclass, jmethodID, ...);
	jchar       (*CallNonvirtualCharMethodV)(JNIEnv*, jobject, jclass, jmethodID, va_list);
	jchar       (*CallNonvirtualCharMethodA)(JNIEnv*, jobject, jclass, jmethodID, jvalue*);
	jshort      (*CallNonvirtualShortMethod)(JNIEnv*, jobject, jclass, jmethodID, ...);
	jshort      (*CallNonvirtualShortMethodV)(JNIEnv*, jobject, jclass, jmethodID, va_list);
	jshort      (*CallNonvirtualShortMethodA)(JNIEnv*, jobject, jclass, jmethodID, jvalue*);
	jint        (*CallNonvirtualIntMethod)(JNIEnv*, jobject, jclass, jmethodID, ...);
	jint        (*CallNonvirtualIntMethodV)(JNIEnv*, jobject, jclass, jmethodID, va_list);
	jint        (*CallNonvirtualIntMethodA)(JNIEnv*, jobject, jclass, jmethodID, jvalue*);
	jlong       (*CallNonvirtualLongMethod)(JNIEnv*, jobject, jclass, jmethodID, ...);
	jlong       (*CallNonvirtualLongMethodV)(JNIEnv*, jobject, jclass, jmethodID, va_list);
	jlong       (*CallNonvirtualLongMethodA)(JNIEnv*, jobject, jclass, jmethodID, jvalue*);
	jfloat      (*CallNonvirtualFloatMethod)(JNIEnv*, jobject, jclass, jmethodID, ...);
	jfloat      (*CallNonvirtualFloatMethodV)(JNIEnv*, jobject, jclass, jmethodID, va_list);
	jfloat      (*CallNonvirtualFloatMethodA)(JNIEnv*, jobject, jclass, jmethodID, jvalue*);
	jdouble     (*CallNonvirtualDoubleMethod)(JNIEnv*, jobject, jclass, jmethodID, ...);
	jdouble     (*CallNonvirtualDoubleMethodV)(JNIEnv*, jobject, jclass, jmethodID, va_list);
	jdouble     (*CallNonvirtualDoubleMethodA)(JNIEnv*, jobject, jclass, jmethodID, jvalue*);
	void        (*CallNonvirtualVoidMethod)(JNIEnv*, jobject, jclass, jmethodID, ...);
	void        (*CallNonvirtualVoidMethodV)(JNIEnv*, jobject, jclass, jmethodID, va_list);
	void        (*CallNonvirtualVoidMethodA)(JNIEnv*, jobject, jclass, jmethodID, jvalue*);
	
	jfieldID    (*GetFieldID)(JNIEnv*, jclass, char*, char*);
	
	jobject     (*GetObjectField)(JNIEnv*, jobject, jfieldID);
	jboolean    (*GetBooleanField)(JNIEnv*, jobject, jfieldID);
	jbyte       (*GetByteField)(JNIEnv*, jobject, jfieldID);
	jchar       (*GetCharField)(JNIEnv*, jobject, jfieldID);
	jshort      (*GetShortField)(JNIEnv*, jobject, jfieldID);
	jint        (*GetIntField)(JNIEnv*, jobject, jfieldID);
	jlong       (*GetLongField)(JNIEnv*, jobject, jfieldID);
	jfloat      (*GetFloatField)(JNIEnv*, jobject, jfieldID);
	jdouble     (*GetDoubleField)(JNIEnv*, jobject, jfieldID);
	
	void        (*SetObjectField)(JNIEnv*, jobject, jfieldID, jobject);
	void        (*SetBooleanField)(JNIEnv*, jobject, jfieldID, jboolean);
	void        (*SetByteField)(JNIEnv*, jobject, jfieldID, jbyte);
	void        (*SetCharField)(JNIEnv*, jobject, jfieldID, jchar);
	void        (*SetShortField)(JNIEnv*, jobject, jfieldID, jshort);
	void        (*SetIntField)(JNIEnv*, jobject, jfieldID, jint);
	void        (*SetLongField)(JNIEnv*, jobject, jfieldID, jlong);
	void        (*SetFloatField)(JNIEnv*, jobject, jfieldID, jfloat);
	void        (*SetDoubleField)(JNIEnv*, jobject, jfieldID, jdouble);
	
	jmethodID   (*GetStaticMethodID)(JNIEnv*, jclass, char*, char*);
	
	jobject     (*CallStaticObjectMethod)(JNIEnv*, jclass, jmethodID, ...);
	jobject     (*CallStaticObjectMethodV)(JNIEnv*, jclass, jmethodID, va_list);
	jobject     (*CallStaticObjectMethodA)(JNIEnv*, jclass, jmethodID, jvalue*);
	jboolean    (*CallStaticBooleanMethod)(JNIEnv*, jclass, jmethodID, ...);
	jboolean    (*CallStaticBooleanMethodV)(JNIEnv*, jclass, jmethodID, va_list);
	jboolean    (*CallStaticBooleanMethodA)(JNIEnv*, jclass, jmethodID, jvalue*);
	jbyte       (*CallStaticByteMethod)(JNIEnv*, jclass, jmethodID, ...);
	jbyte       (*CallStaticByteMethodV)(JNIEnv*, jclass, jmethodID, va_list);
	jbyte       (*CallStaticByteMethodA)(JNIEnv*, jclass, jmethodID, jvalue*);
	jchar       (*CallStaticCharMethod)(JNIEnv*, jclass, jmethodID, ...);
	jchar       (*CallStaticCharMethodV)(JNIEnv*, jclass, jmethodID, va_list);
	jchar       (*CallStaticCharMethodA)(JNIEnv*, jclass, jmethodID, jvalue*);
	jshort      (*CallStaticShortMethod)(JNIEnv*, jclass, jmethodID, ...);
	jshort      (*CallStaticShortMethodV)(JNIEnv*, jclass, jmethodID, va_list);
	jshort      (*CallStaticShortMethodA)(JNIEnv*, jclass, jmethodID, jvalue*);
	jint        (*CallStaticIntMethod)(JNIEnv*, jclass, jmethodID, ...);
	jint        (*CallStaticIntMethodV)(JNIEnv*, jclass, jmethodID, va_list);
	jint        (*CallStaticIntMethodA)(JNIEnv*, jclass, jmethodID, jvalue*);
	jlong       (*CallStaticLongMethod)(JNIEnv*, jclass, jmethodID, ...);
	jlong       (*CallStaticLongMethodV)(JNIEnv*, jclass, jmethodID, va_list);
	jlong       (*CallStaticLongMethodA)(JNIEnv*, jclass, jmethodID, jvalue*);
	jfloat      (*CallStaticFloatMethod)(JNIEnv*, jclass, jmethodID, ...);
	jfloat      (*CallStaticFloatMethodV)(JNIEnv*, jclass, jmethodID, va_list);
	jfloat      (*CallStaticFloatMethodA)(JNIEnv*, jclass, jmethodID, jvalue*);
	jdouble     (*CallStaticDoubleMethod)(JNIEnv*, jclass, jmethodID, ...);
	jdouble     (*CallStaticDoubleMethodV)(JNIEnv*, jclass, jmethodID, va_list);
	jdouble     (*CallStaticDoubleMethodA)(JNIEnv*, jclass, jmethodID, jvalue*);
	void        (*CallStaticVoidMethod)(JNIEnv*, jclass, jmethodID, ...);
	void        (*CallStaticVoidMethodV)(JNIEnv*, jclass, jmethodID, va_list);
	void        (*CallStaticVoidMethodA)(JNIEnv*, jclass, jmethodID, jvalue*);
	
	jfieldID    (*GetStaticFieldID)(JNIEnv*, jclass, char*, char*);
	
	jobject     (*GetStaticObjectField)(JNIEnv*, jclass, jfieldID);
	jboolean    (*GetStaticBooleanField)(JNIEnv*, jclass, jfieldID);
	jbyte       (*GetStaticByteField)(JNIEnv*, jclass, jfieldID);
	jchar       (*GetStaticCharField)(JNIEnv*, jclass, jfieldID);
	jshort      (*GetStaticShortField)(JNIEnv*, jclass, jfieldID);
	jint        (*GetStaticIntField)(JNIEnv*, jclass, jfieldID);
	jlong       (*GetStaticLongField)(JNIEnv*, jclass, jfieldID);
	jfloat      (*GetStaticFloatField)(JNIEnv*, jclass, jfieldID);
	jdouble     (*GetStaticDoubleField)(JNIEnv*, jclass, jfieldID);
	
	void        (*SetStaticObjectField)(JNIEnv*, jclass, jfieldID, jobject);
	void        (*SetStaticBooleanField)(JNIEnv*, jclass, jfieldID, jboolean);
	void        (*SetStaticByteField)(JNIEnv*, jclass, jfieldID, jbyte);
	void        (*SetStaticCharField)(JNIEnv*, jclass, jfieldID, jchar);
	void        (*SetStaticShortField)(JNIEnv*, jclass, jfieldID, jshort);
	void        (*SetStaticIntField)(JNIEnv*, jclass, jfieldID, jint);
	void        (*SetStaticLongField)(JNIEnv*, jclass, jfieldID, jlong);
	void        (*SetStaticFloatField)(JNIEnv*, jclass, jfieldID, jfloat);
	void        (*SetStaticDoubleField)(JNIEnv*, jclass, jfieldID, jdouble);
	
	jstring     (*NewString)(JNIEnv*, jchar*, jsize);
	jsize       (*GetStringLength)(JNIEnv*, jstring);
	jchar*		(*GetStringChars)(JNIEnv*, jstring, jboolean*);
	void        (*ReleaseStringChars)(JNIEnv*, jstring, jchar*);
	jstring     (*NewStringUTF)(JNIEnv*, char*);
	jsize       (*GetStringUTFLength)(JNIEnv*, jstring);
	char*		(*GetStringUTFChars)(JNIEnv*, jstring, jboolean*);
	void		(*ReleaseStringUTFChars)(JNIEnv*, jstring, char*);
	jsize		(*GetArrayLength)(JNIEnv*, jarray);
	jobjectArray(*NewObjectArray)(JNIEnv*, jsize, jclass, jobject);
	jobject		(*GetObjectArrayElement)(JNIEnv*, jobjectArray, jsize);
	void		(*SetObjectArrayElement)(JNIEnv*, jobjectArray, jsize, jobject);
	
	jbooleanArray	(*NewBooleanArray)(JNIEnv*, jsize);
	jbyteArray		(*NewByteArray)(JNIEnv*, jsize);
	jcharArray		(*NewCharArray)(JNIEnv*, jsize);
	jshortArray		(*NewShortArray)(JNIEnv*, jsize);
	jintArray		(*NewIntArray)(JNIEnv*, jsize);
	jlongArray		(*NewLongArray)(JNIEnv*, jsize);
	jfloatArray		(*NewFloatArray)(JNIEnv*, jsize);
	jdoubleArray	(*NewDoubleArray)(JNIEnv*, jsize);
	
	jboolean*	(*GetBooleanArrayElements)(JNIEnv*, jbooleanArray, jboolean*);
	jbyte*		(*GetByteArrayElements)(JNIEnv*, jbyteArray, jboolean*);
	jchar*		(*GetCharArrayElements)(JNIEnv*, jcharArray, jboolean*);
	jshort*		(*GetShortArrayElements)(JNIEnv*, jshortArray, jboolean*);
	jint*		(*GetIntArrayElements)(JNIEnv*, jintArray, jboolean*);
	jlong*		(*GetLongArrayElements)(JNIEnv*, jlongArray, jboolean*);
	jfloat*		(*GetFloatArrayElements)(JNIEnv*, jfloatArray, jboolean*);
	jdouble*	(*GetDoubleArrayElements)(JNIEnv*, jdoubleArray, jboolean*);
	
	void        (*ReleaseBooleanArrayElements)(JNIEnv*, jbooleanArray, jboolean*, jint);
	void        (*ReleaseByteArrayElements)(JNIEnv*, jbyteArray, jbyte*, jint);
	void        (*ReleaseCharArrayElements)(JNIEnv*, jcharArray, jchar*, jint);
	void        (*ReleaseShortArrayElements)(JNIEnv*, jshortArray, jshort*, jint);
	void        (*ReleaseIntArrayElements)(JNIEnv*, jintArray, jint*, jint);
	void        (*ReleaseLongArrayElements)(JNIEnv*, jlongArray, jlong*, jint);
	void        (*ReleaseFloatArrayElements)(JNIEnv*, jfloatArray, jfloat*, jint);
	void        (*ReleaseDoubleArrayElements)(JNIEnv*, jdoubleArray, jdouble*, jint);
	
	void        (*GetBooleanArrayRegion)(JNIEnv*, jbooleanArray, jsize, jsize, jboolean*);
	void        (*GetByteArrayRegion)(JNIEnv*, jbyteArray, jsize, jsize, jbyte*);
	void        (*GetCharArrayRegion)(JNIEnv*, jcharArray, jsize, jsize, jchar*);
	void        (*GetShortArrayRegion)(JNIEnv*, jshortArray, jsize, jsize, jshort*);
	void        (*GetIntArrayRegion)(JNIEnv*, jintArray, jsize, jsize, jint*);
	void        (*GetLongArrayRegion)(JNIEnv*, jlongArray, jsize, jsize, jlong*);
	void        (*GetFloatArrayRegion)(JNIEnv*, jfloatArray, jsize, jsize, jfloat*);
	void        (*GetDoubleArrayRegion)(JNIEnv*, jdoubleArray, jsize, jsize, jdouble*);
	
	void        (*SetBooleanArrayRegion)(JNIEnv*, jbooleanArray, jsize, jsize, jboolean*);
	void        (*SetByteArrayRegion)(JNIEnv*, jbyteArray, jsize, jsize, jbyte*);
	void        (*SetCharArrayRegion)(JNIEnv*, jcharArray, jsize, jsize, jchar*);
	void        (*SetShortArrayRegion)(JNIEnv*, jshortArray, jsize, jsize, jshort*);
	void        (*SetIntArrayRegion)(JNIEnv*, jintArray, jsize, jsize, jint*);
	void        (*SetLongArrayRegion)(JNIEnv*, jlongArray, jsize, jsize, jlong*);
	void        (*SetFloatArrayRegion)(JNIEnv*, jfloatArray, jsize, jsize, jfloat*);
	void        (*SetDoubleArrayRegion)(JNIEnv*, jdoubleArray, jsize, jsize, jdouble*);
	
	jint        (*RegisterNatives)(JNIEnv*, jclass, JNINativeMethod*, jint);
	jint        (*UnregisterNatives)(JNIEnv*, jclass);
	jint        (*MonitorEnter)(JNIEnv*, jobject);
	jint        (*MonitorExit)(JNIEnv*, jobject);
	jint        (*GetJavaVM)(JNIEnv*, JavaVM**);
	
	void        (*GetStringRegion)(JNIEnv*, jstring, jsize, jsize, jchar*);
	void        (*GetStringUTFRegion)(JNIEnv*, jstring, jsize, jsize, char*);
	
	void*       (*GetPrimitiveArrayCritical)(JNIEnv*, jarray, jboolean*);
	void        (*ReleasePrimitiveArrayCritical)(JNIEnv*, jarray, void*, jint);
	
	jchar*		(*GetStringCritical)(JNIEnv*, jstring, jboolean*);
	void        (*ReleaseStringCritical)(JNIEnv*, jstring, jchar*);
	
	jweak       (*NewWeakGlobalRef)(JNIEnv*, jobject);
	void        (*DeleteWeakGlobalRef)(JNIEnv*, jweak);
	
	jboolean	(*ExceptionCheck)(JNIEnv*);
	
	jobject     (*NewDirectByteBuffer)(JNIEnv*, void*, jlong);
	void*       (*GetDirectBufferAddress)(JNIEnv*, jobject);
	jlong       (*GetDirectBufferCapacity)(JNIEnv*, jobject);
	
	/* added in JNI 1.6 */
	jobjectRefType (*GetObjectRefType)(JNIEnv*, jobject);
};

struct JNIInvokeInterface
{
	void*       reserved0;
	void*       reserved1;
	void*       reserved2;
	
	jint        (*DestroyJavaVM)(JavaVM*);
	jint        (*AttachCurrentThread)(JavaVM*, JNIEnv**, void*);
	jint        (*DetachCurrentThread)(JavaVM*);
	jint        (*GetEnv)(JavaVM*, void**, jint);
	jint        (*AttachCurrentThreadAsDaemon)(JavaVM*, JNIEnv**, void*);
};

//
// Pthread
//

struct pthread_t
{
	void *		 p;             /* Pointer to actual object */
	unsigned int x;             /* Extra information - reuse count etc */
};

struct pthread_attr_t
{
	uint32_t	flags;
	void *		stack_base;
	size_t		stack_size;
	size_t		guard_size;
	int32_t		sched_policy;
	int32_t		sched_priority;
	
	#ifdef __LP64__
	char	__reserved[16];
	#endif
};

//
// NDK
//

struct ANativeWindow;
struct AAssetManager;
struct AInputQueue;
struct AInputEvent;

struct ARect
{
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
};

enum
{
	AINPUT_EVENT_TYPE_KEY		= 1, 
	AINPUT_EVENT_TYPE_MOTION	= 2, 
	AINPUT_EVENT_TYPE_FOCUS		= 3, 
};

#define AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT	8
#define AMOTION_EVENT_ACTION_POINTER_INDEX_MASK 	0xFF00
#define AMOTION_EVENT_ACTION_MASK					0x00FF

enum
{
	AMOTION_EVENT_ACTION_DOWN			=  0, 
	AMOTION_EVENT_ACTION_UP				=  1, 
	AMOTION_EVENT_ACTION_MOVE			=  2, 
	AMOTION_EVENT_ACTION_CANCEL			=  3, 
	AMOTION_EVENT_ACTION_OUTSIDE		=  4, 
	AMOTION_EVENT_ACTION_POINTER_DOWN	=  5, 
	AMOTION_EVENT_ACTION_POINTER_UP		=  6, 
	AMOTION_EVENT_ACTION_HOVER_MOVE		=  7, 
	AMOTION_EVENT_ACTION_SCROLL			=  8, 
	AMOTION_EVENT_ACTION_HOVER_ENTER	=  9, 
	AMOTION_EVENT_ACTION_HOVER_EXIT		= 10, 
	AMOTION_EVENT_ACTION_BUTTON_PRESS	= 11, 
	AMOTION_EVENT_ACTION_BUTTON_RELEASE	= 12, 
};

enum
{
	AMOTION_EVENT_BUTTON_PRIMARY			= 1 << 0, 
	AMOTION_EVENT_BUTTON_SECONDARY			= 1 << 1, 
	AMOTION_EVENT_BUTTON_TERTIARY			= 1 << 2, 
	AMOTION_EVENT_BUTTON_BACK				= 1 << 3, 
	AMOTION_EVENT_BUTTON_FORWARD			= 1 << 4, 
	AMOTION_EVENT_BUTTON_STYLUS_PRIMARY 	= 1 << 5, 
	AMOTION_EVENT_BUTTON_STYLUS_SECONDARY	= 1 << 6, 
};

//
// Logging
//

enum
{
	ANDROID_LOG_UNKNOWN	= 0, 
	ANDROID_LOG_DEFAULT	= 1, 
	ANDROID_LOG_VERBOSE	= 2, 
	ANDROID_LOG_DEBUG	= 3, 
	ANDROID_LOG_INFO	= 4, 
	ANDROID_LOG_WARN	= 5, 
	ANDROID_LOG_ERROR	= 6, 
	ANDROID_LOG_FATAL	= 7, 
	ANDROID_LOG_SILENT	= 8, 
};

// dlopen
#define RTLD_LAZY		0x00001
#define RTLD_LOCAL		0x00000

#if defined(__LP64__)
	#define RTLD_NOW	0x00002
	#define RTLD_GLOBAL	0x00100
#else
	#define RTLD_NOW	0x00000
	#define RTLD_GLOBAL	0x00002
#endif

// pthread
#if defined(__LP64__)
	#define PTHREAD_STACK_MIN (4 * PAGE_SIZE)
#else
	#define PTHREAD_STACK_MIN (2 * PAGE_SIZE)
#endif

#define PTHREAD_CREATE_DETACHED 1
#define PTHREAD_CREATE_JOINABLE 0

// mmap
#define MAP_PRIVATE		0x02
#define MAP_ANONYMOUS	0x20
#define PROT_READ		0x1
#define PROT_WRITE		0x2

struct ANativeActivity;
struct ANativeActivityCallbacks
{
    void	(*onStart)(ANativeActivity *activity);
    void	(*onResume)(ANativeActivity *activity);
    void *	(*onSaveInstanceState)(ANativeActivity * activity, size_t *outSize);
    void	(*onPause)(ANativeActivity *activity);
    void	(*onStop)(ANativeActivity *activity);
    void	(*onDestroy)(ANativeActivity *activity);
    void	(*onWindowFocusChanged)(ANativeActivity *activity, int hasFocus);
    void	(*onNativeWindowCreated)(ANativeActivity *activity, ANativeWindow *window);
    void	(*onNativeWindowResized)(ANativeActivity *activity, ANativeWindow *window);
    void	(*onNativeWindowRedrawNeeded)(ANativeActivity *activity, ANativeWindow *window);
    void	(*onNativeWindowDestroyed)(ANativeActivity *activity, ANativeWindow *window);
    void	(*onInputQueueCreated)(ANativeActivity *activity, AInputQueue *queue);
    void	(*onInputQueueDestroyed)(ANativeActivity *activity, AInputQueue *queue);
    void	(*onContentRectChanged)(ANativeActivity *activity, ARect *rect);
    void	(*onConfigurationChanged)(ANativeActivity *activity);
	void	(*onLowMemory)(ANativeActivity *activity);
};

struct ANativeActivity
{
    ANativeActivityCallbacks *callbacks;
    JavaVM *		vm;
    JNIEnv *		env;
    jobject			clazz;
    char *			internalDataPath;
    char *			externalDataPath;
    s32				sdkVersion;
    void *			instance;
    AAssetManager *	assetManager;
    char *			obbPath;
};

//
// Functions
//

extern "C"
{
	// android
	void	ANativeActivity_finish(ANativeActivity *activity);
	int32_t ANativeWindow_setBuffersGeometry(ANativeWindow* window, int32_t width, int32_t height, int32_t format);
	
	int32_t	AInputQueue_getEvent(AInputQueue *queue, AInputEvent **outEvent);
	int32_t	AInputQueue_preDispatchEvent(AInputQueue *queue, AInputEvent *event);
	void	AInputQueue_finishEvent(AInputQueue *queue, AInputEvent *event, int handled);
	int32_t	AInputEvent_getType(AInputEvent* event);
	
	int32_t	AMotionEvent_getAction(AInputEvent *motion_event);
	int32_t	AMotionEvent_getPointerId(AInputEvent *motion_event, size_t pointer_index);
	size_t	AMotionEvent_getPointerCount(AInputEvent *motion_event);
	float	AMotionEvent_getX(AInputEvent *motion_event, size_t pointer_index);
	float	AMotionEvent_getY(AInputEvent *motion_event, size_t pointer_index);
	
	// system
	volatile int*__errno();
	#define errno (*__errno())
	
	void *	mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
	int		clock_getres(clockid_t __clock, timespec *__resolution);
	int		clock_gettime(clockid_t __clock, timespec *__ts);
	time_t	time(time_t *__t);
	tm *	localtime_r(time_t *__t, tm *__tm);
	int		nanosleep(timespec *req, timespec *rem);
	
	int		fstat(int fildes, stat *buf);
	int		open(char *pathname, int flags);
	ssize_t read(int fd, void *buf, size_t count);

	
	// pthread
	int pthread_attr_init(pthread_attr_t *attr);
	int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
	int pthread_attr_destroy(pthread_attr_t *__attr);
	int pthread_create(pthread_t *tid, pthread_attr_t *attr, void *(*start)(void *arg), void *arg);
	
	// DEBUG hotloading
	char *			dlerror();
	void *			dlopen(char *filename, int flag);
	void *			dlsym(void* handle, char* symbol);
	int				dlclose(void *handle);
	
	// DEBUG logging
	// int				__android_log_write(int prio, char *tag, char *text);
	int				__android_log_vprint(int prio, char *tag, char *fmt, va_list ap);
}
