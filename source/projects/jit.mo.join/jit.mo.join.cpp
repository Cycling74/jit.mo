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
    
	jit_mo_join(const atoms& args = {}) {
        name = symbol_unique();
        //attributes["name"]->label = gensym("Name");
    }
	~jit_mo_join() {}
    
	ATTRIBUTE (name, symbol, _jit_sym_nothing) {
		
	}
	END

	ATTRIBUTE (inletct, int, 1) {
		
	}
	END
    
	template<class matrix_type, size_t planecount>
	cell<matrix_type,planecount> calc_cell(cell<matrix_type,planecount> input, const matrix_info& info, matrix_coord& position) {
		cell<matrix_type,planecount> output;
		return output;
	}
	
	
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
        
        if (x==dst && srcout==0)
        {
            switch (updatetype)
            {
            case JPATCHLINE_CONNECT:
                {
                    void *mop=NULL;
                    if((mop=max_jit_obex_adornment_get(dst,_jit_sym_jit_mop))) {
                        t_jit_matrix_info info;
                        void *p=object_method((t_object*)mop,_jit_sym_getoutput,(void*)1);
                        void *m=object_method((t_object*)p,_jit_sym_getmatrix);
						object_method((t_object*)m,_jit_sym_getinfo,&info);
                        //object_post(NULL, "src dim: %ld %ld", info.dim[0], info.dim[1]);
                        object_attr_setlong(src, _jit_sym_dim, info.dim[0]);
                        object_attr_setlong(src, _jit_sym_planecount, 1);
                        object_attr_setsym(src, _jit_sym_type, info.type);
                    }
                    
                    //void *job = object_findregistered(gensym("jitter"), srcname);
                    //object_attr_getlong_array(job, _jit_sym_dim, 2, dim);
                    
                }

                break;
            case JPATCHLINE_DISCONNECT:
                break;
            case JPATCHLINE_ORDER:
                break;
            }
        }

    }END

};

void *max_jit_mo_join_new(t_symbol *s, long argc, t_atom *argv)
{
    max_jit_wrapper* x;
    void *o,*m,*mop;
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

            //inlets = n;

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
    return (x);
}

void max_jit_mo_join_free(max_jit_wrapper *x)
{
    max_jit_mop_free(x);
    jit_object_free(max_jit_obex_jitob_get(x));
    max_jit_object_free(x);
}

t_jit_err jit_mo_join_matrix_calc(void *job, void *inputs, void *outputs)
{
    return 0;
}

t_jit_err max_jit_mo_join_jit_matrix(void *mob, t_symbol *s, long argc, t_atom *argv)
{
    return 0;
}

void ext_main (void* resources) {
    const char* cppname = "jit_mo_join";
	std::string		maxname = c74::min::deduce_maxclassname(__FILE__);
    
	// 1. Boxless Jit Class
	t_class *c = (t_class*)jit_class_new(cppname,(c74::max::method)jit_new<jit_mo_join>, (c74::max::method)jit_free<jit_mo_join>, sizeof( c74::min::minwrap<jit_mo_join> ), 0);
    
	//add mop
	auto mop = jit_object_new(_jit_sym_jit_mop, -1, 1); // #inputs, #outputs
    auto o = object_method(mop,_jit_sym_getoutput,(void*)1);
	jit_attr_setlong(o,_jit_sym_planelink,0);
    jit_class_addadornment(c, mop);
    
	//add methods
	jit_class_addmethod(c, (c74::max::method)jit_mo_join_matrix_calc, "matrix_calc", c74::max::A_CANT, 0);
    
    auto attr = jit_object_new(_jit_sym_jit_attr_offset, "inletct", gensym("long"),ATTR_GET_OPAQUE_USER|ATTR_SET_OPAQUE_USER,
                                (c74::max::method)min_attr_getter<jit_mo_join>, (c74::max::method)min_attr_setter<jit_mo_join>, 0);
    
    jit_class_addattr(c, attr);
    
    jit_class_register(c);
    _jit_mo_join_class = c;

	
	// 2. Max Wrapper Class
	c = class_new(maxname.c_str(),(c74::max::method)max_jit_mo_join_new,(c74::max::method)max_jit_mo_join_free, sizeof(max_jit_wrapper), nullptr, A_GIMME, 0);
	max_jit_class_obex_setup(c, calcoffset(max_jit_wrapper, obex));
	
	max_jit_class_mop_wrap(c, _jit_mo_join_class, MAX_JIT_MOP_FLAGS_OWN_JIT_MATRIX);
	max_jit_class_wrap_standard(c, _jit_mo_join_class, 0);
	
	class_addmethod(c, (c74::max::method)max_jit_mop_assist, "assist", A_CANT, 0);
    class_addmethod(c, (c74::max::method)min_jit_mop_method_patchlineupdate<jit_mo_join>, "patchlineupdate", A_CANT, 0);
	class_addmethod(c, (c74::max::method)max_jit_mo_join_jit_matrix, "jit_matrix", A_GIMME, 0);
    
	c->c_menufun = (c74::max::method)c74::max::gensym(cppname);
	
	class_register(c74::max::CLASS_BOX, c);
    _max_jit_mo_join_class = c;
}

