#!/usr/bin/env pypy3

import sys

import type
import term

# ===== formatting

def reject(s): raise Exception(s)

write = sys.stdout.buffer.write

base32_bytes = '0123456789ABCDEFGHIJKLMNOPQRSTUV'

def base32(n):
  if n < 0: return '-'+base32(-n)
  result = ''
  while n > 0:
    d,n = n%32,n//32
    result += base32_bytes[d:d+1]
  return result[::-1]

def simpleencode(n):
  if n == 0: return '0'
  if n == -1: return '!'
  if n == -2: return '@'
  if n == -3: return '#'
  if n == -4: return '$'
  if n == -5: return '%'
  if n == -6: return '^'
  if n == -7: return '&'
  if n == -8: return '*'
  if n == -9: return '('
  if n == -10: return ')'
  return base32(n)

def trace(tag,idlist,s=None):
  args = tuple(map(simpleencode,idlist))
  if s is not None:
    if len(s) == 0 or ' ' in s or not s.isprintable():
      reject('refusing unusual characters in name')
    args += (s,)
  result = tag
  lastbyte = ' '
  for arg in args:
    firstbyte = arg[0]
    if lastbyte in base32_bytes:
      if firstbyte in ' '+base32_bytes:
        result += ' '
    result += arg
    lastbyte = arg[-1]
  result += '\n'
  write(result.encode())

def compress(x,reference):
  assert x >= 0
  y = x-reference
  if y >= 0: return x
  if y >= -10: return y
  if -y >= x: return x
  if -32*y <= x: return y
  if len(simpleencode(y)) <= len(simpleencode(x)): return y
  return x

# ===== type names

typename2arity = {
  type.BOOL: 0,
  type.FUN: 2,
  type.IND: 0
}

def typename_add(name,arity):
  if name in typename2arity: reject(f'type name {name} already used')
  typename2arity[name] = arity

# ===== constant names

Atype = type.var('A')
Atype2bool = type.fun(Atype,type.bool)
constname2type = {
  '=': type.fun(Atype,Atype2bool),
  '@': type.fun(Atype2bool,Atype),
}

def constant_add(name,y):
  if name in constname2type: reject(f'constant name {name} already used')
  constname2type[name] = y
  return term.const(name,y)

def constant_subst(name,y2z):
  if name not in constname2type: reject(f'constant name {name} undefined')
  return term.const(name,type.subst(constname2type[name],y2z))

def find_subst(y2z,y,z):
  if type.is_var(y):
    if y not in y2z: y2z[y] = z
    if y2z[y] != z: reject(f'type mismatch')
    return
  if type.app_name(y) != type.app_name(z): reject(f'type mismatch')
  for suby,subz in zip(type.app_args(y),type.app_args(z)):
    find_subst(y2z,suby,subz)

def mk_const(name,z):
  if name not in constname2type: reject(f'constant name {name} undefined')
  y = constname2type[name]
  y2z = {}
  find_subst(y2z,y,z)
  result = constant_subst(name,y2z)
  assert term.ty(result) == z
  return result

# ===== types

id2type = []
type2id = {}

def typeid2compress(id):
  return compress(id,len(id2type))

def type2compress(ty):
  return typeid2compress(type2id[ty])

def trace_type(ty):
  try:
    id = type2id[ty]
  except KeyError:
    if type.is_var(ty):
      name = type.var_name(ty)
      assert len(name) > 0
      trace('a',(),name)
    else:
      name = type.app_name(ty)
      args = type.app_args(ty)
      if len(name) == 0: reject('empty name')
      if name not in typename2arity: reject(f'undefined type {name}')
      if len(args) != typename2arity[name]: reject('wrong number of app arguments')
      args = tuple(trace_type(arg)[1] for arg in args)
      ids = (len(args),)+tuple(map(typeid2compress,args))
      trace('b',ids,name)
    id = len(id2type)
    id2type.append(ty)
    type2id[ty] = id
    return ty,id
  return id2type[id],id

# ===== terms

id2term = []
term2id = {}

def termid2compress(id):
  return compress(id,len(id2term))

def term2compress(tm):
  return termid2compress(term2id[tm])

def trace_term(tm):
  try:
    id = term2id[tm]
  except KeyError:
    ty,tyid = trace_type(term.ty(tm))
    if term.is_const(tm):
      name = term.const_name(tm)
      assert len(name) > 0
      tm = mk_const(name,ty)
      ids = typeid2compress(tyid),
      trace('c',ids,name)
    elif term.is_var(tm):
      s = term.var_name(tm)
      assert len(s) > 0
      ids = typeid2compress(tyid),
      trace('d',ids,s)
    elif term.is_eval(tm):
      f,x = term.eval_funpoint(tm)
      _,fid = trace_term(f)
      _,xid = trace_term(x)
      ids = typeid2compress(tyid),termid2compress(fid),termid2compress(xid)
      trace('e',ids)
    else:
      v,b = term.mapsto_varout(tm)
      _,vid = trace_term(v)
      _,bid = trace_term(b)
      ids = typeid2compress(tyid),termid2compress(vid),termid2compress(bid)
      trace('f',ids)
    id = len(id2term)
    id2term.append(tm)
    term2id[tm] = id
    return tm,id
  return id2term[id],id

# ===== theorems

id2thm = []
thm2id = {}

def thmid2compress(id):
  return compress(id,len(id2thm))

def thm2compress(th):
  return thmid2compress(thm2id[th])

def thm_known(th):
  return th in thm2id

def trace_thm(th):
  try:
    id = thm2id[th]
  except:
    H,C = th
    hcid = tuple(trace_term(x)[1] for x in H+(C,))
    ids = tuple(map(termid2compress,hcid))
    trace('t',ids)
    id = len(id2thm)
    id2thm.append(th)
    thm2id[th] = id
    return th,id
  return id2thm[id],id

# ===== inferences

def hypotheses_merge(H0,H1):
  return tuple(term.alphaorder_merge(H0,H1))

def hypotheses_map(f,H):
  return term.alphaorder_map(f,H)

def AXIOM(t):
  t,tid = trace_term(t)
  if term.ty(t) != type.bool: reject('AXIOM: non-bool')
  result = (),t
  if thm_known(result): return result
  ids = termid2compress(tid),
  trace('X',ids)
  return trace_thm(result)[0]

def REFL(t):
  t,tid = trace_term(t)
  result = (),term.eq(t,t)
  if thm_known(result): return result
  ids = termid2compress(tid),
  trace('R',ids)
  return trace_thm(result)[0]

def ASSUME(t):
  t,tid = trace_term(t)
  if term.ty(t) != type.bool: reject('ASSUME: non-bool')
  result = (t,),t
  if thm_known(result): return result
  ids = termid2compress(tid),
  trace('U',ids)
  return trace_thm(result)[0]

def BETA(t):
  t,tid = trace_term(t)
  if not term.is_eval(t): reject('BETA: non-eval')
  f,x = term.eval_funpoint(t)
  if not term.is_mapsto(f): reject('BETA: non-mapsto')
  v,y = term.mapsto_varout(f)
  if x != v: reject('BETA: variable mismatch')
  result = (),term.eq(t,y)
  if thm_known(result): return result
  ids = termid2compress(tid),
  trace('B',ids)
  return trace_thm(result)[0]

def TRANS(th,TH):
  (h,c),thid = trace_thm(th)
  (H,C),THid = trace_thm(TH)
  if not term.is_eq(c): reject('TRANS: first conclusion not equality')
  if not term.is_eq(C): reject('TRANS: second conclusion not equality')
  l,r = term.eq_split(c)
  L,R = term.eq_split(C)
  if not term.alphaequivalent(r,L): reject('TRANS: mismatch')
  result = hypotheses_merge(h,H),term.eq(l,R)
  if thm_known(result): return result
  ids = thmid2compress(thid),thmid2compress(THid)
  trace('V',ids)
  return trace_thm(result)[0]

def MK_COMB(th,TH):
  (h,c),thid = trace_thm(th)
  (H,C),THid = trace_thm(TH)
  if not term.is_eq(c): reject('MK_COMB: first conclusion not equality')
  if not term.is_eq(C): reject('MK_COMB: second conclusion not equality')
  l,r = term.eq_split(c)
  L,R = term.eq_split(C)
  if not type.is_fun(term.ty(l)): reject('MK_COMB: first equality is not of functions')
  y,_ = type.fun_args(term.ty(l))
  if term.ty(L) != y: reject('MK_COMB: type mismatch')
  result = hypotheses_merge(h,H),term.eq(term.eval(l,L),term.eval(r,R))
  if thm_known(result): return result
  ids = thmid2compress(thid),thmid2compress(THid)
  trace('E',ids)
  return trace_thm(result)[0]

def EQ_MP(th,TH):
  (h,c),thid = trace_thm(th)
  (H,C),THid = trace_thm(TH)
  if not term.is_eq(c): reject('EQ_MP: first conclusion not equality')
  l,r = term.eq_split(c)
  if not term.alphaequivalent(l,C): reject('EQ_MP: mismatch')
  result = hypotheses_merge(h,H),r
  if thm_known(result): return result
  ids = thmid2compress(thid),thmid2compress(THid)
  trace('P',ids)
  return trace_thm(result)[0]

def DEDUCT_ANTISYM_RULE(th,TH):
  (h,c),thid = trace_thm(th)
  (H,C),THid = trace_thm(TH)
  h = [t for t in h if not term.alphaequivalent(t,C)]
  H = [t for t in H if not term.alphaequivalent(t,c)]
  result = hypotheses_merge(h,H),term.eq(c,C)
  if thm_known(result): return result
  ids = thmid2compress(thid),thmid2compress(THid)
  trace('Z',ids)
  return trace_thm(result)[0]

def ABS(tm,th):
  tm,tmid = trace_term(tm)
  (h,c),thid = trace_thm(th)
  if not term.is_var(tm): reject('ABS: non-variable')
  if not term.is_eq(c): reject('ABS: non-equality')
  l,r = term.eq_split(c)
  if any(tm in term.freevars(x) for x in h): reject('ABS: variable is free in hypotheses')
  result = h,term.eq(term.mapsto(tm,l),term.mapsto(tm,r))
  if thm_known(result): return result
  ids = termid2compress(tmid),thmid2compress(thid)
  trace('F',ids)
  return trace_thm(result)[0]

def INST(th,vwlist):
  (h,c),thid = trace_thm(th)
  v2w = {}
  for v,w in vwlist:
    if not term.is_var(v): reject('INST: non-var')
    if term.ty(v) != term.ty(w): reject('INST: type mismatch')
    if v not in v2w: v2w[v] = w
  h = hypotheses_map((lambda x:term.subst_term(x,v2w)),h)
  result = h,term.subst_term(c,v2w)
  if thm_known(result): return result
  args = ()
  for v,w in vwlist:
    args += trace_term(v)[1],trace_term(w)[1]
  ids = thmid2compress(thid),
  ids += tuple(map(termid2compress,args))
  trace('S',ids)
  return trace_thm(result)[0]

def INST_TYPE(th,xylist):
  (h,c),thid = trace_thm(th)
  x2y = {}
  for x,y in xylist:
    if not type.is_var(x): reject('INST_TYPE: non-tyvar (HOL Light ignores this)')
    if x not in x2y: x2y[x] = y
  h = hypotheses_map((lambda t:term.subst_type(t,x2y)),h)
  result = h,term.subst_type(c,x2y)
  if thm_known(result): return result
  args = ()
  for x,y in xylist:
    args += trace_type(x)[1],trace_type(y)[1]
  ids = thmid2compress(thid),
  ids += tuple(map(typeid2compress,args))
  trace('T',ids)
  return trace_thm(result)[0]

def NEW_DEFINITION(t,newname):
  t,tid = trace_term(t)
  if not term.is_eq(t): reject('NEW_DEFINITION: non-equality')
  l,r = term.eq_split(t)
  if not term.is_var(l): reject('NEW_DEFINITION: non-variable')
  if term.var_name(l) != newname: reject('NEW_DEFINITION: name mismatch')
  Ctype = term.ty(l)
  if not term.is_closed(r): reject('NEW_DEFINITION: free variables')
  if not set(term.tyvars(r)).issubset(type.vars(Ctype)): reject('NEW_DEFINITION: missing tyvars')
  C = constant_add(newname,Ctype)
  result = (),term.eq(C,r)
  if thm_known(result): return result
  ids = termid2compress(tid),
  trace('C',ids,newname)
  return trace_thm(result)[0]

typedefs = 0

def TYPE_DEFINITION(reptype,repterm,th,abstypename,absname,repname,tyvarnames):
  global typedefs

  reptype,reptypeid = trace_type(reptype)
  repterm,reptermid = trace_term(repterm)
  (h,c),thid = trace_thm(th)
  tyvars = tuple(map(type.var,tyvarnames))

  if len(h) > 0: reject('TYPE_DEFINITION: hypotheses')
  if not term.is_eval(c): reject('TYPE_DEFINITION: non-eval')
  P,x = term.eval_funpoint(c)
  if repterm != P: reject('TYPE_DEFINITION: repterm mismatch')
  if reptype != term.ty(x): reject('TYPE_DEFINITION: reptype mismatch')
  if not term.is_closed(P): reject('TYPE_DEFINITION: free variables')
  if tyvars != tuple(sorted(set(term.tyvars(P)))): reject('TYPE_DEFINITION: tyvars mismatch')

  typename_add(abstypename,len(tyvars))
  abstype = type.app(abstypename,*tyvars)

  abs = constant_add(absname,type.fun(reptype,abstype))
  rep = constant_add(repname,type.fun(abstype,reptype))

  absvar = term.var('a',abstype)
  repvar = term.var('r',reptype)

  absthm = (),term.eq(term.eval(abs,term.eval(rep,absvar)),absvar)
  repthm = (),term.eq(term.eval(P,repvar),term.eq(term.eval(rep,term.eval(abs,repvar)),repvar))

  ids = typeid2compress(reptypeid),termid2compress(reptermid),thmid2compress(thid)
  args = (abstypename,absname,repname)+tyvarnames
  args = ']'+']'.join(args)+']'
  trace('A',ids,args)
  absthm,absthmid = trace_thm(absthm)
  trace('D',(-1,))
  typedefs += 1
  repthm,repthmid = trace_thm(repthm)

  return absthm,repthm

# ===== theorem names

thmnames = set()
# XXX: exclude duplicates

def NAME(th,s):
  if len(s) == 0 or ' ' in s or not s.isprintable(): reject('unusual characters in name')
  if s in thmnames: reject('theorem name used already')
  th,thid = trace_thm(th)
  ids = thmid2compress(thid),
  trace('/',ids,s)
