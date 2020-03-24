module Js = Js_of_ocaml.Js;
module Dom = Js_of_ocaml.Dom;
module Dom_html = Js_of_ocaml.Dom_html;

module RowMap = Map.Make(Int);
module ColMap = {
  include Map.Make(Int);

  let find_before = (col, map) =>
    map |> find_last_opt(c => Int.compare(c, col) < 0);
  let find_before_eq = (col, map) =>
    map |> find_last_opt(c => Int.compare(c, col) <= 0);

  let find_after = (col, map) =>
    map |> find_first_opt(c => Int.compare(c, col) > 0);
  let find_after_eq = (col, map) =>
    map |> find_first_opt(c => Int.compare(c, col) >= 0);
};

type t = RowMap.t(ColMap.t(CursorPath.rev_t));
type binding = ((int, int), CursorPath.rev_t);

module Builder: {
  type cmap = t;
  type t;
  let init: unit => t;
  let add: ((int, int), CursorPath.rev_t, t) => unit;
  let build: t => cmap;
} = {
  type cmap = t;
  type t = ref(cmap);

  let init = () => ref(RowMap.empty);

  let add = ((row, col), rev_path, builder) => {
    let col_map =
      switch (builder^ |> RowMap.find_opt(row)) {
      | None => ColMap.empty
      | Some(map) => map
      };
    builder :=
      builder^ |> RowMap.add(row, col_map |> ColMap.add(col, rev_path));
  };

  let build = builder => builder^;
};

let num_rows = cmap => RowMap.cardinal(cmap);

let find = ((row, col), cmap) =>
  cmap |> RowMap.find(row) |> ColMap.find(col);

let start_of_row = (row, cmap) =>
  cmap
  |> RowMap.find(row)
  |> ColMap.min_binding
  |> (
    fun
    | (col, rev_path) => ((row, col), rev_path)
  );

let end_of_row = (row, cmap) =>
  cmap
  |> RowMap.find(row)
  |> ColMap.max_binding
  |> (
    fun
    | (col, rev_path) => ((row, col), rev_path)
  );

let find_before_within_row = ((row, col), cmap) =>
  cmap
  |> RowMap.find(row)
  |> ColMap.find_before(col)
  |> Option.map(((col, rev_path)) => ((row, col), rev_path));

let find_after_within_row = ((row, col), cmap) =>
  cmap
  |> RowMap.find(row)
  |> ColMap.find_after(col)
  |> Option.map(((col, rev_path)) => ((row, col), rev_path));

let find_nearest_within_row = ((row, col), cmap) => {
  let col_map = cmap |> RowMap.find(row);
  switch (
    col_map |> ColMap.find_before_eq(col),
    col_map |> ColMap.find_after_eq(col),
  ) {
  | (None, None) =>
    failwith(
      "CursorMap has row with no caret positions: " ++ string_of_int(row),
    )
  | (Some((nearest_col, rev_path)), None)
  | (None, Some((nearest_col, rev_path))) => ((row, nearest_col), rev_path)
  | (Some((col', rev_path)), _) when col' == col => ((row, col), rev_path)
  | (Some((col_before, rev_path_before)), Some((col_after, rev_path_after))) =>
    col - col_before <= col_after - col
      ? ((row, col_before), rev_path_before)
      : ((row, col_after), rev_path_after)
  };
};