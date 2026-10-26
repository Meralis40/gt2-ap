/* The few definitions libgcc2.c and tinfo.cc take from <stddef.h>, for a
   32-bit target (tools/libgcc.py builds with -nostdinc). */
#ifndef _STDDEF_H
#define _STDDEF_H
typedef unsigned int size_t;
typedef int ptrdiff_t;
#ifndef __cplusplus
typedef unsigned int wchar_t;
#endif
#define NULL 0
#define offsetof(T, m) ((size_t) &((T *) 0)->m)
#endif
