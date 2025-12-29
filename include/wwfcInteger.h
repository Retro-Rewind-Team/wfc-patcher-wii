#ifndef WWFC_INTEGER
#define WWFC_INTEGER

#ifdef WWFC_HAVE_STDINT
#  include <stdint.h>
typedef int8_t wwfc_int8_t;
typedef int16_t wwfc_int16_t;
typedef int32_t wwfc_int32_t;
typedef int64_t wwfc_int64_t;
typedef uint8_t wwfc_uint8_t;
typedef uint16_t wwfc_uint16_t;
typedef uint32_t wwfc_uint32_t;
typedef uint64_t wwfc_uint64_t;
#else
#  ifdef __INT8_TYPE__
typedef __INT8_TYPE__ wwfc_int8_t;
#  else
typedef signed char wwfc_int8_t;
#  endif
#  ifdef __INT16_TYPE__
typedef __INT16_TYPE__ wwfc_int16_t;
#  else
typedef short wwfc_int16_t;
#  endif
#  ifdef __INT32_TYPE__
typedef __INT32_TYPE__ wwfc_int32_t;
#  else
typedef int wwfc_int32_t;
#  endif
#  ifdef __INT64_TYPE__
typedef __INT64_TYPE__ wwfc_int64_t;
#  else
typedef long long int wwfc_int64_t;
#  endif
#  ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__ wwfc_uint8_t;
#  else
typedef unsigned char wwfc_int8_t;
#  endif
#  ifdef __UINT16_TYPE__
typedef __UINT16_TYPE__ wwfc_uint16_t;
#  else
typedef unsigned short wwfc_int16_t;
#  endif
#  ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ wwfc_uint32_t;
#  else
typedef unsigned int wwfc_uint32_t;
#  endif
#  ifdef __UINT64_TYPE__
typedef __UINT64_TYPE__ wwfc_uint64_t;
#  else
typedef long long unsigned int wwfc_uint64_t;
#  endif
#endif

#endif // WWFC_INTEGER