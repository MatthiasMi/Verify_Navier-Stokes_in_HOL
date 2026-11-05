#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "in.h"
#include "out.h"
#define outs out_string
#include "holtrace_base32.h"

// ===== parsing

static long long scan_subid(char **s,long long id)
{
  return holtrace_base32(s,"id",id,id);
}

// ===== basic printing

static int double_backslashes = 0;

// name is not allowed to contain double quotes but can contain backslashes
// also: \n-terminated, not \0-terminated
static void print_name(char *s)
{
  while (*s != '\n') {
    if (double_backslashes && *s == '\\') out("\\",1);
    out(s,1);
    ++s;
  }
}

// more general version for A tags
// for efficiency, avoid merging with print_name
static void print_until(char *s,int split)
{
  while (*s != split) {
    if (double_backslashes && *s == '\\') out("\\",1);
    out(s,1);
    ++s;
  }
}

static void print_abstype(char *abstypename,char *tyvars,char split)
{
  outs("c[");
  print_until(abstypename,split);
  outs("][");
  while (tyvars && *tyvars != '\n') {
    char *tyvar = tyvars;
    do ++tyvars; while (*tyvars != split && *tyvars != '\n');
    outs("[v[");
    print_until(tyvar,*tyvars);
    outs("]]");
    if (*tyvars == split) ++tyvars;
  }
  outs("]");
}

// ===== types

static long long numtypes;
static long long *type2pos;

static void print_type(long long typeid)
{
  if (typeid < 0 || typeid >= numtypes) {
    fprintf(stderr,"typeid %lld out of range\n",typeid);
    exit(100);
  }
  char *s = in_buf+type2pos[typeid];
  char tag = *s++;
  if (tag == 'a') {
    outs("v[");
    print_name(s);
    outs("]");
    return;
  }
  if (tag == 'b') {
    // e.g. "b2 0 1 fun" means fun(0,1)
    // with the first 2 meaning 2 args
    long long numargs = scan_subid(&s,-1);
    char *name = s;
    for (long long i = 0;i < numargs;++i)
      scan_subid(&name,typeid);
    outs("c[");
    print_name(name);
    outs("][");
    for (long long i = 0;i < numargs;++i) {
      outs("[");
      print_type(scan_subid(&s,typeid));
      outs("]");
    }
    outs("]");
    return;
  }
  fprintf(stderr,"typeid %lld: unrecognized tag %c\n",typeid,tag);
  exit(100);
}

// ===== terms

static long long numterms;
static long long *term2pos;
static long long *term2type;

static void print_term(long long termid)
{
  if (termid < 0 || termid >= numterms) {
    fprintf(stderr,"termid %lld out of range\n",termid);
    exit(100);
  }
  char *s = in_buf+term2pos[termid];
  char tag = *s++;
  long long y,z;
  switch(tag) {
    case 'c': case 'd':
      y = scan_subid(&s,term2type[termid]);
      outs((tag == 'c') ? "c(" : "v(");
      print_name(s);
      outs(")(");
      print_type(y);
      outs(")");
      return;
    case 'e': case 'f':
      scan_subid(&s,term2type[termid]);
      y = scan_subid(&s,termid);
      z = scan_subid(&s,termid);
      outs((tag == 'e') ? "C(" : "A(");
      print_term(y);
      outs(")(");
      print_term(z);
      outs(")");
      return;
  }
  fprintf(stderr,"termid %lld: unrecognized tag %c\n",termid,tag);
  exit(100);
}

static void print_term_v_as_c(long long termid)
{
  if (termid < 0 || termid >= numterms) {
    fprintf(stderr,"termid %lld out of range\n",termid);
    exit(100);
  }
  char *s = in_buf+term2pos[termid];
  if (*s++ != 'd') {
    fprintf(stderr,"termid %lld: v_as_c failure\n",termid);
    exit(100);
  }
  long long y = scan_subid(&s,term2type[termid]);
  outs("c(");
  print_name(s);
  outs(")(");
  print_type(y);
  outs(")");
}

static void print_term_fv_as_fc(long long termid)
{
  if (termid < 0 || termid >= numterms) {
    fprintf(stderr,"termid %lld out of range\n",termid);
    exit(100);
  }
  char *s = in_buf+term2pos[termid];
  if (*s++ != 'e') {
    fprintf(stderr,"termid %lld: fv_as_fc failure\n",termid);
    exit(100);
  }
  scan_subid(&s,term2type[termid]);
  long long y = scan_subid(&s,termid);
  long long z = scan_subid(&s,termid);
  outs("C(");
  print_term(y);
  outs(")(");
  print_term_v_as_c(z);
  outs(")");
}

static void print_term_fvx_as_fcx(long long termid)
{
  if (termid < 0 || termid >= numterms) {
    fprintf(stderr,"termid %lld out of range\n",termid);
    exit(100);
  }
  char *s = in_buf+term2pos[termid];
  if (*s++ != 'e') {
    fprintf(stderr,"termid %lld: fvx_as_fcx failure\n",termid);
    exit(100);
  }
  scan_subid(&s,term2type[termid]);
  long long y = scan_subid(&s,termid);
  long long z = scan_subid(&s,termid);
  outs("C(");
  print_term_fv_as_fc(y);
  outs(")(");
  print_term(z);
  outs(")");
}

// ===== theorems

static long long numthms;
static long long *thm2pos;
static long long *thm2term;

static void print_theorem(long long thmid,long long ptid,int allowhypotheses)
{
  if (thmid < 0 || thmid >= numthms) {
    fprintf(stderr,"thmid %lld out of range\n",thmid);
    exit(100);
  }
  char *s = in_buf+thm2pos[thmid];

  if (*s != 't') {
    fprintf(stderr,"thmid %lld points to non-t\n",thmid);
    exit(100);
  }
  ++s;

  if (allowhypotheses) {
    outs("{\"id\": ");
    out_longlong(ptid);
    outs(", \"th\": {\"hy\": [");
  }
  long long hypothesisnum = 0;
  long long termid = thm2term[thmid];
  for (;;) {
    long long subid = scan_subid(&s,termid);
    if (*s == '\n') {
      if (allowhypotheses) outs("], \"cc\": \"");
      print_term(subid);
      break;
    }
    if (!allowhypotheses) {
      fprintf(stderr,"thmid %lld has hypotheses\n",thmid);
      exit(100);
    }
    if (hypothesisnum > 0) outs(", ");
    outs("\"");
    print_term(subid);
    outs("\"");
    ++hypothesisnum;
  }
  if (allowhypotheses) outs("\"}}\n");
}

// ===== inferences

static long long numinfs;
static long long numtypedefs;
static long long *inf2pos;
static long long *inf2thm;
static long long *inf2term;
static long long *inf2type;
static long long *inf2typedef;
static long long *typedef2inf;
static char **typedef2line;

static void print_inference(long long infid,long long tid0,long long tid1)
{
  if (infid < 0 || infid >= numinfs) {
    fprintf(stderr,"infid %lld out of range\n",infid);
    exit(100);
  }
  char *s = in_buf+inf2pos[infid];
  char tag = *s++;
  long long t,u,thmid;

  switch(tag) {
    case 'X': case 'R': case 'U': case 'B':
      // all of these use one previous term
      t = scan_subid(&s,inf2term[infid]);
      if (tag == 'X') outs("AXIOM\", \"");
      if (tag == 'R') outs("REFL\", \"");
      if (tag == 'U') outs("ASSUME\", \"");
      if (tag == 'B') outs("BETA\", \"");
      print_term(t);
      outs("\"");
      break;
    case 'V': case 'E': case 'P': case 'Z':
      // all of these use two previous theorems
      thmid = inf2thm[infid];
      t = scan_subid(&s,thmid);
      u = scan_subid(&s,thmid);
      if (tag == 'V') outs("TRANS\", ");
      if (tag == 'E') outs("MK_COMB\", ");
      if (tag == 'P') outs("EQ_MP\", ");
      if (tag == 'Z') outs("DEDUCT_ANTISYM_RULE\", ");
      out_longlong(tid0); // XXX: check vs. t?
      outs(", ");
      out_longlong(tid1); // XXX: check vs. u?
      break;
    case 'F':
      t = scan_subid(&s,inf2term[infid]);
      u = scan_subid(&s,inf2thm[infid]);
      outs("ABS\", ");
      out_longlong(tid0); // XXX: check vs. u?
      outs(", \"");
      print_term(t);
      outs("\"");
      break;
    case 'S': case 'T':
      if (tag == 'S') outs("INST\", ");
      if (tag == 'T') outs("INST_TYPE\", ");
      t = scan_subid(&s,inf2thm[infid]);
      out_longlong(tid0); // XXX: check vs. t?
      outs(", [");
      int flagfirst = 1;
      long long id = tag == 'S' ? inf2term[infid] : inf2type[infid];
      while (*s != '\n') {
        t = scan_subid(&s,id);
        u = scan_subid(&s,id);
        if (!flagfirst) outs(", ");
        flagfirst = 0;
        outs("[\"");
        if (tag == 'S') print_term(t);
        if (tag == 'T') print_type(t);
        outs("\", \"");
        if (tag == 'S') print_term(u);
        if (tag == 'T') print_type(u);
        outs("\"]");
      }
      outs("]");
      break;
    case 'C':
      outs("DEFINITION\", \"");
      print_term_fvx_as_fcx(scan_subid(&s,inf2term[infid]));
      outs("\", \"");
      print_name(s);
      outs("\"");
      break;
    case 'A': case 'D':
      ;
      long long Ainfid = infid;
      if (tag == 'D') {
        long long id = scan_subid(&s,inf2typedef[infid]);
        Ainfid = typedef2inf[id];
        s = typedef2line[id]+1;
        if (Ainfid < 0 || Ainfid >= infid) {
          fprintf(stderr,"infid %lld from typedef %lld out of range\n",Ainfid,id);
          exit(100);
        }
      }
      long long reptype = scan_subid(&s,inf2type[Ainfid]);
      long long repprop = scan_subid(&s,inf2term[Ainfid]);
      t = scan_subid(&s,inf2thm[Ainfid]);
      int state = 0;
      char split = 0;
      char *abstypename = 0;
      char *absname = 0;
      char *repname = 0;
      char *tyvars = 0;
      for (;*s != '\n';++s)
        switch(state) {
          case 0: split = *s; ++state; break;
          case 1: abstypename = s; ++state; // fall through
          case 2: if (*s == split) ++state; break;
          case 3: absname = s; ++state; // fall through
          case 4: if (*s == split) ++state; break;
          case 5: repname = s; ++state; // fall through
          case 6: if (*s == split) { ++state; tyvars = s+1; }
        }
      if (state != 7) {
        if (tag == 'D')
          fprintf(stderr,"infid %lld: misformatted A line via D line\n",Ainfid);
        else
          fprintf(stderr,"infid %lld: misformatted A line\n",Ainfid);
        exit(100);
      }
      outs("TYPE_DEFINITION\", ");
      out_longlong(tid0); // XXX: check vs. t?
      if (tag == 'A') {
        outs(", \"C(C(c(=)(c[fun][["); print_abstype(abstypename,tyvars,split);
        outs("][c[fun][["); print_abstype(abstypename,tyvars,split);
        outs("][c[bool][]]]]]))(C(c("); print_until(absname,split);
        outs(")(c[fun][["); print_type(reptype);
        outs("]["); print_abstype(abstypename,tyvars,split);
        outs("]]))(C(c("); print_until(repname,split);
        outs(")(c[fun][["); print_abstype(abstypename,tyvars,split);
        outs("]["); print_type(reptype);
        outs("]]))(v(a)("); print_abstype(abstypename,tyvars,split);
        outs(")))))(v(a)("); print_abstype(abstypename,tyvars,split);
        outs("))\", \""); print_until(absname,split);
      } else {
        outs(", \"C(C(c(=)(c[fun][[c[bool][]][c[fun][[c[bool][]][c[bool][]]]]]))(C(");
        print_term(repprop);
        outs(")(v(r)("); print_type(reptype);
        outs("))))(C(C(c(=)(c[fun][["); print_type(reptype);
        outs("][c[fun][["); print_type(reptype);
        outs("][c[bool][]]]]]))(C(c("); print_until(repname,split);
        outs(")(c[fun][["); print_abstype(abstypename,tyvars,split);
        outs("]["); print_type(reptype);
        outs("]]))(C(c("); print_until(absname,split);
        outs(")(c[fun][["); print_type(reptype);
        outs("]["); print_abstype(abstypename,tyvars,split);
        outs("]]))(v(r)("); print_type(reptype);
        outs(")))))(v(r)("); print_type(reptype);
        outs(")))\", \""); print_until(repname,split);
      }
      outs("\"");
      break;
    default:
      fprintf(stderr,"infid %lld: unrecognized tag %c\n",infid,tag);
      exit(100);
  }
}

// ===== setup

static void internal(void)
{
  fprintf(stderr,"internal failure\n");
  exit(100);
}

static void nomem(void)
{
  fprintf(stderr,"malloc failed\n");
  exit(111);
}

static void allocate(void)
{
  numtypes = 0;
  numterms = 0;
  numthms = 0;
  numinfs = 0;
  numtypedefs = 0;

  int startline = 0; // to skip first line
  long long linenum = 0;
  for (long long i = 0;i < in_bytes;++i) {
    char c = in_buf[i];
    if (c < 32 || c >= 127) {
      if (c != '\n') {
        fprintf(stderr,"byte %lld is not printable ASCII\n",i);
        exit(100);
      }
    }
    if (c == '\n') {
      ++linenum;
      startline = 1;
      continue;
    }
    if (startline) {
      switch(c) {
        case 'a': case 'b':
          ++numtypes; break;
        case 'c': case 'd': case 'e': case 'f':
          ++numterms; break;
        case 't':
          ++numthms; break;
        case 'A': case 'B': case 'C': case 'D':
        case 'E': case 'F': case 'P': case 'R':
        case 'S': case 'T': case 'U': case 'V':
        case 'X': case 'Z':
          if (c == 'A') ++numtypedefs;
          ++numinfs; break;
        case '/': case ',': case ';':
          break;
        default:
          fprintf(stderr,"line %lld: unrecognized tag %c\n",linenum,c);
          exit(100);
      }
      startline = 0;
    }
  }

  type2pos = calloc(numtypes,sizeof(long long));
  if (!type2pos) nomem();
  term2pos = calloc(numterms,sizeof(long long));
  if (!term2pos) nomem();
  term2type = calloc(numterms,sizeof(long long));
  if (!term2type) nomem();
  thm2pos = calloc(numthms,sizeof(long long));
  if (!thm2pos) nomem();
  thm2term = calloc(numthms,sizeof(long long));
  if (!thm2term) nomem();
  inf2pos = calloc(numinfs,sizeof(long long));
  if (!inf2pos) nomem();
  inf2type = calloc(numinfs,sizeof(long long));
  if (!inf2type) nomem();
  inf2term = calloc(numinfs,sizeof(long long));
  if (!inf2term) nomem();
  inf2thm = calloc(numinfs,sizeof(long long));
  if (!inf2thm) nomem();
  inf2typedef = calloc(numinfs,sizeof(long long));
  if (!inf2typedef) nomem();
  typedef2inf = calloc(numtypedefs,sizeof(long long));
  if (!typedef2inf) nomem();
  typedef2line = calloc(numtypedefs,sizeof(long long));
  if (!typedef2line) nomem();
}

static void linemap(void)
{
  long long typeid = 0;
  long long termid = 0;
  long long thmid = 0;
  long long infid = 0;
  long long typedefid = 0;

  int startline = 0; // to skip first line
  for (long long i = 0;i < in_bytes;++i) {
    char tag = in_buf[i];
    if (tag == '\n') {
      startline = 1;
      continue;
    }
    if (startline) {
      switch(tag) {
        case 'a': case 'b':
          if (typeid >= numtypes) internal();
          type2pos[typeid] = i;
          ++typeid; break;
        case 'c': case 'd': case 'e': case 'f':
          if (termid >= numterms) internal();
          term2pos[termid] = i;
          term2type[termid] = typeid;
          ++termid; break;
        case 't':
          if (thmid >= numthms) internal();
          thm2pos[thmid] = i;
          thm2term[thmid] = termid;
          ++thmid; break;
        case 'A': case 'B': case 'C': case 'D':
        case 'E': case 'F': case 'P': case 'R':
        case 'S': case 'T': case 'U': case 'V':
        case 'X': case 'Z':
          if (tag == 'A') {
            if (typedefid >= numtypedefs) internal();
            typedef2inf[typedefid] = infid;
            typedef2line[typedefid] = in_buf+i;
            ++typedefid;
          }
          if (infid >= numinfs) internal();
          inf2pos[infid] = i;
          inf2type[infid] = typeid;
          inf2term[infid] = termid;
          inf2thm[infid] = thmid;
          inf2typedef[infid] = typedefid;
          ++infid; break;
        case '/': case ',': case ';':
          break;
      }
      startline = 0;
    }
  }
  if (!startline) {
    fprintf(stderr,"partial final line not permitted\n");
    exit(100);
  }
  if (typeid != numtypes) internal();
  if (termid != numterms) internal();
  if (thmid != numthms) internal();
  if (infid != numinfs) internal();
  if (typedefid != numtypedefs) internal();
}

static int printnames = 0;
static int printproofs = 0;
static int printtheorems = 0;
static int printnamedtheorems = 0;
static char **printthmlist = 0;
static char **printithmlist = 0;

static void handle_postulate(long long pos,long long termid,const char *label)
{
  if (!printnamedtheorems) return;
  for (long long posend = pos;;++posend) {
    if (posend < 0 || posend >= in_bytes) {
      fprintf(stderr,"pos %lld stretches beyond array\n",pos);
      exit(100);
    }
    if (in_buf[posend] == '\n') break;
  }
  char *s = in_buf+pos+1;

  long long subid = scan_subid(&s,termid);

  outs(label);
  outs(" ");
  double_backslashes = 0;
  print_term(subid);
  outs("\n");
}

static void handle_typedef(long long pos,long long infid)
{
  if (!printnamedtheorems) return;
  if (infid < 0 || infid >= numinfs) internal();
  for (long long posend = pos;;++posend) {
    if (posend < 0 || posend >= in_bytes) {
      fprintf(stderr,"pos %lld stretches beyond array\n",pos);
      exit(100);
    }
    if (in_buf[posend] == '\n') break;
  }
  char *s = in_buf+pos+1;
  scan_subid(&s,inf2type[infid]);
  long long repprop = scan_subid(&s,inf2term[infid]);
  scan_subid(&s,inf2thm[infid]);

  outs("typedef ");
  double_backslashes = 0;
  print_term(repprop);
  outs(" ");
  print_name(s);
  outs("\n");
}

static void handle_slash(long long pos,long long thmid,long long ptid)
{
  if (!printthmlist && !printnamedtheorems && !printnames) return;
  for (long long posend = pos;;++posend) {
    if (posend < 0 || posend >= in_bytes) {
      fprintf(stderr,"ptid %lld beyond array\n",ptid);
      exit(100);
    }
    if (in_buf[posend] == '\n') break;
  }
  char *s = in_buf+pos;
  if (*s++ != '/') internal();

  long long subid = scan_subid(&s,thmid);
  char *name = s;
  char *endname = name;
  while (*endname != ' ' && *endname != '\n') ++endname;
  int printthistheorem = printnamedtheorems;
  if (printthmlist) {
    char **x = printthmlist;
    while (*x) {
      if (endname-name == (long long) strlen(*x))
        if (strncmp(name,*x,strlen(*x)) == 0)
          printthistheorem = 1;
      ++x;
    }
  }
  if (printthistheorem) {
    outs("theorem ");
    out(name,endname-name);
    outs(" ");
    double_backslashes = 0;
    print_theorem(subid,ptid,0);
    outs("\n");
  }
  if (printnames) {
    outs("{\"id\": ");
    out_longlong(ptid);
    outs(", \"nm\": \"");
    out(name,endname-name);
    outs("\"}\n");
  }
}

static void handle_comma(long long pos,long long thmid,long long infid,long long ptid)
{
  for (long long posend = pos;;++posend) {
    if (posend < 0 || posend >= in_bytes) {
      fprintf(stderr,"ptid %lld beyond array\n",ptid);
      exit(100);
    }
    if (in_buf[posend] == '\n') break;
  }
  char *s = in_buf+pos;
  if (*s++ != ',') internal();

  int printthistheorem = printtheorems;
  if (printithmlist) {
    char **x = printithmlist;
    while (*x) {
      char *endptr = 0;
      long long target = strtoll(*x,&endptr,10);
      if (*endptr == 0 && ptid == target)
        printthistheorem = 1;
      ++x;
    }
  }
  if (printthistheorem || printproofs) {
    long long subthmid = scan_subid(&s,thmid);
    long long subinfid = scan_subid(&s,infid);
    long long ptid0 = 0;
    if (*s != '\n') ptid0 = scan_subid(&s,ptid);
    long long ptid1 = 0;
    if (*s != '\n') ptid1 = scan_subid(&s,ptid);
    double_backslashes = 1;
    if (printthistheorem) print_theorem(subthmid,ptid,1);
    if (printproofs) {
      outs("{\"id\": ");
      out_longlong(ptid);
      outs(", \"pr\": [\"");
      print_inference(subinfid,ptid0,ptid1);
      outs("]}\n");
    }
  }
}

static void doit(void)
{
  long long typeid = 0;
  long long termid = 0;
  long long thmid = 0;
  long long infid = 0;
  long long ptid = 0;

  int startline = 0; // to skip first line
  for (long long i = 0;i < in_bytes;++i) {
    char tag = in_buf[i];
    if (tag == '\n') {
      startline = 1;
      continue;
    }
    if (startline) {
      switch(tag) {
        case 'a': case 'b':
          ++typeid; break;
        case 'c': case 'd': case 'e': case 'f':
          ++termid; break;
        case 't':
          ++thmid; break;
        case 'A': case 'B': case 'C': case 'D':
        case 'E': case 'F': case 'P': case 'R':
        case 'S': case 'T': case 'U': case 'V':
        case 'X': case 'Z':
          if (tag == 'A') handle_typedef(i,infid);
          if (tag == 'C') handle_postulate(i,termid,"definition");
          if (tag == 'X') handle_postulate(i,termid,"axiom");
          ++infid; break;
        case ';':
          ++ptid; break;
        case ',':
          handle_comma(i,thmid,infid,ptid);
          ++ptid; break;
        case '/':
          handle_slash(i,thmid,ptid);
          break;
      }
      startline = 0;
    }
  }
  if (thmid != numthms) internal();
  if (infid != numinfs) internal();
}

int main(int argc,char **argv)
{
  if (argc >= 2) {
    if (strcmp(argv[1],"proofs") == 0) printproofs = 1;
    if (strcmp(argv[1],"theorems") == 0) printtheorems = 1;
    if (strcmp(argv[1],"names") == 0) printnames = 1;
    if (strcmp(argv[1],"namedtheorems") == 0) printnamedtheorems = 1;
    if (strcmp(argv[1],"thm") == 0) printthmlist = argv+2;
    if (strcmp(argv[1],"ithm") == 0) printithmlist = argv+2;
  }
      
  in_init();

  if (in_bytes < 11 || memcmp(in_buf,"HOLTrace 1\n",11)) {
    fprintf(stderr,"input does not start with HOLTrace 1\n");
    exit(100);
  }

  allocate();
  linemap();
  doit();

  out_flush();
  return 0;
}
