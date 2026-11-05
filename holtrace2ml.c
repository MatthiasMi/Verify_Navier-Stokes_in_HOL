#include <stdio.h>
#include <stdlib.h>
#include "in.h"
#include "out.h"
#define outs out_string
#include "holtrace_base32.h"

typedef long long num;

static num scan_subid(char **s,num id)
{
  return holtrace_base32(s,"id",id,id);
}

static void print_until(const char *s,char split)
{
  for (;;) {
    char c = *s++;
    if (c == split) return;
    if (c < 32 || c >= 127) {
      fprintf(stderr,"prohibited character %d\n",(int) c);
      exit(100);
    }
    if (c == '"' || c == '\\') out("\\",1);
    out(&c,1);
  }
}

static void print_name(char *s)
{
  print_until(s,'\n');
}

num typeid = 0;
num termid = 0;
num thmid = 0;
num infid = 0;
num typedefid = 0;

static void handleline(char *s,num linenum)
{
  char tag = *s++;

  // types
  if (tag == 'a') {
    outs("let Y"); out_longlong(typeid);
    outs("=mk_vartype(\"");
    print_name(s);
    outs("\");;\n");
    ++typeid;
    return;
  }
  if (tag == 'b') {
    num numargs = scan_subid(&s,-1);
    char *name = s;
    for (num i = 0;i < numargs;++i)
      scan_subid(&name,typeid);
    outs("let Y"); out_longlong(typeid);
    outs("=mk_type(\"");
    print_name(name);
    outs("\",[");
    for (num i = 0;i < numargs;++i) {
      if (i > 0) outs(";");
      outs("Y");
      out_longlong(scan_subid(&s,typeid));
    }
    outs("]);;\n");
    ++typeid;
    return;
  }

  // terms
  if (tag == 'c' || tag == 'd') {
    num y = scan_subid(&s,typeid);
    outs("let E"); out_longlong(termid);
    if (tag == 'c')
      outs("=mk_mconst(\"");
    else
      outs("=mk_var(\"");
    print_name(s);
    outs("\",Y");
    out_longlong(y);
    outs(");;\n");
    ++termid;
    return;
  }
  if (tag == 'e' || tag == 'f') {
    scan_subid(&s,typeid);
    num t = scan_subid(&s,termid);
    num u = scan_subid(&s,termid);
    outs("let E"); out_longlong(termid);
    if (tag == 'e')
      outs("=mk_comb(E");
    else
      outs("=mk_abs(E");
    out_longlong(t);
    outs(",E");
    out_longlong(u);
    outs(");;\n");
    ++termid;
    return;
  }

  // theorems
  if (tag == 't') {
    if (!infid) {
      fprintf(stderr,"line %lld: must have inference before first t line)\n",linenum);
      exit(100);
    }
    outs("let T"); out_longlong(thmid);
    outs("=lastinf;;\n");
    outs("assert(dest_thm(T"); out_longlong(thmid);
    outs(")=([");
    int flagfirst = 1;
    for (;;) {
      num h = scan_subid(&s,termid);
      if (*s == '\n') {
        outs("],E");
        out_longlong(h);
        outs("));;\n");
        break;
      }
      if (!flagfirst) outs(";");
      flagfirst = 0;
      outs("E");
      out_longlong(h);
    }
    ++thmid;
    return;
  }

  // primitive inferences
  if (tag == 'B' || tag == 'R' || tag == 'U') {
    outs("let lastinf=");
    if (tag == 'B') outs("BETA E");
    if (tag == 'R') outs("REFL E");
    if (tag == 'U') outs("ASSUME E");
    out_longlong(scan_subid(&s,termid));
    outs(";;\n");
    ++infid;
    return;
  }
  if (tag == 'E') {
    outs("let lastinf=");
    outs("MK_COMB(T");
    out_longlong(scan_subid(&s,thmid));
    outs(",T");
    out_longlong(scan_subid(&s,thmid));
    outs(");;\n");
    ++infid;
    return;
  }
  if (tag == 'F') {
    outs("let lastinf=");
    outs("ABS E");
    out_longlong(scan_subid(&s,termid));
    outs(" T");
    out_longlong(scan_subid(&s,thmid));
    outs(";;\n");
    ++infid;
    return;
  }
  if (tag == 'P' || tag == 'V' || tag == 'Z') {
    outs("let lastinf=");
    if (tag == 'P') outs("EQ_MP T");
    if (tag == 'V') outs("TRANS T");
    if (tag == 'Z') outs("DEDUCT_ANTISYM_RULE T");
    out_longlong(scan_subid(&s,thmid));
    outs(" T");
    out_longlong(scan_subid(&s,thmid));
    outs(";;\n");
    ++infid;
    return;
  }
  if (tag == 'S' || tag == 'T') {
    num thm = scan_subid(&s,thmid);
    outs("let lastinf=");
    if (tag == 'S')
      outs("INST[");
    else
      outs("INST_TYPE[");
    int flagfirst = 1;
    num id = tag == 'S' ? termid : typeid;
    char idletter = tag == 'S' ? 'E' : 'Y';
    while (*s != '\n') {
      num from = scan_subid(&s,id);
      num to = scan_subid(&s,id);
      if (!flagfirst) outs(";");
      flagfirst = 0;
      out(&idletter,1); out_longlong(to);
      outs(",");
      out(&idletter,1); out_longlong(from);
    }
    outs("]T"); out_longlong(thm);
    outs(";;\n");
    ++infid;
    return;
  }

  // other inferences
  if (tag == 'X') {
    outs("let lastinf=");
    outs("holtrace_axiom E");
    out_longlong(scan_subid(&s,termid));
    outs(";;\n");
    ++infid;
    return;
  }
  if (tag == 'C') {
    outs("let lastinf=");
    outs("holtrace_definition E");
    out_longlong(scan_subid(&s,termid));
    outs(" \"");
    print_name(s);
    outs("\";;\n");
    ++infid;
    return;
  }
  if (tag == 'A') {
    num reptype = scan_subid(&s,typeid);
    num repprop = scan_subid(&s,termid);
    num repth = scan_subid(&s,thmid);
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
      fprintf(stderr,"line %lld: misformatted A line\n",linenum);
      exit(100);
    }
    for (int stage = 0;stage < 2;++stage) {
      if (stage == 0)
        outs("let lastinf=");
      else {
        outs("let typedef"); out_longlong(typedefid);
        outs("=");
      }
      outs("holtrace_type_definition");
      outs(stage ? "D" : "A");
      outs(" Y"); out_longlong(reptype);
      outs(" E"); out_longlong(repprop);
      outs(" T"); out_longlong(repth);
      outs(" \""); print_until(abstypename,split);
      outs("\" \""); print_until(absname,split);
      outs("\" \""); print_until(repname,split);
      outs("\" [");
      const char *tmp = tyvars;
      while (tmp && *tmp != '\n') {
        const char *tyvar = tmp;
        do ++tmp; while (*tmp != split && *tmp != '\n');
        outs("\"");
        print_until(tyvar,*tmp);
        outs("\";");
        if (*tmp == split) ++tmp;
      }
      outs("];;\n");
    }
    ++infid;
    ++typedefid;
    return;
  }
  if (tag == 'D') {
    num id = scan_subid(&s,typedefid);
    outs("let lastinf=typedef");
    out_longlong(id);
    outs(";;\n");
    ++infid;
    return;
  }

  // other tags

  if (tag == '/') {
    num z = scan_subid(&s,thmid);
    char *t = s;
    for (;;) {
      char c = *t;
      if (c == '_' || c == '\'' || c == '.' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        ++t;
        continue;
      }
      break;
    }
    if (t == s || *t != '\n' || *s == '\'' || (*s >= '0' && *s <= '9')) {
      fprintf(stderr,"line %lld: unsuitable name\n",linenum);
      exit(100);
    }
    // important to do dest_thm here in case theorem is augmented
    // (by, e.g., ProofTrace)
    outs("assert(dest_thm(T"); out_longlong(z);
    outs(")=dest_thm(");
    print_name(s);
    outs("));;\n");
    return;
  }

  if (tag == ',') return;
  if (tag == ';') return;

  fprintf(stderr,"line %lld: unrecognized tag\n",linenum);
  exit(100);
}

int main()
{
  in_init();

  outs("let holtrace_axiom term =\n");
  outs("let target = [],term in\n");
  outs("let istarget = fun f -> dest_thm f = target in\n");
  outs("find istarget (axioms());;\n");
  outs("\n");

  outs("let holtrace_definition term name =\n");
  outs("let v,t = dest_eq term in\n");
  outs("let vname,vtype = dest_var(v) in\n");
  outs("if vname <> name then\n");
  outs("  failwith \"name mismatch\"\n");;
  outs("else\n");
  outs("let c = mk_mconst(vname,vtype) in\n");
  outs("let target = [],mk_eq(c,t) in\n");
  outs("let istarget = fun f -> dest_thm f = target in\n");
  outs("if not(exists istarget (definitions())) then\n");
  outs("  failwith \"constant not defined\"\n");
  outs("else\n");
  outs("find istarget (definitions());;\n"); 
  outs("\n");

  for (int stage = 0;stage < 2;++stage) {
    outs("let holtrace_type_definition");
    outs(stage ? "D" : "A");
    outs(" reptype repprop repth abstypename absname repname tyvarnames =\n");
    outs("let tyvars = map mk_vartype tyvarnames in\n");
    outs("let abstype = mk_type(abstypename,tyvars) in\n");
    outs("let thconcl = match dest_thm repth with | [],concl -> concl | _ -> failwith \"thconcl failure\" in\n");
    outs("let thprop,_ = try dest_comb thconcl with Failure _ -> failwith \"thprop failure\" in assert(repprop = thprop);\n");
    outs("let abs = mk_mconst(absname,mk_fun_ty reptype abstype) in\n");
    outs("let rep = mk_mconst(repname,mk_fun_ty abstype reptype) in\n");
    if (stage) {
      outs("let repvar = mk_var(\"r\",reptype) in\n");
      outs("let target = mk_eq(mk_comb(repprop,repvar),mk_eq(mk_comb(rep,mk_comb(abs,repvar)),repvar)) in\n");
    } else {
      outs("let absvar = mk_var(\"a\",abstype) in\n");
      outs("let target = mk_eq(mk_comb(abs,mk_comb(rep,absvar)),absvar) in\n");
    }
    outs("let istarget = fun f -> dest_thm f = ([],target) in\n");
    outs("find istarget (holtrace_typedef_thms());;\n"); 
    outs("\n");
  }

  if (in_bytes < 11 || memcmp(in_buf,"HOLTrace 1\n",11)) {
    fprintf(stderr,"input does not start with HOLTrace 1\n");
    exit(100);
  }

  num linenum = 0;
  char *linestart = in_buf;
  for (num i = 0;i < in_bytes;++i)
    if (in_buf[i] == '\n') {
      if (++linenum > 1)
        handleline(linestart,linenum);
      linestart = &in_buf[i+1];
    }

  out_flush();
  return 0;
}
