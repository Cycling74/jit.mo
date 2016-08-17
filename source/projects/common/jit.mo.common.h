/// @file
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Rob Ramirez
///	@license	Usage of this file and its contents is governed by the MIT License

#pragma once

#include "c74_min.h"

namespace jit_mo {
    
    static const c74::min::symbol sym_automatic = "automatic";
    static const c74::min::symbol sym_bang = "bang";
    static const c74::min::symbol sym_delta = "delta";
    static const c74::min::symbol sym_maxwrapper = "maxwrapper";
    static const c74::min::symbol sym_drawto = "drawto";
    static const c74::min::symbol sym_dest_closing = "dest_closing";

    void max_jit_mo_addfuncob(c74::min::max_jit_wrapper *mob, c74::max::t_object *ob);
    void max_jit_mo_removefuncob(c74::min::max_jit_wrapper *mob, c74::max::t_object *ob);

    void check_animobs_defer();

    // static singleton class to manage unbound mo.join and mo.func objects
    // mo.func obs are checked when any jit.mo.join name attribute is set.
    // mo.join obs are checked by a timer which walks the patcher looking for valid gl contexts

    class jit_mo_singleton {
        
    public:
        
        c74::min::timer metro {nullptr, MIN_FUNCTION {
            c74::max::defer_low(nullptr, (c74::max::method)check_animobs_defer, nullptr, 0, nullptr);
            return {};
        }};
        
        void add_animob(c74::max::t_object *o, c74::max::t_object *patcher);
        
        void remove_animob(c74::max::t_object *o);
        
        void check_animobs();
        
        
        void add_funcob(c74::max::t_object *o);
        
        void remove_funcob(c74::max::t_object *o);
        
        void check_funcobs(c74::max::t_symbol* joinname);
        
        
        static jit_mo_singleton &instance();
        
    private:
        jit_mo_singleton() {}
        
        const long metro_interval = 500;
        std::unordered_map<c74::max::t_object*, c74::max::t_object*> animobs;
        std::vector <c74::max::t_object*>funcobs;
        
    };
}