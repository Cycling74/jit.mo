/// @file	
///	@ingroup 	minexamples
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Timothy Place
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"

using namespace c74::min;
using namespace c74::max;

class jit_mo_gen : public object, matrix_operator {
public:
	
	inlet	input	= { this, "(matrix) Input", "matrix" };
	outlet	output	= { this, "(matrix) Output", "matrix" };
	
	
	jit_mo_gen(atoms args) {}
	~jit_mo_gen() {}

	
	// TODO: mode attr for how to handle the edges
	

	ATTRIBUTE (name, symbol, _jit_sym_nothing) {
		double value = args[0];
		
		if (value < 0)
			value = 0;
		
		args[0] = value;
	}
	END

	
	template<class matrix_type, size_t planecount>
	cell<matrix_type,planecount> calc_cell(cell<matrix_type,planecount> input, const matrix_info& info, matrix_coord& position) {
		cell<matrix_type,planecount> output;
		
		return output;
	}
	
	
private:


};

typedef enum _patchline_updatetype {
	JPATCHLINE_DISCONNECT=0,
	JPATCHLINE_CONNECT=1,
	JPATCHLINE_ORDER=2
} t_patchline_updatetype;

t_max_err max_jit_gl_node_patchlineupdate(t_object *x, t_object *patchline, long updatetype, t_object *src, long srcout, t_object *dst, long dstin)
{
	t_max_err err=MAX_ERR_NONE;

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
	}
	return err;
}

void ext_main (void* resources) {

	std::string		maxname = c74::min::deduce_maxclassname(__FILE__);

	
	
	// 1. Boxless Jit Class

	c74::min::atoms	a;
	jit_mo_gen	dummy(a);
	
	c74::min::this_jit_class = (c74::max::t_class*)c74::max::jit_class_new(
																		   "jit_mo_gen",
																		   (c74::max::method)c74::min::jit_new<jit_mo_gen>,
																		   (c74::max::method)c74::min::jit_free<jit_mo_gen>,
																		   sizeof( c74::min::minwrap<jit_mo_gen> ),
																		   0);
	
	//add mop
	auto mop = c74::max::jit_object_new(c74::max::_jit_sym_jit_mop, 1, 1); // #inputs, #outputs
	c74::max::jit_class_addadornment(c74::min::this_jit_class, mop);

	//add methods
	c74::max::jit_class_addmethod(c74::min::this_jit_class, (c74::max::method)c74::min::jit_matrix_calc<jit_mo_gen>, "matrix_calc", c74::max::A_CANT, 0);

	//add attributes
	long attrflags = c74::max::ATTR_GET_DEFER_LOW | c74::max::ATTR_SET_USURP_LOW;
	

	for (auto& an_attribute : dummy.attributes) {
		std::string		attr_name = an_attribute.first;
		auto			attr = c74::max::jit_object_new(
											 c74::max::_jit_sym_jit_attr_offset,
											 attr_name.c_str(),
											 c74::max::_jit_sym_float64,
											 attrflags,
											 (c74::max::method)c74::min::min_attr_getter<jit_mo_gen>,
											 (c74::max::method)c74::min::min_attr_setter<jit_mo_gen>,
											 0
											 );
		c74::max::jit_class_addattr(c74::min::this_jit_class, attr);
	}
	
	jit_class_register(c74::min::this_jit_class);

	
	// 2. Max Wrapper Class
	
	c74::min::this_class = c74::max::class_new(
											   maxname.c_str(),
											   (c74::max::method)c74::min::max_jit_mop_new<jit_mo_gen>,
											   (c74::max::method)c74::min::max_jit_mop_free<jit_mo_gen>,
											   sizeof( c74::min::max_jit_wrapper ),
											   nullptr,
											   c74::max::A_GIMME,
											   0
											   );

	c74::max::max_jit_class_obex_setup(c74::min::this_class, calcoffset(c74::min::max_jit_wrapper, obex));
	
	c74::max::max_jit_class_mop_wrap(c74::min::this_class, c74::min::this_jit_class, 0);			// attrs & methods for name, type, dim, planecount, bang, outputmatrix, etc
	c74::max::max_jit_class_wrap_standard(c74::min::this_class, c74::min::this_jit_class, 0);		// attrs & methods for getattributes, dumpout, maxjitclassaddmethods, etc
	
	c74::max::class_addmethod(c74::min::this_class, (c74::max::method)c74::max::max_jit_mop_assist, "assist", c74::max::A_CANT, 0);	// standard matrix-operator (mop) assist fn
	c74::max::class_addmethod(c74::min::this_class, (c74::max::method)c74::max::max_jit_mop_assist, "patchlineupdate", c74::max::A_CANT, 0);
    
	// the menufun isn't used anymore, so we are repurposing it here to store the name of the jitter class we wrap
	c74::min::this_class->c_menufun = (c74::max::method)c74::max::gensym("jit_mo_gen");
	
	c74::max::class_register(c74::max::CLASS_BOX, c74::min::this_class);
}

