/// @file
///	@copyright	Copyright 2018 The Jit.Mo Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#include "jit.mo.singleton.h"

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
using max::object_findregistered;
using max::max_jit_obex_jitob_get;
using max::t_jit_err;
using max::t_jit_op_info;
using max::t_jit_matrix_info;


void      jit_mo_join_update_anim(t_object* job, t_atom* a);
t_jit_err jit_mo_join_attach(t_object* job, t_object* cob);
void      jit_mo_join_detach(t_object* job, t_object* cob);
void      jit_mo_join_automatic_attrfilter(t_object* job, void* attr, long argc, t_atom* argv);
t_jit_err jit_mo_join_matrix_calc(t_object* x, t_object* inputs, t_object* outputs);

t_jit_err max_jit_mo_join_dim(max_jit_wrapper* mob, t_symbol* attr, long argc, t_atom* argv);
t_jit_err max_jit_mo_join_jit_matrix(max_jit_wrapper* x, t_symbol* s, long argc, t_atom* argv);
void      max_jit_mo_join_int(max_jit_wrapper* mob, long v);


class jit_mo_join : public object<jit_mo_join>, public matrix_operator<> {
public:
	MIN_DESCRIPTION {"Combine jit.mo streams and output a multi-plane matrix. "
					"Automatically connects to jit.world to drive animations. "
					"Inputs add multiple jit.mo/matrix inputs for additive control."};
	MIN_TAGS		{"jit.mo, Generators"};
	MIN_AUTHOR		{"Cycling '74"};
	MIN_RELATED		{"jit.mo.func, jit.mo.field, jit.mo.time, jit.anim.path, jit.world"};

	
	outlet<> output {this, "(matrix) Output", "matrix"};

	
	argument<number> inletcount {this, "Inlet Count", "Set the number of inlets and planecount of output.", false, nullptr};

	argument<number> dimarg {this, "Dimension",
		"Set the dimension (number of elements) of the output matrix and any attached [jit.mo.func] objects. jit.mo "
		"objects only support matrices with a dimcount of 1.",
		false, nullptr
	};
	
	~jit_mo_join() {
		freeing  = true;
		symbol n = name;
		
		for (auto a : attached_funcobs) {
			// setting attached objects join attribute to this name while freeing will add them to unbound list
			// will ensure that deletion followed by undo will still function
			object_attr_setsym(a.second, gensym("join"), n);
		}
		
		if(implicit_ctx) jit_object_free(implicit_ctx);
		jit_object_free(animator);
	}

	
	attribute<bool> enable {this, "enable", true,
		title {"Enable Animation"},
		description {"Enable Animation (default = 1). This affects any connected jit.mo.func objects"}
	};

	attribute<double> speed {this, "speed", 1.0,
		title {"Speed"},
		description {"Animation speed (default = 1.). Scales animation speed of all connected jit.mo.func objects"}
	};

	attribute<double> scale {this, "scale", 1,
		title {"Scale"},
		description {"Output multiplier (default = 1.0)."}
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

	attribute<symbol> name {this, "name", symbol(true),
		title {"Name"},
		description {"Object name (default = UID)."},
		setter { MIN_FUNCTION {
			t_object* o        = nullptr;
			symbol    new_name = args[0];

			if (name == new_name)
				return args;

			if ((o = (t_object*)object_findregistered(sym_jitter, new_name))) {
				symbol old_name = name;
				return {old_name};
			}

			if (initialized()) {
				register_and_setup(new_name);
			}

			return args;
		}}
	};

	template<class matrix_type, size_t planecount>
	cell<matrix_type, planecount> calc_cell(cell<matrix_type, planecount> input, const matrix_info& info, matrix_coord& position) {
		cell<matrix_type, planecount> output;
		return output;
	}

	template<typename U>
	void calculate_vector(const matrix_info& info, long n, t_jit_op_info* inop, t_jit_op_info* outop) {
		auto       in      = (U*)inop->p;
		auto       out     = (U*)outop->p;
		const auto instep  = info.m_in_info->planecount;
		const auto outstep = info.m_out_info->planecount;

		// TODO: allow multiplane input
		for (auto j = 0; j < n; ++j) {
			for (auto k = 0; k < instep && k < outstep; ++k)
				out[curplane] += (*(in + instep * k) * scale);

			in += instep;
			out += outstep;
		}
	}

	t_jit_err matrix_calc(t_object* in_matrix, t_object* out_matrix, long plane, t_object* mob) {
		t_jit_matrix_info in_minfo;
		t_jit_matrix_info out_minfo;
		max::object_method(in_matrix, max::_jit_sym_getinfo, &in_minfo);
		max::object_method(out_matrix, max::_jit_sym_getinfo, &out_minfo);

		t_jit_op_info in_opinfo;
		t_jit_op_info out_opinfo;

		auto n = out_minfo.dim[0];
		max::object_method(in_matrix, max::_jit_sym_getdata, &in_opinfo.p);
		max::object_method(out_matrix, max::_jit_sym_getdata, &out_opinfo.p);

		matrix_info info(&in_minfo, (c74::min::uchar*)in_opinfo.p, &out_minfo, (c74::min::uchar*)out_opinfo.p);

		maxob    = mob;
		curplane = plane;

		if (request_clear) {
			max::object_method(out_matrix, max::_jit_sym_clear);
			request_clear = false;
		}

		auto in_savelock  = max::object_method(in_matrix, max::_jit_sym_lock, (void*)1);
		auto out_savelock = max::object_method(out_matrix, max::_jit_sym_lock, (void*)1);

		if (in_minfo.type == max::_jit_sym_char)
			calculate_vector<c74::min::uchar>(info, n, &in_opinfo, &out_opinfo);
		else if (in_minfo.type == max::_jit_sym_long)
			calculate_vector<int>(info, n, &in_opinfo, &out_opinfo);
		else if (in_minfo.type == max::_jit_sym_float32)
			calculate_vector<float>(info, n, &in_opinfo, &out_opinfo);
		else if (in_minfo.type == max::_jit_sym_float64)
			calculate_vector<double>(info, n, &in_opinfo, &out_opinfo);


		max::object_method(out_matrix, max::_jit_sym_lock, out_savelock);
		max::object_method(in_matrix, max::_jit_sym_lock, in_savelock);

		if (plane == 0 && object_attr_getlong(maxobj(), sym_automatic) == 0) {
			request_clear = true;
		}

		return max::JIT_ERR_NONE;
	}

	void update_animation(t_atom* av) {
		if (enable) {
			if (i_s.update(speed, interval)) {
				object_attr_touch(maxob_from_jitob(maxobj()), gensym("speed"));
			}

			for (const auto& n : attached_funcobs) {
				object_attr_setfloat(n.second, sym_delta, atom_getfloat(av) * (speed));
				max::object_method(maxob_from_jitob(n.second), (t_symbol*)sym_bang);
			}

			if (maxob)
				max_jit_mop_outputmatrix(maxob);

			request_clear = true;
		}
	}

	void update_attached_dim(long dim) {
		count = dim;
		for (const auto& n : attached_funcobs)
			object_attr_setlong(maxob_from_jitob(n.second), max::_jit_sym_dim, count);
	}

	t_jit_err attach(t_object* child) {
		if (!freeing) {
			attached_funcobs.insert( {string_from_obptr(child), child});
			update_attached_dim(count);
			return max::JIT_ERR_NONE;
		}
		return max::JIT_ERR_GENERIC;
	}

	void detach(t_object* child) {
		if (!freeing)
			attached_funcobs.erase(string_from_obptr(child));
	}

	void automatic_filter(long automatic) {
		// TODO: remove or add to implicit check list
	}

private:
	message<> jitclass_setup {this, "jitclass_setup", MIN_FUNCTION {
		t_class* c = args[0];

		// add mop
		auto mop = max::jit_object_new(max::_jit_sym_jit_mop, -1, 1);
		auto o   = max::object_method(mop, max::_jit_sym_getoutput, (void*)1);
		jit_attr_setlong(o, max::_jit_sym_planelink, 0);
		max::jit_class_addadornment(c, (max::t_jit_object*)mop);

		// add methods
		jit_class_addmethod(c, (method)jit_mo_join_matrix_calc, "matrix_calc", max::A_CANT,
			0);    // handles case where used in JS
		jit_class_addmethod(c, (method)jit_mo_join_update_anim, "update_anim", max::A_CANT, 0);
		jit_class_addmethod(c, (method)jit_mo_join_attach, "attach", max::A_CANT, 0);
		jit_class_addmethod(c, (method)jit_mo_join_detach, "detach", max::A_CANT, 0);

		jit_class_addinterface(c, jit_class_findbyname(gensym("jit_anim_animator")),
			calcoffset(minwrap<jit_mo_join>, m_min_object) + calcoffset(jit_mo_join, animator), 0);

		return {};
	}};

	message<> maxclass_setup {this, "maxclass_setup", MIN_FUNCTION {
		t_class* c = args[0];

		max_jit_class_mop_wrap(c, this_jit_class, max::MAX_JIT_MOP_FLAGS_OWN_JIT_MATRIX | max::MAX_JIT_MOP_FLAGS_OWN_DIM | max::MAX_JIT_MOP_FLAGS_OWN_NOTIFY);
		max_jit_class_wrap_standard(c, this_jit_class, 0);

		auto attr = max::jit_object_new(max::_jit_sym_jit_attr_offset_array, "dim", max::_jit_sym_long, max::JIT_MATRIX_MAX_DIMCOUNT,
			max::ATTR_GET_DEFER_LOW | max::ATTR_SET_USURP_LOW, (method)max::max_jit_mop_getdim, (method)max_jit_mo_join_dim, 0);
		max_jit_class_addattr(c, attr);

		class_addmethod(c, (method)max_jit_mo_join_jit_matrix, "jit_matrix", max::A_GIMME, 0);
		class_addmethod(c, (method)max_jit_mo_join_int, "int", max::A_LONG, 0);
		class_addmethod(c, (method)max_jit_mo_addfuncob, "addfuncob", max::A_CANT, 0);
		class_addmethod(c, (method)max_jit_mo_removefuncob, "removefuncob", max::A_CANT, 0);

		class_addmethod(c, (method)max::max_jit_mop_assist, "assist", max::A_CANT, 0);    // standard matrix-operator (mop) assist fn
		return {};
	}};

	message<> setup {this, "setup", MIN_FUNCTION {
		animator = (t_object*)max::jit_object_new(gensym("jit_anim_animator"), maxobj());
		max::attr_addfilterset_proc(
			object_attr_get(animator, symbol("automatic")), (method)jit_mo_join_automatic_attrfilter);

		register_and_setup(name);

		return {};
	}};

	message<> mop_setup {this, "mop_setup", MIN_FUNCTION {
		void*     o = maxobj();
		t_object* x = args[args.size() - 1];

		long              i, n;

		max_jit_obex_jitob_set(x, o);
		max_jit_obex_dumpout_set(x, outlet_new(x, NULL));
		max_jit_mop_setup(x);

		// add fake inputs + one real input
		if (args.size() > 1) {
			n = clamp<int>(args[0], 1, max::JIT_MATRIX_MAX_PLANECOUNT);
		}
		else {
			n = 3;
		}

		for (i = n; i > 1; i--)
			max_jit_obex_proxy_new(x, i - 1);    // right to left

		max_jit_mop_variable_addinputs(x, 1);    // only used to fake out the matrix calc method
		max_jit_mop_inputs(x);
		max_jit_mop_outputs(x);

		jit_attr_setsym(x, max::_jit_sym_type, max::_jit_sym_float32);

		// set planecount since it is not linked.
		void* m = max_jit_mop_getoutput(x, 1);
		jit_attr_setlong(m, max::_jit_sym_planecount, n);

		if (args.size() > 3) {
			long   argc = 3;
			t_atom argv[3];

			for (int i = 0; i < 3; i++)
				argv[i] = args[i];

			max_jit_mop_matrix_args(x, argc, argv);
		}
		else {
			long dim = (args.size() > 2 ? (long)args[1] : 1);
			jit_attr_setlong(x, max::_jit_sym_dim, dim);
		}

		update_mop_props(x);
		return {};
	}};

	message<> maxob_setup {this, "maxob_setup", MIN_FUNCTION {
		long atm = object_attr_getlong(maxobj(), sym_automatic);
		
		if (atm && object_attr_getsym(maxobj(), sym_drawto) == max::_jit_sym_nothing) {
			implicit_ctx = (t_object*)max::jit_object_new(gensym("jit_gl_implicit"));
			ictx_name = object_attr_getsym(implicit_ctx, max::_jit_sym_name);
			jit_object_attach(ictx_name, maxob_from_jitob(maxobj()));
		}

		return {};
	}};

	typedef enum _patchline_updatetype {
		JPATCHLINE_DISCONNECT = 0,
		JPATCHLINE_CONNECT    = 1,
		JPATCHLINE_ORDER      = 2
	} t_patchline_updatetype;

	message<> patchlineupdate {this, "patchlineupdate", MIN_FUNCTION {
		t_object* x = args[0];
		// t_object *patchline = args[1];
		long      updatetype = args[2];
		t_object* src        = args[3];
		long      srcout     = args[4];
		t_object* dst        = args[5];
		// long dstin = args[6];
		symbol n = name;

		// TODO: we could allow setting dim on any object with a dim attribute
		if (x == dst && srcout == 0 && object_classname_compare(src, gensym("jit.mo.func"))) {

			switch (updatetype) {
				// TODO: create patchcord_join attribute to distiguish from explicit user setting
				case JPATCHLINE_CONNECT:
					// attached_funcobs.insert( {name, src});
					// object_attr_setlong(src, max::_jit_sym_planecount, 1);
					// object_attr_setsym(src, max::_jit_sym_type, type);
					object_attr_setlong(src, max::_jit_sym_dim, count);
					object_attr_setsym(src, gensym("join"), n);

					break;
				case JPATCHLINE_DISCONNECT:
					// attached_funcobs.erase(name);
					object_attr_setsym(src, gensym("join"), max::_jit_sym_nothing);
					break;
				case JPATCHLINE_ORDER:
					break;
			}
		}
		return {};
	}};

	message<> notify {this, "notify", MIN_FUNCTION {
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
		max_jit_mop_notify(args[0], args[1], args[2]);
		return {max::JIT_ERR_NONE};
	}};

    
    message<> fileusage {this, "fileusage",
        MIN_FUNCTION {
            fileusage_addpackage(args, "jit.mo", {{"externals", "init", "interfaces", "patchers"}});
            return {};
       }
    };


	void register_and_setup(symbol new_name) {
		symbol nothing = max::_jit_sym_nothing;
		if (!(name == nothing))
			object_unregister(maxobj());

		if (!(new_name == nothing))
			object_register(sym_jitter, new_name, maxobj());

		for (const auto& n : attached_funcobs)
			object_attr_setsym(n.second, gensym("join"), new_name);

		jit_mo_singleton::instance().check_funcobs(new_name);
	}

	void update_mop_props(void* mob) {
		t_jit_matrix_info info;
		void*             mop = max_jit_obex_adornment_get(mob, max::_jit_sym_jit_mop);
		void*             p   = max::object_method((t_object*)mop, max::_jit_sym_getoutput, (void*)1);
		void*             m   = max::object_method((t_object*)p, max::_jit_sym_getmatrix);
		max::object_method((t_object*)m, max::_jit_sym_getinfo, &info);
		count = info.dim[0];
	}

	std::string string_from_obptr(t_object* ob) {
		std::stringstream ss;
		ss << ob;
		return ss.str();
	}

	std::unordered_map<std::string, t_object*>	attached_funcobs;
	t_object*                                  	animator      { nullptr };
	t_object*                                  	maxob         { nullptr };
	t_object* 									implicit_ctx { nullptr };
	symbol										ictx_name;
	int                                        	curplane      { 0 };
	bool                                       	request_clear { true };
	long                                       	count         { 1 };
	bool                                      	freeing       { false };
	interval_speed								i_s;
};

MIN_EXTERNAL(jit_mo_join);


void jit_mo_join_update_anim(t_object* job, t_atom* a) {
	minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
	self->m_min_object.update_animation(a);
}

t_jit_err jit_mo_join_attach(t_object* job, t_object* cob) {
	minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
	return self->m_min_object.attach(cob);
}

void jit_mo_join_detach(t_object* job, t_object* cob) {
	minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
	self->m_min_object.detach(cob);
}

void jit_mo_join_automatic_attrfilter(t_object* job, void* attr, long argc, t_atom* argv) {
	minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
	if (argc && argv) {
		long automatic = atom_getlong(argv);
		self->m_min_object.automatic_filter(automatic);
	}
}

// handles case where used in JS
t_jit_err jit_mo_join_matrix_calc(t_object* x, t_object* inputs, t_object* outputs) {
	if (!x || !inputs || !outputs)
		return max::JIT_ERR_INVALID_PTR;

	t_jit_err err        = max::JIT_ERR_NONE;
	long      in_count   = (long)max::object_method(inputs, gensym("getsize"));
	auto      out_matrix = (t_object*)max::object_method(outputs, max::_jit_sym_getindex, 0);

	if (in_count <= 0)
		in_count = 1;

	for (long j = 0; j < in_count; j++) {
		auto in_matrix = (t_object*)max::object_method(inputs, max::_jit_sym_getindex, (void*)j);
		if (in_matrix && out_matrix) {
			minwrap<jit_mo_join>* job = (minwrap<jit_mo_join>*)(x);
			err                       = job->m_min_object.matrix_calc(in_matrix, out_matrix, j, nullptr);
		}
		else {
			return max::JIT_ERR_INVALID_PTR;
		}
	}
	return err;
}

t_jit_err max_jit_mo_join_dim(max_jit_wrapper* mob, t_symbol* attr, long argc, t_atom* argv) {
	if (argv && argc) {
		void*                 job  = max_jit_obex_jitob_get(mob);
		minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
		self->m_min_object.update_attached_dim(atom_getlong(argv));
		max_jit_mop_dim(mob, attr, argc, argv);
	}

	return max::JIT_ERR_NONE;
}

void max_jit_mo_join_int(max_jit_wrapper* mob, long v) {
	void*                 job  = max_jit_obex_jitob_get(mob);
	minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
	self->m_min_object.enable  = (bool)v;
}

t_jit_err max_jit_mo_join_jit_matrix(max_jit_wrapper* x, t_symbol* s, long argc, t_atom* argv) {
	void* mop;
	long  err = max::JIT_ERR_NONE;

	if (!(mop = max_jit_obex_adornment_get(x, max::_jit_sym_jit_mop)))
		return max::JIT_ERR_GENERIC;

	if (argc && argv) {
		t_symbol* matrixname = atom_getsym(argv);
		void*     matrix     = object_findregistered(sym_jitter, matrixname);
		if (matrix && max::object_method((t_object*)matrix, max::_jit_sym_class_jit_matrix)) {
			void* input = max::object_method((t_object*)mop, max::_jit_sym_getinput, (void*)1);
			max::object_method((t_object*)input, max::_jit_sym_matrix, matrix);
			jit_attr_setsym(input, max::_jit_sym_matrixname, matrixname);

			auto in_mop_io = (t_object*)max::object_method(
				(t_object*)max::object_method((t_object*)mop, max::_jit_sym_getinputlist), max::_jit_sym_getindex, 0);
			auto out_mop_io = (t_object*)max::object_method(
				(t_object*)max::object_method((t_object*)mop, max::_jit_sym_getoutputlist), max::_jit_sym_getindex, 0);
			auto in_matrix  = (t_object*)max::object_method(in_mop_io, (t_symbol*)k_sym_getmatrix);
			auto out_matrix = (t_object*)max::object_method(out_mop_io, (t_symbol*)k_sym_getmatrix);

			if (!in_matrix || !out_matrix)
				return max::JIT_ERR_INVALID_PTR;

			long                  plane = max::max_jit_obex_inletnumber_get(x);
			minwrap<jit_mo_join>* job   = (minwrap<jit_mo_join>*)max_jit_obex_jitob_get(x);
			err                         = job->m_min_object.matrix_calc(in_matrix, out_matrix, plane, (t_object*)x);

			if (plane == 0 && object_attr_getlong(job, sym_automatic) == 0) {
				max::max_jit_mop_outputmatrix(x);
			}
		}
		else {
			jit_error_code(x, max::JIT_ERR_MATRIX_UNKNOWN);
		}
	}

	return err;
}
