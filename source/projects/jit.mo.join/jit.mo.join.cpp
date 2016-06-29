/// @file	
///	@ingroup 	minexamples
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Timothy Place
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"

using namespace c74::min;
using namespace c74::max;

static t_class *_jit_mo_join_class = NULL;
static t_class *_max_jit_mo_join_class = NULL;

class jit_mo_join : public object<jit_mo_join>, matrix_operator {
public:
    
	jit_mo_join(const atoms& args = {}) {}
	~jit_mo_join() {
        jit_object_free(animator);
    }
    
    void setup(t_object *job) {
        animator = jit_object_new(gensym("jit_anim_animator"), job);
    }
    
    void update_mop_props(void *mob) {
        t_jit_matrix_info info;
        void *mop=max_jit_obex_adornment_get(mob,_jit_sym_jit_mop);
        void *p=object_method((t_object*)mop,_jit_sym_getoutput,(void*)1);
        void *m=object_method((t_object*)p,_jit_sym_getmatrix);
        object_method((t_object*)m,_jit_sym_getinfo,&info);
        count = info.dim[0];
        type = info.type;
    }
    
	ATTRIBUTE (inletct, int, 1) {
	}
	END
    
    ATTRIBUTE (count, int, 1) {
        update_attached_dim(atom_getlong(&args[0]));
	}
	END
    
	template<class matrix_type, size_t planecount>
	cell<matrix_type,planecount> calc_cell(cell<matrix_type,planecount> input, const matrix_info& info, matrix_coord& position) {
		cell<matrix_type,planecount> output;
		return output;
	}
    
    template<typename U>
    void calculate_vector(const matrix_info& info, long n, t_jit_op_info* inop, t_jit_op_info* outop) {
        auto in = (U*)inop->p;
        auto out = (U*)outop->p;
        const auto instep = info.in_info->planecount;
        const auto outstep = info.out_info->planecount;
        
        for (auto j=0; j<n; ++j) {
            for (auto k=0; k<instep && k<outstep; ++k)
                out[plane] = *(in+instep*k);
            
            in += instep;
            out += outstep;
        }
    }
    
    void update(t_atom *av) {
        for( const auto& n : m_attached ) {
            object_attr_setfloat(n.second, sym_delta, atom_getfloat(av));
            object_method(n.second, sym_bang);
        }
    }
    
	int plane;
    t_object *animator;
    
private:
    typedef enum _patchline_updatetype {
        JPATCHLINE_DISCONNECT=0,
        JPATCHLINE_CONNECT=1,
        JPATCHLINE_ORDER=2
    } t_patchline_updatetype;

    METHOD(patchlineupdate) {
        t_object *x = args[0];
       // t_object *patchline = args[1];
        long updatetype = args[2];
        t_object *src = args[3];
        long srcout = args[4];
        t_object *dst = args[5];
        //long dstin = args[6];
        
        if (x==dst && srcout==0 && object_classname_compare(src, gensym("jit.mo.gen")))
        {
            std::stringstream ss;
            ss << src;
            std::string name = ss.str();

            switch (updatetype)
            {
            case JPATCHLINE_CONNECT:
                m_attached.insert({name, src});
                object_attr_setlong(src, _jit_sym_planecount, 1);
                object_attr_setsym(src, _jit_sym_type, type);
                object_attr_setlong(src, _jit_sym_dim, count);
                break;
            case JPATCHLINE_DISCONNECT:
                m_attached.erase(name);
                break;
            case JPATCHLINE_ORDER:
                break;
            }
        }

    }END
    
    void update_attached_dim(long dim) {
        for( const auto& n : m_attached )
            object_attr_setlong(n.second, _jit_sym_dim, dim);
    }
    
    std::unordered_map<std::string, t_object*> m_attached;
    t_symbol *type = _jit_sym_float32;
    symbol sym_bang = "bang";
    symbol sym_delta = "delta";

};

void *max_jit_mo_join_new(t_symbol *s, long argc, t_atom *argv)
{
    max_jit_wrapper* x;
    void *o=NULL,*m,*mop;
    t_jit_matrix_info info;
    long i,n;
    
    this_jit_class = _jit_mo_join_class;
    this_class = _max_jit_mo_join_class;
    
    if ((x=(max_jit_wrapper *)max_jit_object_alloc(_max_jit_mo_join_class,gensym("jit_mo_join")))) {
        if ((o=jit_object_new(gensym("jit_mo_join")))) {
            max_jit_obex_jitob_set(x,o);
            max_jit_obex_dumpout_set(x,outlet_new(x,NULL));
            max_jit_mop_setup(x);
            //add fake inputs + one real input
            if (argc&&(n=atom_getlong(argv))) {
                n = clamp<int>(n, 1, JIT_MATRIX_MAX_PLANECOUNT);
            } else {
                n=4;
            }
            
            jit_attr_setlong(o, gensym("inletct"), n);

            for (i=n; i>1; i--) {
                max_jit_obex_proxy_new(x,i-1); //right to left
            }
            max_jit_mop_variable_addinputs(x,1);//only used to fake out the matrix calc method
            max_jit_mop_inputs(x);
            max_jit_mop_outputs(x);
            max_jit_mop_matrix_args(x,argc,argv);
            //set planecount since it is not linked.
            m = max_jit_mop_getoutput(x,1);
            jit_attr_setlong(m,_jit_sym_planecount,n);
            //set adapt true if only plane argument(should come after matrix_args call)
            if ((max_jit_attr_args_offset(argc,argv)<=1) &&
                    (mop=max_jit_obex_adornment_get(x,_jit_sym_jit_mop)))
            {
                jit_attr_setlong(mop,_jit_sym_adapt,1);
            }

            max_jit_attr_args(x,argc,argv);
        } else {
            object_error(&x->ob,"jit.mo.join: could not allocate object");
            object_free(x);
            x = NULL;
        }
    }
    
    if(o) {
        minwrap<jit_mo_join>* job = (minwrap<jit_mo_join>*)o;
        job->obj.update_mop_props(x);
    }
    
    return (x);
}

void max_jit_mo_join_free(max_jit_wrapper *x)
{
    max_jit_mop_free(x);
    jit_object_free(max_jit_obex_jitob_get(x));
    max_jit_object_free(x);
}


t_jit_err max_jit_mo_join_jit_matrix(max_jit_wrapper *x, t_symbol *s, long argc, t_atom *argv)
{
	void *mop;
	long err=JIT_ERR_NONE;

	if (!(mop=max_jit_obex_adornment_get(x,_jit_sym_jit_mop)))
		return JIT_ERR_GENERIC;

	if (argc&&argv) {
		t_symbol *matrixname = atom_getsym(argv);
		void *matrix = object_findregistered(_jit_sym_jitter, matrixname);
		if (matrix && object_method((t_object*)matrix, _jit_sym_class_jit_matrix)) {
            void *p = object_method((t_object*)mop,_jit_sym_getinput,(void*)1);
            object_method((t_object*)p,_jit_sym_matrix,matrix);
            jit_attr_setsym(p,_jit_sym_matrixname,matrixname);
            
            auto in_mop_io = (t_object*)object_method((t_object*)object_method((t_object*)mop,_jit_sym_getinputlist), _jit_sym_getindex, 0);
            auto out_mop_io = (t_object*)object_method((t_object*)object_method((t_object*)mop,_jit_sym_getoutputlist), _jit_sym_getindex, 0);
            auto in_matrix = (t_object*)object_method(in_mop_io, k_sym_getmatrix);
            auto out_matrix = (t_object*)object_method(out_mop_io, k_sym_getmatrix);
            
            if (!in_matrix || !out_matrix)
                return JIT_ERR_INVALID_PTR;
		
			auto in_savelock = object_method(in_matrix, _jit_sym_lock, (void*)1);
			auto out_savelock = object_method(out_matrix, _jit_sym_lock, (void*)1);
			
			t_jit_matrix_info in_minfo;
			t_jit_matrix_info out_minfo;
			object_method(in_matrix, _jit_sym_getinfo, &in_minfo);
			object_method(out_matrix, _jit_sym_getinfo, &out_minfo);
			
            t_jit_op_info	in_opinfo;
            t_jit_op_info	out_opinfo;
            
            auto n = out_minfo.dim[0];
			object_method(in_matrix, _jit_sym_getdata, &in_opinfo.p);
			object_method(out_matrix, _jit_sym_getdata, &out_opinfo.p);
            
            matrix_info info(&in_minfo, (char*)in_opinfo.p, &out_minfo, (char*)out_opinfo.p);
            minwrap<jit_mo_join>* job = (minwrap<jit_mo_join>*)max_jit_obex_jitob_get(x);
            job->obj.plane = max_jit_obex_inletnumber_get(x);

            if (in_minfo.type == _jit_sym_char)
                job->obj.calculate_vector<uchar>(info, n, &in_opinfo, &out_opinfo);
            else if (in_minfo.type == _jit_sym_long)
                job->obj.calculate_vector<int>(info, n, &in_opinfo, &out_opinfo);
            else if (in_minfo.type == _jit_sym_float32)
                job->obj.calculate_vector<float>(info, n, &in_opinfo, &out_opinfo);
            else if (in_minfo.type == _jit_sym_float64)
                job->obj.calculate_vector<double>(info, n, &in_opinfo, &out_opinfo);
            

			object_method(out_matrix, _jit_sym_lock, out_savelock);
			object_method(in_matrix, _jit_sym_lock, in_savelock);
            
            if (job->obj.plane==0)
                max_jit_mop_outputmatrix(x);
		} else {
			jit_error_code(x,JIT_ERR_MATRIX_UNKNOWN);
		}
	}

	return err;
}

t_jit_err max_jit_mo_join_dim(max_jit_wrapper *mob, t_symbol *attr, long argc, t_atom *argv)
{
    if(argv && argc) {
        void* job = max_jit_obex_jitob_get(mob);
        object_attr_setlong(job, gensym("count"), atom_getlong(argv));
        max_jit_mop_dim(mob, attr, argc, argv);
	}

	return JIT_ERR_NONE;
}

void jit_mo_join_update_anim(t_object *job, t_atom *a)
{
    minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
    self->obj.update(a);
}

void jit_mo_join_setup(t_object *job)
{
    minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
    self->obj.setup(job);
}

void ext_main (void* resources) {
    const char* cppname = "jit_mo_join";
	std::string	maxname = c74::min::deduce_maxclassname(__FILE__);
    
	// 1. Boxless Jit Class
	t_class *c = (t_class*)jit_class_new(cppname,(c74::max::method)jit_new<jit_mo_join>, (c74::max::method)jit_free<jit_mo_join>, sizeof( c74::min::minwrap<jit_mo_join> ), 0);
    
	//add mop
	auto mop = jit_object_new(_jit_sym_jit_mop, -1, 1); // #inputs, #outputs
    auto o = object_method(mop,_jit_sym_getoutput,(void*)1);
	jit_attr_setlong(o,_jit_sym_planelink,0);
    jit_class_addadornment(c, mop);
    
	//add methods
	jit_class_addmethod(c, (c74::max::method)jit_mo_join_update_anim, "update_anim", A_CANT, 0);
    jit_class_addmethod(c, (c74::max::method)jit_mo_join_setup, "setup", A_CANT, 0);
    
    auto attr = jit_object_new(_jit_sym_jit_attr_offset, "inletct", _jit_sym_long,ATTR_GET_OPAQUE_USER|ATTR_SET_OPAQUE_USER,
                              (c74::max::method)min_attr_getter<jit_mo_join>, (c74::max::method)min_attr_setter<jit_mo_join>, 0);
    jit_class_addattr(c, attr);
    
    attr = jit_object_new(_jit_sym_jit_attr_offset, "count", _jit_sym_long,ATTR_GET_OPAQUE_USER|ATTR_SET_OPAQUE_USER,
                              (c74::max::method)min_attr_getter<jit_mo_join>, (c74::max::method)min_attr_setter<jit_mo_join>, 0);
    jit_class_addattr(c, attr);
    
    jit_class_addinterface(c, jit_class_findbyname(gensym("jit_anim_animator")), calcoffset(minwrap<jit_mo_join>, obj) + calcoffset(jit_mo_join, animator), 0);

    jit_class_register(c);
    _jit_mo_join_class = c;

	
	// 2. Max Wrapper Class
	c = class_new(maxname.c_str(),(c74::max::method)max_jit_mo_join_new,(c74::max::method)max_jit_mo_join_free, sizeof(max_jit_wrapper), nullptr, A_GIMME, 0);
	max_jit_class_obex_setup(c, calcoffset(max_jit_wrapper, obex));
	
	max_jit_class_mop_wrap(c, _jit_mo_join_class, MAX_JIT_MOP_FLAGS_OWN_JIT_MATRIX|MAX_JIT_MOP_FLAGS_OWN_DIM);
	max_jit_class_wrap_standard(c, _jit_mo_join_class, 0);
    
	attr = jit_object_new(_jit_sym_jit_attr_offset_array,"dim",_jit_sym_long,JIT_MATRIX_MAX_DIMCOUNT, ATTR_GET_DEFER_LOW|ATTR_SET_USURP_LOW,
                         (c74::max::method)max_jit_mop_getdim,(c74::max::method)max_jit_mo_join_dim,0);
	max_jit_class_addattr(c,attr);
	
	class_addmethod(c, (c74::max::method)max_jit_mop_assist, "assist", A_CANT, 0);
    class_addmethod(c, (c74::max::method)min_jit_mop_method_patchlineupdate<jit_mo_join>, "patchlineupdate", A_CANT, 0);
	class_addmethod(c, (c74::max::method)max_jit_mo_join_jit_matrix, "jit_matrix", A_GIMME, 0);
    
	c->c_menufun = (c74::max::method)c74::max::gensym(cppname);
	
	class_register(c74::max::CLASS_BOX, c);
    _max_jit_mo_join_class = c;
}
