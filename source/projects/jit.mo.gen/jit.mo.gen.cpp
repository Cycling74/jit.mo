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

	outlet	output	= { this, "(matrix) Output", "matrix" };

	c74::min::method setup = { this, "setup", MIN_FUNCTION {
		if (classname() == "jit.mo.line")
			type = functypes::line;
		else if (classname() == "jit.mo.tri")
			type = functypes::tri;
		else if (classname() == "jit.mo.sin")
			type = functypes::sin;
		return {};
	}};

	attribute<symbol> type = {
		this,
		"type",
		functypes::function,
		title {"Function Type"},
		range {functypes::function, functypes::line, functypes::sin, functypes::tri}
	};

	attribute<double> scale = { this, "scale", 1, title {"Scale"} };

	attribute<double> freq = { this, "freq", 1, title {"Frequency"} };

	attribute<double> phase = { this, "phase", 0, title {"Phase"} };

	attribute<double> speed = { this, "speed", 0, title {"Speed"} };

	attribute<double> offset = { this, "offset", 0, title {"Offset"} };

	attribute<double> delta = { this, "delta", 0, title {"Delta Time"} };

	attribute<double> start = { this, "start", -1., title {"Start Line"} };

	attribute<double> end = { this, "end", 1., title {"End Line"} };

	attribute<double> rand_amt = { this, "rand_amt", 0, title {"Random Amount"} };

	c74::min::method rand = { this, "rand", MIN_FUNCTION {
		reseed = true;
		return {};
	}};

	c74::min::method maxob_setup = { this, "maxob_setup", MIN_FUNCTION {
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
		phase = phase + (delta * speed);
		phase = std::fmod(phase, 2.0);

		if(type == functypes::sin) {
			val = (norm * 2. - 1.) * freq + phase;
			val = sin(val*M_PI);
		}
		else if (type == functypes::tri) {
			val = norm * freq * 2.0 + phase;
			val = foldit(val, 0., 1.);
			val = (val * 2.0 - 1.0);
		}
		else if (type == functypes::line) {
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

	static double foldit(double x, double lo, double hi) {
		long di;
		double m,d,tmp;

		if (lo > hi) {
			tmp = lo;
			lo  = hi;
			hi  = tmp;
		}
		if (lo)
			x -= lo;
		m = hi-lo;
		if (m) {
			if (x < 0.)
				x = -x;
			if (x>m) {
				if (x>(m*2.)) {
					d = x / m;
					di = (long) d;
					d = d - (double) di;
					if (di%2) {
						if (d < 0) {
							d = -1. - d;
						} else {
							d = 1. - d;
						}
					}
					x = d * m;
					if (x < 0.)
						x = m+x;
				} else {
					x = m-(x-m);
				}
			}
		} else x = 0.; //don't divide by zero

		return x + lo;
	}

	std::vector<double>	randvals;
	bool reseed = true;
};

MIN_EXTERNAL(jit_mo_gen);
