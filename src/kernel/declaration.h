/*
Copyright (c) 2014 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#pragma once
#include <algorithm>
#include <string>
#include <limits>
#include "util/rc.h"
#include "kernel/expr.h"

namespace lean {
/** \brief Reducibility hints are used in the convertibility checker (aka is_def_eq predicate),
     whenever checking a constraint such as

           (f ...) =?= (g ...)

     where f and g are definitions, and the checker has to decide which one will be unfolded.
     If f (g) is Opaque,             then g (f) is unfolded if it is also not marked as Opaque.
     Else if f (g) is Abbreviation,  then f (g) is unfolded if g (f) is also not marked as Abbreviation.
     Else if f and g are Regular,    then we unfold the one with the biggest definitional height.
     Otherwise unfold both.

     The definitional height is by default computed by the kernel. It only takes into account
     other Regular definitions used in a definition.

     For definitions marked as regular, we also have a hint for constraints such as

          (f a) =?= (f b)

     if self_opt == true, then checker will first try to solve (a =?= b), only if it fails,
     it unfolds f.

     Remark: the hint only affects performance. */
class reducibility_hints {
public:
    enum kind {
        Regular,
        Opaque,
        Abbreviation
    };
private:
    kind      m_kind;
    unsigned  m_height; /* definitional height */
    bool      m_self_opt;
    reducibility_hints(kind k, unsigned h = 0, bool self_opt = false):m_kind(k), m_height(h), m_self_opt(self_opt) {}
public:
    static reducibility_hints mk_regular(unsigned h, bool self_opt = true) { return reducibility_hints(Regular, h, self_opt); }
    static reducibility_hints mk_opaque() { return reducibility_hints(Opaque); }
    static reducibility_hints mk_abbreviation() { return reducibility_hints(Abbreviation); }

    /** Given h1 and h2 the hints for definitions f1 and f2, then
        result is
            <  0 If f1 should be unfolded
            == 0 If f1 and f2 should be unfolded
            >  0 If f2 should be unfolded */
    friend int compare(reducibility_hints const & h1, reducibility_hints const & h2);

    unsigned get_height() const { return m_height; }
    kind get_kind() const { return m_kind; }
    bool is_regular() const { return m_kind == Regular; }
    bool use_self_opt() const { return m_self_opt; }
};

int compare(reducibility_hints const & h1, reducibility_hints const & h2);

/** \brief Environment definitions, theorems, axioms and variable declarations. */
class declaration {
    struct cell;
    cell * m_ptr;
    explicit declaration(cell * ptr);
    friend struct cell;
public:
    /**
       \brief The default constructor creates a reference to a "dummy"
       declaration.  The actual "dummy" declaration is not relevant, and
       no procedure should rely on the kind of declaration used.

       We have a default constructor because some collections only work
       with types that have a default constructor.
    */
    declaration();
    declaration(declaration const & s);
    declaration(declaration && s);
    ~declaration();

    friend void swap(declaration & a, declaration & b) { std::swap(a.m_ptr, b.m_ptr); }

    declaration & operator=(declaration const & s);
    declaration & operator=(declaration && s);

    friend bool is_eqp(declaration const & d1, declaration const & d2) { return d1.m_ptr == d2.m_ptr; }

    bool is_definition() const;
    bool is_axiom() const;
    bool is_theorem() const;
    bool is_constant_assumption() const;

    bool is_trusted() const;

    name const & get_name() const;
    level_param_names const & get_univ_params() const;
    unsigned get_num_univ_params() const;
    expr const & get_type() const;

    expr const & get_value() const;

    reducibility_hints const & get_hints() const;

    friend declaration mk_definition(name const & n, level_param_names const & params, expr const & t, expr const & v,
                                     reducibility_hints const & hints, bool trusted);
    friend declaration mk_definition(environment const & env, name const & n, level_param_names const & params, expr const & t,
                                     expr const & v, bool use_conv_opt, bool trusted);
    friend declaration mk_theorem(name const &, level_param_names const &, expr const &, expr const &);
    friend declaration mk_axiom(name const & n, level_param_names const & params, expr const & t);
    friend declaration mk_constant_assumption(name const & n, level_param_names const & params, expr const & t, bool trusted);
};

inline optional<declaration> none_declaration() { return optional<declaration>(); }
inline optional<declaration> some_declaration(declaration const & o) { return optional<declaration>(o); }
inline optional<declaration> some_declaration(declaration && o) { return optional<declaration>(std::forward<declaration>(o)); }

declaration mk_definition(name const & n, level_param_names const & params, expr const & t, expr const & v,
                          reducibility_hints const & hints, bool trusted = true);
declaration mk_definition(environment const & env, name const & n, level_param_names const & params, expr const & t, expr const & v,
                          bool use_conv_opt = true, bool trusted = true);
declaration mk_theorem(name const & n, level_param_names const & params, expr const & t, expr const & v);
declaration mk_axiom(name const & n, level_param_names const & params, expr const & t);
declaration mk_constant_assumption(name const & n, level_param_names const & params, expr const & t, bool trusted = true);

/** \brief Similar to mk_definition but infer the value of trusted flag.
    That is, set it to false if \c t or \c v contains a untrusted declaration. */
declaration mk_definition_inferring_trusted(environment const & env, name const & n, level_param_names const & params,
                                            expr const & t, expr const & v, reducibility_hints const & hints);
declaration mk_definition_inferring_trusted(environment const & env, name const & n, level_param_names const & params,
                                            expr const & t, expr const & v, bool use_self_opt);
/** \brief Similar to mk_constant_assumption but infer the value of trusted flag.
    That is, set it to false if \c t or \c v contains a untrusted declaration. */
declaration mk_constant_assumption_inferring_trusted(environment const & env, name const & n,
                                                     level_param_names const & params, expr const & t);

void initialize_declaration();
void finalize_declaration();
}
