/// @file
///	@copyright	Copyright 2018 The Jit.Mo Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#pragma once

#include "c74_min.h"

namespace jit_mo {

	static const c74::min::symbol sym_automatic    = "automatic";
	static const c74::min::symbol sym_bang         = "bang";
	static const c74::min::symbol sym_delta        = "delta";
	static const c74::min::symbol sym_maxwrapper   = "maxwrapper";
	static const c74::min::symbol sym_drawto       = "drawto";
	static const c74::min::symbol sym_dest_closing = "dest_closing";
	static const c74::min::symbol sym_phase        = "phase";
	static const c74::min::symbol sym_attr_modified = "attr_modified";
	static const c74::min::symbol sym_jitter = "jitter";

	namespace functypes {
		static const c74::min::symbol sin      = "sin";
		static const c74::min::symbol saw      = "saw";
		static const c74::min::symbol line     = "line";
		static const c74::min::symbol tri      = "tri";
		static const c74::min::symbol perlin   = "perlin";
		static const c74::min::symbol function = "function";
	};    // namespace functypes


	double update_phase_frome_delta(double delta, double phase, double speed, bool loop) {
		double pval = phase + (delta * speed * 2.0);    // default is one cycle / second
		if (loop || pval < 2.0) {
			phase = std::fmod(pval, 2.0);
		}
		else if (pval >= 2.0) {
			phase = 2.0;
		}
		
		if(speed < 0 && phase < 0) {
			if(loop) {
				phase += 2.;
			}
			else {
				phase = 0.;
			}
		}
		return phase;
	}

	c74::max::t_object* maxob_from_jitob(c74::max::t_object* job) {
		c74::max::t_object* mwrap = NULL;
		object_obex_lookup(job, sym_maxwrapper, &mwrap);
		return mwrap;
	}

	struct interval_speed {
		bool update(c74::min::attribute<double>& speed, const c74::min::attribute<c74::min::time_value>& interval) {
			double msecs = interval;
			if(msecs > 0) {
				if (msecs != interval_ms || speed != last_interval_speed) {
					interval_ms = msecs;
					speed       = 1.0 / (msecs / 1000.);
					last_interval_speed = speed;
					return true;
				}
			}
			else {
				interval_ms = 0;
			}
			return false;
		};
		double interval_ms = 0;
		double last_interval_speed { 0. };
	};

}    // namespace jit_mo
