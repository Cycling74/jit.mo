/// @file	
///	@ingroup 	minexamples
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Timothy Place
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"

using namespace c74::min;
using namespace c74::max;

class jit_mo_gen : public object<jit_mo_gen>, matrix_operator {
public:
	
	outlet	output	= { this, "(matrix) Output", "matrix" };
	
	
	jit_mo_gen(atoms args) {
        name = symbol_unique();
        //attributes["name"]->label = gensym("Name");
    }
	~jit_mo_gen() {}
	
    void setup(atoms args) {
        object_register(gensym("jitter"), (symbol)name, args[0]);
    }

	ATTRIBUTE (name, symbol, _jit_sym_nothing) {
		//void *job = max_jit_obex_jitob_get(maxobj);
	}
	END
    
    ATTRIBUTE (gentype, symbol, gensym("sin")) {
		
	}
	END
    
    ATTRIBUTE (amp, double, 1.0) {
		
	}
	END
	
    ATTRIBUTE (freq, double, 1.0) {
		
	}
	END
    
    ATTRIBUTE (phase, double, 0.0) {
		
	}
	END
    
	template<class matrix_type, size_t planecount>
	cell<matrix_type,planecount> calc_cell(cell<matrix_type,planecount> input, const matrix_info& info, matrix_coord& position) {
		cell<matrix_type,planecount> output;
		double snorm = (double)position.x() / (double)info.out_info->dim[0];
        snorm = snorm*2. - 1.;
        double val = snorm * freq + phase;
        val *= M_PI;
        output[0] = sin(val) * amp;
		return output;
	}
	
	
private:
    typedef enum _patchline_updatetype {
        JPATCHLINE_DISCONNECT=0,
        JPATCHLINE_CONNECT=1,
        JPATCHLINE_ORDER=2
    } t_patchline_updatetype;
    
    METHOD(patchlineupdate) {
        /*t_object *x = args[0];
        t_object *patchline = args[1];
        long updatetype = args[2];
        t_object *src = args[3];
        long srcout = args[4];
        t_object *dst = args[5];
        long dstin = args[6];
        
        if (x==src && srcout==0)
        {
            t_symbol *srcname = object_attr_getsym(x, _jit_sym_name);
            t_symbol *dstname = object_attr_getsym(dst, _jit_sym_name);

            switch (updatetype)
            {
            case JPATCHLINE_CONNECT:
                break;
            case JPATCHLINE_DISCONNECT:
                break;
            case JPATCHLINE_ORDER:
                break;
            }
        }*/

    }END


};

MIN_EXTERNAL(jit_mo_gen);

