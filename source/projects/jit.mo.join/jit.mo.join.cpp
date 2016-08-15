/// @file
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Rob Ramirez
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"

using namespace c74::min;
using namespace c74::max;

void jit_mo_join_update_anim(t_object *job, t_atom *a);
void jit_mo_join_attach(t_object *job, t_object *cob);
void jit_mo_join_detach(t_object *job, t_object *cob);

t_jit_err max_jit_mo_join_dim(max_jit_wrapper *mob, t_symbol *attr, long argc, t_atom *argv);
t_jit_err max_jit_mo_join_jit_matrix(max_jit_wrapper *x, t_symbol *s, long argc, t_atom *argv);
void max_jit_mo_join_int(max_jit_wrapper *mob, long v);
void max_jit_mo_join_addfuncob(max_jit_wrapper *mob, t_object *ob);
void max_jit_mo_join_removefuncob(max_jit_wrapper *mob, t_object *ob);

static const symbol ps_automatic = "automatic";

// static singleton class to hold unbound jit.mo.func objects.
// checked when any jit.mo.join name attribute is set.
// allows for setting jit.mo.func join attribute prior to creation of that object

class funcob_container {
    std::vector <t_object*>funcobs;
    
    funcob_container() {}
    
public:
    void add_ob(t_object *o) {
        funcobs.push_back(o);
    }
    
    void remove_ob(t_object *o) {
        int i = 0;
        for(auto a : funcobs) {
            if(a == o) {
                funcobs.erase(funcobs.begin() + i);
                break;
            }
            i++;
        }
    }
    
    void check_obs(t_symbol* joinname) {
        if(funcobs.size()) {
            typedef std::vector <t_object*>::iterator fobitr;
            
            fobitr iter = funcobs.begin();
            while (iter != funcobs.end()) {
                t_object *o = *iter;
                t_symbol* joinsym = object_attr_getsym(o, gensym("join"));
                if(joinname == joinsym) {
                    t_object *joinob = (t_object*)object_findregistered(_jit_sym_jitter, joinname);
                    if(joinob) {
                        object_method(joinob, gensym("attach"), o);
                        iter = funcobs.erase(iter);
                        continue;
                    }
                }
                ++iter;
            }
        }
    }

    static funcob_container &instance() {
        static funcob_container s_instance;
        return s_instance;
    }
};

class jit_mo_join : public object<jit_mo_join>, matrix_operator {
public:
    
	jit_mo_join(const atoms& args = {}) {}
	~jit_mo_join() {
        for(auto a : m_attached) {
            funcob_container::instance().add_ob(a.second);
        }
        jit_object_free(animator);
    }
	
    outlet	output	= { this, "(matrix) Output", "matrix" };
    
    attribute<double> speed { this, "speed", 1.0, title {"Speed"} };
    
    attribute<symbol> name { this, "name", _jit_sym_nothing, title {"Name"},
        setter { MIN_FUNCTION {
            if(initialized()) {
                t_object *o = nullptr;
                symbol new_name = args[0];
                symbol nothing = _jit_sym_nothing;
                
                if (name == new_name)
                    return args;
                
                if ((o = (t_object*)object_findregistered(_jit_sym_jitter, new_name))) {
                    //error("name %s already in use. anode does not allow multiple bindings", new_name->s_name);
                    symbol old_name = name;
                    return {old_name};
                }
                
                if(!(name == nothing))
                    object_unregister(m_maxobj);
                
                if(!(new_name == nothing))
                    object_register(_jit_sym_jitter, new_name, m_maxobj);
                
                for( const auto& n : m_attached )
                    object_attr_setlong(n.second, gensym("join"), new_name);
                
                funcob_container::instance().check_obs(new_name);
            }
        
            return args;
        }}
    };
    
	template<class matrix_type, size_t planecount>
	cell<matrix_type,planecount> calc_cell(cell<matrix_type,planecount> input, const matrix_info& info, matrix_coord& position) {
		cell<matrix_type,planecount> output;
		return output;
	}
    
    template<typename U>
    void calculate_vector(const matrix_info& info, long n, t_jit_op_info* inop, t_jit_op_info* outop) {
        auto in = (U*)inop->p;
        auto out = (U*)outop->p;
        const auto instep = info.in_info->planecount;
        const auto outstep = info.out_info->planecount;
        
        // TODO: allow multiplane input
        for (auto j=0; j<n; ++j) {
            for (auto k=0; k<instep && k<outstep; ++k)
                out[plane] += *(in+instep*k);
            
            in += instep;
            out += outstep;
        }
    }
	
    void update_mop_props(void *mob) {
        t_jit_matrix_info info;
        void *mop=max_jit_obex_adornment_get(mob,_jit_sym_jit_mop);
        void *p=object_method((t_object*)mop,_jit_sym_getoutput,(void*)1);
        void *m=object_method((t_object*)p,_jit_sym_getmatrix);
        object_method((t_object*)m,_jit_sym_getinfo,&info);
        count = info.dim[0];
        type = info.type;
    }
	
    void update(t_atom *av) {
        if(enable) {
            for( const auto& n : m_attached ) {
                //const time_interval& speed_val = interval;
                //double secs = speed_val;
                //secs /= 1000.;
                
                object_attr_setfloat(n.second, sym_delta, atom_getfloat(av)*speed);
                object_method(maxob_from_jitob(n.second), sym_bang);
            }
            
            if(mob)
                max_jit_mop_outputmatrix(mob);
            
            request_clear = true;
        }
    }
    
    void update_attached_dim(long dim) {
        count = dim;
        for( const auto& n : m_attached )
            object_attr_setlong(maxob_from_jitob(n.second), _jit_sym_dim, count);
    }
    
    void attach(t_object *child) {
        m_attached.insert({namefromobptr(child), child});
        update_attached_dim(count);
    }
    
    void detach(t_object *child) {
        m_attached.erase(namefromobptr(child));
    }
    
    void do_int(long v) {
        enable = (bool)v;
    }
    
    bool enable = true;
	int plane = 0;
    bool request_clear = true;
    t_object *animator = nullptr;
    t_object *mob = nullptr;
    
private:
    
    message jitclass_setup = { this, "jitclass_setup", MIN_FUNCTION {
        t_class *c = args[0];

        // add mop
        auto mop = jit_object_new(_jit_sym_jit_mop, -1, 1);
        auto o = object_method(mop,_jit_sym_getoutput,(void*)1);
        jit_attr_setlong(o,_jit_sym_planelink,0);
        jit_class_addadornment(c, mop);
        
        //add methods
        jit_class_addmethod(c, (method)jit_mo_join_update_anim, "update_anim", A_CANT, 0);
        jit_class_addmethod(c, (method)jit_mo_join_attach,      "attach", A_CANT, 0);
        jit_class_addmethod(c, (method)jit_mo_join_detach,      "detach", A_CANT, 0);
        
        jit_class_addinterface(c, jit_class_findbyname(gensym("jit_anim_animator")), calcoffset(minwrap<jit_mo_join>, min_object) + calcoffset(jit_mo_join, animator), 0);
        
        return {};
    }};

    message maxclass_setup = { this, "maxclass_setup", MIN_FUNCTION {
        t_class *c = args[0];
        
        max_jit_class_mop_wrap(c, this_jit_class, MAX_JIT_MOP_FLAGS_OWN_JIT_MATRIX|MAX_JIT_MOP_FLAGS_OWN_DIM);
        max_jit_class_wrap_standard(c, this_jit_class, 0);
        
        auto attr = jit_object_new(_jit_sym_jit_attr_offset_array,"dim",_jit_sym_long,JIT_MATRIX_MAX_DIMCOUNT, ATTR_GET_DEFER_LOW|ATTR_SET_USURP_LOW,
                              (method)max_jit_mop_getdim,(method)max_jit_mo_join_dim,0);
        max_jit_class_addattr(c,attr);
        
        class_addmethod(c, (method)max_jit_mo_join_jit_matrix,  "jit_matrix", A_GIMME, 0);
        class_addmethod(c, (method)max_jit_mo_join_int,         "int", A_LONG, 0);
        class_addmethod(c, (method)max_jit_mo_join_addfuncob,   "addfuncob", A_CANT, 0);
        class_addmethod(c, (method)max_jit_mo_join_removefuncob, "removefuncob", A_CANT, 0);
        
        return {};
    }};
    
    message setup = { this, "setup", MIN_FUNCTION {
        //name = symbol_unique();
        animator = jit_object_new(gensym("jit_anim_animator"), m_maxobj);
        return {};
    }};
    
    message mob_setup = { this, "mob_setup", MIN_FUNCTION {
        void *o = m_maxobj;
        t_object *x = args[args.size()-1];
        
        t_jit_matrix_info info;
        long i,n;
        
        max_jit_obex_jitob_set(x,o);
        max_jit_obex_dumpout_set(x,outlet_new(x,NULL));
        max_jit_mop_setup(x);
        
        //add fake inputs + one real input
        if (args.size() > 1) {
            n = clamp<int>(args[0], 1, JIT_MATRIX_MAX_PLANECOUNT);
        }
        else {
            n=3;
        }
        
        for (i = n; i > 1; i--) max_jit_obex_proxy_new(x, i-1); //right to left
        
        max_jit_mop_variable_addinputs(x,1);//only used to fake out the matrix calc method
        max_jit_mop_inputs(x);
        max_jit_mop_outputs(x);
        
        jit_attr_setsym(x, _jit_sym_type, _jit_sym_float32);
        
        //set planecount since it is not linked.
        void *m = max_jit_mop_getoutput(x,1);
        jit_attr_setlong(m,_jit_sym_planecount,n);
        
        if(args.size() > 3) {
            long argc = 3;
            t_atom argv[3];
            
            for(int i = 0; i < 3; i++)
                argv[i] = args[i];
            
            max_jit_mop_matrix_args(x,argc,argv);
        }
        else {
            long dim = (args.size() > 2 ? (long)args[1] : 1);
            jit_attr_setlong(x, _jit_sym_dim, dim);
        }
        
        //else if ((max_jit_attr_args_offset(argc,argv)<=1) && (mop=max_jit_obex_adornment_get(x,_jit_sym_jit_mop)))
        //    jit_attr_setlong(mop,_jit_sym_adapt,1);
        
        update_mop_props(x);

        return {};
    }};
    
    message maxob_setup = { this, "maxob_setup", MIN_FUNCTION {
        if(name == symbol(""))
            name = symbol_unique();
        
        return {};
    }};

    typedef enum _patchline_updatetype {
        JPATCHLINE_DISCONNECT=0,
        JPATCHLINE_CONNECT=1,
        JPATCHLINE_ORDER=2
    } t_patchline_updatetype;
    
    message patchlineupdate = { this, "patchlineupdate", MIN_FUNCTION {
        t_object *x = args[0];
       // t_object *patchline = args[1];
        long updatetype = args[2];
        t_object *src = args[3];
        long srcout = args[4];
        t_object *dst = args[5];
        //long dstin = args[6];
        symbol n = name;
        
        // TODO: we could allow setting dim on any object with a dim attribute
        if (x == dst && srcout == 0 && object_classname_compare(src, gensym("jit.mo.func"))) {

            switch (updatetype) {
                // TODO: create patchcord_join attribute to distiguish from explicit user setting
                case JPATCHLINE_CONNECT:
                    //m_attached.insert({name, src});
                    //object_attr_setlong(src, _jit_sym_planecount, 1);
                    //object_attr_setsym(src, _jit_sym_type, type);
                    object_attr_setlong(src, _jit_sym_dim, count);
                    object_attr_setsym(src, gensym("join"), n);
                    
                    break;
                case JPATCHLINE_DISCONNECT:
                    //m_attached.erase(name);
                    object_attr_setsym(src, gensym("join"), _jit_sym_nothing);
                    break;
                case JPATCHLINE_ORDER:
                    break;
            }
        }
        return {};
    }};
    
    std::string namefromobptr(t_object *ob) {
        std::stringstream ss;
        ss << ob;
        return ss.str();
    }
    
    t_object *maxob_from_jitob(t_object *job) {
        t_object *mwrap=NULL;
        object_obex_lookup(job, sym_maxwrapper, &mwrap);
        return mwrap;
    }
    
    std::unordered_map<std::string, t_object*> m_attached;
    t_symbol *type = _jit_sym_float32;
    long count = 1;
    
    const symbol sym_bang = "bang";
    const symbol sym_delta = "delta";
    const symbol sym_maxwrapper = "maxwrapper";

};

MIN_EXTERNAL(jit_mo_join);

void jit_mo_join_update_anim(t_object *job, t_atom *a)
{
    minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
    self->min_object.update(a);
}

void jit_mo_join_attach(t_object *job, t_object *cob)
{
    minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
    self->min_object.attach(cob);
}

void jit_mo_join_detach(t_object *job, t_object *cob)
{
    minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
    self->min_object.detach(cob);
}

t_jit_err max_jit_mo_join_dim(max_jit_wrapper *mob, t_symbol *attr, long argc, t_atom *argv)
{
    if(argv && argc) {
        void* job = max_jit_obex_jitob_get(mob);
        minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
        self->min_object.update_attached_dim(atom_getlong(argv));
        max_jit_mop_dim(mob, attr, argc, argv);
    }
    
    return JIT_ERR_NONE;
}

void max_jit_mo_join_int(max_jit_wrapper *mob, long v)
{
    void* job = max_jit_obex_jitob_get(mob);
    minwrap<jit_mo_join>* self = (minwrap<jit_mo_join>*)job;
    self->min_object.do_int(v);
}

void max_jit_mo_join_addfuncob(max_jit_wrapper *mob, t_object *ob)
{
    funcob_container::instance().add_ob(ob);
}

void max_jit_mo_join_removefuncob(max_jit_wrapper *mob, t_object *ob)
{
    funcob_container::instance().remove_ob(ob);
}

t_jit_err max_jit_mo_join_jit_matrix(max_jit_wrapper *x, t_symbol *s, long argc, t_atom *argv)
{
    void *mop;
    long err=JIT_ERR_NONE;
    
    if (!(mop=max_jit_obex_adornment_get(x,_jit_sym_jit_mop)))
        return JIT_ERR_GENERIC;
    
    if (argc&&argv) {
        t_symbol *matrixname = atom_getsym(argv);
        void *matrix = object_findregistered(_jit_sym_jitter, matrixname);
        if (matrix && object_method((t_object*)matrix, _jit_sym_class_jit_matrix)) {
            void *p = object_method((t_object*)mop,_jit_sym_getinput,(void*)1);
            object_method((t_object*)p,_jit_sym_matrix,matrix);
            jit_attr_setsym(p,_jit_sym_matrixname,matrixname);
            
            auto in_mop_io = (t_object*)object_method((t_object*)object_method((t_object*)mop,_jit_sym_getinputlist), _jit_sym_getindex, 0);
            auto out_mop_io = (t_object*)object_method((t_object*)object_method((t_object*)mop,_jit_sym_getoutputlist), _jit_sym_getindex, 0);
            auto in_matrix = (t_object*)object_method(in_mop_io, k_sym_getmatrix);
            auto out_matrix = (t_object*)object_method(out_mop_io, k_sym_getmatrix);
            
            if (!in_matrix || !out_matrix)
                return JIT_ERR_INVALID_PTR;
            
            t_jit_matrix_info in_minfo;
            t_jit_matrix_info out_minfo;
            object_method(in_matrix, _jit_sym_getinfo, &in_minfo);
            object_method(out_matrix, _jit_sym_getinfo, &out_minfo);
            
            t_jit_op_info	in_opinfo;
            t_jit_op_info	out_opinfo;
            
            auto n = out_minfo.dim[0];
            object_method(in_matrix, _jit_sym_getdata, &in_opinfo.p);
            object_method(out_matrix, _jit_sym_getdata, &out_opinfo.p);
            
            matrix_info info(&in_minfo, (char*)in_opinfo.p, &out_minfo, (char*)out_opinfo.p);
            minwrap<jit_mo_join>* job = (minwrap<jit_mo_join>*)max_jit_obex_jitob_get(x);
            job->min_object.mob = (t_object*)x;
            job->min_object.plane = max_jit_obex_inletnumber_get(x);
            
            if(job->min_object.request_clear) {
                object_method(out_matrix, _jit_sym_clear);
                job->min_object.request_clear = false;
            }
            
            auto in_savelock = object_method(in_matrix, _jit_sym_lock, (void*)1);
            auto out_savelock = object_method(out_matrix, _jit_sym_lock, (void*)1);
            
            if (in_minfo.type == _jit_sym_char)
                job->min_object.calculate_vector<c74::min::uchar>(info, n, &in_opinfo, &out_opinfo);
            else if (in_minfo.type == _jit_sym_long)
                job->min_object.calculate_vector<int>(info, n, &in_opinfo, &out_opinfo);
            else if (in_minfo.type == _jit_sym_float32)
                job->min_object.calculate_vector<float>(info, n, &in_opinfo, &out_opinfo);
            else if (in_minfo.type == _jit_sym_float64)
                job->min_object.calculate_vector<double>(info, n, &in_opinfo, &out_opinfo);
            
            
            object_method(out_matrix, _jit_sym_lock, out_savelock);
            object_method(in_matrix, _jit_sym_lock, in_savelock);
            
            if (job->min_object.plane==0 && object_attr_getlong(job, ps_automatic)==0 ) {
                max_jit_mop_outputmatrix(x);
                object_method(out_matrix, _jit_sym_clear);
            }
        } else {
            jit_error_code(x,JIT_ERR_MATRIX_UNKNOWN);
        }
    }
    
    return err;
}
