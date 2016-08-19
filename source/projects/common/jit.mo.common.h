/// @file
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Rob Ramirez
///	@license	Usage of this file and its contents is governed by the MIT License

#pragma once

#include "c74_min.h"

namespace jit_mo {

    static const c74::min::symbol sym_automatic = "automatic";
    static const c74::min::symbol sym_bang = "bang";
    static const c74::min::symbol sym_delta = "delta";
    static const c74::min::symbol sym_maxwrapper = "maxwrapper";
    static const c74::min::symbol sym_drawto = "drawto";
    static const c74::min::symbol sym_dest_closing = "dest_closing";
    
    namespace functypes {
        static const c74::min::symbol sin = "sin";
        static const c74::min::symbol saw = "saw";
        static const c74::min::symbol line = "line";
        static const c74::min::symbol tri = "tri";
        static const c74::min::symbol perlin = "perlin";
        static const c74::min::symbol function = "function";
    };
    
    double update_phase_frome_delta(double delta, double phase, double speed, bool loop) {
        double pval = phase + (delta * speed * 2.0); // default is one cycle / second
        
        if(loop || pval < 2.0) {
            phase = std::fmod(pval, 2.0);
        }
        return phase;
    }
    
    c74::max::t_object *maxob_from_jitob(c74::max::t_object *job) {
        c74::max::t_object *mwrap=NULL;
        object_obex_lookup(job, sym_maxwrapper, &mwrap);
        return mwrap;
    }
    
    /*double func_calc(const c74::min::atoms &args) {
        double val = 0;
        //double norm = (double)position.x() / (double)(info.out_info->dim[0]-1);
        double inputval = args[0];
        c74::min::symbol functype = args[1];
        double freq = args[2];
        double phase = args[3];
        double start = args[4];
        double end = args[5];
        long period = args[6];
        
        if(functype == functypes::sin) {
            val = (inputval * 2. - 1.) * freq + phase;
            val = sin(val*M_PI);
        }
        else if (functype == functypes::tri) {
            val = inputval * freq * 2.0 + phase;
            val = c74::min::math::fold(val, 0., 1.);
            val = (val * 2.0 - 1.0);
        }
        else if (functype == functypes::line) {
            if(inputval == 0.)
                val = start;
            else if(inputval == 1.)
                val = end;
            else
                val = (start*(1.-inputval) + end*inputval);
        }
        else if (functype == functypes::perlin) {
            val = (fmod(inputval * 2. * freq + phase, 2.0) - 1.) * (double)period;
            val = pnoise1(val, period);
        }
        
        return val;
    }*/
    
    struct interval_speed {
        
        bool update(c74::min::attribute<double>& speed, c74::min::attribute<c74::min::time_value>& interval) {
            double msecs = interval;
            if(msecs != interval_ms ) {
                interval_ms = msecs;
                speed = 1.0 / (msecs / 1000.);
                return true;
            }
            return false;
        };
        
        double interval_ms = 0;
    };

}