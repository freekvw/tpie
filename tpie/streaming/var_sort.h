// -*- mode: c++; tab-width: 4; indent-tabs-mode: t; eval: (progn (c-set-style "stroustrup") (c-set-offset 'innamespace 0)); -*-
// vi:set ts=4 sts=4 sw=4 noet :
// Copyright 2009, 2010, The TPIE development team
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
#ifndef _TPIE_STREAMING_SORT_H
#define _TPIE_STREAMING_SORT_H

#include <algorithm>
#include <iostream>
#include <tpie/mm_manager.h>
#include <tpie/streaming/concepts.h>
#include <tpie/streaming/memory.h>
#include <vector>
#include <tpie/var_file_stream.h>
#include <tpie/streaming/sort_base.h>

namespace tpie {
namespace streaming {

template <typename item_t, typename size_extractor_t>
struct var_merger_strategy {
	typedef std::pair<const item_t &, memory_size_type> qi_t;
	fixed_allocator<item_t> item;
	inline void store_item(qi_t & p, var_file_stream<item_t, size_extractor_t, fixed_allocator<item_t> > * stream) {
		std::swap(item, stream.allocator());
	}
	inline const item_t & fetch_item() {
		return item.allocate(0);
	}
};

template <typename item_t, typename comp_t, typename size_extractor_t, typename begin_data_t>
class var_sort_base: public sort_base_crtp<
	item_t, 
	comp_t, 
	var_merger_strategy<item_t, size_extractor_t>,
	begin_data_t, 
	var_file_stream<item_t, size_extractor_t, fixed_allocator<item_t> >,
	var_sort_base<item_t, comp_t, size_extractor_t, begin_data_t> 
	> {
protected:
	char * buffer;
	memory_size_type bufferSize;
	memory_size_type bufferItems;
	memory_size_type bufferBytes;
	memory_size_type maximumItemSize;
	stream_size_type size; //Number of items
	comp_t comp;

	size_extractor_t m_extract;
	//Todo i am a comparator and that is quite ugly!!
	bool operator()(memory_size_type ia, memory_size_type ib) const {
		const item_t & a = *reinterpret_cast<item_t*>(buffer + ia);
		const item_t & b = *reinterpret_cast<item_t*>(buffer + ib);
		return comp(a,b);
	};

	inline memory_size_type * index_begin() {
		return reinterpret_cast<memory_size_type*>(buffer+bufferSize)-bufferItems;
	}
	
	inline memory_size_type * index_end() {
		return reinterpret_cast<memory_size_type*>(buffer+bufferSize);
	}

	typedef var_file_stream<item_t, size_extractor_t, fixed_allocator<item_t> > vfs_t;
	typedef sort_base_crtp<item_t, 
						   comp_t, 
						   var_merger_strategy<item_t, size_extractor_t>,
						   begin_data_t, 
						   vfs_t,
						   var_sort_base<item_t, comp_t, size_extractor_t, begin_data_t> > parent_t;
	using parent_t::fileBase;
	using parent_t::m_blockFactor;
	using parent_t::memory_in;
	using parent_t::memory_out;
	using parent_t::nextFile;
	using parent_t::m_comp;
public:
	var_sort_base(comp_t comp=comp_t(), double blockFactor=1.0): parent_t(comp, blockFactor) {
		bufferSize = 0;
	};

	inline void allocateBuffer(stream_size_type) {
		memory_size_type mem = memory_in()  - parent_t::base_memory() - vfs_t::memory_usage();
		bufferSize = mem;
		//TODO ensure that mem is less then "consecutive_memory_available"
		buffer = new char[bufferSize];
		bufferItems=0;
		size=0;
	}

	void sortRun() {
		std::sort(index_begin(), index_end(), *this);
	}
	
	void flush() {
		sortRun();
		vfs_t stream(m_extract, fixed_allocator<item_t>(0), m_blockFactor);
		stream.open(name(nextFile));
		for(memory_size_type * i=index_begin(); i != index_end(); ++i) 
			stream.write(*reinterpret_cast<item_t*>(buffer+*i));
		size += bufferItems;
		bufferItems = 0;
		bufferBytes = 0;
	}

	inline memory_size_type calculateHighArity() const {
		return 2;
	}
	
	inline void push(const item_t & item) {
		size_extractor_t e;
		memory_size_type itemSize = e.size(*reinterpret_cast<const typename size_extractor_t::header_t*>(&item));
		maximumItemSize = std::max(maximumItemSize, itemSize);
		if ( (bufferItems+1) * sizeof(memory_size_type) + bufferBytes + itemSize > bufferSize)
			flush();
		*(reinterpret_cast<memory_size_type *>(buffer+bufferSize) - (bufferItems + 1)) = bufferBytes;
		memcpy(buffer+bufferBytes, reinterpret_cast<const char*>(&item), itemSize);
		bufferBytes += itemSize;
		++bufferItems;
	}
};


template <class dest_t,
		  class size_extractor_t,
		  class comp_t=std::less<typename dest_t::item_type>
		  >
class var_sort: public sort_crtp<
	dest_t, 
	var_sort_base<typename dest_t::item_type, comp_t, size_extractor_t, typename dest_t::begin_data_type>,
	var_sort<dest_t, size_extractor_t, comp_t> > {
private:
	typedef sort_crtp<dest_t, 
					  var_sort_base<
						  typename dest_t::item_type, comp_t, size_extractor_t, typename dest_t::begin_data_type>,
					  var_sort<dest_t, size_extractor_t, comp_t> > parent_t;
public:
	typedef typename dest_t::item_type item_type;
	var_sort(dest_t & dest, comp_t comp=comp_t(), double blockFactor=1.0): parent_t(dest, comp, blockFactor) {}

	inline memory_size_type mergeArity() {
		return 2;
		//memory_fits<pull_stream_source< ami::stream<item_type> > >::fits(memoryOut() - baseMinMem());
	}

	inline void pushBuffer() {
		for(memory_size_type * i=parent_t::index_begin(); i != parent_t::index_end(); ++i) 
			parent_t::m_dest.push(*reinterpret_cast<item_type*>(parent_t::buffer+*i));
	}
	
};


}
}
#endif //_TPIE_STREAMING_SORT_H
