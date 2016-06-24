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
	
	jit_mo_gen(const atoms& args = {}) {
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
		double norm = (double)position.x() / (double)info.out_info->dim[0];
        double snorm = norm*2. - 1.;
        
        if(gentype == s_sin) {
            double val = snorm * freq + phase;
            val *= M_PI;
            output[0] = sin(val) * amp;
        }
        else if (gentype == s_spread) {
            output[0] = (snorm * freq + phase) * amp;
        }
        else if (gentype == s_saw) {
            double val = norm * freq * 2.0 + phase;
            val = foldit(val, 0., 1.);
            output[0] = val *2.0 - 1.0;
        }
		return output;
	}
	
	
private:

    double foldit(double x, double lo, double hi)
    {
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
    
    const symbol s_sin = "sin";
    const symbol s_spread = "spread";
    const symbol s_saw = "saw";

};

MIN_EXTERNAL(jit_mo_gen);

