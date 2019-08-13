/// @file
///	@copyright	Copyright 2018 The Jit.Mo Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#pragma once

#include "c74_ui.h"
#include "c74_ui_graphics.h"
#include "jit.mo.common.h"

namespace jit_mo {

	using namespace c74::min;
	using namespace c74::max;

	// static singleton class to manage unbound mo.func objects
	// mo.func obs are checked when any jit.mo.join name attribute is set.

	class jit_mo_singleton {
	public:
		void add_funcob(t_object* o) {
			funcobs.push_back(o);
		}

		void remove_funcob(t_object* o) {
			int i = 0;
			for (auto a : funcobs) {
				if (a == o) {
					funcobs.erase(funcobs.begin() + i);
					break;
				}
				i++;
			}
		}

		void check_funcobs(t_symbol* joinname) {
			if (funcobs.size()) {
				typedef std::vector<t_object*>::iterator fobitr;

				fobitr iter = funcobs.begin();
				while (iter != funcobs.end()) {
					t_object* o       = *iter;
					t_symbol* joinsym = object_attr_getsym(o, gensym("join"));
					if (joinname == joinsym) {
						t_object* joinob = (t_object*)object_findregistered(_jit_sym_jitter, joinname);
						if (joinob) {
							object_method(joinob, gensym("attach"), o);
							iter = funcobs.erase(iter);
							continue;
						}
					}
					++iter;
				}
			}
		}

		static jit_mo_singleton& instance() {
			static jit_mo_singleton s_instance;
			return s_instance;
		}
		
	private:
		jit_mo_singleton() {}
		std::vector<c74::max::t_object*>                             funcobs;
	};

	void max_jit_mo_addfuncob(max_jit_wrapper* mob, t_object* ob) {
		jit_mo_singleton::instance().add_funcob(ob);
	}

	void max_jit_mo_removefuncob(max_jit_wrapper* mob, t_object* ob) {
		jit_mo_singleton::instance().remove_funcob(ob);
	}

}    // namespace jit_mo
