#!/usr/bin/env pypy3

# goal: parse formats produced by hol-light/ProofTrace
# in particular: parse_type, parse_term, parse_thm
# but leave json.loads() to callers

from pyparsing import StringEnd,Literal,Word,ZeroOrMore,OneOrMore,Optional,Forward,alphas,nums
import term
import type

needdistinct = type.TYVAR,type.APP,term.CONST,term.VAR,term.EVAL,term.MAPSTO
assert len(set(needdistinct)) == len(needdistinct)

def group(f,x):
  def t(r): return f(*tuple(r))
  return x.copy().setParseAction(t)

lparen = Literal('(').suppress()
rparen = Literal(')').suppress()
lbracket = Literal('[').suppress()
rbracket = Literal(']').suppress()

# not: parens, brackets, double quotes, slashes
name = Word(alphas+nums+r'!#$%&*+,-./:;<=>?@\^_`{|}~'+"'")

Type = Forward()
Typevar = group(type.var,Literal('v').suppress()+lbracket+name+rbracket)
Typeapp = group(type.app,Literal('c').suppress()+lbracket+name+rbracket+lbracket+ZeroOrMore(lbracket+Type+rbracket)+rbracket)
Type <<= Typevar|Typeapp

Term = Forward()
Termvar = group(term.var,Literal('v').suppress()+lparen+name+rparen+lparen+Type+rparen)
Termconst = group(term.const,Literal('c').suppress()+lparen+name+rparen+lparen+Type+rparen)
Termcomb = group(term.eval,Literal('C').suppress()+lparen+Term+rparen+lparen+Term+rparen)
Termabs = group(term.mapsto,Literal('A').suppress()+lparen+Term+rparen+lparen+Term+rparen)
Term <<= Termvar|Termconst|Termcomb|Termabs

def parse_type(x):
  return (Type+StringEnd()).parseString(x)[0]

def parse_term(x):
  return (Term+StringEnd()).parseString(x)[0]

def parse_thm(x):
  hypotheses = tuple(parse_term(h) for h in x['hy'])
  conclusion = parse_term(x['cc'])
  return hypotheses,conclusion

def test():
  import json
  from type import app,bool,fun
  from term import const,var,eval,mapsto

  print('test ptparse')

  num = app('num')
  real = app('real')
  Int = app('int')
  Atype = type.var('A')

  # from: Printf.printf "%s\n" (type_string `:num`);;
  x = 'c[num][]'
  assert parse_type(x) == num

  # from: Printf.printf "%s\n" (type_string `:num->bool`);;
  x = 'c[fun][[c[num][]][c[bool][]]]'
  assert parse_type(x) == fun(num,bool)

  # from: Printf.printf "%s\n" (type_string `:num->int->bool`);;
  x = 'c[fun][[c[num][]][c[fun][[c[int][]][c[bool][]]]]]'
  assert parse_type(x) == fun(num,fun(Int,bool))

  # from: Printf.printf "%s\n" (type_string `:((num#1)->int)->(A->bool)`);;
  x = 'c[fun][[c[fun][[c[prod][[c[num][]][c[1][]]]][c[int][]]]][c[fun][[v[A]][c[bool][]]]]]'
  assert parse_type(x) == fun(fun(app('prod',num,app('1')),Int),fun(Atype,bool))

  # from: Printf.printf "%s\n" (type_string `:(A->bool)->A`);;
  # (this type is an example under "tyvars" in HOL Light manual)
  x = 'c[fun][[c[fun][[v[A]][c[bool][]]]][v[A]]]'
  assert parse_type(x) == fun(fun(Atype,bool),Atype)

  # from: Printf.printf "%s\n" (term_string `m:num`);;
  x = 'v(m)(c[num][])'
  assert parse_term(x) == var('m',num)

  # from: Printf.printf "%s\n" (term_string `m:A`);;
  x = 'v(m)(v[A])'
  assert parse_term(x) == var('m',Atype)

  # from: Printf.printf "%s\n" (term_string `r zpow z`);;
  x = 'C(C(c(real_zpow)(c[fun][[c[real][]][c[fun][[c[int][]][c[real][]]]]]))(v(r)(c[real][])))(v(z)(c[int][]))'
  assert parse_term(x) == eval(eval(const('real_zpow',fun(real,fun(Int,real))),var('r',real)),var('z',Int))

  # from: Printf.printf "%s\n" (term_string `x = 1 /\ y = 2 /\ !z. z >= 0`);;
  # (this term is an example under "frees" in HOL Light manual)
  x = 'C(C(c(/\\)(c[fun][[c[bool][]][c[fun][[c[bool][]][c[bool][]]]]]))(C(C(c(=)(c[fun][[c[num][]][c[fun][[c[num][]][c[bool][]]]]]))(v(x)(c[num][])))(C(c(NUMERAL)(c[fun][[c[num][]][c[num][]]]))(C(c(BIT1)(c[fun][[c[num][]][c[num][]]]))(c(_0)(c[num][]))))))(C(C(c(/\\)(c[fun][[c[bool][]][c[fun][[c[bool][]][c[bool][]]]]]))(C(C(c(=)(c[fun][[c[num][]][c[fun][[c[num][]][c[bool][]]]]]))(v(y)(c[num][])))(C(c(NUMERAL)(c[fun][[c[num][]][c[num][]]]))(C(c(BIT0)(c[fun][[c[num][]][c[num][]]]))(C(c(BIT1)(c[fun][[c[num][]][c[num][]]]))(c(_0)(c[num][])))))))(C(c(!)(c[fun][[c[fun][[c[num][]][c[bool][]]]][c[bool][]]]))(A(v(z)(c[num][]))(C(C(c(>=)(c[fun][[c[num][]][c[fun][[c[num][]][c[bool][]]]]]))(v(z)(c[num][])))(C(c(NUMERAL)(c[fun][[c[num][]][c[num][]]]))(c(_0)(c[num][])))))))'
  # for regression, not manually checked:
  assert parse_term(x) == eval(eval(const('/\\',fun(bool,fun(bool,bool))),eval(eval(const('=',fun(num,fun(num,bool))),var('x',num)),eval(const('NUMERAL',fun(num,num)),eval(const('BIT1',fun(num,num)),const('_0',num))))),eval(eval(const('/\\',fun(bool,fun(bool,bool))),eval(eval(const('=',fun(num,fun(num,bool))),var('y',num)),eval(const('NUMERAL',fun(num,num)),eval(const('BIT0',fun(num,num)),eval(const('BIT1',fun(num,num)),const('_0',num)))))),eval(const('!',fun(fun(num,bool),bool)),mapsto(var('z',num),eval(eval(const('>=',fun(num,fun(num,bool))),var('z',num)),eval(const('NUMERAL',fun(num,num)),const('_0',num)))))))

  # from: Printf.printf "%s\n" (thm_string (ASSUME `p:bool`));;
  x = '{"hy": ["v(p)(c[bool][])"], "cc": "v(p)(c[bool][])"}'
  x = json.loads(x)
  assert parse_thm(x) == ((var('p',bool),),var('p',bool))

  # from: Printf.printf "%s\n" (thm_string (UNDISCH_ALL(REWRITE_RULE[IMP_CONJ](ISPECL[`y:num`;`x:num`;`z:num`]EQ_TRANS))));;
  # i.e. x = z, y = x |-> y = z
  x = '{"hy": ["C(C(c(=)(c[fun][[c[num][]][c[fun][[c[num][]][c[bool][]]]]]))(v(x)(c[num][])))(v(z)(c[num][]))", "C(C(c(=)(c[fun][[c[num][]][c[fun][[c[num][]][c[bool][]]]]]))(v(y)(c[num][])))(v(x)(c[num][]))"], "cc": "C(C(c(=)(c[fun][[c[num][]][c[fun][[c[num][]][c[bool][]]]]]))(v(y)(c[num][])))(v(z)(c[num][]))"}'
  x = json.loads(x)
  x_num = var('x',num)
  y_num = var('y',num)
  z_num = var('z',num)
  assert parse_thm(x) == ((term.eq(x_num,z_num),term.eq(y_num,x_num)),term.eq(y_num,z_num))

  # from: Printf.printf "%s\n" (thm_string EQ_REFL);;                           
  x = '{"hy": [], "cc": "C(c(!)(c[fun][[c[fun][[v[A]][c[bool][]]]][c[bool][]]]))(A(v(x)(v[A]))(C(C(c(=)(c[fun][[v[A]][c[fun][[v[A]][c[bool][]]]]]))(v(x)(v[A])))(v(x)(v[A]))))"}'
  x = json.loads(x)
  x_A = var('x',Atype)
  assert parse_thm(x) == ((),eval(const('!',fun(fun(Atype,bool),bool)),mapsto(x_A,term.eq(x_A,x_A))))

  # from: Printf.printf "%s\n" (thm_string REAL_ZPOW_2);;
  x = '{"hy": [], "cc": "C(c(!)(c[fun][[c[fun][[c[real][]][c[bool][]]]][c[bool][]]]))(A(v(x)(c[real][]))(C(C(c(=)(c[fun][[c[real][]][c[fun][[c[real][]][c[bool][]]]]]))(C(C(c(real_zpow)(c[fun][[c[real][]][c[fun][[c[int][]][c[real][]]]]]))(v(x)(c[real][])))(C(c(int_of_num)(c[fun][[c[num][]][c[int][]]]))(C(c(NUMERAL)(c[fun][[c[num][]][c[num][]]]))(C(c(BIT0)(c[fun][[c[num][]][c[num][]]]))(C(c(BIT1)(c[fun][[c[num][]][c[num][]]]))(c(_0)(c[num][]))))))))(C(C(c(real_mul)(c[fun][[c[real][]][c[fun][[c[real][]][c[real][]]]]]))(v(x)(c[real][])))(v(x)(c[real][])))))"}'
  x = json.loads(x)
  # for regression, not manually checked:
  assert parse_thm(x) == ((),eval(const('!',fun(fun(real,bool),bool)),mapsto(var('x',real),eval(eval(const('=',fun(real,fun(real,bool))),eval(eval(const('real_zpow',fun(real,fun(Int,real))),var('x',real)),eval(const('int_of_num',fun(num,Int)),eval(const('NUMERAL',fun(num,num)),eval(const('BIT0',fun(num,num)),eval(const('BIT1',fun(num,num)),const('_0',num))))))),eval(eval(const('real_mul',fun(real,fun(real,real))),var('x',real)),var('x',real))))))

if __name__ == '__main__':
  test()
