/-
Copyright (c) 2016 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.
Authors: Leonardo de Moura

Helper tactic for showing that a type has decidable equality.
-/
prelude
import init.meta.tactic

namespace tactic
open expr

/- Return tt iff e's type is of the form `(I_name ...)` -/
meta_definition is_type_app_of (e : expr) (I_name : name) : tactic bool :=
do t ← infer_type e,
   return $ is_constant_of (get_app_fn t) I_name

/- Auxiliary function for using brec_on "dictionary" -/
private meta_definition mk_rec_inst_aux : expr → nat → tactic expr
| F 0     := do
  P ← mk_app `prod.pr1 [F],
  mk_app `prod.pr1 [P]
| F (n+1) := do
  F' ← mk_app `prod.pr2 [F],
  mk_rec_inst_aux F' n

/- Construct brec_on "recursive value". F_name is the name of the brec_on "dictionary".
   Type of the F_name hypothesis should be of the form (below (C ...)) where C is a constructor.
   The result is the "recursive value" for the (i+1)-th recursive value of the constructor C. -/
meta_definition mk_brec_on_rec_value (F_name : name) (i : nat) : tactic expr :=
do F ← get_local F_name,
   mk_rec_inst_aux F i

meta_definition constructor_num_fields (c : name) : tactic nat :=
do env     ← get_env,
   decl    ← returnex $ environment.get env c,
   ctype   ← return $ declaration.type decl,
   arity   ← get_pi_arity ctype,
   I       ← returnopt $ environment.inductive_type_of env c,
   nparams ← return (environment.inductive_num_params env I),
   return $ arity - nparams

private meta_definition mk_name_list_aux : name → nat → nat → list name → list name × nat
| p i 0     l := (list.reverse l, i)
| p i (j+1) l := mk_name_list_aux p (i+1) j (mk_num_name p i :: l)

private meta_definition mk_name_list (p : name) (i : nat) (n : nat) : list name × nat :=
mk_name_list_aux p i n []

/- Return a list of names of the form [p.i, ..., p.{i+n}] where n is
   the number of fields of the constructor c -/
meta_definition mk_constructor_arg_names (c : name) (p : name) (i : nat) : tactic (list name × nat) :=
do nfields ← constructor_num_fields c,
   return $ mk_name_list p i nfields

private meta_definition mk_constructors_arg_names_aux : list name → name → nat → list (list name) → tactic (list (list name))
| []      p i r := return (list.reverse r)
| (c::cs) p i r := do
  v : list name × nat ← mk_constructor_arg_names c p i,
  match v with (l, i') := mk_constructors_arg_names_aux cs p i' (l :: r) end

/- Given an inductive datatype I with k constructors and where constructor i has n_i fields,
   return the list [[p.1, ..., p.n_1], [p.{n_1 + 1}, ..., p.{n_1 + n_2}], ..., [..., p.{n_1 + ... + n_k}]] -/
meta_definition mk_constructors_arg_names (I : name) (p : name) : tactic (list (list name)) :=
do env ← get_env,
   cs  ← return $ environment.constructors_of env I,
   mk_constructors_arg_names_aux cs p 1 []

end tactic
