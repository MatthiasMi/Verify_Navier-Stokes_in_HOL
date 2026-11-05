#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "in.h"

char *in_buf;
long long in_bytes;

void in_init(void)
{
  if (in_buf) return;

  size_t in_alloc = 1048576;

  // even for regular files, obtain snapshot rather than using mmap
  // but still peek at file size to allocate sensibly to begin with
  // also: don't use getdelim; it would change the \0 semantics

  struct stat st;
  if (fstat(0,&st) == -1) {
    fprintf(stderr,"in: fstat failed: %s\n",strerror(errno));
    exit(111);
  }
  if (S_ISREG(st.st_mode)) {
    in_alloc = st.st_size;
    if (in_alloc < 1048576) in_alloc = 1048576;
  }

  in_bytes = 0;
  in_buf = malloc(in_alloc);
  if (!in_buf) {
    fprintf(stderr,"in: malloc failed\n");
    exit(111);
  }
  for (;;) {
    size_t todo = 1048576;
    while (in_alloc-in_bytes < todo) {
      size_t newalloc = in_alloc+in_alloc/8+1;
      if (newalloc < in_alloc) {
        fprintf(stderr,"in: newalloc overflow\n");
        exit(111);
      }
      in_buf = realloc(in_buf,newalloc);
      if (!in_buf) {
        fprintf(stderr,"in: realloc failed\n");
        exit(111);
      }
      in_alloc = newalloc;
    }
    ssize_t r = read(0,in_buf+in_bytes,todo);
    if (r == 0) {
      memset(in_buf+in_bytes,0,in_alloc-in_bytes);
      return;
    }
    if (r < 0) {
      fprintf(stderr,"in: read failed: %s\n",strerror(errno));
      exit(111);
    }
    in_bytes += r;
  }
}
