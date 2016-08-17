/// @file
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Rob Ramirez
///	@license	Usage of this file and its contents is governed by the MIT License

#include "jit.mo.common.h"
#include "c74_ui.h"
#include "c74_ui_graphics.h"

using namespace c74::min;
using namespace c74::max;
using namespace jit_mo;

void jit_mo::max_jit_mo_addfuncob(max_jit_wrapper *mob, t_object *ob)
{
    jit_mo_singleton::instance().add_funcob(ob);
}

void jit_mo::max_jit_mo_removefuncob(max_jit_wrapper *mob, t_object *ob)
{
    jit_mo_singleton::instance().remove_funcob(ob);
}

void jit_mo::check_animobs_defer() {
    jit_mo_singleton::instance().check_animobs();
}

void jit_mo_singleton::add_funcob(t_object *o) {
    funcobs.push_back(o);
}

void jit_mo_singleton::remove_funcob(t_object *o) {
    int i = 0;
    for(auto a : funcobs) {
        if(a == o) {
            funcobs.erase(funcobs.begin() + i);
            break;
        }
        i++;
    }
}

jit_mo_singleton &jit_mo_singleton::instance() {
    static jit_mo_singleton s_instance;
    return s_instance;
}

void jit_mo_singleton::check_funcobs(t_symbol* joinname) {
    if(funcobs.size()) {
        typedef std::vector <t_object*>::iterator fobitr;
        
        fobitr iter = funcobs.begin();
        while (iter != funcobs.end()) {
            t_object *o = *iter;
            t_symbol* joinsym = object_attr_getsym(o, gensym("join"));
            if(joinname == joinsym) {
                t_object *joinob = (t_object*)object_findregistered(_jit_sym_jitter, joinname);
                if(joinob) {
                    object_method(joinob, gensym("attach"), o);
                    iter = funcobs.erase(iter);
                    continue;
                }
            }
            ++iter;
        }
    }
}

void jit_mo_singleton::add_animob(t_object *o, t_object *patcher) {
    animobs.insert({o, patcher});
    if(animobs.size() == 1)
        metro.delay(metro_interval);
}

void jit_mo_singleton::remove_animob(t_object *o) {
    animobs.erase(o);
}

t_symbol * find_context(t_object *patcher, t_symbol *classname, t_object **parent) {
    *parent = 0;
    
    if(patcher) {
        t_object *jbox = jpatcher_get_firstobject(patcher);
        while(jbox) {
            t_object *o = jbox_get_object(jbox);
            t_symbol *oname = object_classname(o);
            if(oname == classname) {
                if(classname == gensym("jit.world"))
                    return object_attr_getsym(o, sym_drawto);
                else
                    return object_attr_getsym(o, gensym("name"));
            }
            else {
                t_object *jitob = (t_object *)max_jit_obex_jitob_get(o);
                if(jitob) {
                    t_symbol *cname = object_classname(jitob);
                    if(cname == classname) {
                        return object_attr_getsym(o, gensym("name"));
                    }
                }
            }
            
            jbox = jbox_get_nextobject(jbox);
        }
        *parent = object_attr_getobj(patcher, gensym("parentpatcher"));
    }
    return _jit_sym_nothing;
}

void jit_mo_singleton::check_animobs() {
    t_object *patcher = nullptr, *prevpatcher = nullptr, *ppatcher = nullptr;
    t_symbol *name = _jit_sym_nothing;
    std::vector<t_object*> erase;
    
    for(auto a : animobs) {
        patcher = a.second;
        
        if(prevpatcher != patcher)
            name = _jit_sym_nothing;
        
        while(patcher && name == _jit_sym_nothing) {
            name = find_context(patcher, gensym("jit.world"), &ppatcher);
            if(name != _jit_sym_nothing) {
                break;
            }
            
            name = find_context(patcher, gensym("jit_window"), &ppatcher);
            if(name != _jit_sym_nothing) {
                break;
            }
            
            name = find_context(patcher, gensym("jit.pwindow"), &ppatcher);
            if(name != _jit_sym_nothing) {
                break;
            }
            patcher = ppatcher;
        }
        
        if(name != _jit_sym_nothing) {
            t_object *joinob = a.first;
            
            // attach max box to context
            t_object *mwrap=NULL;
            object_obex_lookup(joinob, sym_maxwrapper, &mwrap);
            jit_object_attach(name, mwrap);
            
            // set drawto of joinob to context name
            object_attr_setsym(joinob, sym_drawto, name);
            //std::cout << "adding to context " << name->s_name << std::endl;
            
            erase.push_back(a.first);
        }
        prevpatcher = patcher;
    }
    
    for(auto a : erase) {
        animobs.erase(a);
    }
    
    if(animobs.size() > 0)
        metro.delay(metro_interval);
}
