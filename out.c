#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "out.h"

char out_buf[OUT_MAX];
int out_pos;

void out_write1(const char *s,unsigned long long slen)
{
  while (slen > 0) {
    size_t todo = OUT_MAX;
    if (slen < todo) todo = slen;
    ssize_t w = write(1,s,todo);
    if (w == 0) {
      sleep(1);
      continue;
    }
    if (w < 0) {
      if (errno == EINTR) continue;
      fprintf(stderr,"out: write failed: %s\n",strerror(errno));
      exit(111);
    }
    s += w;
    slen -= w;
  }
}
