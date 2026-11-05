Printf.printf("===== loading holtrace_save and holtrace_restore\n");;

(*
XXX: should reject partial final line on restore

XXX: could automatically rename constants etc. to avoid collisions
but this would be two-pass, so easier as a separate tool

XXX: could inject new theorems into Toploop bindings

XXX: could do more extensive dedup here to save memory:
rebuild types, rebuild terms, re-prove theorems

XXX: but really want external hashcons to speed up hashing
or at least caching hashes on object creation, even without dedup

XXX: maybe should randomize hashing

XXX: might encounter integer overflows on 32-bit machines

XXX: inf_def now allows alpha-equivalent variants of existing def;
maybe also allow alpha-equivalence for typedef?
*)

(* ===== save *)

module Holtrace_save: sig

  val REFL: term -> thm
  val TRANS: thm -> thm -> thm
  val MK_COMB: thm * thm -> thm
  val ABS: term -> thm -> thm
  val BETA: term -> thm
  val ASSUME: term -> thm
  val EQ_MP: thm -> thm -> thm
  val DEDUCT_ANTISYM_RULE: thm -> thm -> thm
  val INST_TYPE: (hol_type * hol_type) list -> thm -> thm
  val INST: (term * term) list -> thm -> thm

  val new_axiom: term -> thm
  val new_basic_definition: term -> thm
  val new_basic_type_definition: string -> string * string -> thm -> thm * thm

  val holtrace_nameonethm: string * thm -> unit
  val holtrace_namethm: (string * thm) list -> unit

  val holtrace_stop: unit -> unit

  val holtrace_typedef_thms: unit -> thm list

end = struct

  let orig_REFL = REFL
  let orig_TRANS = TRANS
  let orig_MK_COMB = MK_COMB
  let orig_ABS = ABS
  let orig_BETA = BETA
  let orig_ASSUME = ASSUME
  let orig_EQ_MP = EQ_MP
  let orig_DEDUCT_ANTISYM_RULE = DEDUCT_ANTISYM_RULE
  let orig_INST_TYPE = INST_TYPE
  let orig_INST = INST

  let orig_new_axiom = new_axiom
  let orig_new_basic_definition = new_basic_definition
  let orig_new_basic_type_definition = new_basic_type_definition

  let envvar = Sys.getenv_opt("HOLTRACE_SAVE")

  let enabled = Option.is_some envvar

  let fd =
    if enabled then
      open_out_bin (Option.get envvar)
    else
      (* presumably there is an easier way to do this *)
      let fd = open_out_bin "/dev/null" in
      Out_channel.close fd;
      fd

  let tracing = ref enabled

  let trace_flush() = if !tracing then Out_channel.flush fd

  let trace_char c = if !tracing then output_char fd c

  let trace_string s = if !tracing then output_string fd s

  let trace_stop() = if !tracing then begin Out_channel.close fd; tracing := false end

  (* ===== holtrace encoding *)

  let rec base32 n =
    if n < 0 then "-" ^ (base32(-n))
    else if n == 0 then ""
    else
      let d = n mod 32 in
      let c = Char.chr(d + (if d < 10 then 48 else 55)) in
      base32(n/32) ^ String.make 1 c

  let is_base32char c =
    (c >= '0' && c <= '9') || (c >= 'A' && c <= 'V')

  let simpleencode n =
    match n with
    | -1 -> "!"
    | -2 -> "@"
    | -3 -> "#"
    | -4 -> "$"
    | -5 -> "%"
    | -6 -> "^"
    | -7 -> "&"
    | -8 -> "*"
    | -9 -> "("
    | -10 -> ")"
    | 0 -> "0"
    | _ -> base32 n

  let rec base32_len n =
    if n < 0 then 1 + base32_len(-n)
    else if n == 0 then 0
    else 1 + base32_len(n/32)

  let simpleencode_len n =
    if n >= -10 && n <= 9 then 1
    else base32_len n

  let is_printablenospaces s =
    String.for_all (fun c -> c > ' ' && c <= '~') s

  let need_version = ref true

  let trace tag idlist s =
    if not(is_printablenospaces s) then
      failwith "bad name"
    else
      if !need_version then begin
        trace_string "HOLTrace 1\n";
        need_version := false
      end;
      trace_char tag;
      let idlist = map simpleencode idlist in
      let args =
        if String.length s = 0 then idlist
        else idlist@[s]
      in
      let lastbyte = ref ' ' in
      let doit s = (
        (* s is nonempty now *)
        let firstbyte = s.[0] in
        if is_base32char !lastbyte && (is_base32char firstbyte || (firstbyte == ' '))
        then trace_char ' ';
        trace_string s;
        lastbyte := s.[String.length s - 1]
      ) in
      List.iter doit args;
      trace_char '\n';
      trace_flush()

  let compress reference id =
    if id < 0 then failwith "internal bug, negative integer" else
    let y = id-reference in
    if y >= 0 then id else
    if y >= -10 then y else
    if -y >= id then id else
    if -32*y <= id then y else
    if simpleencode_len y <= simpleencode_len id then y else id

  (* ===== types *)

  let some_type = bool_ty
  let id2type = Dynarray.make 0 some_type
  let type2id = Hashtbl.create 16384

  let typeid2compress id =
    compress (Dynarray.length id2type) id

  let type2compress ty =
    typeid2compress (Hashtbl.find type2id ty)

  let rec dedup_trace_type_core ty =
    try
      let id = Hashtbl.find type2id ty in
      Dynarray.get id2type id,id
    with Not_found -> begin
      begin
        match ty with
        | Tyvar name ->
          assert(String.length name > 0);
          trace 'a' [] name
        | Tyapp(name,args) ->
          let args = map (snd o dedup_trace_type_core) args in
          assert(String.length name > 0);
          trace 'b' ((length args)::(map typeid2compress args)) name
      end;
      let id = Dynarray.length id2type in
      Dynarray.add_last id2type ty;
      Hashtbl.add type2id ty id;
      ty,id
    end

  let dedup_trace_type_calls = ref 0
  let dedup_trace_type_time = ref 0.0

  let dedup_trace_type tm =
    dedup_trace_type_time := !dedup_trace_type_time -. Sys.time();
    let result = dedup_trace_type_core tm in
    dedup_trace_type_time := !dedup_trace_type_time +. Sys.time();
    dedup_trace_type_calls := !dedup_trace_type_calls + 1;
    result

  (* ===== terms *)

  let some_constant = mk_const("=",[])
  let id2term = Dynarray.make 0 some_constant
  let term2id = Hashtbl.create 16384

  let termid2compress id =
    compress (Dynarray.length id2term) id

  let term2compress tm =
    termid2compress (Hashtbl.find term2id tm)

  let rec dedup_trace_term_core tm =
    try
      let id = Hashtbl.find term2id tm in
      Dynarray.get id2term id,id
    with Not_found -> begin
      begin
        match tm with
        | Const(s,ty) ->
          let _,tyid = dedup_trace_type ty in
          assert(String.length s > 0);
          trace 'c' [typeid2compress tyid] s
        | Var(s,ty) ->
          let _,tyid = dedup_trace_type ty in
          assert(String.length s > 0);
          trace 'd' [typeid2compress tyid] s
        | Comb(f,x) ->
          let _,fid = dedup_trace_term_core f in
          let _,xid = dedup_trace_term_core x in
          let _,tyid = dedup_trace_type(type_of tm) in
          trace 'e' [typeid2compress tyid;termid2compress fid;termid2compress xid] ""
        | Abs(v,b) ->
          let _,vid = dedup_trace_term_core v in
          let _,bid = dedup_trace_term_core b in
          let _,tyid = dedup_trace_type(type_of tm) in
          trace 'f' [typeid2compress tyid;termid2compress vid;termid2compress bid] ""
      end;
      let id = Dynarray.length id2term in
      Dynarray.add_last id2term tm;
      Hashtbl.add term2id tm id;
      tm,id
    end

  let dedup_trace_term_calls = ref 0
  let dedup_trace_term_time = ref 0.0

  let dedup_trace_term tm =
    dedup_trace_term_time := !dedup_trace_term_time -. Sys.time();
    let result = dedup_trace_term_core tm in
    dedup_trace_term_time := !dedup_trace_term_time +. Sys.time();
    dedup_trace_term_calls := !dedup_trace_term_calls + 1;
    result

  (* ===== theorems *)

  let some_thm = orig_REFL some_constant
  let id2thm = Dynarray.make 0 some_thm
  let thm2id = Hashtbl.create 16384

  let knownthm th = Hashtbl.mem thm2id th

  let thmid2compress id =
    compress (Dynarray.length id2thm) id

  let thm2compress th =
    thmid2compress (Hashtbl.find thm2id th)

  let dedup_trace_thm_calls = ref 0
  let dedup_trace_thm_time = ref 0.0

  let dedup_trace_thm_core th =
    try
      let id = Hashtbl.find thm2id th in
      Dynarray.get id2thm id,id
    with Not_found -> begin
      let h,c = dest_thm th in
      let hc = h@[c] in
      dedup_trace_thm_time := !dedup_trace_thm_time +. Sys.time();
      let hc = map (snd o dedup_trace_term) hc in
      dedup_trace_thm_time := !dedup_trace_thm_time -. Sys.time();
      trace 't' (map termid2compress hc) "";
      let id = Dynarray.length id2thm in
      Dynarray.add_last id2thm th;
      Hashtbl.add thm2id th id;
      th,id
    end

  let dedup_trace_thm th =
    dedup_trace_thm_time := !dedup_trace_thm_time -. Sys.time();
    let result = dedup_trace_thm_core th in
    dedup_trace_thm_time := !dedup_trace_thm_time +. Sys.time();
    dedup_trace_thm_calls := !dedup_trace_thm_calls + 1;
    if !dedup_trace_thm_calls mod 100000 = 0 then begin
      Printf.printf "Holtrace_save so far: %d -> %d thms %.2f s, %d -> %d terms %.2f s, %d -> %d types %.2f s\n%!"
        !dedup_trace_thm_calls (Dynarray.length id2thm) !dedup_trace_thm_time
        !dedup_trace_term_calls (Dynarray.length id2term) !dedup_trace_term_time
        !dedup_trace_type_calls (Dynarray.length id2type) !dedup_trace_type_time
    end;
    result

  let dedup_trace_thm_noid =
    fst o dedup_trace_thm

  (* ===== inferences *)

  let oneterm tag inf tm =
    if !tracing then
      let tm,tmid = dedup_trace_term tm in
      let result = inf tm in
      if knownthm result then result else begin
        trace tag [termid2compress tmid] "";
        dedup_trace_thm_noid result
      end
    else inf tm

  let trace_BETA = oneterm 'B' orig_BETA
  let trace_REFL = oneterm 'R' orig_REFL
  let trace_ASSUME = oneterm 'U' orig_ASSUME
  let trace_new_axiom = oneterm 'X' orig_new_axiom

  let twothms tag inf th TH =
    if !tracing then
      let th,thid = dedup_trace_thm th in
      let TH,THid = dedup_trace_thm TH in
      let result = inf th TH in
      if knownthm result then result else begin
        trace tag (map thmid2compress [thid;THid]) "";
        dedup_trace_thm_noid result
      end
    else inf th TH

  let trace_MK_COMB(th,TH) = twothms 'E' (fun th TH -> orig_MK_COMB(th,TH)) th TH
  let trace_EQ_MP = twothms 'P' orig_EQ_MP
  let trace_TRANS = twothms 'V' orig_TRANS
  let trace_DEDUCT_ANTISYM_RULE = twothms 'Z' orig_DEDUCT_ANTISYM_RULE

  let trace_ABS tm th =
    if !tracing then
      let tm,tmid = dedup_trace_term tm in
      let th,thid = dedup_trace_thm th in
      let result = orig_ABS tm th in
      if knownthm result then result else begin
        trace 'F' [termid2compress tmid;thmid2compress thid] "";
        dedup_trace_thm_noid result
      end
    else orig_ABS tm th
    
  let trace_INST tmlist th =
    if !tracing then
      let th,thid = dedup_trace_thm th in
      let dedup_trace_termpair(w,v) = fst(dedup_trace_term w),fst(dedup_trace_term v) in
      let tmlist = map dedup_trace_termpair tmlist in
      let result = orig_INST tmlist th in
      if knownthm result then result else begin
        let listlist = map (fun(w,v) -> [v;w]) tmlist in
        let flat = List.flatten listlist in
        trace 'S' ((thmid2compress thid)::(map term2compress flat)) "";
        dedup_trace_thm_noid result
      end
    else orig_INST tmlist th

  let trace_INST_TYPE tylist th =
    if !tracing then
      let th,thid = dedup_trace_thm th in
      let dedup_trace_typepair(z,y) = fst(dedup_trace_type z),fst(dedup_trace_type y) in
      let tylist = map dedup_trace_typepair tylist in
      let result = orig_INST_TYPE tylist th in
      if knownthm result then result else begin
        let listlist = map (fun(z,y) -> [y;z]) tylist in
        let flat = List.flatten listlist in
        trace 'T' ((thmid2compress thid)::(map type2compress flat)) "";
        dedup_trace_thm_noid result
      end
    else orig_INST_TYPE tylist th

  let trace_new_basic_definition tm =
    if !tracing then
      let tm,tmid = dedup_trace_term tm in
      let result = orig_new_basic_definition tm in
      if knownthm result then result else begin
        let c,cdef = dest_eq tm in
        let cname,_ = dest_var c in
        assert(String.length cname > 0);
        trace 'C' [termid2compress tmid] cname;
        dedup_trace_thm_noid result
      end
    else orig_new_basic_definition tm

  let trace_new_basic_type_definition tyname (absname,repname) th =
    if !tracing then
      let th,thid = dedup_trace_thm th in
      let ath,rth = orig_new_basic_type_definition tyname (absname,repname) th in
      if knownthm ath && knownthm rth then ath,rth else begin
        let repprop,witness = dest_comb(concl th) in
        let reptype = type_of witness in
        let reptype,reptypeid = dedup_trace_type reptype in
        let repprop,reppropid = dedup_trace_term repprop in
        let tyvars = sort (<=) (type_vars_in_term repprop) in
        let tyvarnames = map dest_vartype tyvars in
        let words = tyname::absname::repname::tyvarnames in
        let text = "]" ^ (String.concat "]" words) ^ "]" in
        let compressed = [typeid2compress reptypeid;termid2compress reppropid;thmid2compress thid] in
        trace 'A' compressed text;
        let ath = dedup_trace_thm_noid ath in
        trace 'D' [-1] "";
        let rth = dedup_trace_thm_noid rth in
        ath,rth
      end
    else orig_new_basic_type_definition tyname (absname,repname) th

  (* ===== theorem names *)

  let names_saved_already = Hashtbl.create 100

  let trace_nameonethm(name,th) =
    if Hashtbl.mem names_saved_already name then
      () (* just ignore (whether old th or not) *)
    else if knownthm th then begin
      assert(String.length(name) > 0);
      trace '/' [thm2compress th] name;
      Hashtbl.add names_saved_already name true
    end else
      failwith "unknown theorem; was HOLTrace running?"

  let trace_namethm l = List.iter trace_nameonethm (List.rev l)

  (* ===== switch *)

  let REFL = if enabled then trace_REFL else orig_REFL
  let TRANS = if enabled then trace_TRANS else orig_TRANS
  let MK_COMB = if enabled then trace_MK_COMB else orig_MK_COMB
  let ABS = if enabled then trace_ABS else orig_ABS
  let BETA = if enabled then trace_BETA else orig_BETA
  let ASSUME = if enabled then trace_ASSUME else orig_ASSUME
  let EQ_MP = if enabled then trace_EQ_MP else orig_EQ_MP
  let DEDUCT_ANTISYM_RULE = if enabled then trace_DEDUCT_ANTISYM_RULE else orig_DEDUCT_ANTISYM_RULE
  let INST_TYPE = if enabled then trace_INST_TYPE else orig_INST_TYPE
  let INST = if enabled then trace_INST else orig_INST

  let new_axiom = if enabled then trace_new_axiom else orig_new_axiom
  let new_basic_definition = if enabled then trace_new_basic_definition else orig_new_basic_definition
  let almost_new_basic_type_definition = if enabled then trace_new_basic_type_definition else orig_new_basic_type_definition

  let holtrace_nameonethm = if enabled then trace_nameonethm else ignore
  let holtrace_namethm = if enabled then trace_namethm else ignore
  let holtrace_stop = if enabled then trace_stop else ignore

  (* ===== even if tracing is disabled, save typedef theorems *)

  let typedef_thms = ref ([]:thm list)

  let new_basic_type_definition tyname (absname,repname) th =
    let ath,rth = almost_new_basic_type_definition tyname (absname,repname) th in
    typedef_thms := rth::ath::!typedef_thms;
    ath,rth

  let holtrace_typedef_thms() = !typedef_thms

end

include Holtrace_save


(* ===== restore *)

module Holtrace_restore: sig

  val holtrace_restore: (string * thm) list -> string -> unit
  val holtrace_restore_time: (string * thm) list -> string -> unit
  val holtrace_restore_allownew: (string * thm) list -> string -> unit
  val holtrace_restore_skipthmchecks: (string * thm) list -> string -> unit
  val holtrace_restore_skipthmchecks_allownew: (string * thm) list -> string -> unit

  val holtrace_restore_worker: (unit -> float) -> bool -> bool -> bool -> (string * thm) list -> string -> unit

  val holtrace_restore_id2type: int -> hol_type
  val holtrace_restore_id2term: int -> term
  val holtrace_restore_id2thm: int -> thm
  val holtrace_restore_name2thm: string -> thm

end = struct

  (* ===== parsing *)

  let parse_subid line pos id =
    let linelen = String.length line in
    let rec parse_nonneg pos result =
      assert(result < 288230376151711744);
      if pos >= linelen then pos,result else
      let c = line.[pos] in
      if c = ' ' then pos+1,result else
      if c >= '0' && c <= '9' then
        parse_nonneg (pos+1) (result*32+(Char.code c-48))
      else if c >= 'A' && c <= 'V' then
        parse_nonneg (pos+1) (result*32+(Char.code c-55))
      else pos,result
    in
    let parse_allowneg pos =
      if pos >= linelen then pos,0 else
      let c = line.[pos] in
      if c = '-' then
        let outpos,n = parse_nonneg (pos+1) 0 in
        outpos,id-n
      else let outpos,n = parse_nonneg pos 0 in
        assert(outpos > pos);
        outpos,n
    in
    let parse_core pos =
      if pos >= linelen then pos,0 else
      match line.[pos] with
      | '!' -> pos+1,id-1
      | '@' -> pos+1,id-2
      | '#' -> pos+1,id-3
      | '$' -> pos+1,id-4
      | '%' -> pos+1,id-5
      | '^' -> pos+1,id-6
      | '&' -> pos+1,id-7
      | '*' -> pos+1,id-8
      | '(' -> pos+1,id-9
      | ')' -> pos+1,id-10
      | _ -> parse_allowneg pos
    in
    let pos,n = parse_core pos in
    assert(n >= 0 && (n < id || id < 0));
    pos,n
  
  let parse_name line pos =
    String.sub line pos (String.length line - pos)

  let parse_pairs line pos id xfrm =
    let rec work result pos =
      if pos = String.length(line) then result else
      let pos,idfrom = parse_subid line pos id in
      let pos,idto = parse_subid line pos id in
      work ((xfrm idto,xfrm idfrom)::result) pos
    in
    rev(work [] pos)

  let parse_many_subid_reverse many line pos id =
    let rec work result many pos =
      if many <= 0 then pos,result else
      let pos,n = parse_subid line pos id in
      work (n::result) (many-1) pos
    in work [] many pos

  (* ===== types *)

  let type_id = ref 0
  
  let some_type = bool_ty
  let holtrace_types = Dynarray.make 0 some_type
  
  let holtrace_types_add x =
    assert(Dynarray.length holtrace_types = !type_id);
    Dynarray.add_last holtrace_types x;
    type_id := 1 + !type_id
  
  let holtrace_vartype line =
    holtrace_types_add (mk_vartype(parse_name line 1))
  
  let holtrace_type line =
    let pos,n = parse_subid line 1 (-1) in
    let pos,revargs = parse_many_subid_reverse n line pos !type_id in
    let args = rev revargs in
    let prevtypes = map (Dynarray.get holtrace_types) args in
    let name = parse_name line pos in
    holtrace_types_add (mk_type(name,prevtypes))
  
  (* ===== terms *)
  
  let term_id = ref 0

  let some_term = mk_const("=",[])
  let holtrace_terms = Dynarray.make 0 some_term
  
  let holtrace_terms_add x =
    assert(Dynarray.length holtrace_terms = !term_id);
    Dynarray.add_last holtrace_terms x;
    term_id := 1 + !term_id
  
  let holtrace_const line =
    let pos,tyid = parse_subid line 1 !type_id in
    let ty = Dynarray.get holtrace_types tyid in
    let name = parse_name line pos in
    holtrace_terms_add (mk_mconst(name,ty))
  
  let holtrace_var line =
    let pos,tyid = parse_subid line 1 !type_id in
    let ty = Dynarray.get holtrace_types tyid in
    let name = parse_name line pos in
    holtrace_terms_add (mk_var(name,ty))
  
  let holtrace_comb line =
    let pos,tyid = parse_subid line 1 !type_id in
    let pos,id0 = parse_subid line pos !term_id in
    let pos,id1 = parse_subid line pos !term_id in
    assert(pos = String.length line);
    let t0 = Dynarray.get holtrace_terms id0 in
    let t1 = Dynarray.get holtrace_terms id1 in
    holtrace_terms_add (mk_comb(t0,t1))
  
  let holtrace_abs line =
    let pos,tyid = parse_subid line 1 !type_id in
    let pos,id0 = parse_subid line pos !term_id in
    let pos,id1 = parse_subid line pos !term_id in
    assert(pos = String.length line);
    let t0 = Dynarray.get holtrace_terms id0 in
    let t1 = Dynarray.get holtrace_terms id1 in
    holtrace_terms_add (mk_abs(t0,t1))

  (* ===== axioms *)

  let stats_axiom_matching = ref 0
  let stats_axiom_creating = ref 0

  let holtrace_axiom allownewaxioms term =
    let target = [],term in
    let istarget f = (dest_thm f = target) in
    try
      let th = List.find istarget (axioms()) in
      Printf.printf "matching axiom\n%!";
      stats_axiom_matching := 1 + !stats_axiom_matching;
      th
    with Not_found ->
      if allownewaxioms then begin
        Printf.printf "creating axiom\n%!";
        stats_axiom_creating := 1 + !stats_axiom_creating;
        new_axiom term
      end else
        failwith "allownewaxioms not set, refusing new axiom"
  
  (* ===== theorems indexed by inf_id *)
  
  let thm_id = ref 0
  let inf_id = ref 0
  let typedef_id = ref 0
  
  let some_thm = REFL some_term
  let holtrace_thms = Dynarray.make 0 some_thm
  let infs = Dynarray.make 0 some_thm
  let typedefs = Dynarray.make 0 some_thm
  let name2thm = Hashtbl.create 100
  
  let infs_add x =
    assert(Dynarray.length infs = !inf_id);
    Dynarray.add_last infs x;
    inf_id := 1 + !inf_id
  
  let typedefs_add x =
    assert(Dynarray.length typedefs = !typedef_id);
    Dynarray.add_last typedefs x;
    typedef_id := 1 + !typedef_id
  
  let inf_term line inf =
    let pos,id0 = parse_subid line 1 !term_id in
    assert(pos = String.length line);
    let t0 = Dynarray.get holtrace_terms id0 in
    infs_add (inf t0)
  
  let inf_thTH line inf =
    let pos,id0 = parse_subid line 1 !thm_id in
    let pos,id1 = parse_subid line pos !thm_id in
    assert(pos = String.length line);
    let t0 = Dynarray.get holtrace_thms id0 in
    let t1 = Dynarray.get holtrace_thms id1 in
    infs_add (inf t0 t1)
  
  let inf_termth line inf =
    let pos,id0 = parse_subid line 1 !term_id in
    let pos,id1 = parse_subid line pos !thm_id in
    assert(pos = String.length line);
    let t0 = Dynarray.get holtrace_terms id0 in
    let t1 = Dynarray.get holtrace_thms id1 in
    infs_add (inf t0 t1)
  
  let constname2thm = Hashtbl.create 1009

  let constname2thm_init() =
    Hashtbl.clear constname2thm;
    let doit th =
      let c,_ = dest_eq(concl th) in
      let cname,_ = dest_const c in
      Hashtbl.add constname2thm cname th
    in
    List.iter doit (definitions())

  let stats_def_matching = ref 0
  let stats_def_creating = ref 0

  let inf_def allownewdefs line =
    let defC term name =
      let v,t = dest_eq term in
      let vname,vtype = dest_var v in
      if vname <> name then
        failwith "name mismatch"
      else
      if Hashtbl.mem constname2thm vname then begin
        Printf.printf "matching constant %s\n%!" vname;
        stats_def_matching := 1 + !stats_def_matching;
        let oldth = Hashtbl.find constname2thm vname in
        let c = mk_mconst(vname,vtype) in
        let newconcl = mk_eq(c,t) in
        let newconclrefl = REFL newconcl in
        EQ_MP newconclrefl oldth
      end else begin
        if allownewdefs then begin
          Printf.printf "creating constant %s\n%!" vname;
          stats_def_creating := 1 + !stats_def_creating;
          new_basic_definition term
        end else failwith("allownewdefs not set (maybe try holtrace_restore_allownew?), refusing new definition of "^vname)
      end
    in
    let pos,id0 = parse_subid line 1 !term_id in
    let name = parse_name line pos in
    let t0 = Dynarray.get holtrace_terms id0 in
    infs_add (defC t0 name)

  let is_known_type_theorem concl =
    let ok f = (dest_thm f = ([],concl)) in
    exists ok (holtrace_typedef_thms())

  let find_type_theorem concl =
    let ok f = (dest_thm f = ([],concl)) in
    find ok (holtrace_typedef_thms())
  
  let inf_typedef_retrieve reptype repprop abstypename absname repname tyvarnames =
    let tyvars = map mk_vartype tyvarnames in
    let abstype = mk_type(abstypename,tyvars) in
    let abs = mk_mconst(absname,mk_fun_ty reptype abstype) in
    let rep = mk_mconst(repname,mk_fun_ty abstype reptype) in
    let absvar = mk_var("a",abstype) in
    let repvar = mk_var("r",reptype) in
    let atarget = mk_eq(mk_comb(abs,mk_comb(rep,absvar)),absvar) in
    let rtarget = mk_eq(mk_comb(repprop,repvar),mk_eq(mk_comb(rep,mk_comb(abs,repvar)),repvar)) in
    let athm = find_type_theorem atarget in
    let rthm = find_type_theorem rtarget in
    athm,rthm

  let can_inf_typedef_retrieve reptype repprop abstypename absname repname tyvarnames =
    try
      let _,_ = inf_typedef_retrieve reptype repprop abstypename absname repname tyvarnames in
      true
    with Failure _ -> false

  let stats_typedef_matching = ref 0
  let stats_typedef_creating = ref 0

  let inf_typedef allownewdefs line =
    let pos,reptype = parse_subid line 1 !type_id in
    let pos,repprop = parse_subid line pos !term_id in
    let pos,repth = parse_subid line pos !thm_id in
    let word = parse_name line pos in
    let reptype = Dynarray.get holtrace_types reptype in
    let repprop = Dynarray.get holtrace_terms repprop in
    let repth = Dynarray.get holtrace_thms repth in
    let thconcl = 
      match dest_thm repth with
      | [],concl -> concl
      | _ -> failwith line
    in
    let thprop,_ =
      try dest_comb thconcl
      with Failure _ -> failwith line
    in
    assert(repprop = thprop);
    assert(word.[0] = ']');
    let args = String.split_on_char ']' word in
    assert(hd(rev args) = "");
    let args = rev(tl(rev args)) in
    match args with
    | ""::abstypename::absname::repname::tyvarnames ->
      if can_inf_typedef_retrieve reptype repprop abstypename absname repname tyvarnames
      then begin
        Printf.printf "matching typedef %s\n%!" abstypename;
        stats_typedef_matching := 1 + !stats_typedef_matching;
        let athm,rthm = inf_typedef_retrieve reptype repprop abstypename absname repname tyvarnames in
        typedefs_add rthm;
        infs_add athm
      end else begin
        if allownewdefs then begin
          Printf.printf "creating typedef %s\n%!" abstypename;
          stats_typedef_creating := 1 + !stats_typedef_creating;
          let athm,rthm = new_basic_type_definition abstypename (absname,repname) repth in
          typedefs_add rthm;
          infs_add athm
        end else failwith("allownewdefs not set (maybe try holtrace_restore_allownew?), refusing new definition of "^abstypename)
      end
    | _ -> failwith line

  let inf_typedef_followup line =
    let pos,typedef = parse_subid line 1 !typedef_id in
    let rthm = Dynarray.get typedefs typedef in
    infs_add rthm

  let inf_inst line =
    let pos,id0 = parse_subid line 1 !thm_id in
    let xfrm = Dynarray.get holtrace_terms in
    let arg = parse_pairs line pos !term_id xfrm in
    let t0 = Dynarray.get holtrace_thms id0 in
    let thm = INST arg t0 in
    infs_add thm

  let inf_inst_type line =
    let pos,id0 = parse_subid line 1 !thm_id in
    let argmap = Dynarray.get holtrace_types in
    let arg = parse_pairs line pos !type_id argmap in
    let t0 = Dynarray.get holtrace_thms id0 in
    let thm = INST_TYPE arg t0 in
    infs_add thm
  
  let holtrace_thms_add x =
    while Dynarray.length holtrace_thms < !thm_id do
      Dynarray.add_last holtrace_thms some_thm
    done;
    assert(Dynarray.length holtrace_thms = !thm_id);
    Dynarray.add_last holtrace_thms x;
    thm_id := 1 + !thm_id
  
  let holtrace_thm checkthmstatements line =
    let linelen = String.length(line) in
    let rec all_subid result pos =
      if pos >= linelen then result else
      let pos,n = parse_subid line pos !term_id in
      all_subid (n::result) pos
    in
    let chhhid = all_subid [] 1 in
    let chhh = map (Dynarray.get holtrace_terms) chhhid in
    let c = hd chhh in
    let hhh = rev(tl chhh) in
    let thm = Dynarray.get infs (!inf_id-1) in
    if checkthmstatements then assert((hhh,c) = dest_thm(thm));
    holtrace_thms_add thm
  
  let thmname_acceptable_firstchar c =
    (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_')

  let thmname_acceptable_char c =
    thmname_acceptable_firstchar c || (c == '\'') || (c == '.')

  let thmname_acceptable s =
    String.length s > 0
    && thmname_acceptable_firstchar s.[0]
    && String.for_all thmname_acceptable_char s

  let stats_thm_matching = ref 0
  let stats_thm_creating = ref 0

  let holtrace_thmname previousthms line =
    let pos,id0 = parse_subid line 1 !thm_id in
    let name = parse_name line pos in
    if not(thmname_acceptable name) then
      failwith ("refusing unusual characters in theorem name "^name)
    else if Hashtbl.mem name2thm name then
      failwith "theorem name already used"
    else begin
      let th = Dynarray.get holtrace_thms id0 in
      Hashtbl.add name2thm name th;
      if exists (fun (n,t) -> n = name) previousthms then begin
        Printf.printf "matching thm %s\n%!" name;
        stats_thm_matching := 1 + !stats_thm_matching;
        let known = assoc name previousthms in
        assert(dest_thm known = dest_thm th)
      end else begin
        Printf.printf "creating thm %s\n%!" name;
        stats_thm_creating := 1 + !stats_thm_creating;
      end
    end
  
  (* ===== main *)

  let holtrace_restore_worker bench allownewaxioms allownewdefs checkthmstatements previousthms fn =
    let total_time = ref 0.0 in
    total_time := !total_time -. bench();

    stats_axiom_matching := 0;
    stats_axiom_creating := 0;
    stats_def_matching := 0;
    stats_def_creating := 0;
    stats_typedef_matching := 0;
    stats_typedef_creating := 0;
    stats_thm_matching := 0;
    stats_thm_creating := 0;

    type_id := 0;
    term_id := 0;
    thm_id := 0;
    inf_id := 0;
    typedef_id := 0;

    Dynarray.clear holtrace_types;
    Dynarray.clear holtrace_terms;
    Dynarray.clear holtrace_thms;
    Dynarray.clear infs;
    Dynarray.clear typedefs;
    Hashtbl.clear name2thm;
    constname2thm_init();

    let tag_occurrences = Array.make 256 0 in
    let tag_time = Array.make 256 0.0 in
    let linecount = ref 0 in
  
    let fd = open_in_bin fn in
    try
      while true do
        let line = input_line fd in
        linecount := !linecount + 1;
        if !linecount = 1 then assert(line = "HOLTrace 1")
        else
        let tag = line.[0] in
        let tagcode = Char.code(tag) in
        tag_occurrences.(tagcode) <- tag_occurrences.(tagcode) + 1;
        tag_time.(tagcode) <- tag_time.(tagcode) -. bench();
        ( match tag with
          | 'a' -> holtrace_vartype line
          | 'b' -> holtrace_type line
          | 'c' -> holtrace_const line
          | 'd' -> holtrace_var line
          | 'e' -> holtrace_comb line
          | 'f' -> holtrace_abs line
          | 't' -> holtrace_thm checkthmstatements line
          | '/' -> holtrace_thmname previousthms line
          | 'X' -> inf_term line (holtrace_axiom allownewaxioms)
          | 'B' -> inf_term line BETA
          | 'R' -> inf_term line REFL
          | 'U' -> inf_term line ASSUME
          | 'F' -> inf_termth line ABS
          | 'P' -> inf_thTH line EQ_MP
          | 'V' -> inf_thTH line TRANS
          | 'Z' -> inf_thTH line DEDUCT_ANTISYM_RULE
          | 'E' -> inf_thTH line (fun t0 t1 -> MK_COMB(t0,t1))
          | 'S' -> inf_inst line
          | 'T' -> inf_inst_type line
          | 'C' -> inf_def allownewdefs line
          | 'A' -> inf_typedef allownewdefs line
          | 'D' -> inf_typedef_followup line
          | ';' -> ()
          | ',' -> ()
          | _ -> failwith line
        );
        tag_time.(tagcode) <- tag_time.(tagcode) +. bench();
      done
    with End_of_file -> close_in fd;
    assert(!linecount >= 1);
    total_time := !total_time +. bench();
    Printf.printf "types: %d\n" !type_id;
    Printf.printf "terms: %d\n" !term_id;
    Printf.printf "theorems: %d\n" !thm_id;
    Printf.printf "inferences: %d\n" !inf_id;
    Printf.printf "axioms: %d = matching %d + creating %d\n" (!stats_axiom_matching + !stats_axiom_creating) !stats_axiom_matching !stats_axiom_creating;
    Printf.printf "typedefs: %d = matching %d + creating %d\n" !typedef_id !stats_typedef_matching !stats_typedef_creating;
    Printf.printf "constants: %d = matching %d + creating %d\n" (!stats_def_matching + !stats_def_creating) !stats_def_matching !stats_def_creating;
    Printf.printf "named thms: %d = matching %d + creating %d\n" (!stats_thm_matching + !stats_thm_creating) !stats_thm_matching !stats_thm_creating;
    for c = 33 to 126 do
      if tag_occurrences.(c) > 0 then
        Printf.printf "tag %c %12f seconds %12d occurrences\n" (Char.chr(c)) tag_time.(c) tag_occurrences.(c)
    done;
    Printf.printf "total %12f seconds\n%!" !total_time

  let holtrace_restore = holtrace_restore_worker (fun x -> 0.0) false false true
  let holtrace_restore_time = holtrace_restore_worker Sys.time false false true
  let holtrace_restore_allownew = holtrace_restore_worker (fun x -> 0.0) true true true
  let holtrace_restore_skipthmchecks = holtrace_restore_worker (fun x -> 0.0) false false false
  let holtrace_restore_skipthmchecks_allownew = holtrace_restore_worker (fun x -> 0.0) true true false

  let holtrace_restore_id2type = Dynarray.get holtrace_types
  let holtrace_restore_id2term = Dynarray.get holtrace_terms
  let holtrace_restore_id2thm = Dynarray.get holtrace_thms
  let holtrace_restore_name2thm = Hashtbl.find name2thm

end

include Holtrace_restore

Printf.printf("===== finished loading holtrace_save and holtrace_restore\n");;
