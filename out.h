#ifndef out_h
#define out_h

#include <string.h>

#define OUT_MAX 16384
extern char out_buf[OUT_MAX];
extern int out_pos; // always between 0 and OUT_MAX
extern void out_write1(const char *,unsigned long long);

static inline
void out_flush(void)
{
  if (!out_pos) return;
  out_write1(out_buf,out_pos);
  out_pos = 0;
}

static inline
void out(const char *out_x,long long out_xlen)
{
  if (out_xlen < 0) return;
  if (out_pos) {
    if (out_xlen <= OUT_MAX-out_pos) {
      memcpy(out_buf+out_pos,out_x,out_xlen);
      out_pos += out_xlen;
      return;
    }
    out_flush();
  }
  if (out_xlen < OUT_MAX) {
    memcpy(out_buf,out_x,out_xlen);
    out_pos = out_xlen;
    return;
  }
  out_write1(out_x,out_xlen);
}

static inline
void out_string(const char *out_x)
{
  out(out_x,strlen(out_x));
}

static inline
void out_ulonglong(unsigned long long out_u)
{
  int out_ulen = 0; // will output out_ulen+1 bytes
  unsigned long long out_tenlen = 1; // nice tea
  unsigned long long out_q = out_u;
  while (out_q > 9) {
    out_tenlen *= 10;
    out_q /= 10;
    ++out_ulen;
  }
  do {
    char out_c = '0'+((out_u/out_tenlen)%10);
    out_tenlen /= 10;
    out(&out_c,1);
  } while (out_ulen--);
}

static inline
void out_longlong(long long out_i)
{
  if (out_i >= 0)
    out_ulonglong(out_i);
  else {
    out("-",1);
    out_ulonglong(-(unsigned long long) out_i);
  }
}

#endif
