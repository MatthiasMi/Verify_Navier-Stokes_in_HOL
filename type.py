#!/usr/bin/env pypy3

# base types in hol light:
BOOL = 'bool'
FUN = 'fun'
IND = 'ind'

# this package's abbreviations:
TYVAR = 'a'
APP = 'b'
# need TYVAR<APP to match ocaml's comparisons for alphaorder
# (at least what seem to be ocaml's comparisons; is this documented?)

def var(name): return TYVAR,name
def is_var(y): return y[0] == TYVAR
def var_name(y):
  assert is_var(y)
  return y[1]

def app(name,*args): return (APP,name)+args
def is_app(y): return y[0] == APP
def app_name(y):
  assert is_app(y)
  return y[1]
def app_args(y):
  assert is_app(y)
  return y[2:]

bool = app(BOOL)
num = app('num')

def fun(y,z): return app(FUN,y,z)
def is_fun(y):
  return is_app(y) and app_name(y) == FUN
def fun_args(y):
  assert app_name(y) == FUN
  return app_args(y)

# yields type variables in type y
# leaving up to caller to eliminate repetitions
def vars(y):
  if is_var(y):
    yield y
  else:
    for z in app_args(y):
      yield from vars(z)

# should match type_subst in hol-light/fusion.ml
def subst(y,y2z):
  if is_var(y): return y2z.get(y,y)
  name = app_name(y)
  args = app_args(y)
  return app(name,*(subst(x,y2z) for x in args))

def test():
  print('test type')

  Atype = var('A')
  assert Atype == (TYVAR,'A')
  assert is_var(Atype)
  assert var_name(Atype) == 'A'
  assert not is_app(Atype)

  Btype = var('B')
  assert Btype == (TYVAR,'B')
  assert is_var(Btype)
  assert var_name(Btype) == 'B'
  assert not is_app(Btype)

  ok = False
  try: app_name(Atype)
  except: ok = True
  assert ok

  ok = False
  try: app_args(Atype)
  except: ok = True
  assert ok

  assert bool == app(BOOL)
  assert bool == (APP,BOOL)
  assert is_app(bool)
  assert not is_fun(bool)
  assert app_name(bool) == BOOL
  assert app_args(bool) == ()
  assert not is_var(bool)

  assert app(FUN,Atype,bool) == (APP,FUN,Atype,bool)
  assert fun(Atype,bool) == (APP,FUN,Atype,bool)
  assert is_app(fun(Atype,bool))
  assert is_fun(fun(Atype,bool))
  assert app_name(fun(Atype,bool)) == FUN
  assert app_args(fun(Atype,bool)) == (Atype,bool)
  assert not is_var(fun(Atype,bool))
  assert fun_args(fun(Atype,bool)) == (Atype,bool)

  ok = False
  try: var_name(bool)
  except: ok = True
  assert ok

  ok = False
  try: fun_split(bool)
  except: ok = True
  assert ok

  assert list(vars(bool)) == []
  assert list(vars(fun(bool,bool))) == []
  assert set(vars(fun(fun(Atype,bool),Atype))) == set([Atype])
  assert set(vars(fun(fun(Atype,Btype),Atype))) == set([Atype,Btype])

if __name__ == '__main__':
  test()
