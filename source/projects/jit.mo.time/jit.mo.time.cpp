/// @file
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Rob Ramirez
///	@license	Usage of this file and its contents is governed by the MIT License

#include "jit.mo.common.h"
#include "jit.mo.singleton.h"

// taken from: https://github.com/stegu/perlin-noise
#include "noise1234.h"

using namespace c74::min;
using namespace c74::max;
using namespace jit_mo;

void jit_mo_time_update_anim(t_object *job, t_atom *a);

namespace timemodes {
    static const symbol accum = "accum";
    static const symbol delta = "delta";
    static const symbol function = "function";
};

class jit_mo_time : public object<jit_mo_time>, matrix_operator {
public:
    
    MIN_DESCRIPTION { "Output time values using specified mode." };
    MIN_TAGS		{ "jit.mo, Generators" };
    MIN_AUTHOR		{ "Cycling '74" };
    MIN_RELATED		{ "jit.mo.join, jit.mo.field, jit.mo.func" };
    
    
    jit_mo_time(const atoms& args = {}) {
        patcher = (t_object *)gensym("#P")->s_thing;
    }
    
    ~jit_mo_time() {
        if(implicit)
            jit_mo_singleton::instance().remove_animob(m_maxobj);

        jit_object_free(animator);
    }

    attribute<bool> enable { this, "enable", true, title {"Enable Animation"} };
    
    attribute<symbol> mode { this, "mode", timemodes::accum,
        title {"Time Output Mode"},
        description { "The time output mode." },
        range { timemodes::accum, timemodes::delta, timemodes::function }
    };
    
    attribute<symbol> functype { this, "functype", functypes::line,
        title {"Function Type"},
        description { "The fuction type used when mode = function." },
        range {functypes::line, functypes::sin, functypes::saw, functypes::tri, functypes::perlin}
    };
    
    attribute<time_value> interval { this, "interval", 0., title {"Timing Interval"}, };
    
    attribute<bool> loop { this, "loop", true, title {"Loop"}, description { "Enable and disable phase looping when animating (default = 1)" } };
    
    attribute<double> scale { this, "scale", 1, title {"Scale"}, description { "Output multiplier (default = 1.0)." } };
    
    attribute<double> freq { this, "freq", 1, title {"Frequency"}, description { "Output frequency (default = 1.0)." } };
    
    attribute<double> phase { this, "phase", 0, title {"Phase"}, description { "Output phase offset (default = 0.0)." } };
    
    attribute<double> speed { this, "speed", 0, title {"Speed"}, description { "Animation speed multiplier (default = 0.0)." } };
    
    attribute<double> offset { this, "offset", 0, title {"Offset"}, description { "Output offset (default = 0.0)." } };
    
    attribute<double> delta { this, "delta", 0, title {"Delta Time"},
        description { "Frame delta time for animating graph (default = 0.0). When bound to <o>jit.mo.join</o> this value is set automatically." },
        setter { MIN_FUNCTION {
            double val = args[0];
            phase = update_phase_frome_delta(val, phase, speed, loop);
            return args;
        }}
    };
    
    attribute<double> start { this, "start", -1., title {"Start Line"}, description { "Line function start (default = -1.0)." } };
    
    attribute<double> end { this, "end", 1., title {"End Line"}, description { "Line function end (default = 1.0)." } };
    
    attribute<double> rand_amt { this, "rand_amt", 0, title {"Random Amount"}, description { "Scales the random offset value (default = 0.0)." } };
    
    attribute<long> period { this, "period", 8, title {"Period Length"},
        description { "The period length for the perlin noise function (default = 8)." },
        setter { MIN_FUNCTION {
            long v = args[0];
            return { std::max(v, 1L) };
        }}
    };
    
    message reset { this, "reset", MIN_FUNCTION {
        accum_time = 0.;
       return {};
    }};
    
    void update_animation(t_atom *av) {
        if(enable) {
            if(i_s.update(speed, interval)) {
                object_attr_touch(maxob_from_jitob(m_maxobj), gensym("speed"));
            }
            
            double val = atom_getfloat(av);
            
            if(mode == timemodes::accum) {
                accum_time += (val*speed);
                val = accum_time;
            }
            else if(mode == timemodes::delta) {
                val *= speed;
            }
            else {
                delta = val;
                double norm = phase / 2.;
                
                if(functype == functypes::saw) {
                    val = fmod(norm * freq, 1.);
                }
                else if(functype == functypes::sin) {
                    val = (norm * 2. - 1.) * freq;
                    val = sin(val*M_PI);
                }
                else if (functype == functypes::tri) {
                    val = norm * freq * 2.0;
                    val = math::fold(val, 0., 1.);
                    val = (val * 2.0 - 1.0);
                }
                else if (functype == functypes::line) {
                    if(norm == 0.)
                        val = start;
                    else if(norm == 1.)
                        val = end;
                    else
                        val = (start*(1.-norm) + end*norm);
                }
                else if (functype == functypes::perlin) {
                    val = (fmod(norm * 2. * freq, 2.0) - 1.) * (double)period;
                    val = pnoise1(val, period);
                }
                
                if (rand_amt != 0.0) {
                    val += math::random(-1., 1.)*rand_amt;
                }
                
                val *= scale;
                val += offset;
            }
            
            outlet_float(timeoutlet, val);
        }
    }

    template<class matrix_type, size_t planecount>
    cell<matrix_type,planecount> calc_cell(cell<matrix_type,planecount> input, const matrix_info& info, matrix_coord& position) {
        cell<matrix_type,planecount> output;
        return output;
    }

private:

    message jitclass_setup { this, "jitclass_setup", MIN_FUNCTION {
        t_class *c = args[0];
        
        jit_class_addmethod(c, (method)jit_mo_time_update_anim, "update_anim", A_CANT, 0);
        jit_class_addinterface(c, jit_class_findbyname(gensym("jit_anim_animator")), calcoffset(minwrap<jit_mo_time>, min_object) + calcoffset(jit_mo_time, animator), 0);
        
        return {};
    }};
    
    message maxclass_setup { this, "maxclass_setup", MIN_FUNCTION {
        t_class *c = args[0];
        max_jit_class_wrap_standard(c, this_jit_class, 0);
        return {};
    }};
    
    message setup { this, "setup", MIN_FUNCTION {
        animator = jit_object_new(gensym("jit_anim_animator"), m_maxobj);
        //attr_addfilterset_proc(object_attr_get(animator, symbol("automatic")), (method)jit_mo_time_automatic_attrfilter);
    
        return {};
    }};
    
    message mop_setup { this, "mop_setup", MIN_FUNCTION {
        void *o = m_maxobj;
        t_object *x = args[args.size()-1];

        max_jit_obex_jitob_set(x,o);
        max_jit_obex_dumpout_set(x,outlet_new(x,NULL));
        timeoutlet = outlet_new(x,NULL);
        
        return {};
    }};
    
    message maxob_setup { this, "maxob_setup", MIN_FUNCTION {
        long atm = object_attr_getlong(m_maxobj, gensym("automatic"));
        
        if(atm && object_attr_getsym(m_maxobj, sym_drawto) == _jit_sym_nothing) {
            jit_mo_singleton::instance().add_animob(m_maxobj, patcher);
            implicit = true;
        }
    
        return {};
    }};
    
    message notify { this, "notify", MIN_FUNCTION {
        symbol s = args[2];
        if(s == sym_dest_closing) {
            if(implicit) {
                object_attr_setsym(animator, sym_drawto, _jit_sym_nothing);
                jit_mo_singleton::instance().add_animob(m_maxobj, patcher);
            }
        }
        return { JIT_ERR_NONE };
    }};
    
private:
    t_object*   patcher = nullptr;
    t_object*   animator = nullptr;
    void*       timeoutlet = nullptr;
    bool        implicit = false;
    double      accum_time = 0.;
    
    interval_speed i_s;
};

MIN_EXTERNAL(jit_mo_time);

void jit_mo_time_update_anim(t_object *job, t_atom *a)
{
    minwrap<jit_mo_time>* self = (minwrap<jit_mo_time>*)job;
    self->min_object.update_animation(a);
}
