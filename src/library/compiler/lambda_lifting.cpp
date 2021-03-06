/*
Copyright (c) 2016 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include "kernel/instantiate.h"
#include "kernel/abstract.h"
#include "kernel/for_each_fn.h"
#include "kernel/inductive/inductive.h"
#include "library/normalize.h"
#include "library/util.h"
#include "library/vm/vm.h"
#include "library/compiler/util.h"
#include "library/compiler/erase_irrelevant.h"
#include "library/compiler/compiler_step_visitor.h"

namespace lean {
class lambda_lifting_fn : public compiler_step_visitor {
    buffer<pair<name, expr>> m_new_procs;
    name                     m_prefix;
    unsigned                 m_idx;

    expr declare_aux_def(expr const & value) {
        /* Try to avoid unnecessary aux decl by
           1- Apply eta-reduction
           2- Check if the result is of the form (f ...) where f is
              a) VM builtin functions OR
              b) A function without builtin support (i.e., it is not a constructor, cases_on or projection) */
        expr new_value  = try_eta(value);
        expr const & fn = get_app_fn(new_value);
        if (is_constant(fn)) {
            name const & n = const_name(fn);
            if (is_vm_builtin_function(n) ||
                (!inductive::is_intro_rule(env(), n) && !is_cases_on_recursor(env(), n) && !is_projection(env(), n)))
                return new_value;
        }
        name aux_name = mk_fresh_name(env(), m_prefix, "_lambda", m_idx);
        m_new_procs.emplace_back(aux_name, value);
        return mk_constant(aux_name);
    }

    typedef rb_map<unsigned, local_decl, unsigned_rev_cmp> idx2decls;

    void collect_locals(expr const & e, idx2decls & r) {
        local_context const & lctx = ctx().lctx();
        for_each(e, [&](expr const & e, unsigned) {
                if (is_local_decl_ref(e)) {
                    local_decl d = *lctx.get_local_decl(e);
                    r.insert(d.get_idx(), d);
                }
                return true;
            });
    }

    expr visit_lambda_core(expr const & e) {
        type_context::tmp_locals locals(m_ctx);
        expr t = e;
        while (is_lambda(t)) {
            lean_assert(is_neutral_expr(binding_domain(t)));
            locals.push_local(binding_name(t), binding_domain(t), binding_info(t));
            t = binding_body(t);
        }
        t = instantiate_rev(t, locals.size(), locals.data());
        t = visit(t);
        return locals.mk_lambda(t);
    }

    expr abstract_locals(expr e, buffer<expr> & locals) {
        idx2decls map;
        collect_locals(e, map);
        if (map.empty()) {
            return e;
        } else {
            while (!map.empty()) {
                /* remove local_decl with biggest idx */
                local_decl d = map.erase_min();
                expr l       = d.mk_ref();
                if (auto v = d.get_value()) {
                    collect_locals(*v, map);
                    e = instantiate(abstract_local(e, l), *v);
                } else {
                    locals.push_back(l);
                    e = abstract_local(e, l);
                    e = mk_lambda(d.get_name(), d.get_type(), e);
                }
            }
            return e;
        }
    }

    virtual expr visit_lambda(expr const & e) override {
        expr new_e = visit_lambda_core(e);
        buffer<expr> locals;
        new_e  = abstract_locals(new_e, locals);
        expr c = declare_aux_def(new_e);
        return mk_rev_app(c, locals);
    }

    virtual expr visit_let(expr const & e) override {
        type_context::tmp_locals locals(m_ctx);
        expr t = e;
        while (is_let(t)) {
            lean_assert(is_neutral_expr(let_type(t)));
            expr v = visit(instantiate_rev(let_value(t), locals.size(), locals.data()));
            locals.push_let(let_name(t), let_type(t), v);
            t = let_body(t);
        }
        t = instantiate_rev(t, locals.size(), locals.data());
        t = visit(t);
        return locals.mk_let(t);
    }

    expr visit_cases_on_minor(unsigned data_sz, expr e) {
        type_context::tmp_locals locals(ctx());
        for (unsigned i = 0; i < data_sz; i++) {
            if (is_lambda(e)) {
                expr l = locals.push_local_from_binding(e);
                e = instantiate(binding_body(e), l);
            } else {
                expr l = locals.push_local("a", mk_neutral_expr());
                e = mk_app(e, l);
            }
        }
        e = visit(e);
        return locals.mk_lambda(e);
    }

    /* We should preserve the lambda's in minor premises */
    expr visit_cases_on_app(expr const & e) {
        buffer<expr> args;
        expr const & fn = get_app_args(e, args);
        lean_assert(is_constant(fn));
        name const & rec_name       = const_name(fn);
        name const & I_name         = rec_name.get_prefix();
        /* erase_irrelevant already removed parameters and indices from cases_on applications */
        unsigned nminors            = *inductive::get_num_minor_premises(env(), I_name);
        unsigned nparams            = *inductive::get_num_params(env(), I_name);
        unsigned arity              = nminors + 1 /* major premise */;
        unsigned major_idx          = 0;
        unsigned first_minor_idx    = 1;
        /* This transformation assumes eta-expansion have already been applied.
           So, we should have a sufficient number of arguments. */
        lean_assert(args.size() >= arity);
        buffer<name> cnames;
        get_intro_rule_names(env(), I_name, cnames);
        /* Process major premise */
        args[major_idx]        = visit(args[major_idx]);
        /* Process extra arguments */
        for (unsigned i = arity; i < args.size(); i++)
            args[i] = visit(args[i]);
        /* Process minor premises */
        for (unsigned i = 0, j = first_minor_idx; i < cnames.size(); i++, j++) {
            unsigned carity   = get_constructor_arity(env(), cnames[i]);
            lean_assert(carity >= nparams);
            unsigned cdata_sz = carity - nparams;
            args[j] = visit_cases_on_minor(cdata_sz, args[j]);
        }
        return mk_app(fn, args);
    }

    virtual expr visit_app(expr const & e) override {
        expr const & fn = get_app_fn(e);
        if (is_constant(fn) && is_cases_on_recursor(env(), const_name(fn))) {
            return visit_cases_on_app(e);
        } else {
            return compiler_step_visitor::visit_app(beta_reduce(e));
        }
    }

public:
    lambda_lifting_fn(environment const & env, name const & prefix):
        compiler_step_visitor(env), m_prefix(prefix), m_idx(1) {
    }

    void operator()(buffer<pair<name, expr>> & procs) {
        for (auto p : procs) {
            expr val     = p.second;
            expr new_val = is_lambda(val) ? visit_lambda_core(val) : visit(val);
            m_new_procs.emplace_back(p.first, new_val);
        }
        procs.clear();
        procs.append(m_new_procs);
    }
};

void lambda_lifting(environment const & env, name const & prefix, buffer<pair<name, expr>> & procs) {
    return lambda_lifting_fn(env, prefix)(procs);
}
}
