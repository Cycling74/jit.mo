/// @file	
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Rob Ramirez
///	@license	Usage of this file and its contents is governed by the MIT License

#include "jit.mo.common.h"

// taken from: https://github.com/stegu/perlin-noise
#include "noise1234.h"

using namespace c74::min;
using namespace c74::max;
using namespace jit_mo;

class jit_mo_func : public object<jit_mo_func>, matrix_operator {
public:

    MIN_DESCRIPTION { "Generate single dim matrices using specified function." };
    MIN_TAGS		{ "jit.mo, Generators" };
    MIN_AUTHOR		{ "Cycling '74" };
    MIN_RELATED		{ "jit.mo.join, jit.mo.field" };
    
	outlet	output	= { this, "(matrix) Output", "matrix" };

    jit_mo_func(const atoms& args = {}) {}
    
    ~jit_mo_func() {
    
        if(!(join == symbol())) {
            update_parent(symbol());
        }
    
        if(unbound) {
            t_object *mojoin = (t_object*)newinstance(symbol("jit.mo.join"), 0, NULL);
            if(mojoin) {
                object_method(mojoin, gensym("removefuncob"), m_maxobj);
                object_free(mojoin);
            }
        }
    }

	attribute<symbol> functype { this, "functype", functypes::line, title {"Function Type"},
        description { "The fuction type used for generating matrices." },
		range {functypes::line, functypes::sin, functypes::saw, functypes::tri, functypes::perlin}
	};

    attribute<bool> loop { this, "loop", true, title {"Loop"},
        description { "Enable and disable phase looping when animating (default = 1)" }
    };
    
	attribute<double> scale { this, "scale", 1, title {"Scale"},
        description { "Output multiplier (default = 1.0)." }
    };

	attribute<double> freq { this, "freq", 1, title {"Frequency"},
        description { "Output frequency (default = 1.0)." }
    };

	attribute<double> phase { this, "phase", 0, title {"Phase"},
        description { "Output phase offset (default = 0.0)." }
    };

	attribute<double> speed { this, "speed", 0, title {"Speed"},
        description { "Animation speed multiplier (default = 0.0)." }
    };

	attribute<double> offset { this, "offset", 0, title {"Offset"},
        description { "Output offset (default = 0.0)." }
    };

	attribute<double> delta { this, "delta", 0, title {"Delta Time"},
        description {
            "Frame delta time for animating graph (default = 0.0). \
            When bound to <o>jit.mo.join</o> this value is set automatically."
        },
		setter { MIN_FUNCTION {
            double val = args[0];
            double pval = phase + (val * speed * 2.0); // default is one cycle / second
            
            if(loop || pval < 2.0) {
                phase = std::fmod(pval, 2.0);
                object_attr_touch(maxob_from_jitob(m_maxobj), sym_phase);
            }
			return args;
		}}
	};

	attribute<double> start { this, "start", -1., title {"Start Line"},
        description { "Line function start (default = -1.0)." }
    };

	attribute<double> end { this, "end", 1., title {"End Line"},
        description { "Line function end (default = 1.0)." }
    };

	attribute<double> rand_amt { this, "rand_amt", 0, title {"Random Amount"},
        description { "Scales the random offset value (default = 0.0)." }
    };
    
    attribute<long> period { this, "period", 8, title {"Period Length"},
        description { "The period length for the perlin noise function (default = 8)." },
        setter { MIN_FUNCTION {
            long v = args[0];
            return { std::max(v, 1L) };
        }}
    };
    
    attribute<symbol> join { this, "join", _jit_sym_nothing, title {"Join name"},
        description {
            "Sets the <o>jit_mo_join</o> object binding. \
            When set, animation parameters are controlled by the named object."
        },
        setter { MIN_FUNCTION {
            update_parent(args[0]);
            return args;
        }}
    };
    
	message rand = { this, "rand",
        "Generate new random values for rand_amt offset.",
        MIN_FUNCTION {
            reseed = true;
            return {};
        }
    };

    message setup = { this, "setup", MIN_FUNCTION {
        if (classname() == "jit.mo.line")
            functype = functypes::line;
        else if (classname() == "jit.mo.tri")
            functype = functypes::tri;
        else if (classname() == "jit.mo.sin")
            functype = functypes::sin;
        else if (classname() == "jit.mo.saw")
            functype = functypes::saw;
        else if (classname() == "jit.mo.perlin")
            functype = functypes::perlin;
        return {};
    }};

	message maxob_setup = { this, "maxob_setup", MIN_FUNCTION {
		t_object *mob=maxob_from_jitob(m_maxobj);
		long dim = object_attr_getlong(mob, _jit_sym_dim);
        
        if(join == symbol())
            object_attr_setlong(mob, _jit_sym_dim, args.size()>0 ? (long)args[0] : dim);
        object_attr_setlong(mob, _jit_sym_planecount, 1);
        object_attr_setsym(mob, _jit_sym_type, _jit_sym_float32);

		/*if(args.size() < 1)
			object_attr_setlong(mob, _jit_sym_planecount, 1);
		if(args.size() < 2)
			object_attr_setsym(mob, _jit_sym_type, _jit_sym_float32);
		if(args.size() < 3)
			object_attr_setlong(mob, _jit_sym_dim, 10);*/

		return {};
	}};

	// TODO: multiplane
	template<class matrix_type, size_t planecount>
	cell<matrix_type,planecount> calc_cell(cell<matrix_type,planecount> input, const matrix_info& info, matrix_coord& position) {
		cell<matrix_type,planecount> output;
		double val = 0;
		double norm = (double)position.x() / (double)(info.out_info->dim[0]-1);

        if(functype == functypes::saw) {
            val = fmod(norm * freq + phase, 1.);
        }
        else if(functype == functypes::sin) {
			val = (norm * 2. - 1.) * freq + phase;
			val = sin(val*M_PI);
		}
		else if (functype == functypes::tri) {
			val = norm * freq * 2.0 + phase;
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
            val = (fmod(norm * 2. * freq + phase, 2.0) - 1.) * (double)period;
            val = pnoise1(val, period);
		}

		if (rand_amt != 0.0) {
			if(position.x() >= randvals.size())
				randvals.push_back(math::random(-1., 1.));
			else if(reseed)
				randvals[position.x()] = math::random(-1., 1.);

			val += randvals[position.x()]*rand_amt;

			if(position.x() == info.out_info->dim[0]-1)
				reseed = false;
		}
		
		val *= scale;
		val += offset;
		
		output[0] = val;
		return output;
	}

private:

    void update_parent (symbol name) {
        symbol curjoin = join;
        
        if(!(curjoin == symbol(_jit_sym_nothing))) {
            t_object *o = (t_object*)object_findregistered(_jit_sym_jitter, curjoin);
            if(o) {
                object_method(o, gensym("detach"), m_maxobj);
            }
        }
        
        if(!(name == symbol(_jit_sym_nothing))) {
            bool success = false;
            t_object *o = (t_object*)object_findregistered(_jit_sym_jitter, name);
            if(o) {
                if(JIT_ERR_NONE == (t_jit_err)object_method(o, gensym("attach"), m_maxobj)) {
                    success = true;
                }
            }
            
            if(!success) {
                // add to global list that's checked in join name attr setter
                t_object *mojoin = (t_object*)newinstance(symbol("jit.mo.join"), 0, NULL);
                if(mojoin) {
                    unbound = true;
                    object_method(mojoin, gensym("addfuncob"), m_maxobj);
                    object_free(mojoin);
                }
            }
        }
    }

	std::vector<double>	randvals;
	bool                reseed = true;
    bool                unbound = false;
};

MIN_EXTERNAL(jit_mo_func);
