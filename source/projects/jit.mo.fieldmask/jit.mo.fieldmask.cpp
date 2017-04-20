/// @file
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Rob Ramirez
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"
#include "jit.mo.common.h"

using namespace c74::min;
using namespace c74::max;
using namespace std;

class jit_mo_fieldmask : public object<jit_mo_fieldmask>, matrix_operator {
public:
    MIN_DESCRIPTION { "Field mask for 3 plane jit.mo streams. Calculates a mask value depending on distance from a defined spatial location. Can be used to perform arbitrary manipulations based on location" };
    MIN_TAGS		{ "jit.mo,Manipulators" };
    MIN_AUTHOR		{ "Cycling '74" };
    MIN_RELATED		{ "jit.mo.join,jit.mo.func,jit.mo.time,jit.mo.field" };
    
    inlet<>	input	= { this, "(matrix) Input", "matrix" };
	outlet<>	output	= { this, "(matrix) Output", "matrix" };
	
	jit_mo_fieldmask(const atoms& args = {}) {}
	~jit_mo_fieldmask() {}
    
    attribute<double> radius { this, "radius", 0.5, title {"Radius"},
        description {"Radius value (default = 0.5). Radius defines the spherical area around the location affected by the field" }
    };
    
    attribute<double> falloff { this, "falloff", 0.5, title {"Fall off"},
        description {"Falloff value (default = 0.5). Determines the amount of falloff at the edge of the field, specified as a decimal fraction of the radius" },
        setter { MIN_FUNCTION {
			return { std::fmax(args[0], 0.00001) };
		}}
    };
    
    attribute<vector<double>> location { this, "location", {0.0, 0.0, 0.0},
        title{"Location"},
        description {"Location value (default = 0. 0. 0.). Defines the center point of the field." }
    };
    
    attribute<double> scale { this, "scale", 1, title {"Scale"},
        description { "Output multiplier (default = 1.0)." }
    };
    
    attribute<double> offset { this, "offset", 0, title {"Offset"},
        description { "Output offset (default = 0.0)." }
    };
    
	template<class matrix_type, size_t planecount>
	cell<matrix_type,planecount> calc_cell(cell<matrix_type,planecount> input, const matrix_info& info, matrix_coord& position) {
        const vector<double>& location = this->location;
		cell<matrix_type,planecount> output;
        cell<double,planecount> norm;
        double length = 0;
        
        for(auto i=0 ; i<info.planecount(); i++) {
            norm[i] = input[i] - location[i];
            length = length+norm[i]*norm[i];
        }
        length = (length!= 0. ? sqrt(length) : 0.);
        
        for(auto i=0 ; i<info.planecount(); i++)
            norm[i] = (norm[i] / length);
        
		double fradius = radius;
		double ffalloff = falloff;
		matrix_type diff = matrix_type(fradius - ffalloff);
        auto mix = smoothstep<matrix_type>(matrix_type(double(radius)), diff, length);
        
        for(auto i=0 ; i<info.planecount(); i++)
            output[i] = mix*scale+offset ;
    
		return output;
	}
    
    template<typename T>
    T smoothstep(const T& edge0, const T& edge1, const T& x) {
        T t = MIN_CLAMP((x - edge0) / (edge1 - edge0), 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }
	
	
private:
    
    message<> fileusage = { this, "fileusage", MIN_FUNCTION {
        jit_mo::fileusage(args[0]);
        return {};
    }};

};

MIN_EXTERNAL(jit_mo_fieldmask);

