module Vdom = Virtual_dom.Vdom;
open Virtual_dom.Vdom;
open Node;
open Attr;
//open OptUtil.Syntax;
open Assistant;

let action_abbrev =
  fun
  | InsertVar => "var"
  | InsertApp => "app"
  | InsertLit => "lit";

let action_view =
    (
      inject,
      font_metrics,
      act: assistant_action,
      is_selected: bool,
      prefix_overlay: string,
    ) => {
  let {action, result, res_ty, category, _} = act;
  let width = 80; //TODO: unhardcode?
  let abbr = action_abbrev(category);
  let prefix_overlay =
    StringUtil.match_prefix(prefix_overlay, act.text) ? prefix_overlay : "";
  div(
    [
      classes(["choice"] @ (is_selected ? ["selected"] : [])),
      on_click(_ => {
        Event.Many([
          Event.Prevent_default,
          Event.Stop_propagation,
          inject(ModelAction.AcceptSuggestion(action)),
        ])
      }),
    ],
    [div([classes(["category", abbr])], [text(abbr)])]
    @ [div([classes(["overlay"])], [text(prefix_overlay)])]
    @ UHCode.codebox_view(~font_metrics, width, result)
    @ [
      span([classes(["type-ann"])], [text(" : ")]),
      span([classes(["type"])], [HTypCode.view(res_ty)]),
    ],
  );
};

let view =
    (
      ~inject: ModelAction.t => Vdom.Event.t,
      ~font_metrics: FontMetrics.t,
      settings: Settings.CursorInspector.t,
      cursor_info: CursorInfo.t,
      u_gen: MetaVarGen.t,
    )
    : Vdom.Node.t => {
  switch (AssistantCommon.promote_cursor_info(cursor_info, u_gen)) {
  | None => text("error")
  | Some(cursor) =>
    let actions = compute_actions(cursor);
    let selected_index =
      switch (settings.assistant_selection) {
      | None => 0
      | Some(i) =>
        let z = List.length(actions) == 0 ? 0 : i mod List.length(actions);
        z + (z < 0 ? List.length(actions) : 0);
      };
    let actions = ListUtil.rotate_n(selected_index, actions);
    let prefix_string = Assistant.get_filter_string(cursor_info.cursor_term);
    let action_views =
      List.mapi(
        (i, a) =>
          action_view(inject, font_metrics, a, i == 0, prefix_string),
        actions,
      );
    div([id("assistant")], action_views);
  };
};
