(** 
    The Abstract Syntax Tree.
    This IL allows nested expressions, making it closer to VEX and
    the concrete syntax than our SSA form. However, in most cases, this
    makes analysis harder, so you will generally want to convert to SSA
    for analysis.

    @author Ivan Jager
*)

open ExtList
open Type

module D = Debug.Make(struct let name = "AST" and default=`Debug end)
open D

type var = Var.t
type decl = var

type exp = 
  | Load of exp * exp * exp * typ  (** Load(arr,idx,endian, t) *)
  | Store of exp * exp * exp * exp * typ  (** Store(arr,idx,val, endian, t) *)
  | BinOp of binop_type * exp * exp
  | UnOp of unop_type * exp
  | Var of var
  | Lab of string
  | Int of int64 * typ
  | Cast of cast_type * typ * exp (** Cast to a new type. *)
  | Let of var * exp * exp
  | Unknown of string * typ

type attrs = Type.attributes

type stmt =
  | Move of var * exp * attrs  (** Assign the value on the right to the
				      var on the left *)
  | Jmp of exp * attrs (** Jump to a label/address *)
  | CJmp of exp * exp * exp * attrs
      (** Conditional jump. If e1 is true, jumps to e2, otherwise jumps to e3 *)
  | Label of label * attrs (** A label we can jump to *)
  | Halt of exp * attrs
  | Assert of exp * attrs
  | Comment of string * attrs (** A comment to be ignored *)
  | Special of string * attrs (** A "special" statement. (does magic) *)

type program = stmt list


let expr_of_lab = function
  | Name s -> Lab s
  | Addr a -> Int(a, REG_64)


let lab_of_exp = function
  | Lab s -> Some(Name s)
  | Int(i, t) ->
      (* FIXME: figure out where the bits_of_with function should live *)
      let bits = match t with
	| REG_1 -> 1
	| REG_8 -> 8
	| REG_16 -> 16
	| REG_32 -> 32
	| REG_64 -> 64
	| _ -> invalid_arg "bits_of_width"
      in
      Some(Addr(Int64.logand i (Int64.pred(Int64.shift_left 1L bits))))
  | _ -> None
    

let exp_false = Int(0L, REG_1)
let exp_true = Int(1L, REG_1)

let exp_and e1 e2 = BinOp(AND, e1, e2)
let exp_or e1 e2 = BinOp(OR, e1, e2)
let exp_not e = UnOp(NOT, e)
let exp_implies e1 e2 = exp_or (exp_not e1) e2
