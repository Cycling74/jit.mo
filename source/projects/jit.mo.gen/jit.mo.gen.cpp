/// @file	
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Rob Ramirez
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"

using namespace c74::min;
using namespace c74::max;

namespace functypes {
	static const symbol sin = "sin";
	static const symbol line = "line";
	static const symbol tri = "tri";
	static const symbol function = "function";
};

class jit_mo_gen : public object<jit_mo_gen>, matrix_operator {
public:

    MIN_DESCRIPTION { "Generate single dim matrices using specified function." };
    MIN_TAGS		{ "jit.mo, Generators" };
    MIN_AUTHOR		{ "Cycling '74" };
    MIN_RELATED		{ "jit.mo.join, jit.mo.field" };
    
	outlet	output	= { this, "(matrix) Output", "matrix" };

	message setup = { this, "setup", MIN_FUNCTION {
		if (classname() == "jit.mo.line")
			functype = functypes::line;
		else if (classname() == "jit.mo.tri")
			functype = functypes::tri;
		else if (classname() == "jit.mo.sin")
			functype = functypes::sin;
		return {};
	}};

	attribute<symbol> functype { this, "functype", functypes::function,
		title {"Function Type"},
        description { "The fuction type used for generating matrices." },
		range {functypes::function, functypes::line, functypes::sin, functypes::tri}
	};

	attribute<double> scale { this, "scale", 1, title {"Scale"}, description { "Output multiplier (default = 1.0)." } };

	attribute<double> freq { this, "freq", 1, title {"Frequency"}, description { "Output frequency (default = 1.0)." } };

	attribute<double> phase { this, "phase", 0, title {"Phase"}, description { "Output phase offset (default = 0.0)." } };

	attribute<double> speed { this, "speed", 0, title {"Speed"}, description { "Animation speed multiplier (default = 0.0)." } };

	attribute<double> offset { this, "offset", 0, title {"Offset"}, description { "Output offset (default = 0.0)." } };

	attribute<double> delta { this, "delta", 0, title {"Delta Time"},
        description { "Frame delta time for animating graph (default = 0.0). When bound to <o>jit.mo.join</o> this value is set automatically." },
		setter { MIN_FUNCTION {
			double val = args[0];
			phase = phase + (val * speed * 2.0); // default is one cycle / second	
			phase = std::fmod(phase, 2.0);
			return args;
		}}
	};

	attribute<double> start { this, "start", -1., title {"Start Line"}, description { "Line function start (default = -1.0)." } };

	attribute<double> end { this, "end", 1., title {"End Line"}, description { "Line function end (default = 1.0)." } };

	attribute<double> rand_amt { this, "rand_amt", 0, title {"Random Amount"}, description { "Scales the random offset value (default = 0.0)." } };
    
	message rand = { this, "rand", "Generate new random values for rand_amt offset.",
        MIN_FUNCTION {
            reseed = true;
            return {};
        }
    };

	message maxob_setup = { this, "maxob_setup", MIN_FUNCTION {
		t_object *mob=NULL;
		object_obex_lookup(m_maxobj, gensym("maxwrapper"), &mob);
		if (args.size() < 3) {
			object_attr_setlong(mob, _jit_sym_dim, args.size()>1 ? (long)args[1] : 1);
			object_attr_setlong(mob, _jit_sym_planecount, args.size()>0 ? (long)args[0] : 1);
			object_attr_setsym(mob, _jit_sym_type, _jit_sym_float32);
		}

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

		if(functype == functypes::sin) {
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
		else {
			
		}

		if(rand_amt) {
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


	std::vector<double>	randvals;
	bool reseed = true;
};

MIN_EXTERNAL(jit_mo_gen);
