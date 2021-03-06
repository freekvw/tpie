// -*- mode: c++; tab-width: 4; indent-tabs-mode: t; c-file-style: "stroustrup"; -*-
// vi:set ts=4 sts=4 sw=4 noet :
// Copyright 2008, The TPIE development team
// 
// This file is part of TPIE.
// 
// TPIE is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the
// Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
// 
// TPIE is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with TPIE.  If not, see <http://www.gnu.org/licenses/>

// The tpie_stats class for recording statistics. The parameter C is
// the number of statistics to be recorded.
#ifndef _TPIE_STATS_H
#define _TPIE_STATS_H

// Get definitions for working with Unix and Windows
#include <tpie/portability.h>

namespace tpie {
    
///////////////////////////////////////////////////////////////////////////
/// Encapsulates statistics about a TPIE object.
/// \sa stats_stream
///////////////////////////////////////////////////////////////////////////
    template<int C>
    class stats {
    private:
	
	// The array storing the C statistics.
	TPIE_OS_OFFSET stats_[C];
	
    public:
	
	// Reset all counts to 0.
	void reset() {
	    for (int i = 0; i < C; i++)
		stats_[i] = 0;
	}

	// Default constructor. Set all counts to 0.
	stats() {
	    reset();
	}

	// Copy constructor.
	stats(const stats<C>& ts) {
	    for (int i = 0; i < C; i++)
		stats_[i] = ts.stats_[i];
	}

	// Record ONE event of type t.
	void record(int t) {
	    stats_[t]++;
	}

	// Record k events of type t.
	void record(int t, TPIE_OS_OFFSET k) {
	    stats_[t] += k;
	}

	// Record the events stored in s.
	void record(const stats<C>& s) {
	    for (int i = 0; i < C; i++)
		stats_[i] += s.stats_[i];
	}
	
	// Set the number of type t events to k.
	void set(int t, TPIE_OS_OFFSET k) {
	    stats_[t] = k;
	}
	
	// Inquire the number of type t events.
	TPIE_OS_OFFSET get(int t) const {
	    return stats_[t];
	}
	
	// Destructor.
	~stats() {}
    };

    template<int C>
    const stats<C> operator-(const stats<C> & lhs, 
			     const stats<C> & rhs) {
	stats<C> res;
	for (int i = 0; i < C; i++)
	    res.stats_[i] = lhs.stats_[i] - rhs.stats_[i];
	return res;
    }

}  //  tpie namespace

#endif //_TPIE_STATS_H
