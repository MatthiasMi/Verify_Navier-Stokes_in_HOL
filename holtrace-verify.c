#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "vector.h"
#include "in.h"

typedef uint8_t uint8;
VECTOR_define_inline(uint8)

typedef long long num;
VECTOR_define_inline(num)

// ===== general constraints

#define CUTOFF 288230376151711743LL
// but likely to abort before that from running out of heap space
// also, could abort from running out of stack space for recursion

// want this in production, ignoring NDEBUG, always having side effects
#define know(property) ((property) ? 0 : (fprintf(stderr,"code line %d: internal error: failed %s\n",__LINE__,#property), exit(100)))

static num curline = 1; // 1-numbered for error messages

static void reject(const char *err)
{
  fprintf(stderr,"line %lld: %s\n",curline,err);
  exit(100);
}

// ===== general parsing

static const num offset[128] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,1,0,3,4,5,7,0,9,10,8,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
} ;

// some allowed variations:
// will accept leading zeros
// will not require, e.g., id-1 to be !
// will accept space even when not needed
static num subid_core(const char **s,num id)
{
  const char *t = *s;
  char c = *t;
  if (c == '\n') reject("missing numeric argument");
  num o = offset[127&c];
  if (o) {
    *s = t+1;
    return id-o;
  }

  int negate = 0;
  if (c == '-') { ++t; negate = 1; }
  num result = 0;
  for (;;) {
    c = *t;
    if (result >= CUTOFF) {
      fprintf(stderr,"line %lld: refusing to handle number %lld\n",curline,result);
      exit(100);
    }
    if (c >= '0' && c <= '9')
      result = 32*result+(c-'0');
    else if (c >= 'A' && c <= 'V')
      result = 32*result+(c-'7');
    else break;
    ++t;
  }
  if (*t == ' ') ++t;
  if (t == *s) reject("non-numeric argument where numeric is expected");
  *s = t;
  return negate ? id-result : result;
}

static num subid(const char **s,num id)
{
  num result = subid_core(s,id);
  if (result < 0 || (result >= id && id >= 0)) {
    fprintf(stderr,"line %lld: result %lld out of range\n",curline,result);
    exit(100);
  }
  return result;
}

static void noteol(const char *s)
{
  if (*s == '\n') reject("empty name");
}

static void eol(const char *s)
{
  if (*s != '\n') reject("extra arguments");
}

// ===== str

static vector_uint8 str_collection;

typedef struct {
  num pos;
  num len;
} str;
VECTOR_define_inline(str)

static str str_fromptr(const char *s,char terminator)
{
  str result;
  result.pos = vector_uint8_len(&str_collection);
  num len = 0;
  for (;;) {
    char c = *s++;
    if (c == terminator) break;
    vector_uint8_append(&str_collection,c);
    ++len;
  }
  result.len = len;
  return result;
}

static str str_primed(str s,num primes)
{
  if (primes <= 0) return s;
  str result;
  result.pos = vector_uint8_len(&str_collection);
  for (num i = 0;i < s.len;++i) {
    uint8 c = vector_uint8_read(&str_collection,s.pos+i);
    vector_uint8_append(&str_collection,c);
  }
  for (num i = 0;i < primes;++i)
    vector_uint8_append(&str_collection,'\'');
  result.len = s.len+primes;
  if (result.len >= CUTOFF) reject("too many primes");
  return result;
}

static void str_fromptr_split(vector_str *result,const char *s,char terminator,char split)
{
  vector_str_empty(result);
  str x;
  x.pos = vector_uint8_len(&str_collection);
  x.len = 0;
  for (;;) {
    char c = *s++;
    if (c == terminator || c == split) {
      vector_str_append(result,x);
      if (c == terminator) break;
      x.pos = vector_uint8_len(&str_collection);
      x.len = 0;
      continue;
    }
    vector_uint8_append(&str_collection,c);
    ++x.len;
  }
}

static int str_eq(str s,str t)
{
  num len = s.len;
  if (len != t.len) return 0;
  for (num i = 0;i < len;++i) {
    uint8 c = vector_uint8_read(&str_collection,s.pos+i);
    uint8 d = vector_uint8_read(&str_collection,t.pos+i);
    if (c != d) return 0;
  }
  return 1;
}

static int str_eq_primed(str s,str t,num tprimes)
{
  num len = t.len;
  if (len != s.len-tprimes) return 0;
  for (num i = 0;i < len;++i) {
    uint8 c = vector_uint8_read(&str_collection,s.pos+i);
    uint8 d = vector_uint8_read(&str_collection,t.pos+i);
    if (c != d) return 0;
  }
  for (num i = 0;i < tprimes;++i) {
    uint8 c = vector_uint8_read(&str_collection,s.pos+t.len+i);
    if (c != '\'') return 0;
  }
  return 1;
}

static int str_cmp(str s,str t)
{
  for (num i = 0;;++i) {
    if (i >= s.len) return (i >= t.len) ? 0 : -1;
    if (i >= t.len) return 1;
    uint8 c = vector_uint8_read(&str_collection,s.pos+i);
    uint8 d = vector_uint8_read(&str_collection,t.pos+i);
    if (c < d) return -1;
    if (c > d) return 1;
  }
}

static str str_const_bool;
static str str_const_ind;
static str str_const_fun;
static str str_const_a;
static str str_const_r;
static str str_const_equalssign;
static str str_const_atsign;

static void str_const_init(void)
{
  str_const_bool = str_fromptr("bool",0);
  str_const_ind = str_fromptr("ind",0);
  str_const_fun = str_fromptr("fun",0);
  str_const_a = str_fromptr("a",0);
  str_const_r = str_fromptr("r",0);
  str_const_equalssign = str_fromptr("=",0);
  str_const_atsign = str_fromptr("@",0);
}

static num hash_mix(num h)
{
  uint32_t h0 = h;
  uint32_t h1 = h>>32;
  h0 ^= (h1<<5)|(h1>>27);
  h1 += (h0<<7)|(h0>>25);
  h0 ^= (h1<<11)|(h1>>21);
  h1 += (h0<<16)|(h0>>16);
  h0 ^= (h1<<5)|(h1>>27);
  h1 += (h0<<7)|(h0>>25);
  h = h1;
  h <<= 32;
  h |= h0;
  return h;
}

static num str_hash(str s)
{
  num result = 632422315470652589LL;
  for (num i = 0;i < s.len;++i) {
    uint8 c = vector_uint8_read(&str_collection,s.pos+i);
    result = hash_mix(result+c);
  }
  return result;
}

// ===== type operations

static num numtypes;

typedef struct {
  num pos; // position in type_data_state
} type_ptr;

static inline type_ptr pos2type(num pos) {
  type_ptr result;
  result.pos = pos;
  return result;
}

typedef struct {
  str name;
  num subtypes; // -1 for tyvar
  num subtypepos; // position in type_subtype_state
  num hash;
} type_data;

VECTOR_define_inline(type_data)
VECTOR_define_inline(type_ptr)

static vector_type_data type_data_state;
static inline type_data type2data(type_ptr type) {
  return vector_type_data_read(&type_data_state,type.pos);
}
static inline num type2hash(type_ptr type) {
  return type2data(type).hash;
}

static vector_type_ptr type_subtype_state;
static inline type_ptr pos2subtype(num pos) {
  return vector_type_ptr_read(&type_subtype_state,pos);
}

static type_ptr type_new_tyvar(str name)
{
  type_ptr result = pos2type(vector_type_data_len(&type_data_state));
  type_data t;
  t.name = name;
  t.subtypes = -1;
  t.subtypepos = -1;
  t.hash = str_hash(name);
  vector_type_data_append(&type_data_state,t);
  return result;
}

static type_ptr type_new_app(str name,vector_type_ptr *args)
{
  type_ptr result = pos2type(vector_type_data_len(&type_data_state));
  type_data t;
  t.subtypepos = vector_type_ptr_len(&type_subtype_state);

  num arity = vector_type_ptr_len(args);
  num hash = hash_mix(7152249725402530200+arity);

  for (num i = 0;i < arity;++i) {
    type_ptr subtype = vector_type_ptr_read(args,i);
    vector_type_ptr_append(&type_subtype_state,subtype);
    hash = hash_mix(hash+type2hash(subtype));
  }

  t.name = name;
  t.subtypes = arity;
  t.hash = hash_mix(hash+str_hash(name));
  vector_type_data_append(&type_data_state,t);
  return result;
}

static type_ptr type_new_bool(void)
{
  type_ptr result = pos2type(vector_type_data_len(&type_data_state));
  static vector_type_ptr args;
  know(vector_type_ptr_len(&args) == 0);
  type_new_app(str_const_bool,&args);
  return result;
}

static type_ptr type_new_fun(type_ptr from,type_ptr to)
{
  type_ptr result = pos2type(vector_type_data_len(&type_data_state));
  static vector_type_ptr args;
  vector_type_ptr_write(&args,0,from);
  vector_type_ptr_write(&args,1,to);
  know(vector_type_ptr_len(&args) == 2);
  type_new_app(str_const_fun,&args);
  return result;
}

static int type_cmp(type_ptr type,type_ptr TYPE)
{
  if (type.pos == TYPE.pos) return 0;
  type_data t = type2data(type);
  type_data T = type2data(TYPE);
  if (t.subtypes >= 0 && T.subtypes < 0) return 1;
  if (T.subtypes >= 0 && t.subtypes < 0) return -1;
  int result = str_cmp(t.name,T.name);
  if (result) return result;
  if (t.subtypes < 0) return 0;
  know(T.subtypes == t.subtypes);
  for (num i = 0;i < t.subtypes;++i) {
    type_ptr subtype = pos2subtype(t.subtypepos+i);
    type_ptr SUBTYPE = pos2subtype(T.subtypepos+i);
    result = type_cmp(subtype,SUBTYPE);
    if (result) return result;
  }
  return 0;
}

static int type_is(type_ptr type,type_ptr TYPE)
{
  if (type.pos == TYPE.pos) return 1;
  type_data t = type2data(type);
  type_data T = type2data(TYPE);
  if (t.hash != T.hash) return 0;
  if (t.subtypes != T.subtypes) return 0;
  if (!str_eq(t.name,T.name)) return 0;
  if (t.subtypes < 0) return 1;
  know(T.subtypes == t.subtypes);
  for (num i = 0;i < t.subtypes;++i) {
    type_ptr subtype = pos2subtype(t.subtypepos+i);
    type_ptr SUBTYPE = pos2subtype(T.subtypepos+i);
    if (!type_is(subtype,SUBTYPE)) return 0;
  }
  return 1;
}

static int type_is_tyvar(type_ptr type)
{
  type_data t = type2data(type);
  return t.subtypes < 0;
}

static int type_is_tyvarnamed(type_ptr type,str name)
{
  type_data t = type2data(type);
  if (t.subtypes >= 0) return 0;
  return str_eq(t.name,name);
}

// does not constrain arguments
static int type_is_appnamed(type_ptr type,str name)
{
  type_data t = type2data(type);
  if (t.subtypes < 0) return 0;
  return str_eq(t.name,name);
}

static int type_is_bool(type_ptr type)
{
  type_data t = type2data(type);
  if (t.subtypes != 0) return 0;
  return str_eq(t.name,str_const_bool);
}

static int type_is_fun(type_ptr type,type_ptr *from,type_ptr *to)
{
  type_data t = type2data(type);
  if (t.subtypes != 2) return 0;
  if (!str_eq(t.name,str_const_fun)) return 0;
  if (from) *from = pos2subtype(t.subtypepos);
  if (to) *to = pos2subtype(t.subtypepos+1);
  return 1;
}

static int type_is_funfromto(type_ptr type,type_ptr from,type_ptr to)
{
  type_data t = type2data(type);
  if (t.subtypes != 2) return 0;
  if (!str_eq(t.name,str_const_fun)) return 0;
  if (!type_is(from,pos2subtype(t.subtypepos))) return 0;
  if (!type_is(to,pos2subtype(t.subtypepos+1))) return 0;
  return 1;
}

// XXX simplicity: could merge with type_some_subst_is
static int type_is_equality(type_ptr type)
{
  type_ptr from,to,tofrom,toto;
  if (!type_is_fun(type,&from,&to)) return 0;
  if (!type_is_fun(to,&tofrom,&toto)) return 0;
  if (!type_is(from,tofrom)) return 0;
  return type_is_bool(toto);
}

// XXX simplicity: could merge with type_some_subst_is
static int type_is_choice(type_ptr type)
{
  type_ptr from,to,fromfrom,fromto;
  if (!type_is_fun(type,&from,&to)) return 0;
  if (!type_is_fun(from,&fromfrom,&fromto)) return 0;
  if (!type_is(fromfrom,to)) return 0;
  return type_is_bool(fromto);
}

// 1 if TYPE is result of tyvarnames->abstype substitution into type
static int type_tyvarnames_subst_is(type_ptr type,type_ptr TYPE,type_ptr abstype,vector_str *tyvarnames)
{
  num numtyvars = vector_str_len(tyvarnames);
  if (!numtyvars) return type_is(type,TYPE);

  type_data t = type2data(type);
  type_data T = type2data(TYPE);

  if (t.subtypes >= 0) {
    if (t.subtypes != T.subtypes) return 0;
    if (!str_eq(t.name,T.name)) return 0;
    for (num i = 0;i < t.subtypes;++i) {
      type_ptr subtype = pos2subtype(t.subtypepos+i);
      type_ptr SUBTYPE = pos2subtype(T.subtypepos+i);
      if (!type_tyvarnames_subst_is(subtype,SUBTYPE,abstype,tyvarnames))
        return 0;
    }
    return 1;
  }

  for (num i = 0;i < numtyvars;++i) {
    str tyvarname = vector_str_read(tyvarnames,i);
    if (type_is_tyvarnamed(type,tyvarname)) {
      // e.g. tyvars are A B C
      // and we've found that type matches B
      // so now want to check that TYPE matches second position in abstype
      type_data a = type2data(abstype);
      know(a.subtypes == numtyvars);
      type_ptr abstypei = pos2subtype(a.subtypepos+i);
      return type_is(TYPE,abstypei);
    }
  }

  // type is tyvar not in tyvars
  // so look for exact match
  return type_is(type,TYPE);
}

static type_ptr type_subst_recursion(type_ptr type,vector_type_ptr *from,vector_type_ptr *to,vector_type_ptr *tmp)
{
  type_data t = type2data(type);

  if (t.subtypes < 0) {
    num len = vector_type_ptr_len(from);
    know(len == vector_type_ptr_len(to));
    for (num i = 0;i < len;++i)
      if (type_is(type,vector_type_ptr_read(from,i)))
        return vector_type_ptr_read(to,i);
    return type;
  }

  int changed = 0;
  num tmplen = vector_type_ptr_len(tmp);
  for (num i = 0;i < t.subtypes;++i) {
    type_ptr subtype = pos2subtype(t.subtypepos+i);
    type_ptr SUBTYPE = type_subst_recursion(subtype,from,to,tmp);
    vector_type_ptr_append(tmp,SUBTYPE);
    if (!changed && !type_is(subtype,SUBTYPE)) changed = 1;
  }
  if (changed) {
    static vector_type_ptr args;
    vector_type_ptr_empty(&args);
    for (num i = 0;i < t.subtypes;++i)
      vector_type_ptr_append(&args,vector_type_ptr_read(tmp,tmplen+i));
    type = type_new_app(t.name,&args);
  }

  vector_type_ptr_trunc(tmp,tmplen);
  return type;
}

static type_ptr type_subst(type_ptr type,vector_type_ptr *from,vector_type_ptr *to)
{
  static vector_type_ptr tmp;
  know(vector_type_ptr_len(&tmp) == 0);
  return type_subst_recursion(type,from,to,&tmp);
}

static int type_find_subst_append(vector_type_ptr *from,vector_type_ptr *to,type_ptr type,type_ptr TYPE)
{
  type_data t = type2data(type);
  type_data T = type2data(TYPE);

  if (t.subtypes >= 0) {
    if (T.subtypes != t.subtypes) return 0;
    if (!str_eq(t.name,T.name)) return 0;
    for (num i = 0;i < t.subtypes;++i) {
      type_ptr subtype = pos2subtype(t.subtypepos+i);
      type_ptr SUBTYPE = pos2subtype(T.subtypepos+i);
      if (!type_find_subst_append(from,to,subtype,SUBTYPE))
        return 0;
    }
    return 1;
  }

  num len = vector_type_ptr_len(from);
  know(len == vector_type_ptr_len(to));
  for (num i = 0;i < len;++i)
    if (type_is(type,vector_type_ptr_read(from,i)))
      return type_is(TYPE,vector_type_ptr_read(to,i));

  vector_type_ptr_append(from,type);
  vector_type_ptr_append(to,TYPE);
  return 1;
}

// 1 if TYPE is result of some substitution into type
static int type_some_subst_is(type_ptr type,type_ptr TYPE)
{
  static vector_type_ptr from,to;

  vector_type_ptr_empty(&from);
  vector_type_ptr_empty(&to);
  if (!type_find_subst_append(&from,&to,type,TYPE)) return 0;

  return 1;
}

// accumulates unique tyvars in result (not necessarily sorted)
// on top of any existing tyvars
// with uniqueness being determined by type_is
static void type_vars_append(vector_type_ptr *result,type_ptr type)
{
  type_data t = type2data(type);

  if (t.subtypes < 0) {
    for (num i = 0;i < vector_type_ptr_len(result);++i)
      if (type_is(vector_type_ptr_read(result,i),type))
        return;
    vector_type_ptr_append(result,type);
    return;
  }

  for (num i = 0;i < t.subtypes;++i)
    type_vars_append(result,pos2subtype(t.subtypepos+i));
}

static void type_vars(vector_type_ptr *result,type_ptr type)
{
  vector_type_ptr_empty(result);
  type_vars_append(result,type);
}

// ===== term operations

static num numterms;

typedef struct {
  num pos; // position in term_data_state
} term_ptr;

static inline term_ptr pos2term(num pos) {
  term_ptr result;
  result.pos = pos;
  return result;
}

typedef struct {
  type_ptr type;
  num hash;
  num frees;
  num freepos;
  union {
    str name; // for tag 'c' or 'd'
    struct {
      term_ptr v;
      term_ptr x;
    } terms; // for tag 'e' or 'f'
  } split;
  char tag;
} term_data;

VECTOR_define_inline(term_ptr)
VECTOR_define_inline(term_data)

static vector_term_data term_data_state;
static inline term_data term2data(term_ptr term) {
  return vector_term_data_read(&term_data_state,term.pos);
}
static inline num term2hash(term_ptr term) {
  return term2data(term).hash;
}

static vector_term_ptr termfrees_state;

static type_ptr term2type(term_ptr term)
{
  term_data t = term2data(term);
  return t.type;
}

static int term_is(term_ptr term,term_ptr TERM)
{
  if (term.pos == TERM.pos) return 1;
  term_data t = term2data(term);
  term_data T = term2data(TERM);
  if (t.hash != T.hash) return 0;
  if (t.tag != T.tag) return 0;
  if (!type_is(t.type,T.type)) return 0;
  if (t.tag == 'c' || t.tag == 'd') return str_eq(t.split.name,T.split.name);
  if (!term_is(t.split.terms.v,T.split.terms.v)) return 0;
  if (!term_is(t.split.terms.x,T.split.terms.x)) return 0;
  return 1;
}

static int term_alphaorder_cmp_vars_unbound(term_ptr term,term_ptr TERM)
{
  if (term.pos == TERM.pos) return 0;
  term_data t = term2data(term);
  term_data T = term2data(TERM);
  int result = str_cmp(t.split.name,T.split.name);
  if (result) return result;
  return type_cmp(t.type,T.type);
}

// should match ordav in hol-light/fusion.ml
// which is orda when term,TERM are each variables
// caller here guarantees that term,TERM are variables
static int term_alphaorder_cmp_vars(term_ptr term,term_ptr TERM,vector_term_ptr *boundpairs)
{
  num lenboundpairs = vector_term_ptr_len(boundpairs);
  for (num i = 2*(lenboundpairs/2)-2;i >= 0;i -= 2) {
    term_ptr var = vector_term_ptr_read(boundpairs,i);
    term_ptr VAR = vector_term_ptr_read(boundpairs,i+1);
    if (term_is(term,var)) return term_is(TERM,VAR) ? 0 : -1;
    if (term_is(TERM,VAR)) return 1;
  }
  return term_alphaorder_cmp_vars_unbound(term,TERM);
}

// can extend boundpairs while running but restores it
static int term_alphaorder_cmp_recursion(term_ptr term,term_ptr TERM,vector_term_ptr *boundpairs)
{
  if (term.pos == TERM.pos)
    if (vector_term_ptr_len(boundpairs) == 0)
      return 0;

  term_data t = term2data(term);
  term_data T = term2data(TERM);

  if (t.tag > T.tag) return 1;
  if (t.tag < T.tag) return -1;

  if (t.tag == 'd')
    return term_alphaorder_cmp_vars(term,TERM,boundpairs);

  if (t.tag == 'c') {
    int result = str_cmp(t.split.name,T.split.name);
    if (result) return result;
    return type_cmp(t.type,T.type);
  }

  if (t.tag == 'e') {
    int result = term_alphaorder_cmp_recursion(t.split.terms.v,T.split.terms.v,boundpairs);
    if (result) return result;
    return term_alphaorder_cmp_recursion(t.split.terms.x,T.split.terms.x,boundpairs);
  }

  know(t.tag == 'f');

  int result = type_cmp(term2type(t.split.terms.v),term2type(T.split.terms.v));
  if (result) return result;

  num lenboundpairs = vector_term_ptr_len(boundpairs);
  vector_term_ptr_append(boundpairs,t.split.terms.v);
  vector_term_ptr_append(boundpairs,T.split.terms.v);
  result = term_alphaorder_cmp_recursion(t.split.terms.x,T.split.terms.x,boundpairs);
  vector_term_ptr_trunc(boundpairs,lenboundpairs);
  return result;
}

static int term_alphaorder_cmp(term_ptr term,term_ptr TERM)
{
  if (term.pos == TERM.pos) return 0;
  static vector_term_ptr boundpairs;
  know(vector_term_ptr_len(&boundpairs) == 0);
  return term_alphaorder_cmp_recursion(term,TERM,&boundpairs);
}

static int term_has_freevars(term_ptr term)
{
  term_data t = term2data(term);
  return t.frees > 0;
}

static term_ptr term_new_const(type_ptr type,str name)
{
  term_ptr result = pos2term(vector_term_data_len(&term_data_state));
  term_data t;
  t.tag = 'c';
  t.type = type;
  t.split.name = name;

  num hash = 1260898140613410013LL;
  hash = hash_mix(hash+type2hash(type));
  hash = hash_mix(hash+str_hash(name));

  t.hash = hash;
  t.frees = 0;
  t.freepos = 0;

  vector_term_data_append(&term_data_state,t);
  return result;
}

static term_ptr term_new_var(type_ptr type,str name)
{
  term_ptr result = pos2term(vector_term_data_len(&term_data_state));
  term_data t;
  t.tag = 'd';
  t.type = type;
  t.split.name = name;

  num hash = 5725918395758143510LL;
  hash = hash_mix(hash+type2hash(type));
  hash = hash_mix(hash+str_hash(name));

  t.hash = hash;
  t.frees = 1;
  t.freepos = vector_term_ptr_len(&termfrees_state);

  vector_term_ptr_append(&termfrees_state,result);
  vector_term_data_append(&term_data_state,t);
  return result;
}

static term_ptr term_new_eval(type_ptr outputtype,term_ptr funterm,term_ptr pointterm)
{
  term_ptr result = pos2term(vector_term_data_len(&term_data_state));
  num hash = 3679975592003923231LL;
  hash = hash_mix(hash+term2hash(funterm));
  hash = hash_mix(hash+term2hash(pointterm));

  num pos,len;

  term_data f = term2data(funterm);
  term_data p = term2data(pointterm);

  if (!f.frees) {
    // closed function; common case
    pos = p.freepos;
    len = p.frees;
  } else if (!p.frees) {
    pos = f.freepos;
    len = f.frees;
  } else {
    pos = vector_term_ptr_len(&termfrees_state);
    // XXX simplicity: merge with hypotheses_merge
    term_ptr ft = vector_term_ptr_read(&termfrees_state,f.freepos);
    term_ptr pt = vector_term_ptr_read(&termfrees_state,p.freepos);
    num fi = 1;
    num pi = 1;
    for (;;) {
      int order = term_alphaorder_cmp_vars_unbound(ft,pt);
      vector_term_ptr_append(&termfrees_state,(order <= 0 ? ft : pt));
      if (order <= 0 && fi >= f.frees) {
        if (order != 0) vector_term_ptr_append(&termfrees_state,pt);
        while (pi < p.frees)
          vector_term_ptr_append(&termfrees_state,vector_term_ptr_read(&termfrees_state,p.freepos+pi++));
        break;
      }
      if (order >= 0 && pi >= p.frees) {
        if (order != 0) vector_term_ptr_append(&termfrees_state,ft);
        while (fi < f.frees)
          vector_term_ptr_append(&termfrees_state,vector_term_ptr_read(&termfrees_state,f.freepos+fi++));
        break;
      }
      if (order <= 0)
        ft = vector_term_ptr_read(&termfrees_state,f.freepos+fi++);
      if (order >= 0)
        pt = vector_term_ptr_read(&termfrees_state,p.freepos+pi++);
    }
    len = vector_term_ptr_len(&termfrees_state)-pos;
  }

  term_data t;
  t.type = outputtype;
  t.hash = hash;
  t.frees = len;
  t.freepos = pos;
  t.tag = 'e';
  t.split.terms.v = funterm;
  t.split.terms.x = pointterm;

  vector_term_data_append(&term_data_state,t);
  return result;
}

static term_ptr term_new_mapsto(type_ptr outputtype,term_ptr fromterm,term_ptr toterm)
{
  term_ptr result = pos2term(vector_term_data_len(&term_data_state));
  num hash = 4019844455831122549LL;
  hash = hash_mix(hash+term2hash(fromterm));
  hash = hash_mix(hash+term2hash(toterm));

  term_data tt = term2data(toterm);
  num pos = tt.freepos;
  num len = tt.frees;

  for (num i = 0;i < len;++i) {
    term_ptr v = vector_term_ptr_read(&termfrees_state,pos+i);
    if (term_is(fromterm,v)) {
      num newpos = vector_term_ptr_len(&termfrees_state);
      num newlen = 0;
      for (num j = 0;j < len;++j)
        if (j != i) {
          vector_term_ptr_append(&termfrees_state,vector_term_ptr_read(&termfrees_state,pos+j));
          ++newlen;
        }
      pos = newpos;
      len = newlen;
      break;
    }
  }

  term_data t;
  t.type = outputtype;
  t.hash = hash;
  t.frees = len;
  t.freepos = pos;
  t.tag = 'f';
  t.split.terms.v = fromterm;
  t.split.terms.x = toterm;

  vector_term_data_append(&term_data_state,t);
  return result;
}

static term_ptr term_new_eq(term_ptr left,term_ptr right)
{
  type_ptr type = term2type(left);
  know(type_is(type,term2type(right)));
  type_ptr booltype = type_new_bool();
  type_ptr typetobool = type_new_fun(type,booltype);
  type_ptr typetotypetobool = type_new_fun(type,typetobool);
  term_ptr eq = term_new_const(typetotypetobool,str_const_equalssign);
  term_ptr eqleft = term_new_eval(typetobool,eq,left);
  return term_new_eval(booltype,eqleft,right);
}

static int term_is_const(term_ptr term,type_ptr *type,str *name)
{
  term_data t = term2data(term);
  if (t.tag != 'c') return 0;
  if (type) *type = t.type;
  if (name) *name = t.split.name;
  return 1;
}

static int term_is_var(term_ptr term,type_ptr *type,str *name)
{
  term_data t = term2data(term);
  if (t.tag != 'd') return 0;
  if (type) *type = t.type;
  if (name) *name = t.split.name;
  return 1;
}

static term_ptr term_var_primed(term_ptr term,num primes)
{
  term_data t = term2data(term);
  know(t.tag == 'd');
  if (primes == 0) return term;
  str nameprime = str_primed(t.split.name,primes);
  return term_new_var(t.type,nameprime);
}

static int term_is_eval(term_ptr term,type_ptr *type,term_ptr *funterm,term_ptr *pointterm)
{
  term_data t = term2data(term);
  if (t.tag != 'e') return 0;
  if (type) *type = t.type;
  if (funterm) *funterm = t.split.terms.v;
  if (pointterm) *pointterm = t.split.terms.x;
  return 1;
}

static int term_is_mapsto(term_ptr term,type_ptr *type,term_ptr *fromterm,term_ptr *toterm)
{
  term_data t = term2data(term);
  if (t.tag != 'f') return 0;
  if (type) *type = t.type;
  if (fromterm) *fromterm = t.split.terms.v;
  if (toterm) *toterm = t.split.terms.x;
  return 1;
}

// accumulates unique tyvars in result (not necessarily sorted)
// on top of any existing tyvars
static void term_tyvars_append(vector_type_ptr *result,term_ptr term)
{
  term_data t = term2data(term);

  if (t.tag == 'c' || t.tag == 'd') {
    type_vars_append(result,t.type);
    return;
  }

  know(t.tag == 'e' || t.tag == 'f');
  term_tyvars_append(result,t.split.terms.v);
  term_tyvars_append(result,t.split.terms.x);

  // scanning type would be redundant for both e and f
  // and scanning type by itself is insufficient
  // so skip scanning type
}

static void term_tyvars(vector_type_ptr *result,term_ptr term)
{
  vector_type_ptr_empty(result);
  term_tyvars_append(result,term);
}

static int term_is_eqsymbol(term_ptr term)
{
  str name;
  if (!term_is_const(term,0,&name)) return 0;
  return str_eq(name,str_const_equalssign);
}

static int term_is_equality(term_ptr term,term_ptr *left,term_ptr *right)
{
  term_ptr funterm,funfunterm;
  if (!term_is_eval(term,0,&funterm,right)) return 0;
  if (!term_is_eval(funterm,0,&funfunterm,left)) return 0;
  if (!term_is_eqsymbol(funfunterm)) return 0;
  return 1;
}

static int term_alphaequivalent(term_ptr term,term_ptr TERM)
{
  if (term.pos == TERM.pos) return 1;
  return term_alphaorder_cmp(term,TERM) == 0;
}

// 1 if v''' is a free variable in term
static int term_has_free(term_ptr term,term_ptr v,num primes)
{
  type_ptr vtype;
  str vname;
  if (!term_is_var(v,&vtype,&vname)) return 0;

  term_data t = term2data(term);
  for (num i = 0;i < t.frees;++i) {
    term_ptr w = vector_term_ptr_read(&termfrees_state,t.freepos+i);
    type_ptr wtype;
    str wname;
    know(term_is_var(w,&wtype,&wname));
    if (type_is(wtype,vtype))
      if (str_eq_primed(wname,vname,primes))
        return 1;
  }
  return 0;
}

// later entries override earlier entries
// return term if not found
static term_ptr term_map_term(vector_term_ptr *from,vector_term_ptr *to,term_ptr term)
{
  num i = vector_term_ptr_len(from);
  while (i-- > 0)
    if (term_is(term,vector_term_ptr_read(from,i)))
      return vector_term_ptr_read(to,i);
  return term;
}

static term_ptr term_subst(term_ptr term,vector_term_ptr *from,vector_term_ptr *to)
{
  term_data t = term2data(term);

  if (!t.frees) return term;

  int freefrom = 0;
  num fromlen = vector_term_ptr_len(from);
  for (num i = 0;i < fromlen;++i) {
    term_ptr fromi = vector_term_ptr_read(from,i);
    for (num j = 0;j < t.frees;++j)
      if (term_is(fromi,vector_term_ptr_read(&termfrees_state,t.freepos+j))) {
        freefrom = 1;
        break;
      }
    if (freefrom) break;
  }
  if (!freefrom) return term;

  if (t.tag == 'd') return term_map_term(from,to,term);

  if (t.tag == 'e') {
    term_ptr v = t.split.terms.v;
    term_ptr V = term_subst(v,from,to);
    term_ptr x = t.split.terms.x;
    term_ptr X = term_subst(x,from,to);
    if (term_is(v,V) && term_is(x,X)) return term;
    return term_new_eval(t.type,V,X);
  }

  know(t.tag == 'f');
  term_ptr v = t.split.terms.v;
  know(term_is_var(v,0,0));
  term_ptr x = t.split.terms.x;
  term_data xdata = term2data(x);
  num freelen = xdata.frees;
  num freepos = xdata.freepos;

  for (num vprimes = 0;;++vprimes) {
    if (vprimes >= CUTOFF) reject("too many primes");
    // have v |-> x
    // considering rewriting as subst(v) |-> subst(x)
    // where subst replaces v with v'''
    // where number of primes is vprimes
    // and subst replaces other vars as per from->to

    int collision = 0;
    // goal: set collision to 1 if v''' has a collision

    // original algorithm determines collision as follows:
    // * first try vprimes = 0
    //   collision is then: some subst f->m with f!=v has f free in x and v free in m
    // * if collision, move on to larger vprimes
    // * for vprimes>0, define collision as:
    //   v''' free in origsubst(x)
    //   where origsubst is from->to ignoring v

    // there are really three cases for the v''' collision:
    // * if v is free in x: stays as v
    //   so no match for vprimes>0
    //   (and this also can be ignored for vprimes=0)
    // * if f!=v is free in x and f->m:
    //   frees(origsubst(x)) includes frees(m)
    //   so collision if v''' free in m
    //   (which is also a test we want for vprimes=0)
    // * if f!=v is free in x and f not mapped from->to:
    //   frees(origsubst(x)) includes f
    //   so collision if v''' free in f (i.e., v''' = f)

    // in other words, for each f free in x:
    // * if f=v: no collision
    // * else if f->m: collision if v''' free in m
    // * else: collision if v''' free in f

    for (num i = 0;i < freelen;++i) {
      term_ptr f = vector_term_ptr_read(&termfrees_state,freepos+i);
      if (term_is(f,v)) continue;
      term_ptr m = term_map_term(from,to,f);
      if (term_has_free(m,v,vprimes)) {
        collision = 1;
        break;
      }
    }

    if (!collision) {
      // safe to change v to v+vprimes

      term_ptr v = t.split.terms.v;
      term_ptr V = term_var_primed(v,vprimes);

      num len = vector_term_ptr_len(from);
      know(len == vector_term_ptr_len(to));

      if (!term_is(term_map_term(from,to,v),V)) {
        vector_term_ptr_append(from,v);
        vector_term_ptr_append(to,V);
      }

      term_ptr x = t.split.terms.x;
      term_ptr X = term_subst(x,from,to);

      vector_term_ptr_trunc(from,len);
      vector_term_ptr_trunc(to,len);

      if (term_is(v,V) && term_is(x,X)) return term;
      return term_new_mapsto(t.type,V,X);
    }
  }
}

// should match inst in fusion.ml (but algorithm is different)
static term_ptr term_tyvar_subst_recursion(term_ptr term,vector_type_ptr *tyfrom,vector_type_ptr *tyto,vector_term_ptr *boundpairs)
{
  term_data t = term2data(term);
  type_ptr type = t.type;
  type_ptr TYPE = type_subst(type,tyfrom,tyto);

  if (t.tag == 'c') {
    if (type_is(type,TYPE)) return term;
    return term_new_const(TYPE,t.split.name);
  }

  if (t.tag == 'e') {
    term_ptr termfun = t.split.terms.v;
    term_ptr termpoint = t.split.terms.x;
    term_ptr TERMfun = term_tyvar_subst_recursion(termfun,tyfrom,tyto,boundpairs);
    term_ptr TERMpoint = term_tyvar_subst_recursion(termpoint,tyfrom,tyto,boundpairs);
    if (term_is(termfun,TERMfun) && term_is(termpoint,TERMpoint)) return term;
    return term_new_eval(TYPE,TERMfun,TERMpoint);
  }

  num lenboundpairs = vector_term_ptr_len(boundpairs);

  if (t.tag == 'd') {
    for (num i = 2*(lenboundpairs/2)-2;i >= 0;i -= 2) {
      term_ptr var = vector_term_ptr_read(boundpairs,i);
      term_ptr VAR = vector_term_ptr_read(boundpairs,i+1);
      if (term_is(term,var)) return VAR;
    }
    if (type_is(type,TYPE)) return term;
    return term_new_var(TYPE,t.split.name);
  }

  know(t.tag == 'f');

  term_ptr v = t.split.terms.v;
  type_ptr vtype;
  str vname;
  know(term_is_var(v,&vtype,&vname));

  term_ptr x = t.split.terms.x;
  term_data xdata = term2data(x);
  num freelen = xdata.frees;
  num freepos = xdata.freepos;

  type_ptr Vtype = type_subst(vtype,tyfrom,tyto);

  // variable capture at this level means:
  //   subst(v) matches subst(f) for some f free in x
  //   without f being v
  // then have to rename v as v''' for some number of primes

  // note: do not want to rename v now
  // so be careful to override v in boundpairs lookups below

  for (num vprimes = 0;;++vprimes) {
    if (vprimes >= CUTOFF) reject("too many primes");

    // considering v''' as replacement for v
    // (or no replacement if vprimes == 0)
    // does subst(v''') match subst(f) for some free f != v?

    int collision = 0;
    for (num i = 0;i < freelen;++i) {
      term_ptr f = vector_term_ptr_read(&termfrees_state,freepos+i);
      if (term_is(f,v)) continue;

      // does subst(v''') match subst(f)?

      for (num i = 2*(lenboundpairs/2)-2;i >= 0;i -= 2) {
        term_ptr var = vector_term_ptr_read(boundpairs,i);
        if (!term_is(f,var)) continue;
        term_ptr m = vector_term_ptr_read(boundpairs,i+1);
        // f is renamed as m
        // now want to know:
        // does subst(v''') match m?
        type_ptr mtype;
        str mname;
        know(term_is_var(m,&mtype,&mname));
        if (str_eq_primed(mname,vname,vprimes))
          if (type_is(Vtype,mtype)) {
            collision = 1;
            break;
          }
      }
      if (collision) break;

      // f is not in boundpairs, so just do tyfrom->tyto
      type_ptr ftype;
      str fname;
      know(term_is_var(f,&ftype,&fname));
      if (str_eq_primed(fname,vname,vprimes))
        if (type_is(type_subst(ftype,tyfrom,tyto),Vtype)) {
          collision = 1;
          break;
        }
    }

    if (!collision) {
      term_ptr V = term_new_var(Vtype,str_primed(vname,vprimes));

      vector_term_ptr_append(boundpairs,v);
      vector_term_ptr_append(boundpairs,V);
      term_ptr X = term_tyvar_subst_recursion(x,tyfrom,tyto,boundpairs);
      vector_term_ptr_trunc(boundpairs,lenboundpairs);

      if (term_is(v,V) && term_is(x,X)) return term;
      return term_new_mapsto(TYPE,V,X);
    }
  }
}

static term_ptr term_tyvar_subst(term_ptr term,vector_type_ptr *from,vector_type_ptr *to)
{
  static vector_term_ptr boundpairs;
  know(vector_term_ptr_len(&boundpairs) == 0);
  return term_tyvar_subst_recursion(term,from,to,&boundpairs);
}

// ===== theorem operations

static num numthms;

static vector_term_ptr thmdata;
static vector_num thm2datapos;
static vector_term_ptr thm2conclusion;

// extract hypotheses from thmdata[start:stop]
// skipping terms alpha-equivalent to skip if skip>=0
static void hypotheses_extract(vector_term_ptr *result,num start,num stop,term_ptr skip)
{
  vector_term_ptr_empty(result);

  // XXX speed: can usually speed up alpha test with first-pass equality test
  // since we know thmdata[start:stop] are alpha-inequivalent

  for (num i = start;i < stop;++i) {
    term_ptr h = vector_term_ptr_read(&thmdata,i);
    if (skip.pos >= 0 && term_alphaequivalent(h,skip)) continue;
    vector_term_ptr_append(result,h);
  }
}

// same as hypotheses_extract but taking theorem as input
static void hypotheses_extract_th(vector_term_ptr *result,num th,term_ptr skip)
{
  num start = vector_num_read(&thm2datapos,th);
  num stop = vector_num_read(&thm2datapos,th+1);
  hypotheses_extract(result,start,stop,skip);
}

// merge modulo alpha-equivalence
static void hypotheses_merge(vector_term_ptr *result,vector_term_ptr *h,vector_term_ptr *H)
{
  vector_term_ptr_empty(result);

  num hlen = vector_term_ptr_len(h);
  num Hlen = vector_term_ptr_len(H);
  num i = 0;
  num I = 0;

  for (;;) {
    if (i == hlen) {
      while (I < Hlen) vector_term_ptr_append(result,vector_term_ptr_read(H,I++));
      return;
    }
    if (I == Hlen) {
      while (i < hlen) vector_term_ptr_append(result,vector_term_ptr_read(h,i++));
      return;
    }
    term_ptr hi = vector_term_ptr_read(h,i);
    term_ptr HI = vector_term_ptr_read(H,I);
    int order = term_alphaorder_cmp(hi,HI);
    vector_term_ptr_append(result,(order <= 0 ? hi : HI));
    i += (order <= 0);
    I += (order >= 0);
  }
}

static void hypotheses_merge_th(vector_term_ptr *result,num th,num TH,term_ptr thskip,term_ptr THskip)
{
  static vector_term_ptr h;
  static vector_term_ptr H;
  hypotheses_extract_th(&h,th,thskip);
  hypotheses_extract_th(&H,TH,THskip);
  hypotheses_merge(result,&h,&H);
}

static void hypotheses_sort(vector_term_ptr *h)
{
  num sorted = 0;
  num len = vector_term_ptr_len(h);
  for (num i = 0;i < len;++i) {
    term_ptr target = vector_term_ptr_read(h,i);
    // figure out where to insert target within h[:sorted]
    // XXX speed: can try other sorting algorithms
    num j;
    for (j = sorted;j > 0;--j) {
      int order = term_alphaorder_cmp(target,vector_term_ptr_read(h,j-1));
      if (order == 0) j = -1; // alpha-equivalent to earlier input; remove
      if (order >= 0) break;
    }
    // insert at position j, if j >= 0
    if (j < 0) continue;
    if (j < i) {
      for (num k = sorted;k > j;--k)
        vector_term_ptr_write(h,k,vector_term_ptr_read(h,k-1));
      vector_term_ptr_write(h,j,target);
    }
    ++sorted;
  }
  vector_term_ptr_trunc(h,sorted);
}

// ===== name->num dictionaries

// allows only append, not overwriting or removing old entries
// -1 is reserved for "not found"
// XXX speed: maybe use faster associative array

typedef struct {
  // for one->3, two->1, three->4, four->1:
  vector_str names; // one two three four
  vector_num results; // 3 1 4 1
} dict;

static num dict_lookup(dict *d,str name)
{
  num len = vector_str_len(&d->names);
  for (num i = 0;i < len;++i) {
    str dname = vector_str_read(&d->names,i);
    if (str_eq(dname,name))
      return vector_num_read(&d->results,i);
  }
  return -1;
}

static void dict_add(dict *d,str name,num result)
{
  vector_str_append(&d->names,name);
  vector_num_append(&d->results,result);
}

static dict absname2typedef_dict;
static num absname2typedef(str s) { return dict_lookup(&absname2typedef_dict,s); }

static dict repname2typedef_dict;
static num repname2typedef(str s) { return dict_lookup(&repname2typedef_dict,s); }

static dict app2arity_dict;
static num app2arity(str s) { return dict_lookup(&app2arity_dict,s); }

// trying to match new_type in fusion.ml
static void app2arity_add(str s,num arity)
{
  if (dict_lookup(&app2arity_dict,s) >= 0) reject("colliding tyapp name");
  dict_add(&app2arity_dict,s,arity);
}

static void app2arity_init(void)
{
  app2arity_add(str_const_fun,2);
  app2arity_add(str_const_bool,0);
  app2arity_add(str_const_ind,0);
}

static dict const2type_state;
static type_ptr const2type(str s) {
  return pos2type(dict_lookup(&const2type_state,s));
}

static void reject_previous_constnames(str s)
{
  if (str_eq(s,str_const_equalssign)) reject("const name =");
  if (str_eq(s,str_const_atsign)) reject("const name @");
  if (dict_lookup(&const2type_state,s) >= 0) reject("colliding const name");
  if (dict_lookup(&absname2typedef_dict,s) >= 0) reject("colliding const name");
  if (dict_lookup(&repname2typedef_dict,s) >= 0) reject("colliding const name");
}

static void const2type_add(str s,type_ptr result)
{
  reject_previous_constnames(s);
  dict_add(&const2type_state,s,result.pos);
}

static void absname2typedef_add(str s,num result)
{
  reject_previous_constnames(s);
  dict_add(&absname2typedef_dict,s,result);
}

static void repname2typedef_add(str s,num result)
{
  reject_previous_constnames(s);
  dict_add(&repname2typedef_dict,s,result);
}

// ===== typedefs

static num numtypedefs;

static vector_str typedeftyvarnames;

static vector_num typedef2reptype;
static vector_num typedef2repprop;
static vector_str typedef2abstypename;
static vector_str typedef2absname;
static vector_str typedef2repname;
static vector_num typedef2numtyvars;
static vector_num typedef2tyvarnamepos;

// ===== type tags

static void tag_a(const char *s) // Tyvar
{
  noteol(s);
  type_new_tyvar(str_fromptr(s,'\n'));
}

static void tag_b(const char *s) // Tyapp
{
  static vector_type_ptr subtypes;
  vector_type_ptr_empty(&subtypes);

  num arity = subid(&s,-1);
  for (num i = 0;i < arity;++i) {
    type_ptr subtype = pos2type(subid(&s,numtypes));
    vector_type_ptr_append(&subtypes,subtype);
  }
  noteol(s);

  str name = str_fromptr(s,'\n');
  num expectedarity = app2arity(name);
  if (expectedarity < 0) reject("undefined app name");
  if (expectedarity != arity) reject("wrong number of app arguments");

  type_new_app(name,&subtypes);
}

// ===== term tags

static void tag_c(const char *s) // Const
{
  type_ptr type = pos2type(subid(&s,numtypes));
  noteol(s);

  str name = str_fromptr(s,'\n');

  term_new_const(type,name);

  if (str_eq(name,str_const_equalssign)) {
    if (!type_is_equality(type)) reject("non-equality type");
    return;
  }

  if (str_eq(name,str_const_atsign)) {
    if (!type_is_choice(type)) reject("non-choice type");
    return;
  }

  type_ptr registeredtype = const2type(name);
  if (registeredtype.pos >= 0) { // XXX
    if (!type_some_subst_is(registeredtype,type)) reject("const type mismatch");
    return;
  }

  for (num rep = 0;rep < 2;++rep) {
    num registeredtypedef = rep ? repname2typedef(name) : absname2typedef(name);
    if (registeredtypedef >= 0) {
      type_ptr from,to;
      if (!type_is_fun(type,&from,&to)) reject("non-function type with typedef name");

      type_ptr reptype = pos2type(vector_num_read(&typedef2reptype,registeredtypedef));
      str abstypename = vector_str_read(&typedef2abstypename,registeredtypedef);
      num numtyvars = vector_num_read(&typedef2numtyvars,registeredtypedef);
      num tyvarnamepos = vector_num_read(&typedef2tyvarnamepos,registeredtypedef);
      static vector_str tyvarnames;
      vector_str_empty(&tyvarnames);
      for (num i = 0;i < numtyvars;++i)
        vector_str_append(&tyvarnames,vector_str_read(&typedeftyvarnames,tyvarnamepos+i));

      if (rep) {
        str repname = vector_str_read(&typedef2repname,registeredtypedef);
        // expecting: repname:abstype->reptype
        // modulo substitution for tyvars
        know(str_eq(name,repname));
        if (!type_is_appnamed(from,abstypename)) reject("typedef repname-abstype mismatch");
        if (!type_tyvarnames_subst_is(reptype,to,from,&tyvarnames)) reject("typedef repname-reptype mismatch");
      } else {
        str absname = vector_str_read(&typedef2absname,registeredtypedef);
        // expecting: absname:reptype->abstype
        // modulo substitution for tyvars
        // e.g. typedef said absname:(A->B->bool)->abstypename(A,B)
        // where reptype is A->B->bool, tyvars are A,B
        // and now could have absname:((A->B)->(B->C)->bool)->abstypename(A->B,B->C)
        know(str_eq(name,absname));
        if (!type_is_appnamed(to,abstypename)) reject("typedef absname-abstype mismatch");
        if (!type_tyvarnames_subst_is(reptype,from,to,&tyvarnames)) reject("typedef absname-reptype mismatch");
      }
      return;
    }
  }

  reject("undefined constant");
}

static void tag_d(const char *s) // Var
{
  type_ptr type = pos2type(subid(&s,numtypes));
  noteol(s);
  term_new_var(type,str_fromptr(s,'\n'));
}

static void tag_e(const char *s) // Comb
{
  type_ptr outputtype = pos2type(subid(&s,numtypes));
  term_ptr funterm = pos2term(subid(&s,numterms));
  term_ptr pointterm = pos2term(subid(&s,numterms));
  eol(s);

  if (!type_is_funfromto(term2type(funterm),term2type(pointterm),outputtype)) reject("combination mismatch");

  term_new_eval(outputtype,funterm,pointterm);
}

static void tag_f(const char *s) // Abs
{
  type_ptr outputtype = pos2type(subid(&s,numtypes));
  term_ptr fromterm = pos2term(subid(&s,numterms));
  term_ptr toterm = pos2term(subid(&s,numterms));
  eol(s);

  if (!term_is_var(fromterm,0,0)) reject("non-variable lambda");
  if (!type_is_funfromto(outputtype,term2type(fromterm),term2type(toterm))) reject("lambda mismatch");

  term_new_mapsto(outputtype,fromterm,toterm);
}

// ===== A: TYPE_DEFINITION
// (plus D for followup)

static num tag_A_pos;

static void tag_A(const char *s)
{
  type_ptr reptype = pos2type(subid(&s,numtypes));
  term_ptr repprop = pos2term(subid(&s,numterms));
  num th = subid(&s,numthms);
  noteol(s);

  static vector_str args; // beyond reptype,repprop,th
  str_fromptr_split(&args,s,'\n',']');

  num numargs = vector_str_len(&args);
  if (numargs < 5) reject("TYPE_DEFINITION: misformatted");
  if (vector_str_read(&args,0).len != 0) reject("TYPE_DEFINITION: refusing non-bracket");
  if (vector_str_read(&args,numargs-1).len != 0) reject("TYPE_DEFINITION: partial final component");

  str abstypename = vector_str_read(&args,1);
  str absname = vector_str_read(&args,2);
  str repname = vector_str_read(&args,3);

  static vector_str tyvarnames;
  vector_str_empty(&tyvarnames);
  for (num i = 4;i < numargs-1;++i)
    vector_str_append(&tyvarnames,vector_str_read(&args,i));
  num numtyvarnames = vector_str_len(&tyvarnames);

  num thstart = vector_num_read(&thm2datapos,th);
  num thstop = vector_num_read(&thm2datapos,th+1);
  if (thstart != thstop) reject("TYPE_DEFINITION: input theorem has hypotheses");

  term_ptr thconclusion = vector_term_ptr_read(&thm2conclusion,th);

  term_ptr reppropcheck;
  term_ptr reppoint;
  if (!term_is_eval(thconclusion,0,&reppropcheck,&reppoint)) reject("TYPE_DEFINITION: input theorem is not a combination");
  if (!term_is(repprop,reppropcheck)) reject("TYPE_DEFINITION: repprop mismatch");
  if (!type_is(reptype,term2type(reppoint))) reject("TYPE_DEFINITION: reptype mismatch");

  if (term_has_freevars(repprop)) reject("TYPE_DEFINITION: free variables in repprop");

  static vector_type_ptr tyvars;

  term_tyvars(&tyvars,repprop);
  if (vector_type_ptr_len(&tyvars) != numtyvarnames) reject("TYPE_DEFINITION: wrong number of tyvars");

  num prevpos = -1;
  for (num i = 0;i < numtyvarnames;++i) {
    str tyvarname = vector_str_read(&tyvarnames,i);
    num pos;
    for (pos = 0;pos < numtyvarnames;++pos) {
      type_ptr tyvar = vector_type_ptr_read(&tyvars,pos);
      if (type_is_tyvarnamed(tyvar,tyvarname)) break;
    }
    if (pos >= numtyvarnames) reject("TYPE_DEFINITION: tyvar not in repprop");
    if (prevpos >= 0) {
      type_ptr tyvar = vector_type_ptr_read(&tyvars,pos);
      type_ptr prevtyvar = vector_type_ptr_read(&tyvars,prevpos);
      if (type_cmp(tyvar,prevtyvar) != 1) reject("TYPE_DEFINITION: tyvars not in increasing order");
    }
    prevpos = pos;
  }

  num typedefpos = vector_num_len(&typedef2reptype);

  app2arity_add(abstypename,numtyvarnames);
  absname2typedef_add(absname,typedefpos);
  repname2typedef_add(repname,typedefpos);

  tag_A_pos = vector_num_len(&typedef2reptype);

  vector_num_append(&typedef2reptype,reptype.pos);
  vector_num_append(&typedef2repprop,repprop.pos);
  vector_str_append(&typedef2abstypename,abstypename);
  vector_str_append(&typedef2absname,absname);
  vector_str_append(&typedef2repname,repname);
  vector_num_append(&typedef2numtyvars,numtyvarnames);

  vector_num_append(&typedef2tyvarnamepos,vector_str_len(&typedeftyvarnames));
  for (num i = 0;i < numtyvarnames;++i)
    vector_str_append(&typedeftyvarnames,vector_str_read(&tyvarnames,i));
}

#define UNUSED __attribute__((unused))

static term_ptr finish_A(UNUSED vector_term_ptr *hypotheses)
{
  num typedefpos = tag_A_pos;
  type_ptr reptype = pos2type(vector_num_read(&typedef2reptype,typedefpos));
  str abstypename = vector_str_read(&typedef2abstypename,typedefpos);
  str absname = vector_str_read(&typedef2absname,typedefpos);
  str repname = vector_str_read(&typedef2repname,typedefpos);
  num numtyvars = vector_num_read(&typedef2numtyvars,typedefpos);
  num tyvarnamepos = vector_num_read(&typedef2tyvarnamepos,typedefpos);
  static vector_type_ptr tyvars;
  vector_type_ptr_empty(&tyvars);
  for (num i = 0;i < numtyvars;++i)
    vector_type_ptr_append(&tyvars,type_new_tyvar(vector_str_read(&typedeftyvarnames,tyvarnamepos+i)));
  type_ptr abstype = type_new_app(abstypename,&tyvars);
  term_ptr abs = term_new_const(type_new_fun(reptype,abstype),absname);
  term_ptr rep = term_new_const(type_new_fun(abstype,reptype),repname);
  term_ptr absvar = term_new_var(abstype,str_const_a);
  term_ptr absthm = term_new_eq(term_new_eval(abstype,abs,term_new_eval(reptype,rep,absvar)),absvar);
  return absthm;
}

static num tag_D_pos;

static void tag_D(const char *s)
{
  tag_D_pos = subid(&s,numtypedefs);
  eol(s);
}

static term_ptr finish_D(UNUSED vector_term_ptr *hypotheses)
{
  num typedefpos = tag_D_pos;
  type_ptr reptype = pos2type(vector_num_read(&typedef2reptype,typedefpos));
  term_ptr repprop = pos2term(vector_num_read(&typedef2repprop,typedefpos));
  str abstypename = vector_str_read(&typedef2abstypename,typedefpos);
  str absname = vector_str_read(&typedef2absname,typedefpos);
  str repname = vector_str_read(&typedef2repname,typedefpos);
  num numtyvars = vector_num_read(&typedef2numtyvars,typedefpos);
  num tyvarnamepos = vector_num_read(&typedef2tyvarnamepos,typedefpos);
  static vector_type_ptr tyvars;
  vector_type_ptr_empty(&tyvars);
  for (num i = 0;i < numtyvars;++i)
    vector_type_ptr_append(&tyvars,type_new_tyvar(vector_str_read(&typedeftyvarnames,tyvarnamepos+i)));
  type_ptr abstype = type_new_app(abstypename,&tyvars);
  term_ptr abs = term_new_const(type_new_fun(reptype,abstype),absname);
  term_ptr rep = term_new_const(type_new_fun(abstype,reptype),repname);
  term_ptr repvar = term_new_var(reptype,str_const_r);
  term_ptr repthmleft = term_new_eval(type_new_bool(),repprop,repvar);
  term_ptr repthmright = term_new_eq(term_new_eval(reptype,rep,term_new_eval(abstype,abs,repvar)),repvar);
  term_ptr repthm = term_new_eq(repthmleft,repthmright);
  return repthm;
}

// ==== B: BETA
// input: (x|->y)(x)
// output: (x|->y)(x) = y
static term_ptr tag_B_xyx;
static term_ptr tag_B_y;

static void tag_B(const char *s)
{
  term_ptr term = pos2term(subid(&s,numterms));
  eol(s);

  term_ptr funterm,pointterm;
  if (!term_is_eval(term,0,&funterm,&pointterm)) reject("BETA: operation mismatch");
  // now have: funterm(pointterm)

  if (!term_is_var(pointterm,0,0)) reject("BETA: combination is not with variable");
  // redundant given abs restrictions below, but keep for clarity

  term_ptr fromterm,toterm;
  if (!term_is_mapsto(funterm,0,&fromterm,&toterm)) reject("BETA: combination is not of abstraction");
  // now have: (fromterm|->toterm)(pointterm)

  if (!term_is(fromterm,pointterm)) reject("BETA: combination is not of same variable");
  // now have: (fromterm|->toterm)(fromterm)

  tag_B_xyx = term;
  tag_B_y = toterm;
}

static term_ptr finish_B(UNUSED vector_term_ptr *hypotheses)
{
  return term_new_eq(tag_B_xyx,tag_B_y);
}

// ===== C: DEFINITION

static str tag_C_name;
static type_ptr tag_C_type;
static term_ptr tag_C_defn;

static void tag_C(const char *s)
{
  term_ptr term = pos2term(subid(&s,numterms));
  noteol(s);

  term_ptr left,right;
  if (!term_is_equality(term,&left,&right)) reject("DEFINITION: operation mismatch");

  str name = str_fromptr(s,'\n');

  type_ptr type;
  str leftname;
  if (!term_is_var(left,&type,&leftname)) reject("DEFINITION: not a variable");
  if (!str_eq(name,leftname)) reject("DEFINITION: name mismatch");

  if (term_has_freevars(right)) reject("DEFINITION: free variables on right side");

  static vector_type_ptr tyvars;
  type_vars(&tyvars,type);

  static vector_type_ptr tyvarsright;
  term_tyvars(&tyvarsright,right);

  for (num i = 0;i < vector_type_ptr_len(&tyvarsright);++i) {
    type_ptr ty2 = vector_type_ptr_read(&tyvarsright,i);
    int found = 0;
    for (num j = 0;j < vector_type_ptr_len(&tyvars);++j) {
      type_ptr ty = vector_type_ptr_read(&tyvars,j);
      if (type_is(ty,ty2)) {
        found = 1;
        break;
      }
    }
    if (!found) reject("DEFINITION: type variables on right side not included in constant");
  }

  const2type_add(name,type);

  tag_C_name = name;
  tag_C_type = type;
  tag_C_defn = right;
}

static term_ptr finish_C(UNUSED vector_term_ptr *hypotheses)
{
  term_ptr left = term_new_const(tag_C_type,tag_C_name);
  return term_new_eq(left,tag_C_defn);
}

// ===== E: MK_COMB
// input: theorem thleft = thright
// input: theorem THleft = THright
// output: theorem thleft(THleft) = thright(THright)

static num tag_E_th;
static term_ptr tag_E_thleft;
static term_ptr tag_E_thright;
static num tag_E_TH;
static term_ptr tag_E_THleft;
static term_ptr tag_E_THright;
static type_ptr tag_E_to;

static void tag_E(const char *s)
{
  num th = subid(&s,numthms);
  num TH = subid(&s,numthms);
  eol(s);

  term_ptr thconclusion = vector_term_ptr_read(&thm2conclusion,th);
  term_ptr THconclusion = vector_term_ptr_read(&thm2conclusion,TH);

  term_ptr thleft,thright;
  if (!term_is_equality(thconclusion,&thleft,&thright)) reject("MK_COMB: first theorem is not equality");
  term_ptr THleft,THright;
  if (!term_is_equality(THconclusion,&THleft,&THright)) reject("MK_COMB: second theorem is not equality");

  type_ptr from,to;
  if (!type_is_fun(term2type(thleft),&from,&to)) reject("first MK_COMB: type is not function");
  if (!type_is(term2type(THleft),from)) reject("MK_COMB: combination mismatch");

  tag_E_th = th;
  tag_E_thleft = thleft;
  tag_E_thright = thright;
  tag_E_TH = TH;
  tag_E_THleft = THleft;
  tag_E_THright = THright;
  tag_E_to = to;
}

static term_ptr finish_E(vector_term_ptr *hypotheses)
{
  hypotheses_merge_th(hypotheses,tag_E_th,tag_E_TH,pos2term(-1),pos2term(-1));
  term_ptr left = term_new_eval(tag_E_to,tag_E_thleft,tag_E_THleft);
  term_ptr right = term_new_eval(tag_E_to,tag_E_thright,tag_E_THright);
  return term_new_eq(left,right);
}

// ===== F: ABS
// input: variable v
// input: theorem l = r
// output: (v|->l) = (v|->r)

static term_ptr tag_F_var;
static num tag_F_th;
static term_ptr tag_F_left;
static term_ptr tag_F_right;

static void tag_F(const char *s)
{
  term_ptr var = pos2term(subid(&s,numterms));
  num th = subid(&s,numthms);
  eol(s);

  if (!term_is_var(var,0,0)) reject("ABS: first input is not a variable");

  term_ptr thconclusion = vector_term_ptr_read(&thm2conclusion,th);
  term_ptr left,right;
  if (!term_is_equality(thconclusion,&left,&right)) reject("ABS: second input is not an equality");

  num thstart = vector_num_read(&thm2datapos,th);
  num thstop = vector_num_read(&thm2datapos,th+1);

  for (num i = thstart;i < thstop;++i) {
    term_ptr h = vector_term_ptr_read(&thmdata,i);

    term_data t = term2data(h);
    for (num j = 0;j < t.frees;++j)
      if (term_is(var,vector_term_ptr_read(&termfrees_state,t.freepos+j)))
        reject("ABS variable is free in hypothesis");
  }

  tag_F_var = var;
  tag_F_th = th;
  tag_F_left = left;
  tag_F_right = right;
}

static term_ptr finish_F(vector_term_ptr *hypotheses)
{
  hypotheses_extract_th(hypotheses,tag_F_th,pos2term(-1));
  type_ptr type = type_new_fun(term2type(tag_F_var),term2type(tag_F_left));
  term_ptr left = term_new_mapsto(type,tag_F_var,tag_F_left);
  term_ptr right = term_new_mapsto(type,tag_F_var,tag_F_right);
  return term_new_eq(left,right);
}

// ===== P: EQ_MP
// input: theorem L = R
// input: theorem L (modulo alpha-equivalence)
// output: theorem R

static term_ptr tag_P_right;
static num tag_P_th;
static num tag_P_TH;

static void tag_P(const char *s)
{
  num th = subid(&s,numthms);
  num TH = subid(&s,numthms);
  eol(s);

  term_ptr thconclusion = vector_term_ptr_read(&thm2conclusion,th);
  term_ptr THconclusion = vector_term_ptr_read(&thm2conclusion,TH);

  term_ptr left,right;
  if (!term_is_equality(thconclusion,&left,&right)) reject("EQ_MP: operation mismatch");

  if (!term_alphaequivalent(left,THconclusion)) reject("EQ_MP: left-side mismatch");
   
  tag_P_th = th;
  tag_P_TH = TH;
  tag_P_right = right;
}

static term_ptr finish_P(vector_term_ptr *hypotheses)
{
  hypotheses_merge_th(hypotheses,tag_P_th,tag_P_TH,pos2term(-1),pos2term(-1));
  return tag_P_right;
}

// ===== R: REFL

static term_ptr tag_R_term;

static void tag_R(const char *s)
{
  tag_R_term = pos2term(subid(&s,numterms));
  eol(s);
}

static term_ptr finish_R(UNUSED vector_term_ptr *hypotheses)
{
  return term_new_eq(tag_R_term,tag_R_term);
}

// ===== S: INST

static num tag_S_th;
static vector_term_ptr tag_S_from;
static vector_term_ptr tag_S_to;

static void tag_S(const char *s)
{
  tag_S_th = subid(&s,numthms);

  vector_term_ptr_empty(&tag_S_from);
  vector_term_ptr_empty(&tag_S_to);
  for (;;) {
    if (*s == '\n') break;
    term_ptr from = pos2term(subid(&s,numterms));
    if (*s == '\n') reject("INST: odd number of term arguments");
    term_ptr to = pos2term(subid(&s,numterms));

    type_ptr fromtype;
    if (!term_is_var(from,&fromtype,0)) reject("INST: substituting for non-variable (note: S th from to, not S th to from)");
    if (!type_is(fromtype,term2type(to))) reject("INST: type mismatch");

    for (num i = 0;i < vector_term_ptr_len(&tag_S_from);++i)
      if (term_is(from,vector_term_ptr_read(&tag_S_from,i)))
        goto skip;

    vector_term_ptr_append(&tag_S_from,from);
    vector_term_ptr_append(&tag_S_to,to);
    skip: ;
  }
}

static term_ptr finish_S(vector_term_ptr *hypotheses)
{
  num th = tag_S_th;
  num thstart = vector_num_read(&thm2datapos,th);
  num thstop = vector_num_read(&thm2datapos,th+1);
  for (num i = thstart;i < thstop;++i) {
    term_ptr oldh = vector_term_ptr_read(&thmdata,i);
    vector_term_ptr_append(hypotheses,term_subst(oldh,&tag_S_from,&tag_S_to));
  }
  hypotheses_sort(hypotheses);
  term_ptr thconclusion = vector_term_ptr_read(&thm2conclusion,th);
  return term_subst(thconclusion,&tag_S_from,&tag_S_to);
}

// ===== T: INST_TYPE

static num tag_T_th;
static vector_type_ptr tag_T_from;
static vector_type_ptr tag_T_to;

static void tag_T(const char *s)
{
  tag_T_th = subid(&s,numthms);

  vector_type_ptr_empty(&tag_T_from);
  vector_type_ptr_empty(&tag_T_to);
  for (;;) {
    if (*s == '\n') break;
    type_ptr from = pos2type(subid(&s,numtypes));
    if (*s == '\n') reject("INST_TYPE: odd number of type arguments");
    type_ptr to = pos2type(subid(&s,numtypes));

    if (!type_is_tyvar(from)) reject("INST_TYPE: substituting for non-tyvar (note: T th from to, not T th to from)");

    for (num i = 0;i < vector_type_ptr_len(&tag_T_from);++i)
      if (type_is(from,vector_type_ptr_read(&tag_T_from,i)))
        goto skip;

    vector_type_ptr_append(&tag_T_from,from);
    vector_type_ptr_append(&tag_T_to,to);
    skip: ;
  }
}

static term_ptr finish_T(vector_term_ptr *hypotheses)
{
  num th = tag_T_th;
  num thstart = vector_num_read(&thm2datapos,th);
  num thstop = vector_num_read(&thm2datapos,th+1);
  for (num i = thstart;i < thstop;++i) {
    term_ptr oldh = vector_term_ptr_read(&thmdata,i);
    vector_term_ptr_append(hypotheses,term_tyvar_subst(oldh,&tag_T_from,&tag_T_to));
  }
  hypotheses_sort(hypotheses);
  term_ptr thconclusion = vector_term_ptr_read(&thm2conclusion,th);
  return term_tyvar_subst(thconclusion,&tag_T_from,&tag_T_to);
}

// ===== U: ASSUME

static term_ptr tag_U_term;

static void tag_U(const char *s)
{
  term_ptr term = pos2term(subid(&s,numterms));
  eol(s);

  if (!type_is_bool(term2type(term))) reject("ASSUME: non-bool input");

  tag_U_term = term;
}

static term_ptr finish_U(vector_term_ptr *hypotheses)
{
  vector_term_ptr_append(hypotheses,tag_U_term);
  return tag_U_term;
}

// ===== V: TRANS

static num tag_V_th;
static num tag_V_TH;
static term_ptr tag_V_left;
static term_ptr tag_V_right;

static void tag_V(const char *s)
{
  num th = subid(&s,numthms);
  num TH = subid(&s,numthms);
  eol(s);

  term_ptr thconclusion = vector_term_ptr_read(&thm2conclusion,th);
  term_ptr THconclusion = vector_term_ptr_read(&thm2conclusion,TH);

  term_ptr thleft,thright;
  if (!term_is_equality(thconclusion,&thleft,&thright)) reject("TRANS: first operation mismatch");
  term_ptr THleft,THright;
  if (!term_is_equality(THconclusion,&THleft,&THright)) reject("TRANS: second operation mismatch");

  if (!term_alphaequivalent(thright,THleft)) reject("TRANS: input mismatch");

  tag_V_th = th;
  tag_V_TH = TH;
  tag_V_left = thleft;
  tag_V_right = THright;
}

static term_ptr finish_V(vector_term_ptr *hypotheses)
{
  hypotheses_merge_th(hypotheses,tag_V_th,tag_V_TH,pos2term(-1),pos2term(-1));
  return term_new_eq(tag_V_left,tag_V_right);
}

// ===== X: AXIOM

static term_ptr tag_X_term;

static void tag_X(const char *s)
{
  term_ptr term = pos2term(subid(&s,numterms));
  eol(s);

  if (!type_is_bool(term2type(term))) reject("AXIOM: non-bool input");

  tag_X_term = term;
}

static term_ptr finish_X(UNUSED vector_term_ptr *hypotheses)
{
  return tag_X_term;
}

// ===== Z: DEDUCT_ANTISYM_RULE
// if q entails p and p entails q then p = q

static num tag_Z_th;
static num tag_Z_TH;

static void tag_Z(const char *s)
{
  tag_Z_th = subid(&s,numthms);
  tag_Z_TH = subid(&s,numthms);
  eol(s);
}

static term_ptr finish_Z(vector_term_ptr *hypotheses)
{
  num th = tag_Z_th;
  num TH = tag_Z_TH;
  term_ptr left = vector_term_ptr_read(&thm2conclusion,th);
  term_ptr right = vector_term_ptr_read(&thm2conclusion,TH);
  hypotheses_merge_th(hypotheses,th,TH,right,left);
  return term_new_eq(left,right);
}

// ===== theorems

static char lastinftag;

static void tag_t(const char *s)
{
  know(vector_term_ptr_len(&thm2conclusion) == numthms);
  num hypstart = vector_term_ptr_len(&thmdata);
  if (*s == '\n') reject("theorem with no conclusion");
  term_ptr z = pos2term(subid(&s,numterms));
  num numargs = 1;
  while (*s != '\n') {
    vector_term_ptr_append(&thmdata,z);
    z = pos2term(subid(&s,numterms));
    ++numargs;
  }
  num hypstop = vector_term_ptr_len(&thmdata);
  term_ptr conclusion = z;
  vector_term_ptr_append(&thm2conclusion,conclusion);
  know(vector_term_ptr_len(&thm2conclusion) == numthms+1);

  for (num i = hypstart;i+1 < hypstop;++i) {
    term_ptr hthis = vector_term_ptr_read(&thmdata,i);
    term_ptr hnext = vector_term_ptr_read(&thmdata,i+1);
    if (term_alphaorder_cmp(hthis,hnext) != -1)
      reject("refusing out-of-order hypotheses");
  }

  static vector_term_ptr infhyp;
  know(vector_term_ptr_len(&infhyp) == 0);

  term_ptr (*finish)(vector_term_ptr *);

  switch(lastinftag) {
    case 'A': finish = finish_A; break;
    case 'B': finish = finish_B; break;
    case 'C': finish = finish_C; break;
    case 'D': finish = finish_D; break;
    case 'E': finish = finish_E; break;
    case 'F': finish = finish_F; break;
    case 'P': finish = finish_P; break;
    case 'R': finish = finish_R; break;
    case 'S': finish = finish_S; break;
    case 'T': finish = finish_T; break;
    case 'U': finish = finish_U; break;
    case 'V': finish = finish_V; break;
    case 'X': finish = finish_X; break;
    case 'Z': finish = finish_Z; break;
    default:
      reject("theorem without preceding inference");
  }

  // finish_* is allowed to temporarily allocate strings, types, terms
  // which we will (and must) clean up in bulk at the end of this function
  num str_collection_len = vector_uint8_len(&str_collection);
  num type_data_state_len = vector_type_data_len(&type_data_state);
  num type_subtype_state_len = vector_type_ptr_len(&type_subtype_state);
  num term_data_state_len = vector_term_data_len(&term_data_state);
  num termfrees_state_len = vector_term_ptr_len(&termfrees_state);

  term_ptr infconclusion = finish(&infhyp);
  if (!term_is(conclusion,infconclusion)) reject("conclusion mismatch");

  num infhyplen = vector_term_ptr_len(&infhyp);
  if (infhyplen != hypstop-hypstart) reject("wrong number of hypotheses");
  for (num i = 0;i < infhyplen;++i) {
    term_ptr h = vector_term_ptr_read(&infhyp,i);
    term_ptr infh = vector_term_ptr_read(&thmdata,hypstart+i);
    if (!term_is(h,infh)) reject("hypothesis mismatch");
  }

  vector_term_ptr_empty(&infhyp);
  vector_uint8_trunc(&str_collection,str_collection_len);
  vector_type_data_trunc(&type_data_state,type_data_state_len);
  vector_type_ptr_trunc(&type_subtype_state,type_subtype_state_len);
  vector_term_data_trunc(&term_data_state,term_data_state_len);
  vector_term_ptr_trunc(&termfrees_state,termfrees_state_len);
}

// ===== tags not relevant to logic; just check topology

static num numinfs;
static num numpt;

static void tag_slash(const char *s)
{
  subid(&s,numthms);
  noteol(s);
}

static void tag_comma(const char *s)
{
  subid(&s,numthms);
  subid(&s,numinfs);
  if (*s != '\n') subid(&s,numpt);
  if (*s != '\n') subid(&s,numpt);
  eol(s);
}

// ===== dispatcher

static void handleline(num start,num stop)
{
  know(0 <= start);
  know(start <= stop);
  know(stop < in_bytes);
  know(in_buf[stop] == '\n');

  ++curline;
  if (stop <= start) {
    fprintf(stderr,"blank line %lld prohibited\n",curline);
    exit(100);
  }
  
  char tag = in_buf[start];
  const char *s = in_buf+start+1;

  switch(tag) {
    case 'a': tag_a(s); break;
    case 'b': tag_b(s); break;
    case 'c': tag_c(s); break;
    case 'd': tag_d(s); break;
    case 'e': tag_e(s); break;
    case 'f': tag_f(s); break;
    case 't': tag_t(s); break;
    case 'A': tag_A(s); break;
    case 'B': tag_B(s); break;
    case 'C': tag_C(s); break;
    case 'D': tag_D(s); break;
    case 'E': tag_E(s); break;
    case 'F': tag_F(s); break;
    case 'P': tag_P(s); break;
    case 'R': tag_R(s); break;
    case 'S': tag_S(s); break;
    case 'T': tag_T(s); break;
    case 'U': tag_U(s); break;
    case 'V': tag_V(s); break;
    case 'X': tag_X(s); break;
    case 'Z': tag_Z(s); break;
    case '/': tag_slash(s); break;
    case ',': tag_comma(s); break;
    case ';': break;
    default:
      fprintf(stderr,"line %lld: unrecognized tag %c\n",curline,tag);
      exit(100);
  }

  switch(tag) {
    case 'a': case 'b':
      ++numtypes;
      break;
    case 'c': case 'd': case 'e': case 'f':
      ++numterms;
      break;
    case 't':
      ++numthms;
      vector_num_append(&thm2datapos,vector_term_ptr_len(&thmdata));
      break;
    case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'P': case 'R':
    case 'S': case 'T': case 'U': case 'V':
    case 'X': case 'Z':
      if (tag == 'A') ++numtypedefs;
      ++numinfs;
      lastinftag = tag;
      break;
    case ',': case ';':
      ++numpt;
      break;
    case '/':
      break;
    default:
      fprintf(stderr,"line %lld: unrecognized tag %c\n",curline,tag);
      exit(100);
  }
}

// ===== overall driver

static void verify(void)
{
  if (in_bytes < 0) {
    fprintf(stderr,"internal error: negative length\n");
    exit(100);
  }
  if (in_bytes > CUTOFF) {
    fprintf(stderr,"refusing to handle more than %lld bytes\n",CUTOFF);
    exit(100);
  }

  vector_num_append(&thm2datapos,0);

  num start = 0;
  for (num i = 0;i < in_bytes;++i) {
    char c = in_buf[i];
    if (c == '\n') {
      if (start) handleline(start,i);
      start = i+1;
      continue;
    }
    if (c < 32 || c > 126) {
      fprintf(stderr,"byte %d is not printable ASCII\n",(int) c);
      exit(100);
    }
  }
  if (start != in_bytes) reject("subsequent partial final line");
}

int main()
{
  str_const_init();
  app2arity_init();

  in_init();
  if (in_bytes < 11 || memcmp(in_buf,"HOLTrace 1\n",11)) {
    fprintf(stderr,"input does not start with HOLTrace 1\n");
    exit(100);
  }

  verify();
  fprintf(stderr,"verified %lld bytes, %lld lines, %lld types, %lld terms, %lld infs, %lld thms, %lld typedefs\n"
    ,in_bytes,curline
    ,numtypes
    ,numterms
    ,numinfs
    ,numthms
    ,numtypedefs
  );

  return 0;
}
