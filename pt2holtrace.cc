// XXX: do one-pass parsers

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <ext/stdio_filebuf.h>

using namespace std;

static ofstream output;

typedef long long num;

#define TDSPLIT "]"

static void skiptext(string &line,const char *text)
{
  size_t textlen = strlen(text);
  if (line.substr(0,textlen) != text) throw invalid_argument("expected string '"+string(text)+"' not found\n");
  line = line.substr(textlen);
}

static void parsell(string &line,num &result)
{
  size_t pos;
  result = stoll(line,&pos);
  line = line.substr(pos);
}

static void untilquote(string &line,string &result)
{
  int backslash = 0;
  int foundquote = 0;
  string rest;
  for (auto c : line) {
    if (foundquote) { rest += c; continue; }
    if (backslash) { result += c; backslash = 0; continue; }
    if (c == '\\') { backslash = 1; continue; }
    if (c == '"') { foundquote = 1; continue; }
    result += c;
  }
  line = rest;
}

static void parselist(string &line,vector<num> &nums,vector<string> &strs)
{
  int state = 0;
  int bracketlevels = 0;
  num num;
  string str;
  string rest;
  for (auto c: line)
    switch(state) {
      case -1:
        rest += c;
        break;
      case 0:
        if (c != '[') throw invalid_argument("expected left bracket not found");
        ++bracketlevels; state = 1;
        break;
      case 1:
        if (c == ']') {
          --bracketlevels; if (bracketlevels == 0) state = -1;
          break;
        }
        if (c == '[') {
          ++bracketlevels;
          break;
        }
        if (c == ',') break;
        if (c == ' ') break;
        if (c >= '0' && c <= '9') { state = 2; num = c-'0'; break; }
        if (c == '"') { state = 3; str = ""; break; }
        throw invalid_argument("expected quote or right bracket not found");
      case 2:
        if (c == ']') {
          nums.push_back(num);
          --bracketlevels; if (bracketlevels == 0) state = -1;
          break;
        }
        if (c == ',') { nums.push_back(num); state = 1; break; }
        if (c >= '0' && c <= '9') { num = num*10+(c-'0'); break; }
        throw invalid_argument("expected digit or comma or right bracket not found");
      case 3:
        if (c == '\\') { state = 4; break; }
        if (c == '"') { strs.push_back(str); state = 1; break; }
        str += c;
        break;
      case 4:
        str += c;
        state = 3;
    }
  line = rest;
}

// C++ reserves typeid
#define typeid mytypeid

static num typeid;
static num termid;
static num thmid;
static num infid;
static num typedefid;
static num ptid;

static unordered_map<string,num> tyvar2id;
static unordered_map<pair<string,vector<num>>,num,boost::hash<pair<string,vector<num>>>> app2id;

// fun(y,z) type mapping to y and to z:
static unordered_map<num,num> typeid2fromid;
static unordered_map<num,num> typeid2toid;

static unordered_map<pair<string,num>,num,boost::hash<pair<string,num>>> var2id;
static unordered_map<pair<string,num>,num,boost::hash<pair<string,num>>> const2id;
static unordered_map<pair<num,num>,num,boost::hash<pair<num,num>>> comb2id;
static unordered_map<pair<num,num>,num,boost::hash<pair<num,num>>> abs2id;

static unordered_map<num,num> termid2typeid;

static unordered_map<vector<num>,num,boost::hash<vector<num>>> hhhc2id;
static unordered_map<num,num> ptid2thmid;
static unordered_map<pair<vector<num>,string>,num,boost::hash<pair<vector<num>,string>>> infkey2id;

// for the type_definition conversions:
static unordered_map<num,num> combtermid2funid;
static unordered_map<num,num> combthmid2funid;

static unordered_map<tuple<num,num,num,string>,num,boost::hash<tuple<num,num,num,string>>> typedef2id;

static string simpleencode(num x)
{
  if (x == 0) return "0";
  if (x == -1) return "!";
  if (x == -2) return "@";
  if (x == -3) return "#";
  if (x == -4) return "$";
  if (x == -5) return "%";
  if (x == -6) return "^";
  if (x == -7) return "&";
  if (x == -8) return "*";
  if (x == -9) return "(";
  if (x == -10) return ")";
  string result;
  int minussign = 0;
  // XXX: assumes x > -2^63
  if (x < 0) { minussign = 1; x = -x; }
  while (x > 0) {
    result += "0123456789ABCDEFGHIJKLMNOPQRSTUV"[x&31];
    x >>= 5;
  }
  if (minussign) result += "-";
  reverse(result.begin(),result.end());
  return result;
}

static num simpleencode_len(num x)
{
  if (x == 0) return 1;
  num result = 0;
  if (x < 0) { ++result; x = -x; }
  while (x > 0) {
    result += 1;
    x >>= 5;
  }
  return result;
}

static num compress(num x,num reference)
{
  if (x < 0) throw invalid_argument("internal bug, negative integer");
  num y = x-reference;
  if (y >= 0) return x;
  if (y >= -10) return y;
  if (simpleencode_len(y) <= simpleencode_len(x)) return y;
  return x;
}

static int integral(char c)
{
  if (c >= '0' && c <= '9') return 1;
  if (c >= 'A' && c <= 'V') return 1;
  return 0;
}

static void log(char what,const vector<num> &ids,const string &name)
{
  output << what;
  char lastbyte = 0;
  for (num id : ids) {
    string s = simpleencode(id);
    auto firstbyte = s.at(0);
    if (integral(lastbyte))
      if (integral(firstbyte))
        output << ' ';
    output << s;
    lastbyte = s.at(s.length()-1);
  }
  if (name.length() > 0) {
    auto firstbyte = name.at(0);
    if (integral(lastbyte))
      if (integral(firstbyte) || firstbyte == ' ')
        output << ' ';
    output << name;
  }
  output << '\n';
}

static vector<num> string2closings(const string &t)
{
  vector<num> result(t.length(),0);
  vector<num> open;
  vector<char> paren;

  num pos = 0;
  for (char c : t) {
    if (c == '(' || c == '[') {
      open.push_back(pos);
      paren.push_back(c);
    } else if (c == ')' || c == ']') {
      if (open.empty()) throw invalid_argument("too many right parens/brackets");
      if (c == ')' && paren.back() != '(') throw invalid_argument("paren-bracket mismatch");
      if (c == ']' && paren.back() != '[') throw invalid_argument("paren-bracket mismatch");
      result.at(open.back()) = pos+1; // seems more useful than pos
      open.pop_back();
      paren.pop_back();
    }
    ++pos;
  }
  if (!open.empty()) throw invalid_argument("not enough right parens/brackets");

  return result;
}

static num structured_type2id(const string &name,vector<num> &subids)
{
  auto key = make_pair(name,subids);
  if (app2id.count(key) == 0) {
    if (name == "fun") {
      if (subids.size() != 2) throw invalid_argument("function type must have 2 arguments");
      typeid2fromid[typeid] = subids.at(0);
      typeid2toid[typeid] = subids.at(1);
    }
    vector<num> compressed;
    compressed.push_back(subids.size());
    for (num id : subids)
      compressed.push_back(compress(id,typeid));
    log('b',compressed,name);
    app2id[key] = typeid++;
  }
  return app2id[key];
}

static num type2id(const string &t,const vector<num> &closings,num start,num stop)
{
  if (stop <= start) throw invalid_argument("stop <= start");
  char what = t.at(start);
  if (t.at(start+1) != '[') throw invalid_argument("type missing bracket");

  vector<num> subids;

  if (what == 'v') { // v[name]
    if (closings.at(start+1) != stop) throw invalid_argument("wrong number of v type args");
    string name = t.substr(start+2,stop-start-3);
    if (tyvar2id.count(name) == 0) {
      log('a',subids,name);
      tyvar2id[name] = typeid++;
    }
    return tyvar2id[name];
  }

  if (what == 'c') { // c[name][[v[A]][v[B]]]
    num pos = closings.at(start+1);
    string name = t.substr(start+2,pos-start-3);
    if (t.at(pos) != '[') throw invalid_argument("type missing second bracket");
    if (closings.at(pos) != stop) throw invalid_argument("wrong number of c type args");
    ++pos;
    while (pos < stop-1) {
      if (t.at(pos) != '[') throw invalid_argument("type missing inner bracket");
      num nextpos = closings.at(pos);
      subids.push_back(type2id(t,closings,pos+1,nextpos-1));
      if (nextpos <= pos) throw invalid_argument("internal error, nextpos <= pos");
      pos = nextpos;
    }
    return structured_type2id(name,subids);
  }

  throw invalid_argument("unrecognized type symbol");
}

static num type2id(const string &type)
{
  auto closings = string2closings(type);
  return type2id(type,closings,0,type.length());
}

static num abstype2id(num id0,num id1)
{
  vector<num> subids;
  subids.push_back(id0);
  subids.push_back(id1);
  return structured_type2id("fun",subids);
}

static num combtype2id(num id0,num id1)
{
  if (!typeid2fromid.count(id0)) throw invalid_argument("comb from non-function type");
  if (!typeid2toid.count(id0)) throw invalid_argument("comb from non-function type");
  if (typeid2fromid[id0] != id1) throw invalid_argument("comb applied to wrong type");
  return typeid2toid[id0];
}

static num term2id(const string &t,const vector<num> &closings,num start,num stop)
{
  if (stop <= start) throw invalid_argument("stop <= start");
  char what = t.at(start);
  if (t.at(start+1) != '(') throw invalid_argument("term missing paren");
  num middle = closings.at(start+1);
  if (closings.at(middle) != stop) throw invalid_argument("wrong number of term args");

  vector<num> compressed;

  if (what == 'c') {
    auto arg0 = t.substr(start+2,middle-start-3);
    num id1 = type2id(t,closings,middle+1,stop-1);
    auto key = make_pair(arg0,id1);
    if (const2id.count(key) == 0) {
      termid2typeid[termid] = id1;
      compressed.push_back(compress(id1,typeid));
      log('c',compressed,arg0);
      const2id[key] = termid++;
    }
    return const2id[key];
  }
  if (what == 'v') {
    auto arg0 = t.substr(start+2,middle-start-3);
    num id1 = type2id(t,closings,middle+1,stop-1);
    auto key = make_pair(arg0,id1);
    if (var2id.count(key) == 0) {
      termid2typeid[termid] = id1;
      compressed.push_back(compress(id1,typeid));
      log('d',compressed,arg0);
      var2id[key] = termid++;
    }
    return var2id[key];
  }

  num id0 = term2id(t,closings,start+2,middle-1);
  num id1 = term2id(t,closings,middle+1,stop-1);
  auto key = make_pair(id0,id1);

  if (what == 'C') {
    if (comb2id.count(key) == 0) {
      combtermid2funid[termid] = id0;
      auto yid = combtype2id(termid2typeid[id0],termid2typeid[id1]);
      termid2typeid[termid] = yid;
      compressed.push_back(compress(yid,typeid));
      compressed.push_back(compress(id0,termid));
      compressed.push_back(compress(id1,termid));
      log('e',compressed,"");
      comb2id[key] = termid++;
    }
    return comb2id[key];
  }
  if (what == 'A') {
    if (abs2id.count(key) == 0) {
      auto yid = abstype2id(termid2typeid[id0],termid2typeid[id1]);
      termid2typeid[termid] = yid;
      compressed.push_back(compress(yid,typeid));
      compressed.push_back(compress(id0,termid));
      compressed.push_back(compress(id1,termid));
      log('f',compressed,"");
      abs2id[key] = termid++;
    }
    return abs2id[key];
  }

  throw invalid_argument("unrecognized term symbol");
}

static num term2id(const string &term)
{
  auto closings = string2closings(term);
  return term2id(term,closings,0,term.length());
}

static num thm2id(const vector<string> &hypotheses,const string &conclusion)
{
  vector<num> hhhc;
  for (string h : hypotheses)
    hhhc.push_back(term2id(h));
  auto conclusionid = term2id(conclusion);
  hhhc.push_back(conclusionid);

  if (hhhc2id.count(hhhc) == 0) {
    if (hypotheses.size() == 0 && combtermid2funid.count(conclusionid))
      combthmid2funid[thmid] = combtermid2funid[conclusionid];
    vector<num> compressed;
    for (num t : hhhc)
      compressed.push_back(compress(t,termid));
    log('t',compressed,"");
    hhhc2id[hhhc] = thmid++;
  }
  return hhhc2id[hhhc];
}

static num inf2id(vector<num> &subids,char what,vector<num> &compressed,const string &name)
{
  auto key = make_pair(subids,name);
  if (infkey2id.count(key) == 0) {
    log(what,compressed,name);
    infkey2id[key] = infid++;
  }
  return infkey2id[key];
}

static void preprocess_definition(string &term)
{
  string result;
  int parenlevel = 0;
  int leftparens0 = 0;
  int leftparens1 = 0;

  for (char c : term) {
    if (c == '(') {
      if (parenlevel == 0) ++leftparens0;
      if (parenlevel == 1) ++leftparens1;
      ++parenlevel;
      result += c;
      continue;
    }
    if (c == ')') {
      --parenlevel;
      result += c;
      continue;
    }
    if (leftparens0 == 1 && leftparens1 == 2 && parenlevel == 2 && c == 'c') {
      result += 'v';
    } else
      result += c;
  }
  term = result;
}

static int match(vector<string> &result,const string &x,const string &pattern)
{
  num xlen = x.length();
  num xpos = 0;
  num numtargets = result.size();
  vector<int> initialized(numtargets,0);
  for (char c : pattern) {
    if (xpos >= xlen) return 0;
    if (c >= '0' && c <= '9') {
      // XXX: maybe generalize to allow more than 10 entries
      int entry = c-'0';
      if (entry >= numtargets) return 0;
      string target;
      num parenlevel = 0;
      for (;;) {
        if (xpos >= xlen) return 0;
        char xi = x.at(xpos);
        if (xi == '(' || xi == '[')
          ++parenlevel;
        if (xi == ')' || xi == ']') {
          if (parenlevel == 0) break;
          --parenlevel;
        }
        target += xi;
        ++xpos;
      }
      if (initialized.at(entry)) {
        if (result.at(entry) != target) return 0;
      } else {
        initialized.at(entry) = 1;
        result.at(entry) = target;
      }
    } else {
      if (x.at(xpos) != c) return 0;
      ++xpos;
    }
  }
  if (xpos != xlen) return 0;
  return 1;
}

// [v[A]][v[B]] -> A]B]
static string tyvars_pack(string tyvars)
{
  string result;
  while (tyvars.length() > 0) {
    if (tyvars.substr(0,3) != "[v[") throw invalid_argument("tyvars missing open");
    tyvars = tyvars.substr(3);
    size_t pos = tyvars.find("]]");
    if (pos == tyvars.npos) throw invalid_argument("tyvars missing close");
    result += tyvars.substr(0,pos);
    result += TDSPLIT;
    tyvars = tyvars.substr(pos+2);
  }
  return result;
}

static const string Adef("C(C(c(=)(c[fun][[c[0][1]][c[fun][[c[0][1]][c[bool][]]]]]))(C(c(2)(c[fun][[4][c[0][1]]]))(C(c(3)(c[fun][[c[0][1]][4]]))(v(a)(c[0][1])))))(v(a)(c[0][1]))");
static const string Rdef("C(C(c(=)(c[fun][[c[bool][]][c[fun][[c[bool][]][c[bool][]]]]]))(C(5)(v(r)(4))))(C(C(c(=)(c[fun][[4][c[fun][[4][c[bool][]]]]]))(C(c(3)(c[fun][[c[0][1]][4]]))(C(c(2)(c[fun][[4][c[0][1]]]))(v(r)(4)))))(v(r)(4)))");

// ProofTrace does two TYPE_DEFINITION lines
// distinguished by the term absthm (the output theorem):
// first abs(rep(a)) = a, with newname abs
// then repprop(r) = (rep(abs(r)) = r), with newname rep

// HOLTrace format is instead:
// A reptype repprop repth ]abstypename]abs]rep]tyvar1]tyvar2]...
// D

static void preprocess_type_definition(string &name,char &newtag,num &id0,num &id1,num repthmid,const string &absthm,const string &newname)
{
  newtag = 'A';

  if (!combthmid2funid.count(repthmid)) throw invalid_argument("repthm is not comb");
  id1 = combthmid2funid[repthmid];

  vector<string> result(5,"");
  if (!match(result,absthm,Adef)) {
    result.push_back(""); // for repprop
    if (!match(result,absthm,Rdef)) throw invalid_argument("type_definition patterns do not match");
    newtag = 'D';
  }

  auto abstypename = result.at(0);
  auto tyvars = result.at(1);
  auto absname = result.at(2);
  auto repname = result.at(3);
  auto reptype = result.at(4);
  if (abstypename.find(TDSPLIT) != abstypename.npos) throw invalid_argument("impossible tdsplit in abstypename");
  if (absname.find(TDSPLIT) != absname.npos) throw invalid_argument("impossible tdsplit in absname");
  if (repname.find(TDSPLIT) != repname.npos) throw invalid_argument("impossible tdsplit in repname");

  id0 = type2id(reptype);

  if (newtag == 'A') {
    if (newname != absname) throw invalid_argument("newname does not match absname");
  } else {
    if (id1 != term2id(result.at(5))) throw invalid_argument("repprop mismatch");
    if (newname != repname) throw invalid_argument("newname does not match repname");
  }
  name = TDSPLIT;
  name += abstypename; name += TDSPLIT;
  name += absname; name += TDSPLIT;
  name += repname; name += TDSPLIT;
  name += tyvars_pack(tyvars);
}

// not super-stringent
// e.g. flattens lists with any nesting structure
// e.g. doesn't care about relative order of numbers and strings
static void parse(ifstream &names,ifstream &proofs,ifstream &theorems)
{
  unordered_map<num,string> id2name;
  unordered_map<string,char> rule2tag{
    {"TYPE_DEFINITION",'A'},
    {"BETA",'B'},
    {"DEFINITION",'C'},
    {"MK_COMB",'E'},
    {"ABS",'F'},
    {"EQ_MP",'P'},
    {"REFL",'R'},
    {"INST",'S'},
    {"INST_TYPE",'T'},
    {"ASSUME",'U'},
    {"TRANS",'V'},
    {"AXIOM",'X'},
    {"DEDUCT_ANTISYM_RULE",'Z'},
  };
  unordered_map<char,ssize_t> tag2numcount{
    {'A',1},
    {'B',0},
    {'C',0},
    {'E',2},
    {'F',1},
    {'P',2},
    {'R',0},
    {'S',1},
    {'T',1},
    {'U',0},
    {'V',2},
    {'X',0},
    {'Z',2},
  };
  unordered_map<char,ssize_t> tag2strcount{
    {'A',2},
    {'B',1},
    {'C',2},
    {'E',0},
    {'F',1},
    {'P',0},
    {'R',1},
    {'S',-1},
    {'T',-1},
    {'U',1},
    {'V',0},
    {'X',1},
    {'Z',0},
  };

  string line;

  while (getline(names,line)) {
    num id;
    string name;

    skiptext(line,"{\"id\": ");
    parsell(line,id);
    skiptext(line,", \"nm\": \"");
    untilquote(line,name);
    skiptext(line,"}");
    if (line.length() != 0) throw invalid_argument("extra text after names line");

    if (id2name.count(id) > 0) throw invalid_argument("multiple names for same id");
    id2name[id] = name;
  }

  while (getline(proofs,line)) {
    num idpr;
    vector<num> prnums;
    vector<string> prstrs;

    skiptext(line,"{\"id\": ");
    parsell(line,idpr);
    skiptext(line,", \"pr\": ");
    parselist(line,prnums,prstrs);
    skiptext(line,"}");
    if (line.length() != 0) throw invalid_argument("extra text after proofs line");

    if (idpr < ptid || idpr > ptid+300) throw invalid_argument("surprising ID");
    while (idpr > ptid) {
      output << ";\n";
      ++ptid;
    }

    if (!getline(theorems,line)) throw invalid_argument("fewer theorems than proofs");
    num idthm;
    vector<num> thmnums;
    vector<string> hypotheses;
    string conclusion;

    skiptext(line,"{\"id\": ");
    parsell(line,idthm);
    if (idthm != ptid) throw invalid_argument("IDs desynchronized between theorems and proofs");
    skiptext(line,", \"th\": {\"hy\": ");
    parselist(line,thmnums,hypotheses);
    skiptext(line,", \"cc\": \"");
    untilquote(line,conclusion);
    skiptext(line,"}}");
    if (line.length() != 0) throw invalid_argument("extra text after theorems line");
    if (thmnums.size() != 0) throw invalid_argument("numbers inside conclusions");

    if (prstrs.size() == 0) throw invalid_argument("empty proof");
    auto rule = prstrs.at(0);
    if (rule2tag.count(rule) == 0) throw invalid_argument("unrecognized inference rule "+rule);
    auto tag = rule2tag[rule];
    auto expectednumcount = tag2numcount[tag];
    if (expectednumcount != (ssize_t) prnums.size())
      throw invalid_argument("unexpected number of integers for rule "+rule);
    auto expectedstrcount = tag2strcount[tag];
    if (expectedstrcount >= 0)
      if (expectedstrcount+1 != (ssize_t) prstrs.size())
        throw invalid_argument("unexpected number of strings for rule "+rule);

    vector<num> subids;
    vector<num> compressed;
    string name;
    subids.push_back(tag);

    num ptid0 = -1;
    num ptid1 = -1;

    if (tag == 'B' || tag == 'R' || tag == 'U' || tag == 'X') {
      string t0 = prstrs.at(1);
      num id0 = term2id(t0);
      subids.push_back(id0);
      compressed.push_back(compress(id0,termid));
    } else if (tag == 'F') {
      string t0 = prstrs.at(1);
      ptid0 = prnums.at(0);
      num id0 = term2id(t0);
      num id1 = ptid2thmid[ptid0];
      subids.push_back(id0);
      subids.push_back(id1);
      compressed.push_back(compress(id0,termid));
      compressed.push_back(compress(id1,thmid));
    } else if (tag == 'E' || tag == 'P' || tag == 'V' || tag == 'Z') {
      ptid0 = prnums.at(0);
      ptid1 = prnums.at(1);
      num id0 = ptid2thmid[ptid0];
      num id1 = ptid2thmid[ptid1];
      subids.push_back(id0);
      subids.push_back(id1);
      compressed.push_back(compress(id0,thmid));
      compressed.push_back(compress(id1,thmid));
    } else if (tag == 'S' || tag == 'T') {
      ptid0 = prnums.at(0);
      num id0 = ptid2thmid[ptid0];
      subids.push_back(id0);
      compressed.push_back(compress(id0,thmid));
      int first = 1;
      for (auto s1 : prstrs) {
        if (first) { first = 0; continue; }
        tag == 'S' ? term2id(s1) : type2id(s1);
      }
      num reference = tag == 'S' ? termid : typeid;
      first = 1;
      for (auto s1 : prstrs) {
        if (first) { first = 0; continue; }
        num id1 = tag == 'S' ? term2id(s1) : type2id(s1);
        subids.push_back(id1);
        compressed.push_back(compress(id1,reference));
      }
    } else if (tag == 'C') {
      name = prstrs.at(2);
      string t0 = prstrs.at(1);
      preprocess_definition(t0);
      num id0 = term2id(t0);
      subids.push_back(id0);
      compressed.push_back(compress(id0,termid));
    } else if (tag == 'A') {
      ptid0 = prnums.at(0);
      num id2 = ptid2thmid[ptid0];
      string t0 = prstrs.at(1);
      string newname = prstrs.at(2);
      num id0;
      num id1;
      char newtag;
      preprocess_type_definition(name,newtag,id0,id1,id2,t0,newname);
      auto key = make_tuple(id0,id1,id2,name);
      if (newtag == 'D') {
        if (!typedef2id.count(key)) throw invalid_argument("non-canonical TYPE_DEFINITION order");
        num Aid = typedef2id[key];
        tag = 'D';
        if (Aid != typedefid-1) throw invalid_argument("non-adjacent TYPE_DEFINITION");
        subids.push_back(Aid);
        compressed.push_back(compress(Aid,typedefid));
        name = "";
      } else {
        if (typedef2id.count(key)) throw invalid_argument("non-canonical TYPE_DEFINITION order");
        typedef2id[key] = typedefid++;
        subids.push_back(id0);
        compressed.push_back(compress(id0,typeid));
        subids.push_back(id1);
        compressed.push_back(compress(id1,termid));
        subids.push_back(id2);
        compressed.push_back(compress(id2,thmid));
      }
    } else
      throw invalid_argument("surprising tag");

    auto iid = inf2id(subids,tag,compressed,name);
    num id = thm2id(hypotheses,conclusion);

    if (id2name.count(ptid) > 0) {
      vector<num> nameids;
      nameids.push_back(compress(id,thmid));
      log('/',nameids,id2name[ptid]);
    }

    vector<num> ptsubids;
    ptsubids.push_back(compress(id,thmid));
    ptsubids.push_back(compress(iid,infid));
    if (ptid0 >= 0) ptsubids.push_back(compress(ptid0,ptid));
    if (ptid1 >= 0) ptsubids.push_back(compress(ptid1,ptid));

    log(',',ptsubids,"");
    ptid2thmid[ptid++] = id;
  }

  if (getline(theorems,line)) throw invalid_argument("fewer proofs than theorems");
}

int main(int argc,char **argv)
{
  if (argc < 4) {
    cerr << "usage: pt2holtrace pt.names pt.proofs pt.theorems\n";
    exit(100);
  }
  ifstream names(argv[1]);
  ifstream proofs(argv[2]);
  ifstream theorems(argv[3]);

  __gnu_cxx::stdio_filebuf<char> fdbuf(1,ios::out);
  output.ios::rdbuf(&fdbuf);

  output << "HOLTrace 1\n";

  try {
    parse(names,proofs,theorems);
  } catch(exception &e) {
    cerr << e.what() << "\n";
    exit(111);
  }
  return 0;
}
