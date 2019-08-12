/// @file
///	@copyright	Copyright 2018 The Jit.Mo Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#include "jit.mo.common.h"
#include "noise1234.h"

using namespace c74::min;
using namespace c74::max;
using namespace jit_mo;

class jit_mo_func : public object<jit_mo_func>, public matrix_operator<> {
public:
	MIN_DESCRIPTION {"Generate animated single dim matrices using a specified function. "
					"Similar in nature to a sound oscillator, "
					"jit.mo.func can generate time-varying cell values across a matrix based on a given function."};
	MIN_TAGS		{"jit.mo, Generators"};
	MIN_AUTHOR		{"Cycling '74"};
	MIN_RELATED		{"jit.mo.join, jit.mo.field, jit.mo.time, jit.anim.drive, jit.anim.path"};

	outlet<> output	{this, "(matrix) Output", "matrix"};

	argument<number> dimarg {this, "Dimension",
		"Set the dimension (number of elements) of the output matrix. Will be overriden if to attached [jit.mo.join] "
		"object via the @join attribute. jit.mo objects only support matrices with a dimcount of 1.",
		false, nullptr};

	~jit_mo_func() {
		if (!(join == symbol())) {
			update_parent(symbol());
		}

		if (unbound) {
			t_object* mojoin = (t_object*)newinstance(symbol("jit.mo.join"), 0, NULL);
			if (mojoin) {
				object_method(mojoin, gensym("removefuncob"), maxobj());
				object_free(mojoin);
			}
		}
	}

	attribute<symbol> function {this, "function", functypes::line,
		title {"Function Type"},
		description {"The fuction type used for generating matrices. Available functypes are line, sin, saw, tri, perlin"},
		range {functypes::line, functypes::sin, functypes::saw, functypes::tri, functypes::perlin}
	};

	attribute<bool> loop {this, "loop", true,
		title {"Loop"},
		description {"Enable and disable phase looping when animating (default = 1). Non-looped animation can be reset "
					"by setting phase to 0"}
	};

	attribute<double> scale {this, "scale", 1,
		title {"Scale"},
		description {"Output multiplier (default = 1.0)."}
	};

	attribute<double> freq {this, "freq", 1,
		title {"Frequency"},
		description {
			"Output frequency (default = 1.0). Number of times the function is repeated over the width of the matrix"
		}
	};

	attribute<double> phase {this, "phase", 0,
		title {"Phase"},
		description {"Output phase offset (default = 0.0)."}
	};

	attribute<double> speed {this, "speed", 0,
		title {"Speed"},
		description {"Animation speed multiplier (default = 0.0)."}
	};

	attribute<double> offset {this, "offset", 0,
		title {"Offset"},
		description {"Output offset (default = 0.0)."}
	};

	attribute<double> delta {this, "delta", 0,
		title {"Delta Time"},
		description {
			"Frame delta time for animating graph (default = 0.0). "
            "When bound to [jit.mo.join] this value is set automatically."
		},
		setter { MIN_FUNCTION {
			if (initialized()) {
				double val  = args[0];
				double pval = phase + (val * speed * 2.0);    // default is one cycle / second

				if (loop || (pval < 2.0 && pval > -2.)) {
					phase = std::fmod(pval, 2.0);
					object_attr_touch(maxob_from_jitob(maxobj()), sym_phase);
				}
				else if(speed < 0) {
					phase = -2.0;
				}
				else {
					phase = 2.0;
				}
			}
			return args;
		}}
	};

	attribute<double> start {this, "start", -1.,
		title {"Start Line"},
		description {"Line function start (default = -1.0)."}
	};

	attribute<double> end {this, "end", 1.,
		title {"End Line"},
		description {"Line function end (default = 1.0)."}
	};

	attribute<double> rand_amt {this, "rand_amt", 0,
		title {"Random Amount"},
		description {"Scales the random offset value (default = 0.0)."}
	};

	attribute<int> period {this, "period", 8,
		title {"Period Length"},
		description {"The period length for the perlin noise function (default = 8)."},
		setter { MIN_FUNCTION {
			int v = args[0];
			return {std::max(v, 1)};
		}}
	};

	attribute<symbol> join {this, "join", _jit_sym_nothing,
		title {"Join name"},
		description {
			"Sets the [jit.mo.join] object binding. "
            "When set, animation parameters are controlled by the named object."
		},
		setter { MIN_FUNCTION {
			update_parent(args[0]);
			return args;
		}}
	};

	message<> rand = {this, "rand",
		MIN_FUNCTION {
			reseed = true;
			return {};
		},
		"Generate new random values for @rand_amt offset.",
		message_type::defer_low
	};

	// TODO: multiplane
	template<class matrix_type, size_t planecount>
	cell<matrix_type, planecount> calc_cell(cell<matrix_type, planecount> input, const matrix_info& info, matrix_coord& position) {
		cell<matrix_type, planecount> output;
		double                        val  = 0;
		double                        norm = (info.m_out_info->dim[0] > 1 ? (double)position.x() / (double)(info.m_out_info->dim[0] - 1) : 0);

		if (function == functypes::saw) {
			val = fmod(norm * freq + phase, 1.);
		}
		else if (function == functypes::sin) {
			val = (norm * 2. - 1.) * freq + phase;
			val = sin(val * M_PI);
		}
		else if (function == functypes::tri) {
			val = norm * freq * 2.0 + phase;
			val = fold(val, 0., 1.);
			val = (val * 2.0 - 1.0);
		}
		else if (function == functypes::line) {
			if (norm == 0.)
				val = start;
			else if (norm == 1.)
				val = end;
			else
				val = (start * (1. - norm) + end * norm);
		}
		else if (function == functypes::perlin) {
			val = (fmod(norm * 2. * freq + phase, 2.0) - 1.) * (double)period;
			val = pnoise1(val, period);
		}

		if (rand_amt != 0.0) {
			if (position.x() >= randvals.size())
				randvals.push_back(lib::math::random(-1., 1.));
			else if (reseed)
				randvals[position.x()] = lib::math::random(-1., 1.);

			val += randvals[position.x()] * rand_amt;

			if (position.x() == info.m_out_info->dim[0] - 1)
				reseed = false;
		}

		val *= scale;
		val += offset;

		output[0] = val;
		return output;
	}

private:
	message<> setup {this, "setup", MIN_FUNCTION {
	   if (classname() == "jit.mo.line")
		   jit_mo_func::function = functypes::line;
	   else if (classname() == "jit.mo.tri")
		   jit_mo_func::function = functypes::tri;
	   else if (classname() == "jit.mo.sin")
		   jit_mo_func::function = functypes::sin;
	   else if (classname() == "jit.mo.saw")
		   jit_mo_func::function = functypes::saw;
	   else if (classname() == "jit.mo.perlin")
		   jit_mo_func::function = functypes::perlin;
	   return {};
	}};

	message<> maxob_setup {this, "maxob_setup", MIN_FUNCTION {
	   t_object* mob = maxob_from_jitob(maxobj());
	   long      dim = object_attr_getlong(mob, _jit_sym_dim);

	   if (join == symbol())
		   object_attr_setlong(mob, _jit_sym_dim, args.size() > 0 ? (long)args[0] : dim);
	   object_attr_setlong(mob, _jit_sym_planecount, 1);
	   object_attr_setsym(mob, _jit_sym_type, _jit_sym_float32);
	   return {};
   }};

	message<> fileusage {this, "fileusage", MIN_FUNCTION {
	   jit_mo::fileusage(args[0]);
	   return {};
	}};

	void update_parent(symbol name) {
		symbol curjoin = join;

		if (!(curjoin == symbol(_jit_sym_nothing))) {
			t_object* o = (t_object*)object_findregistered(_jit_sym_jitter, curjoin);
			if (o) {
				object_method(o, gensym("detach"), maxobj());
			}
		}

		if (!(name == symbol(_jit_sym_nothing))) {
			bool      success = false;
			t_object* o       = (t_object*)object_findregistered(_jit_sym_jitter, name);
			if (o) {
				if (JIT_ERR_NONE == (t_jit_err)object_method(o, gensym("attach"), maxobj())) {
					success = true;
				}
			}

			if (!success) {
				// add to global list that's checked in join name attr setter
				t_object* mojoin = (t_object*)newinstance(symbol("jit.mo.join"), 0, NULL);
				if (mojoin) {
					unbound = true;
					object_method(mojoin, gensym("addfuncob"), maxobj());
					object_free(mojoin);
				}
			}
		}
	}

	std::vector<double> randvals;
	bool                reseed  {true};
	bool                unbound {false};
};

MIN_EXTERNAL(jit_mo_func);
