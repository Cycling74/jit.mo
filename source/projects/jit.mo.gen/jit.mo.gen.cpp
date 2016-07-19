/// @file	
///	@ingroup 	minexamples
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Timothy Place
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"

using namespace c74::min;
using namespace c74::max;
using namespace std;

namespace gentypes {
    static const symbol sin = "sin";
    static const symbol line = "line";
    static const symbol saw = "saw";
};

class jit_mo_gen : public object<jit_mo_gen>, matrix_operator {
public:
	
	outlet	output	= { this, "(matrix) Output", "matrix" };
	
	jit_mo_gen(const atoms& args = {}) {
        randval = math::random(-1., 1.);
    }
	~jit_mo_gen() {}
    
    c74::min::method setup = { this, "setup", MIN_FUNCTION {
        const t_symbol *s = classname();
        if(s == gensym("jit.mo.line"))
            gentype = gentypes::line;
        else if(s == gensym("jit.mo.saw"))
            gentype = gentypes::saw;
        
        return {};
    }};
    
	attribute<symbol> gentype = {
		this,
		"gentype",
		gentypes::sin,
		title {"Generator Type"},
		range {gentypes::line, gentypes::sin, gentypes::saw}
	};
    
    attribute<double> amp = { this, "amp", 1, title {"Amplitude"} };
	
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
        if(args.size() < 3) {
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

    
	template<class matrix_type, size_t planecount>
	cell<matrix_type,planecount> calc_cell(cell<matrix_type,planecount> input, const matrix_info& info, matrix_coord& position) {
		cell<matrix_type,planecount> output;
		double norm = (double)position.x() / (double)(info.out_info->dim[0]-1);
        phase = phase + (delta * speed);
        phase = mod_float64(phase, 2.0);
        
        if(gentype == gentypes::sin) {
            double val = (norm * 2. - 1.) * freq + phase;
            val *= M_PI;
            output[0] = sin(val) * amp;
        }
        else if (gentype == gentypes::saw) {
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
        
        if(rand_amt) {
            if(position.x() >= randvals.size())
                randvals.push_back(math::random(-1., 1.));
            else if(reseed)
                randvals[position.x()] = math::random(-1., 1.);
            
            output[0] += randvals[position.x()]*rand_amt;
            
            if(position.x() == info.out_info->dim[0]-1)
                reseed = false;
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
    
    vector<double>randvals;
    bool reseed = true;
    double randval;
    
};

MIN_EXTERNAL(jit_mo_gen);

