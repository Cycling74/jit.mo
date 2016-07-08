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
	
	jit_mo_gen(const atoms& args = {}) {}
	~jit_mo_gen() {}
    
    c74::min::method setup = { this, "setup", MIN_FUNCTION {
        const t_symbol *s = classname();
        if(s == gensym("jit.mo.line"))
            gentype = s_line;
        else if(s == gensym("jit.mo.saw"))
            gentype = s_saw;
        
        return {};
    }};
    
    attribute<symbol> gentype = { this, "gentype", gensym("sin"), title {"\"Generator Type\""}, range {"line", "sin","saw"} };
    
    attribute<double> amp = { this, "amp", 1, title {"Amplitude"} };
	
    attribute<double> freq = { this, "freq", 1, title {"Frequency"} };
    
    attribute<double> phase = { this, "phase", 0, title {"Phase"} };
    
    attribute<double> speed = { this, "speed", 0, title {"Speed"} };
    
    attribute<double> offset = { this, "offset", 0, title {"Offset"} };
    
    attribute<double> delta = { this, "delta", 0, title {"\"Delta Time\""} };
    
    attribute<double> start = { this, "start", -1., title {"\"Start Line\""} };
    
    attribute<double> end = { this, "end", 1., title {"\"End Line\""} };
    
        
		return {};
	}};
    
	template<class matrix_type, size_t planecount>
	cell<matrix_type,planecount> calc_cell(cell<matrix_type,planecount> input, const matrix_info& info, matrix_coord& position) {
		cell<matrix_type,planecount> output;
		double norm = (double)position.x() / (double)(info.out_info->dim[0]-1);
        phase = phase + (delta * speed);
        phase = mod_float64(phase, 2.0);
        
        if(gentype == s_sin) {
            double val = (norm * 2. - 1.) * freq + phase;
            val *= M_PI;
            output[0] = sin(val) * amp;
        }
        else if (gentype == s_saw) {
            double val = norm * freq * 2.0 + phase;
            val = foldit(val, 0., 1.);
            output[0] = (val * 2.0 - 1.0) * amp;
        }
        else {  // line
            //output[0] = snorm * amp;
            if(norm == 0.)
                output[0] = start * amp;
            else if(norm == 1.)
                output[0] = end * amp;
            else
                output[0] = (start*(1.-norm) + end*norm) * amp;
        }
        
        output[0] += offset;
		return output;
	}
	
	
private:

    static double foldit(double x, double lo, double hi)
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
    
    static double mod_float64(double a, double b)
    {
        t_int32 n = (t_int32)(a/b);
        a -= n*b;
        if (a < 0)
            return a + b;
        return a;
    }
    
    const symbol s_sin = "sin";
    const symbol s_line = "line";
    const symbol s_saw = "saw";

};

MIN_EXTERNAL(jit_mo_gen);

