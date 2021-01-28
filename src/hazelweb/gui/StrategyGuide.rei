open Virtual_dom;

/**
 * Strategy Guide at the cursor.
 */
let view:
  (
    ~inject: ModelAction.t => Vdom.Event.t,
    Settings.CursorInspector.t,
    CursorInfo.t
  ) =>
  Vdom.Node.t;
