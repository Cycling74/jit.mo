/// @file
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Rob Ramirez
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"

using namespace c74::min;
using namespace c74::max;
using namespace std;

class jit_mo_field : public object<jit_mo_field>, matrix_operator {
public:
    MIN_DESCRIPTION { "Field manipulator for 3 plane jit.mo streams." };
    MIN_TAGS		{ "jit.mo, Manipulators" };
    MIN_AUTHOR		{ "Cycling '74" };
    MIN_RELATED		{ "jit.mo.join, jit.mo.func" };
    
    inlet	input	= { this, "(matrix) Input", "matrix" };
	outlet	output	= { this, "(matrix) Output", "matrix" };
	
	jit_mo_field(const atoms& args = {}) {}
	~jit_mo_field() {}
    
    attribute<double> force { this, "force", 0, title {"Force"},
        description {"Force amount (default = 0.0)." }
    };
	
    attribute<double> radius { this, "radius", 0.5, title {"Radius"},
        description {"Radius value (default = 0.5)." }
    };
    
    attribute<double> falloff { this, "falloff", 0.5, title {"Fall off"},
        description {"Falloff value (default = 0.5)." },
        setter { MIN_FUNCTION {
			return { std::fmax(args[0], 0.00001) };
		}}
    };
    
    attribute<vector<double>> translate { this, "translate", {0.0, 0.0, 0.0},
        title{"Translate"},
        description {"Translation amount (default = 0. 0. 0.)." }
    };
    
    attribute<vector<double>> location { this, "location", {0.0, 0.0, 0.0},
        title{"Location"},
        description {"Location value (default = 0. 0. 0.)." }
    };
    
    attribute<vector<double>> rand_amt { this, "rand_amt", {0.0, 0.0, 0.0},
        title{"Random Amount"},
        description {"Random offset amount (default = 0. 0. 0.)." },
    };
    
    message rand = { this, "rand", "Generate new random values for rand_amt offset.",
        MIN_FUNCTION {
            reseed = true;
            return {};
        }
    };
    
	template<class matrix_type, size_t planecount>
	cell<matrix_type,planecount> calc_cell(cell<matrix_type,planecount> input, const matrix_info& info, matrix_coord& position) {
        const vector<double>& location = this->location;
        const vector<double>& translate = this->translate;
        const vector<double>& rand_amt = this->rand_amt;
		cell<matrix_type,planecount> output;
        cell<double,planecount> norm;
        double length = 0;
        
        for(auto i=0 ; i<info.planecount(); i++) {
            norm[i] = input[i] - location[i];
            length = length+norm[i]*norm[i];
        }
        length = (length!= 0. ? sqrt(length) : 0.);
        
        for(auto i=0 ; i<info.planecount(); i++)
            norm[i] = (norm[i] / length) * force;
        
        const bool dorand(rand_amt[0] || rand_amt[1] || rand_amt[2]);
        
        if(dorand) {
            if(position.x() >= randvals.size())
                randvals.push_back(math::random(-1., 1.));
            else if(reseed)
                randvals[position.x()] = math::random(-1., 1.);
            
            if(position.x() == info.out_info->dim[0]-1)
                reseed = false;
        }

		double fradius = radius;
		double ffalloff = falloff;
		matrix_type diff = matrix_type(fradius - ffalloff);
        auto mix = smoothstep<matrix_type>(matrix_type(double(radius)), diff, length);
        
        for(auto i=0 ; i<info.planecount(); i++)
            output[i] = input[i] + (norm[i]*mix) + (translate[i]*mix) + (dorand ? (randvals[position.x()]*rand_amt[i]*mix) : 0);
        
		return output;
	}
    
    template<typename T>
    T smoothstep(const T& edge0, const T& edge1, const T& x) {
        T t = MIN_CLAMP((x - edge0) / (edge1 - edge0), 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }
	
	
private:
    vector<double>  randvals;
    bool            reseed = true;

};

MIN_EXTERNAL(jit_mo_field);

