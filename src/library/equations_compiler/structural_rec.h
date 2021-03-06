/*
Copyright (c) 2016 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#pragma once
#include "library/type_context.h"
namespace lean {
/** \brief Try to eliminate "recursive calls" in the equations \c e by using brec_on's below.
    If successful, a new set of (non-recursive) equations is produced. The new equations
    have a new argument of type `I.below` and all "recursive calls" are replaced with it.

    The procedure fails when:
    1- \c e is defining more than one function
    2- None of the arguments is a primitive inductive datatype with support for brec_on
       construction, where every recursive call is structurally smaller. */
optional<expr> try_structural_rec(environment & env, options const & opts,
                                  metavar_context & mctx, local_context const & lctx,
                                  expr const & eqns);

void initialize_structural_rec();
void finalize_structural_rec();
}
