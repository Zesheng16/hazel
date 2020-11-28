open Sexplib.Std

(** The syntax of {e Core Smyth}, as defined in {b Figure 3} of the ICFP 2020
    paper. *)

(** The type of hole "names," which is used to identify holes for synthesis. *)
type hole_name = int [@@deriving sexp]

(** A map with domain {!hole_name}. *)
module Hole_map = struct
  module Key = struct
    type t = hole_name [@@deriving sexp]

    let compare = Int.compare
  end

  include Map.Make (Key)

  let t_of_sexp _ = failwith "todo"

  let sexp_of_t sexp_of_v map =
    map |> to_seq |> List.of_seq
    |> sexp_of_list (fun (k, v) ->
           Sexplib.Sexp.List [Key.sexp_of_t k; sexp_of_v v])
end

(** An abbreviation for using {!Hole_map}s. *)
type 'a hole_map = 'a Hole_map.t [@@deriving sexp]

(** Expression types. *)
type typ =
  | TArr of typ * typ  (** Arrow type *)
  | TTuple of typ list  (** Tuple type *)
  | TData of string * typ list  (** Datatype *)
  | TForall of string * typ  (** Universal quantification *)
  | TVar of string  (** Type variable *)
[@@deriving sexp]

(** Patterns. *)
type pat =
  | PVar of string  (** Variable pattern *)
  | PTuple of pat list  (** Tuple pattern *)
  | PWildcard  (** Wildcard pattern *)
[@@deriving sexp]

(** The types of valid parameters in a function signature. *)
type param = PatParam of pat | TypeParam of string [@@deriving sexp]

(** The types of valid arguments in a function application. *)
type exp_arg = EAExp of exp | EAType of typ [@@deriving sexp]

(** Expressions.

    - If the "name" field of a fix expression is [None], then the fix is a
      non-recursive lambda.

    - A "special" application is one that has recursion generated by
      synthesis from {!Term_gen}. This feature is tracked only for end-user
      purposes; it can be used, for example, in a "recursive window" UI (that
      only displays recursive solutions) or in benchmarks for the "Top-1R"
      (etc.) experiments. *)
and exp =
  | EFix of string option * param * exp
      (** Fix expressions [(name, param, body)]*)
  | EApp of bool * exp * exp_arg  (** Applications [(special, head, arg)] *)
  | EVar of string  (** Variables *)
  | ETuple of exp list  (** Tuples *)
  | EProj of int * int * exp  (** Tuple projections [(n, i, arg)] *)
  | ECtor of string * typ list * exp
      (** Constructors [(name, type args, arg)] *)
  | ECase of exp * (string * (pat * exp)) list
      (** Case expressions [(scrutinee, branches)] *)
  | EHole of hole_name  (** Hole expressions *)
  | EAssert of exp * exp  (** Assertions *)
  | ETypeAnnotation of exp * typ  (** Type annotations *)

(** The types of valid arguments in a result function application. *)
type res_arg = RARes of res | RAType of typ [@@deriving sexp]

(** Results.

    - Determinate results: [RFix, RTuple, RCtor].
    - Indeterminate results: [RHole, RApp, RProj, RCase, RCtorInverse]. *)
and res =
  | RFix of env * string option * param * exp  (** Fix closures *)
  | RTuple of res list  (** Tuples *)
  | RCtor of string * res  (** Constructors *)
  | RHole of env * hole_name  (** Hole closures *)
  | RApp of res * res_arg  (** Applications *)
  | RProj of int * int * res  (** Tuple projections *)
  | RCase of env * res * (string * (pat * exp)) list  (** Case results *)
  | RCtorInverse of string * res  (** Inverse constructors *)
[@@deriving sexp]

(** Environments: [(result bindings, type variable bindings)]. *)
and env = (string * res) list * (string * typ) list [@@deriving sexp]

(** Binding specifications. *)
type bind_spec =
  | NoSpec (* No bindspec *)
  | Rec of string (* "Recursive" bindspec *)
  | Arg of string (* "Argument of" bindspec *)
  | Dec of string
[@@deriving sexp]

(* "Decreasing on" bindspec *)

(** Type bindings for type contexts. *)
type type_binding = string * (typ * bind_spec)

(** Polymorphic name "bindings" for type contexts (just the name of the
    variable is needed in the type context). *)
type poly_binding = string

(** Type contexts. *)
type type_ctx = type_binding list * poly_binding list

(** Datatype contexts. *)
type datatype_ctx = (string * (string list * (string * typ) list)) list
[@@deriving sexp]

(** Hole contexts:
    [(hole name, type context, typ, decrease requirement, match depth)]. The
    "decrease requirement", if present, is a function that expressions must
    be decreasing on to fill the hole in question. *)
type hole_ctx = (hole_name * (type_ctx * typ * string option * int)) list

(** "Simple" values. *)
type value =
  | VTuple of value list  (** Tuples *)
  | VCtor of string * value  (** Constructors *)
[@@deriving sexp]

(** Examples. *)
type example =
  | ExTuple of example list  (** Tuples *)
  | ExCtor of string * example  (** Constructors *)
  | ExInputOutput of value * example  (** Input-output examples *)
  | ExTop  (** Top (wildcard) examples *)
[@@deriving sexp]

(** Example constraints, also known as "worlds." *)
type world = env * example [@@deriving sexp]

(** Multiple example constraints (worlds). *)
type worlds = world list [@@deriving sexp]

(** Hole fillings. *)
type hole_filling = exp hole_map [@@deriving sexp]

(** Unfilled holes, also known as "unsolved constraints." *)
type unsolved_constraints = worlds hole_map [@@deriving sexp]

(** Constraints. *)
type constraints = hole_filling * unsolved_constraints [@@deriving sexp]

(** Resumption assertions, as defined in {b Figure 7}. *)
type resumption_assertion = res * value

(** Multiple resumption assertions. *)
type resumption_assertions = resumption_assertion list

(** Term generation ("guessing") goals. *)
type gen_goal = type_ctx * typ * string option

(** Basic synthesis goals. *)
type synthesis_goal = gen_goal * worlds

(** Full notion of synthesis goals, including a hole name. *)
type fill_goal = hole_name * synthesis_goal

(** Parameters for synthesis. *)
type synthesis_params =
  {max_scrutinee_size: int; max_match_depth: int; max_term_size: int}

type output_constraints = worlds hole_map list [@@deriving sexp]
