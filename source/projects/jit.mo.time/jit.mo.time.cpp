/// @file
///	@copyright	Copyright 2018 The Jit.Mo Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#include "jit.mo.common.h"
#include "jit.mo.singleton.h"
#include "noise1234.h"    // https://github.com/stegu/perlin-noise

using namespace c74;
using namespace c74::min;
using namespace jit_mo;

using max::t_object;
using max::t_symbol;
using max::t_atom;
using max::t_class;
using max::method;
using max::gensym;
using max::gettime;
using max::outlet_float;
using max::object_attr_getlong;
using max::max_jit_obex_jitob_get;

void jit_mo_time_update_anim(t_object* job, t_atom* a);
void max_jit_mo_time_int(max_jit_wrapper* mob, long v);
void max_jit_mo_time_bang(max_jit_wrapper* mob);
void max_jit_mo_time_assist(void* x, void* b, long m, long a, char* s);

namespace timemodes {
	static const symbol accum    = "accum";
	static const symbol delta    = "delta";
	static const symbol function = "function";
};    // namespace timemodes


class jit_mo_time : public object<jit_mo_time>, public matrix_operator<> {
public:
	MIN_DESCRIPTION {"Outputs float time values using specified mode for realtime animation. "
					"Can be used to generate control functions in sync with other jit.world and jit.mo objects, "
					"time delta between frames, or accumulated running time."};
	MIN_TAGS		{"jit.mo, Generators"};
	MIN_AUTHOR		{"Cycling '74"};
	MIN_RELATED		{"jit.mo.join, jit.mo.field, jit.mo.func, jit.anim.drive"};

	~jit_mo_time() {
		jit_object_free(animator);
		if(implicit_ctx) jit_object_free(implicit_ctx);
	}


	attribute<bool> enable {this, "enable", true,
		title {"Enable Animation"},
		description {"Enable function output (default = 1)."}
	};

	attribute<symbol> mode {this, "mode", timemodes::accum,
		title {"Time Output Mode"},
		description {
			"How time output is calculated (default = accum). The different modes are accum, function, and delta. "
			"Accum provides accumulated running time. Function uses the specified @function attribute to generate a "
			"periodic function and can be used to generate float LFOs and ramps in sync with the animation graph. "
			"Delta gives the amount of time between frames, which is useful for driving smooth realtime animations."
		},
		range {timemodes::accum, timemodes::delta, timemodes::function}
	};

	attribute<symbol> function {this, "function", functypes::line,
		title {"Function Type"},
		description {
			"The fuction type used when mode = function (default = line). "
            "Line generates linear interpolated values between @start and @end values, sin outputs a sine function, "
			"saw gives a phasor-like repeating ramp, and perlin uses a Perlin Noise function"
		},
		range {functypes::line, functypes::sin, functypes::saw, functypes::tri, functypes::perlin}
	};

	attribute<c74::min::time_value> interval {this, "interval", 0.,
		title {"Timing Interval"},
		description {
			"Animation interval (default = 0 ms). "
			"This attribute uses the Max time format syntax, so the interval can be either fixed or tempo-relative."
			"When set to a non-zero value, the speed attribute is no longer user settable, and will be automatically "
			"set based on the interval value."
		}
	};

	attribute<bool> loop {this, "loop", true,
		title {"Loop"},
		description {
			"Enable and disable phase looping when animating (default = 1). Animation can be reset by setting "
			"phase to 0"
		}
	};

	attribute<bool> loopreport {this, "loopreport", false,
		title {"Loop Report"},
		description {
			"Enable animation loop reporting (default = 0). When enabled the symbol loopnotify is sent out the dumpout "
			"when the animation loops."
		}
	};
	
	attribute<double> scale {this, "scale", 1,
		title {"Scale"},
		description {"Output multiplier (default = 1.0)."}
	};

	attribute<double> freq {this, "freq", 1,
		title {"Frequency"},
		description {"Function frequency (default = 1.0). Specified in Hz"}
	};

	attribute<double> phase {this, "phase", 0,
		title {"Phase"},
		description {"Output phase offset (default = 0.0). Setting this to 0 will restart an animation"}
	};

	attribute<double> speed {this, "speed", 1.,
		title {"Speed"},
		description {"Animation speed multiplier (default = 1.0)."},
		setter { MIN_FUNCTION {
			double val = args[0];
			if((speed >= 0) != (val >= 0)) {
				speeddirchanged = true;
			}
			return args;
		}}
	};

	attribute<double> offset {this, "offset", 0,
		title {"Offset"},
		description {"Output offset (default = 0.0)."}
	};

	attribute<double> delta {this, "delta", 0,
		title {"Delta Time"},
		description {
			"Frame delta time for animating graph (default = 0.0). "
            "When @automatic enabled this value is set automatically by the render context."
		},
		setter { MIN_FUNCTION {
			double val = args[0];
			phase      = update_phase_frome_delta(val, phase, speed, loop);
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

	message<> reset {this, "reset",
		MIN_FUNCTION {
			accum_time = 0.;
			return {};
		},
		"Reset the accumulated time to 0.",
		message_type::defer_low
	};

	message<> update {this, "update",
		MIN_FUNCTION {
			long   time = gettime();
			float  deltatime;
			t_atom av;
			if (prevtime_ms == 0)
				deltatime = 0;
			else
				deltatime = (float)(time - prevtime_ms) * .001f;
			prevtime_ms = time;
			atom_setfloat(&av, deltatime);

			return {atom(update_animation(&av))};
		},
		"Update and output the time or function value when in non-automatic mode.",
		message_type::gimmeback
	};

	double update_animation(t_atom* av) {
		if (enable) {
			if (i_s.update(speed, interval)) {
				object_attr_touch(maxob_from_jitob(maxobj()), gensym("speed"));
			}

			double val = atom_getfloat(av);

			if (mode == timemodes::accum) {
				accum_time += (val * speed);
				val = accum_time;
			}
			else if (mode == timemodes::delta) {
				val *= speed;
			}
			else {
				delta       = val;
				double norm = phase / 2.;
				
				if(loopreport && !speeddirchanged) {
					bool doreport = false;
					if(loop) {
						if((speed > 0. && norm < prevnormval) || (speed < 0. && norm > prevnormval)) {
							doreport = true;
						}
					}
					else if(norm != prevnormval) {
						if((speed > 0. && norm == 1.) || (speed < 0. && norm == 0.)) {
							doreport = true;
						}
					}
					if(doreport) {
						outlet_anything(dumpoutlet, gensym("loopnotify"), 0, NULL);
					}
				}
				speeddirchanged = false;
				prevnormval = norm;

				if (function == functypes::saw) {
					if (norm == 0.)
						val = 0.;
					else if (norm == 1.)
						val = 1.;
					else
						val = fmod(norm * freq, 1.);
				}
				else if (function == functypes::sin) {
					val = (norm * 2. - 1.) * freq;
					val = sin(val * M_PI);
				}
				else if (function == functypes::tri) {
					val = norm * freq * 2.0;
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
					val = (fmod(norm * 2. * freq, 2.0) - 1.) * (double)period;
					val = pnoise1(val, period);
				}

				if (rand_amt != 0.0) {
					val += lib::math::random(-1., 1.) * rand_amt;
				}

				val *= scale;
				val += offset;
			}

			outlet_float(timeoutlet, val);
			return val;
		}
		return 0.;
	}

	template<class matrix_type, size_t planecount>
	cell<matrix_type, planecount> calc_cell(
		cell<matrix_type, planecount> input, const matrix_info& info, matrix_coord& position) {
		cell<matrix_type, planecount> output;
		return output;
	}

private:
	message<> jitclass_setup {this, "jitclass_setup", MIN_FUNCTION {
		t_class* c = args[0];

		jit_class_addmethod(c, (method)jit_mo_time_update_anim, "update_anim", max::A_CANT, 0);
		jit_class_addinterface(c, jit_class_findbyname(gensym("jit_anim_animator")),
			calcoffset(minwrap<jit_mo_time>, m_min_object) + calcoffset(jit_mo_time, animator), 0);

		return {};
	}};

	message<> maxclass_setup {this, "maxclass_setup",
		MIN_FUNCTION {
			t_class* c = args[0];
			max_jit_class_wrap_standard(c, this_jit_class, 0);
			class_addmethod(c, (method)max_jit_mo_time_int, "int", max::A_LONG, 0);
			class_addmethod(c, (method)max_jit_mo_time_bang, "bang", max::A_NOTHING, 0);
			class_addmethod(c, (method)max_jit_mo_time_assist, "assist", max::A_CANT, 0);

			return {};
		}
	};

	message<> setup {this, "setup", MIN_FUNCTION {
		animator = (t_object*)max::jit_object_new(gensym("jit_anim_animator"), maxobj());
		// attr_addfilterset_proc(object_attr_get(animator, symbol("automatic")),
		// (method)jit_mo_time_automatic_attrfilter);

		if (classname() == "jit.time.delta") {
			mode = timemodes::delta;
		}
		// currently, if instantiated from JS classname will be empty
		else if (classname() == symbol()) {
			mode = timemodes::accum;
		}
		else if (!(classname() == "jit.mo.time") && !(classname() == "jit.time")) {

			mode = timemodes::function;

			if (classname() == "jit.time.line")
				jit_mo_time::function = functypes::line;
			else if (classname() == "jit.time.tri")
				jit_mo_time::function = functypes::tri;
			else if (classname() == "jit.time.sin")
				jit_mo_time::function = functypes::sin;
			else if (classname() == "jit.time.saw")
				jit_mo_time::function = functypes::saw;
			else if (classname() == "jit.time.perlin")
				jit_mo_time::function = functypes::perlin;
		}
		return {};
	}};

	message<> mop_setup {this, "mop_setup",
		MIN_FUNCTION {
			void*     o = maxobj();
			t_object* x = args[args.size() - 1];

			max_jit_obex_jitob_set(x, o);
			dumpoutlet = outlet_new(x, NULL);
			max_jit_obex_dumpout_set(x, dumpoutlet);
			timeoutlet = outlet_new(x, NULL);

			return {};
		}
	};

	message<> maxob_setup {this, "maxob_setup",
		MIN_FUNCTION {
			long atm = object_attr_getlong(maxobj(), sym_automatic);

			if (atm && object_attr_getsym(maxobj(), sym_drawto) == max::_jit_sym_nothing) {
				implicit_ctx = (t_object*)max::jit_object_new(gensym("jit_gl_implicit"));
				ictx_name = object_attr_getsym(implicit_ctx, max::_jit_sym_name);
				jit_object_attach(ictx_name, maxob_from_jitob(maxobj()));
			}

			return {};
		}
	};

	message<> notify {this, "notify",
		MIN_FUNCTION {
			if(implicit_ctx) {
				symbol sendername = args[1];
				symbol msg = args[2];
				void* sender = args[3];
				void* data = args[4];
				if(sendername == ictx_name && msg == sym_attr_modified) {
					symbol attrname = (t_symbol*)max::object_method((t_object*)data, max::_jit_sym_getname);
					if(attrname == sym_drawto) {
						object_attr_setsym(maxobj(), sym_drawto, max::object_attr_getsym(sender, sym_drawto));
					}
				}
			}
			return {max::JIT_ERR_NONE};
		}
	};

    
    message<> fileusage {this, "fileusage",
        MIN_FUNCTION {
            fileusage_addpackage(args, "jit.mo", {{"externals", "init", "interfaces", "patchers"}});
            return {};
       }
    };


private:
	t_object* 		animator    { nullptr };
	t_object* 		implicit_ctx { nullptr };
	symbol			ictx_name;
	void*     		timeoutlet  { nullptr };
	void*     		dumpoutlet  { nullptr };
	double    		accum_time  { 0. };
	long      		prevtime_ms { 0 };
	interval_speed	i_s;
	
	double			prevnormval { 0. };
	bool			speeddirchanged { false };
};

MIN_EXTERNAL(jit_mo_time);


void jit_mo_time_update_anim(t_object* job, t_atom* a) {
	minwrap<jit_mo_time>* self = (minwrap<jit_mo_time>*)job;
	self->m_min_object.update_animation(a);
}

void max_jit_mo_time_int(max_jit_wrapper* mob, long v) {
	void*                 job  = max_jit_obex_jitob_get(mob);
	minwrap<jit_mo_time>* self = (minwrap<jit_mo_time>*)job;
	self->m_min_object.enable  = (bool)v;
}

void max_jit_mo_time_bang(max_jit_wrapper* mob) {
	void*                 job  = max_jit_obex_jitob_get(mob);
	minwrap<jit_mo_time>* self = (minwrap<jit_mo_time>*)job;
	if (!object_attr_getlong(job, sym_automatic))
		self->m_min_object.update();
}

void max_jit_mo_time_assist(void* x, void* b, long m, long a, char* s) {
	if (m == 1) {    // input
		sprintf(s, "bang, messages in");
	}
	else {    // output
		if (a == 0) {
			sprintf(s, "(float) current time value");
		}
		else {
			sprintf(s, "dumpout");
		}
	}
}
