open OptUtil.Syntax;

type assistant_action_categories =
  | InsertLit
  | InsertVar
  | InsertApp;

type assistant_action = {
  category: assistant_action_categories,
  text: string,
  action: Action.t,
  result: UHExp.t,
  res_ty: HTyp.t,
};

let mk_var = name =>
  Seq.mk(UHExp.var(name), []) |> UHExp.mk_OpSeq |> UHExp.Block.wrap';

let mk_bool_lit = b =>
  Seq.mk(UHExp.boollit(b), []) |> UHExp.mk_OpSeq |> UHExp.Block.wrap';

let mk_list_nil =
  Seq.mk(UHExp.listnil(), []) |> UHExp.mk_OpSeq |> UHExp.Block.wrap';

let _get_hole_number = (t: CursorInfo.cursor_term): MetaVar.t => {
  switch (t) {
  | Exp(_, EmptyHole(n)) => n
  | _ => failwith("get_hole_number: Not on hole")
  };
};

let wrap_space = (operator, seq) =>
  Seq.S(operator, A(Operators_Exp.Space, seq));

let rec mk_ap_seq_holes =
        (f_ty: HTyp.t, hole_ty: HTyp.t)
        : option(Seq.t(UHExp.operand, UHExp.operator)) => {
  switch (f_ty) {
  | Arrow(_, output_ty) =>
    if (HTyp.consistent(output_ty, hole_ty)) {
      Some(S(EmptyHole(0), E));
    } else {
      let+ affix = mk_ap_seq_holes(output_ty, hole_ty);
      wrap_space(UHExp.EmptyHole(0), affix);
    }
  | _ => None
  };
};

let mk_ap =
    (
      {expected_ty, ctx, u_gen, _}: AssistantCommon.cursor_info_pro,
      f: Var.t,
      f_ty: HTyp.t,
    )
    : option(UHExp.t) => {
  let+ inner_seq = mk_ap_seq_holes(f_ty, expected_ty);
  wrap_space(UHExp.var(f), inner_seq)
  |> UHExp.mk_OpSeq
  |> UHExp.Block.wrap'
  |> Statics_Exp.syn_fix_holes(ctx, u_gen, ~renumber_empty_holes=true)
  |> (((x, _, _)) => x);
};

let mk_var_action =
    (_: AssistantCommon.cursor_info_pro, name: string, ty: HTyp.t)
    : assistant_action => {
  let e = mk_var(name);
  {
    category: InsertVar,
    action: ReplaceAtCursor(UHExp.var(name)),
    //FillExpHole(get_hole_number(term), e),
    text: name,
    result: e,
    res_ty: ty,
  };
};

let mk_app_action =
    (cursor: AssistantCommon.cursor_info_pro, name: string, ty: HTyp.t)
    : assistant_action => {
  let e =
    mk_ap(cursor, name, ty) |> OptUtil.get(_ => failwith("mk_app_action"));
  {
    category: InsertApp,
    text: name,
    action: ReplaceAtCursor(UHExp.Parenthesized(e)),
    //FillExpHole(get_hole_number(cursor.term), e),
    res_ty: ty,
    result: e,
  };
};

let mk_bool_lit_action = (b: bool): assistant_action => {
  {
    category: InsertLit,
    text: string_of_bool(b),
    action: ReplaceAtCursor(UHExp.boollit(b)),
    //FillExpHole(get_hole_number(cursor.term), e),
    res_ty: HTyp.Bool,
    result: mk_bool_lit(b),
  };
};

let mk_nil_list_action: assistant_action = {
  {
    category: InsertLit,
    text: "[]",
    action: ReplaceAtCursor(UHExp.listnil()),
    //FillExpHole(get_hole_number(cursor.term), e),
    res_ty: HTyp.Bool,
    result: mk_list_nil,
  };
};

let compute_lit_actions =
    ({expected_ty, mode, _}: AssistantCommon.cursor_info_pro) => {
  switch (mode) {
  | Synthetic => []
  | Analytic =>
    switch (expected_ty) {
    | Bool => [mk_bool_lit_action(true), mk_bool_lit_action(false)]
    | List(_) => [mk_nil_list_action]
    | _ => []
    }
  | UnknownMode => []
  };
};

let compute_var_actions =
    ({ctx, expected_ty, mode, _} as cursor: AssistantCommon.cursor_info_pro) => {
  switch (mode) {
  | Synthetic =>
    AssistantCommon.extract_vars(ctx, Hole)
    |> List.map(((name, var_ty)) => mk_var_action(cursor, name, var_ty))
  | Analytic =>
    AssistantCommon.extract_vars(ctx, expected_ty)
    |> List.map(((name, var_ty)) => mk_var_action(cursor, name, var_ty))
  | UnknownMode => []
  };
};

let compute_app_actions =
    ({ctx, expected_ty, mode, _} as cursor: AssistantCommon.cursor_info_pro) => {
  switch (mode) {
  | Synthetic
  | UnknownMode => []
  | Analytic =>
    AssistantCommon.fun_vars(ctx, expected_ty)
    |> List.map(((name, f_ty)) => mk_app_action(cursor, name, f_ty))
  };
};

//TODO: sort actions intelligently;
let sort = action_list /*, user_model*/ => action_list;

let get_filter_string = (term: CursorInfo.cursor_term): string => {
  switch (term) {
  | Exp(_, Var(_, _, s)) => s
  | _ => ""
  };
};

let bring_prefix_matches_to_top =
    (prefix: string, actions: list(assistant_action))
    : list(assistant_action) => {
  let gooduns =
    List.filter(
      ({text, _}) => StringUtil.match_prefix(prefix, text),
      actions,
    );
  // NOTE: sort gooduns if they are nontrivial matches
  let gooduns =
    prefix == ""
      ? gooduns
      : List.sort((a1, a2) => String.compare(a1.text, a2.text), gooduns);
  let baduns =
    List.filter(
      ({text, _}) => !StringUtil.match_prefix(prefix, text),
      actions,
    );
  gooduns @ baduns;
};

let compute_actions =
    ({term, _} as cursor: AssistantCommon.cursor_info_pro)
    : list(assistant_action) => {
  // TODO(andrew): bug: if list rotated, filter looks weird
  // BUG: move to next hole should reset scroll position
  compute_var_actions(cursor)
  @ compute_app_actions(cursor)
  @ compute_lit_actions(cursor)
  |> bring_prefix_matches_to_top(get_filter_string(term))
  |> sort;
};
