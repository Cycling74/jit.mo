/// @file	
///	@ingroup 	minexamples
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Timothy Place
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"

using namespace c74::min;
using namespace c74::max;

class jit_mo_join : public object<jit_mo_join>, matrix_operator {
public:
	
	inlet	input1	= { this, "(matrix) Input", "matrix" };
    inlet	input2	= { this, "(matrix) Input", "matrix" };
    inlet	input3	= { this, "(matrix) Input", "matrix" };
    
	outlet	output	= { this, "(matrix) Output", "matrix" };
	
	
	jit_mo_join(atoms args) {
        name = symbol_unique();
        //attributes["name"]->label = gensym("Name");
    }
	jit_mo_join() {}
	
    void setup(atoms args) {
        
    }
    
	ATTRIBUTE (name, symbol, _jit_sym_nothing) {
		
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

MIN_EXTERNAL(jit_mo_join);

