#ifndef VECTOR_h
#define VECTOR_h

#include <stdlib.h>
#include <string.h>

#define VECTOR_declare_typedef(X) \
  typedef struct { \
    X *VECTOR_data; \
    long long VECTOR_len; \
    long long VECTOR_allocated; \
  } vector_##X;

#define VECTOR_declare(X) \
  VECTOR_declare_typedef(X) \
  extern void vector_##X##_allocate(vector_##X *,long long); \
  extern X vector_##X##_read(const vector_##X *,long long); \
  extern void vector_##X##_write(vector_##X *,long long,X); \
  extern void vector_##X##_append(vector_##X *,X); \
  extern void vector_##X##_trunc(vector_##X *,long long); \
  extern void vector_##X##_empty(vector_##X *); \
  extern long long vector_##X##_len(const vector_##X *); \
  extern void vector_##X##_free(vector_##X *); \

#define VECTOR_define_prefix(X,prefix) \
  prefix void vector_##X##_allocate(vector_##X *VECTOR_v,long long VECTOR_n) \
  { \
    void *VECTOR_newdata; \
    \
    if (VECTOR_n <= VECTOR_v->VECTOR_allocated) return; \
    if ((unsigned long long) VECTOR_n >= (1LL<<(8*sizeof(size_t)-3))/sizeof(X)) abort(); \
    if (VECTOR_v->VECTOR_data) { \
      VECTOR_n += VECTOR_n>>3; \
      VECTOR_n += 1; \
    } \
    \
    VECTOR_newdata = reallocarray(VECTOR_v->VECTOR_data,VECTOR_n,sizeof(X)); \
    if (!VECTOR_newdata) abort(); \
    VECTOR_v->VECTOR_data = VECTOR_newdata; \
    memset(VECTOR_v->VECTOR_data+VECTOR_v->VECTOR_allocated,0,sizeof(X)*(VECTOR_n-VECTOR_v->VECTOR_allocated)); \
    VECTOR_v->VECTOR_allocated = VECTOR_n; \
  } \
  \
  prefix X vector_##X##_read(const vector_##X *VECTOR_v,long long VECTOR_pos) \
  { \
    if (VECTOR_pos < 0 || VECTOR_pos >= VECTOR_v->VECTOR_len) { \
      X VECTOR_zero; \
      memset(&VECTOR_zero,0,sizeof VECTOR_zero); \
      return VECTOR_zero; \
    } \
    return VECTOR_v->VECTOR_data[VECTOR_pos]; \
  } \
  \
  prefix void vector_##X##_write(vector_##X *VECTOR_v,long long VECTOR_pos,X VECTOR_x) \
  { \
    if (VECTOR_pos < 0) abort(); \
    if ((unsigned long long) VECTOR_pos >= (1LL<<(8*sizeof(size_t)-3))/sizeof(X)) abort(); \
    vector_##X##_allocate(VECTOR_v,VECTOR_pos+1); \
    if (VECTOR_pos >= VECTOR_v->VECTOR_len) VECTOR_v->VECTOR_len = VECTOR_pos+1; \
    VECTOR_v->VECTOR_data[VECTOR_pos] = VECTOR_x; \
  } \
  prefix void vector_##X##_trunc(vector_##X *VECTOR_v,long long VECTOR_n) \
  { \
    if (VECTOR_v->VECTOR_len > VECTOR_n) { \
      memset(VECTOR_v->VECTOR_data+VECTOR_n,0,(VECTOR_v->VECTOR_len-VECTOR_n)*sizeof(X)); \
      VECTOR_v->VECTOR_len = VECTOR_n; \
    } \
  } \
  prefix void vector_##X##_empty(vector_##X *VECTOR_v) \
  { \
    vector_##X##_trunc(VECTOR_v,0); \
  } \
  prefix long long vector_##X##_len(const vector_##X *VECTOR_v) \
  { \
    return VECTOR_v->VECTOR_len; \
  } \
  prefix void vector_##X##_free(vector_##X *VECTOR_v) \
  { \
    if (VECTOR_v->VECTOR_data) free(VECTOR_v->VECTOR_data); \
    VECTOR_v->VECTOR_data = 0; \
    VECTOR_v->VECTOR_len = 0; \
    VECTOR_v->VECTOR_allocated = 0; \
  } \
  prefix void vector_##X##_append(vector_##X *VECTOR_v,X VECTOR_x) \
  { \
    vector_##X##_write(VECTOR_v,vector_##X##_len(VECTOR_v),VECTOR_x); \
  } \

#define VECTOR_define(X) VECTOR_define_prefix(X,)
#define VECTOR_define_inline(X) \
  VECTOR_declare_typedef(X) \
  VECTOR_define_prefix(X,static inline)

#endif
