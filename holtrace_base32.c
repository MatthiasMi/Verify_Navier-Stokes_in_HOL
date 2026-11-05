#include <stdio.h>
#include <stdlib.h>
#include "holtrace_base32.h"

long long holtrace_base32(char **s,const char *err,long long id,long long toobig)
{
  char *t = *s;
  // handle the common cases first
  if (id >= 10 && toobig >= id) {
    if (*t == '!') { *s = t+1; return id-1; }
    if (*t == '@') { *s = t+1; return id-2; }
    if (*t == '#') { *s = t+1; return id-3; }
    if (*t == '$') { *s = t+1; return id-4; }
    if (*t == '%') { *s = t+1; return id-5; }
    if (*t == '^') { *s = t+1; return id-6; }
    if (*t == '&') { *s = t+1; return id-7; }
    if (*t == '*') { *s = t+1; return id-8; }
    if (*t == '(') { *s = t+1; return id-9; }
    if (*t == ')') { *s = t+1; return id-10; }
  }
  int negate = 0;
  long long result = 0;
  if (*t == '\n') goto done;
  switch(*t) {
    case '!': ++t; result = id-1; goto done;
    case '@': ++t; result = id-2; goto done;
    case '#': ++t; result = id-3; goto done;
    case '$': ++t; result = id-4; goto done;
    case '%': ++t; result = id-5; goto done;
    case '^': ++t; result = id-6; goto done;
    case '&': ++t; result = id-7; goto done;
    case '*': ++t; result = id-8; goto done;
    case '(': ++t; result = id-9; goto done;
    case ')': ++t; result = id-10; goto done;
  }
  if (*t == '-') { ++t; negate = 1; }
  // not enforcing uniqueness:
  // * will accept empty string for 0
  // * will accept leading zeros
  // * will accept IDs that could have used ! etc.
  for (;;) {
    char c = *t;
    if (c >= '0' && c <= '9') {
      if (result >= 288230376151711744LL) goto fail;
      result = 32*result+(c-'0');
    } else if (c >= 'A' && c <= 'V') {
      if (result >= 288230376151711744LL) goto fail;
      result = 32*result+(c-'7');
    } else break;
    ++t;
  }

  done:
  if (negate) result = id-result;
  if (result < 0) goto fail;
  if (toobig >= 0 && result >= toobig) goto fail;
  if (*t == ' ') ++t;
  *s = t;
  return result;

  fail:
  fprintf(stderr,"%s %lld: id %lld out of range\n",err,id,result);
  exit(100);
}
