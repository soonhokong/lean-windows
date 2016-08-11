/*
Copyright (c) 2016 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#pragma once
#include "kernel/expr.h"
namespace lean {
class parser;
class elaborator;
/** \brief Parse explict universe parameters of the form:
           .{u_1 ... u_k}

    The universe parameters are automatically added to the parser scope. */
bool parse_univ_params(parser & p, buffer<name> & lp_names);
/** \brief Parse a declaration header of the form

         c.{u_1 ... u_k} (params) : type

    The result is the pair (c, type). The explicit universe level parameters are stored
    at lp_names, and the optional parameters at params.

    Both lp_names and params are added to the parser scope.

    \remark Caller is responsible for using: parser::local_scope scope2(p, env); */
pair<name, expr> parse_single_header(parser & p, buffer<name> & lp_names, buffer<expr> & params);
/** \brief Parse the header of a mutually recursive declaration. It has the form

        {u_1 ... u_k} id_1, ... id_n (params)

    The explicit universe parameters are stored at lp_names,
    The constant names id_i are stored at c_names.

    Both lp_names and params are added to the parser scope.
    \remark Caller is responsible for adding expressions encoding the c_names to the parser
    scope.
    \remark Caller is responsible for using: parser::local_scope scope2(p, env); */
void parse_mutual_header(parser & p, buffer<name> & lp_names, buffer<name> & c_names, buffer<expr> & params);
/** \brief Parse the header for one of the declarations in a mutually recursive declaration.
    It has the form

         with id : type

    The result is type. */
expr parse_inner_header(parser & p, name const & c_expected);

/** \brief Add section/namespace parameters (and universes) used by params and all_exprs.
    We also add parameters included using the command 'include'.
    lp_names and params are input/output. */
void collect_implicit_locals(parser & p, buffer<name> & lp_names, buffer<expr> & params, buffer<expr> const & all_exprs);

/** \brief Elaborate the types of the parameters \c params, and update the elaborator local context using them.
    Store the elaborated parameters at new_params.

    \post params.size() == new_params.size() */
void elaborate_params(elaborator & elab, buffer<expr> const & params, buffer<expr> & new_params);
}