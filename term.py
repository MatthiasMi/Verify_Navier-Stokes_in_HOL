#!/usr/bin/env pypy3

import type

CONST = 'c'
VAR = 'd'
EVAL = 'e' # aka comb
MAPSTO = 'f'

# requirements for alphaorder to match hol-light/fusion.ml
assert CONST < VAR
assert VAR < EVAL
assert EVAL < MAPSTO

EQ = '='

# ===== basic API

def _op(t): return t[0]

def ty(t): return t[1]

def is_closed(t):
  if t[0] == VAR: return False
  return t[2] == ()

def freevars(t):
  if t[0] == VAR: return (t,)
  return t[2]

def const(name,y):
  assert isinstance(name,str)
  return CONST,y,(),name
def is_const(t): return t[0] == CONST
def const_name(t):
  assert t[0] == CONST
  return t[3]

def var(name,y):
  assert isinstance(name,str)
  return VAR,y,None,name
def is_var(t): return t[0] == VAR
def var_name(t):
  assert t[0] == VAR
  return t[3]

# overriding built-in eval
def eval(fterm,pterm,y=None):
  if y is None: _,y = type.fun_args(ty(fterm))
  assert ty(fterm) == type.fun(ty(pterm),y)
  free = set(freevars(fterm)).union(set(freevars(pterm)))
  return EVAL,y,tuple(free),fterm,pterm
def is_eval(t): return t[0] == EVAL
def eval_fun(t):
  assert t[0] == EVAL
  return t[3]
def eval_point(t):
  assert t[0] == EVAL
  return t[4]
def eval_funpoint(t):
  assert t[0] == EVAL
  return t[3:]

def mapsto(vterm,outterm,y=None):
  assert is_var(vterm)
  if y is None:
    y = type.fun(ty(vterm),ty(outterm))
  else:
    assert y == type.fun(ty(vterm),ty(outterm))
  free = set(freevars(outterm))
  free.discard(vterm)
  return MAPSTO,y,tuple(free),vterm,outterm
def is_mapsto(t): return t[0] == MAPSTO
def mapsto_var(t):
  assert t[0] == MAPSTO
  return t[3]
def mapsto_out(t):
  assert t[0] == MAPSTO
  return t[4]
def mapsto_varout(t):
  assert t[0] == MAPSTO
  return t[3:]

# ===== extra functions

# yield the type variables used in t
def tyvars(t):
  if is_eval(t):
    yield from tyvars(eval_fun(t))
    yield from tyvars(eval_point(t))
  elif is_mapsto(t):
    yield from type.vars(ty(t))
    yield from tyvars(mapsto_out(t))
  else:
    assert is_var(t) or is_const(t)
    yield from type.vars(ty(t))

def eq(t,u):
  y = ty(t)
  assert y == ty(u)
  equality = const(EQ,type.fun(y,type.fun(y,type.bool)))
  return eval(eval(equality,t),u)

def is_eq(t):
  if not is_eval(t): return False
  u,_ = eval_funpoint(t)
  if not is_eval(u): return False
  v,_ = eval_funpoint(u)
  if not is_const(v): return False
  return const_name(v) == EQ

def eq_split(t):
  u,b = eval_funpoint(t)
  v,a = eval_funpoint(u)
  assert const_name(v) == EQ
  return a,b

# ===== alphaorder

# this is a total order on terms modulo alpha-equivalence
# (not alphabetical order)

# should match ordav in hol-light/fusion.ml
# which is orda when t,u are each variables
def alphaorder_cmp_vars(t,u,L=None):
  while L is not None:
    v,w,L = L
    if t == v: return 0 if u == w else -1
    if u == w: return 1
  tkey = var_name(t),ty(t)
  ukey = var_name(u),ty(u)
  return (tkey>ukey)-(tkey<ukey)

# should match orda in hol-light/fusion.ml
def alphaorder_cmp(t,u,L=None):
  opt,opu = _op(t),_op(u)
  if opt != opu: return (opt>opu)-(opt<opu)
  if is_const(t):
    tkey = const_name(t),ty(t)
    ukey = const_name(u),ty(u)
    return (tkey>ukey)-(tkey<ukey)
  if is_var(t):
    return alphaorder_cmp_vars(t,u,L)
  if is_eval(t):
    tfun,tpoint = eval_funpoint(t)
    ufun,upoint = eval_funpoint(u)
    return alphaorder_cmp(tfun,ufun,L) or alphaorder_cmp(tpoint,upoint,L)
  tvar,tout = mapsto_varout(t)
  uvar,uout = mapsto_varout(u)
  tyv,tyw = ty(tvar),ty(uvar)
  return (tyv>tyw)-(tyw>tyv) or alphaorder_cmp(tout,uout,(tvar,uvar,L))

# should match term_union in hol-light/fusion.ml
# assumes input lists are sorted and deduplicated mod alpha
# yields sorted, deduplicated outputs
# when lists collide mod alpha, takes representative from first list
def alphaorder_merge(L,M):
  while True:
    if len(L) == 0:
      yield from M
      return
    if len(M) == 0:
      yield from L
      return
    order = alphaorder_cmp(L[0],M[0])
    yield L[0] if order <= 0 else M[0]
    L,M = L[order<=0:],M[order>=0:]

def alphaequivalent(t,u):
  return alphaorder_cmp(t,u) == 0

# should match term_image in hol-light/fusion.ml
# XXX: this is quadratic number of comparisons for long assumption lists
def alphaorder_map(f,L):
  if len(L) == 0: return ()
  return tuple(alphaorder_merge([f(L[0])],alphaorder_map(f,L[1:])))

# ===== substitutions

# should match vsubst in hol-light/fusion.ml
# substituting w for variable v in t, if dictionary v2w has v:w
def subst_term(t,v2w):
  if len(v2w) == 0: return t
  if is_const(t): return t
  if is_var(t): return v2w.get(t,t)
  if is_eval(t):
    return eval(subst_term(eval_fun(t),v2w),subst_term(eval_point(t),v2w),ty(t))
  v,s = mapsto_varout(t)
  assert is_var(v)
  innerv2w = dict((x,v2w[x]) for x in v2w if x != v)
  if len(innerv2w) == 0: return t
  snew = subst_term(s,innerv2w)
  if snew == s: return t
  if any(v in freevars(innerv2w[x]) and x in freevars(s) for x in innerv2w):
    w = v
    while w in freevars(snew):
      w = var(var_name(w)+"'",ty(w))
    assert v not in innerv2w
    innerv2w[v] = w
    return mapsto(w,subst_term(s,innerv2w),ty(t))
  return mapsto(v,snew,ty(t))

class subst_type_clash(Exception): pass

# should match inst in hol-light/fusion.ml
# substituting z for tyvar y in t, if dictionary y2z has y:z
def subst_type(t,y2z,bindmap={}):
  newty = type.subst(ty(t),y2z)
  if is_const(t):
    return const(const_name(t),newty)
  if is_var(t):
    u = var(var_name(t),newty)
    if bindmap.get(u,t) == t: return u
    raise subst_type_clash(u)
  if is_eval(t):
    return eval(
      subst_type(eval_fun(t),y2z,bindmap),
      subst_type(eval_point(t),y2z,bindmap),
      newty)
  v,u = mapsto_varout(t)
  assert is_var(v)
  w = subst_type(v,y2z)
  innerbindmap = dict(bindmap)
  innerbindmap[w] = v
  try:
    return mapsto(w,subst_type(u,y2z,innerbindmap),newty)
  except subst_type_clash as e:
    if e.args != (w,): raise
  todo = set(subst_type(x,y2z) for x in freevars(u))
  while w in todo:
    w = var(var_name(w)+"'",ty(w))
  z = var(var_name(w),ty(v))
  u = mapsto(z,subst_term(u,{v:z}),ty(t))
  return subst_type(u,y2z,bindmap)

# ===== tests

def test():
  print('test term')

  fun = type.fun
  bool = type.bool
  num = type.app('num')
  Atype = type.var('A')
  Btype = type.var('B')

  # ===== var

  x_A = var('x',Atype)
  assert is_var(x_A)
  assert not is_const(x_A)
  assert not is_eval(x_A)
  assert not is_mapsto(x_A)

  x_B = var('x',Btype)
  xprime_A = var("x'",Atype)
  xprimeprime_A = var("x''",Atype)
  y_A = var('y',Atype)
  y_B = var('y',Btype)
  z_bool = var('z',bool)

  assert x_A == var('x',Atype)
  assert x_A != x_B
  assert x_A != y_A

  # ===== const

  Tbool = const('T',bool)
  assert is_const(Tbool)
  assert not is_var(Tbool)
  assert not is_eval(Tbool)
  assert not is_mapsto(Tbool)

  Ubool = const('U',bool)
  Unum = const('U',num)
  Tnum = const('T',num)
  _0 = const('_0',num)
  numeral = const('NUMERAL',fun(num,num))

  assert Tbool == const('T',bool)
  assert Tbool != _0
  assert Tbool != Ubool
  assert Tbool != Tnum

  # ===== eval

  numeral_0 = eval(numeral,_0,num)
  assert not is_const(numeral_0)
  assert not is_var(numeral_0)
  assert is_eval(numeral_0)
  assert not is_mapsto(numeral_0)

  assert numeral_0 == eval(numeral,_0)
  assert numeral_0 == eval(numeral,_0,num)

  # ===== mapsto

  xtoxeqx = mapsto(x_A,eq(x_A,x_A))
  assert not is_const(xtoxeqx)
  assert not is_var(xtoxeqx)
  assert not is_eval(xtoxeqx)
  assert is_mapsto(xtoxeqx)

  assert xtoxeqx == mapsto(x_A,eq(x_A,x_A),fun(Atype,bool))

  # ===== const_name

  assert const_name(Tbool) == 'T'
  assert const_name(_0) == '_0'
  for t in x_A,numeral_0,xtoxeqx:
    ok = False
    try: const_name(t)
    except: ok = True
    assert ok

  # ===== var_name

  assert var_name(x_A) == 'x'
  assert var_name(xprime_A) == "x'"
  assert var_name(x_B) == 'x'
  assert var_name(y_B) == 'y'
  for t in Tbool,numeral_0,xtoxeqx:
    ok = False
    try: var_name(t)
    except: ok = True
    assert ok

  # ===== eval_fun, eval_point, eval_funpoint

  assert eval_fun(numeral_0) == numeral
  assert eval_point(numeral_0) == _0
  assert eval_funpoint(numeral_0) == (numeral,_0)
  for fail in eval_fun,eval_point,eval_funpoint:
    for t in x_A,Tbool,xtoxeqx:
      ok = False
      try: fail(t)
      except: ok = True
      assert ok

  # ===== mapsto_var, mapsto_out, mapsto_varout

  assert mapsto_var(xtoxeqx) == x_A
  assert mapsto_out(xtoxeqx) == eq(x_A,x_A)
  assert mapsto_varout(xtoxeqx) == (x_A,eq(x_A,x_A))
  for fail in mapsto_var,mapsto_out,mapsto_varout:
    for t in x_A,Tbool,numeral_0:
      ok = False
      try: fail(t)
      except: ok = True
      assert ok

  # ===== eq, eq_split
  # (also used eq earlier)

  assert eq_split(eq(x_A,y_A)) == (x_A,y_A)
  assert is_eq(eq(x_A,y_A))
  for t in x_A,Tbool,numeral_0,xtoxeqx:
    assert not is_eq(t)
    ok = False
    try: eq_split(t)
    except: ok = True
    assert ok

  assert const_name(eval_fun(eval_fun(eq(x_A,y_A)))) == EQ

  # ===== freevars, is_closed

  assert is_closed(xtoxeqx)
  assert sorted(freevars(xtoxeqx)) == []
  assert not is_closed(x_A)
  assert sorted(freevars(x_A)) == sorted([x_A])
  assert not is_closed(eq(x_A,y_A)) == sorted([x_A,y_A])
  assert sorted(freevars(eq(x_A,y_A))) == sorted([x_A,y_A])
  assert sorted(freevars(eq(eq(x_A,y_A),z_bool))) == sorted([x_A,y_A,z_bool])

  # based on frees example in HOL Light manual
  t = eval(eval(const('/\\',fun(bool,fun(bool,bool))),eval(eval(const('=',fun(num,fun(num,bool))),var('x',num)),eval(numeral,eval(const('BIT1',fun(num,num)),_0)))),eval(eval(const('/\\',fun(bool,fun(bool,bool))),eval(eval(const('=',fun(num,fun(num,bool))),var('y',num)),eval(numeral,eval(const('BIT0',fun(num,num)),eval(const('BIT1',fun(num,num)),_0))))),eval(const('!',fun(fun(num,bool),bool)),mapsto(var('z',num),eval(eval(const('>=',fun(num,fun(num,bool))),var('z',num)),eval(numeral,_0))))))
  assert sorted(freevars(t)) == sorted([var('x',num),var('y',num)])

  # ===== ty

  assert ty(x_A) == Atype
  assert ty(x_B) == Btype
  assert ty(Tbool) == bool
  assert ty(numeral_0) == num
  assert ty(xtoxeqx) == fun(Atype,bool)
  assert ty(eq(x_A,y_A)) == bool
  assert ty(eq(eq(x_A,y_A),z_bool)) == bool
  assert ty(mapsto(x_A,y_A)) == fun(Atype,Atype)
  assert ty(mapsto(x_A,y_B)) == fun(Atype,Btype)

  # ===== alphaorder

  for expected,t,u in (
    ( 0,var('x',num),var('x',num)),
    (-1,var('x',num),var('y',num)),
    (-1,var('x',bool),var('y',num)),
    (-1,var('x',bool),var('y',bool)),
    (-1,var('x',bool),var('x',num)),
    ( 1,var('y',num),var('x',num)),
    ( 1,var('y',bool),var('x',num)),
    ( 1,var('y',bool),var('x',bool)),
    ( 1,var('x',num),var('x',bool)),
    ( 0,Tbool,Tbool),
    (-1,Tbool,Ubool),
    (-1,Tbool,Tnum),
    (-1,Tnum,Ubool),
    ( 1,Ubool,Tbool),
    ( 1,Tnum,Tbool),
    ( 1,Ubool,Tnum),
    ( 0,xtoxeqx,xtoxeqx),
    ( 0,xtoxeqx,mapsto(y_A,eq(y_A,y_A))), # alpha-equivalent
    (-1,mapsto(y_A,eq(y_A,y_A)),mapsto(y_A,eq(x_A,x_A))), # bound < free
    ( 1,mapsto(y_A,eq(x_A,x_A)),mapsto(y_A,eq(y_A,y_A))), # free > bound
    (-1,xtoxeqx,mapsto(y_A,eq(x_A,x_A))),
    ( 1,mapsto(y_A,eq(x_A,x_A)),xtoxeqx),
    (-1,xtoxeqx,mapsto(x_A,eq(y_A,y_A))),
    ( 1,mapsto(x_A,eq(y_A,y_A)),xtoxeqx),
    ( 0,numeral_0,numeral_0),
    (-1,Tbool,var('x',num)),
    (-1,Tbool,numeral_0),
    (-1,Tbool,xtoxeqx),
    (-1,var('x',num),numeral_0),
    (-1,var('x',num),xtoxeqx),
    (-1,numeral_0,xtoxeqx),
    ( 1,var('x',num),Tbool),
    ( 1,numeral_0,Tbool),
    ( 1,xtoxeqx,Tbool),
    ( 1,numeral_0,var('x',num)),
    ( 1,xtoxeqx,var('x',num)),
    ( 1,xtoxeqx,numeral_0),
  ):
    if t[0] == VAR and u[0] == VAR:
      assert expected == alphaorder_cmp_vars(t,u)
    assert expected == alphaorder_cmp(t,u)
    if expected == 0:
      assert list(alphaorder_merge([t],[u])) == [t]
    if expected == -1:
      assert list(alphaorder_merge([t],[u])) == [t,u]
      assert list(alphaorder_merge([t],[t,u])) == [t,u]
      assert list(alphaorder_merge([t,u],[u])) == [t,u]
      assert list(alphaorder_merge([t,u],[t,u])) == [t,u]
    if expected == 1:
      assert list(alphaorder_merge([t],[u])) == [u,t]
      assert list(alphaorder_merge([t],[u,t])) == [u,t]
      assert list(alphaorder_merge([u,t],[u])) == [u,t]
      assert list(alphaorder_merge([u,t],[u,t])) == [u,t]

  # ===== subst_term

  assert subst_term(x_A,{}) == x_A
  assert subst_term(x_A,{x_A:x_A}) == x_A
  assert subst_term(x_A,{x_A:y_A}) == y_A
  assert subst_term(x_A,{y_A:x_A}) == x_A
  assert subst_term(x_A,{y_A:y_A}) == x_A
  assert subst_term(eq(x_A,x_A),{x_A:y_A}) == eq(y_A,y_A)
  assert subst_term(eq(x_A,y_A),{x_A:y_A}) == eq(y_A,y_A)
  assert subst_term(mapsto(x_A,eq(x_A,y_A)),{x_A:y_A}) == mapsto(x_A,eq(x_A,y_A))
  assert subst_term(mapsto(x_A,eq(x_A,xprime_A)),{xprime_A:x_A}) == mapsto(xprime_A,eq(xprime_A,x_A))

  # ===== subst_type

  assert subst_type(x_A,{}) == x_A
  assert subst_type(x_A,{Atype:Atype}) == x_A
  assert subst_type(x_A,{Atype:Btype}) == x_B
  assert subst_type(x_A,{Atype:type.fun(num,bool)}) == var('x',type.fun(num,bool))
  assert subst_type(x_A,{Btype:Atype}) == x_A
  assert subst_type(x_B,{Atype:Btype}) == x_B
  assert subst_type(eq(x_A,y_A),{Atype:Btype}) == eq(x_B,y_B)
  assert subst_type(eq(x_A,y_A),{Atype:type.fun(num,bool)}) == eq(var('x',type.fun(num,bool)),var('y',type.fun(num,bool)))
  assert subst_type(mapsto(x_A,eq(x_A,y_A)),{Atype:Btype}) == mapsto(x_B,eq(x_B,y_B))
  assert subst_type(mapsto(x_A,eq(x_B,y_B)),{Btype:Atype}) == mapsto(xprime_A,eq(x_A,y_A))
  assert subst_type(mapsto(x_A,mapsto(xprime_A,eq(x_B,y_B))),{Btype:Atype}) == mapsto(xprime_A,mapsto(xprime_A,eq(x_A,y_A)))
  assert subst_type(mapsto(x_A,mapsto(xprime_A,eq(eq(x_A,xprime_A),eq(x_B,y_B)))),{Btype:Atype}) == mapsto(xprime_A,mapsto(xprimeprime_A,eq(eq(xprime_A,xprimeprime_A),eq(x_A,y_A))))
  assert subst_type(mapsto(x_A,mapsto(xprime_A,eq(eq(xprime_A,xprime_A),eq(x_B,y_B)))),{Btype:Atype}) == mapsto(xprime_A,mapsto(xprime_A,eq(eq(xprime_A,xprime_A),eq(x_A,y_A))))

if __name__ == '__main__':
  test()
