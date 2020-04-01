open Sexplib.Std;

[@deriving sexp]
type t =
  | Rule
  | Case({err: ErrStatus.t})
  | Var({
      err: ErrStatus.t,
      verr: VarErrStatus.t,
      vwarn: VarWarnStatus.t,
      show_use: bool,
    })
  | Operand({err: ErrStatus.t})
  | BinOp({
      op_index: int,
      err: ErrStatus.t,
    })
  | NTuple({
      comma_indices: list(int),
      err: ErrStatus.t,
    })
  | SubBlock({hd_index: int});

let mk_Var =
    (
      ~err: ErrStatus.t=NotInHole,
      ~verr: VarErrStatus.t=NotInVarHole,
      ~vwarn: VarWarnStatus.t=NoWarning,
      ~show_use=false,
      (),
    ) =>
  Var({err, verr, vwarn, show_use});

let mk_Operand = (~err: ErrStatus.t=NotInHole, ()) => Operand({err: err});