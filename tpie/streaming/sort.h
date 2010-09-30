// -*- mode: c++; tab-width: 4; indent-tabs-mode: t; c-file-style: "stroustrup"; -*-
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

#include <tpie/streaming/concepts.h>


#include <vector>
#include <tpie/streaming/sort_base.h>

namespace tpie {
namespace streaming {

	
template <typename item_t>
struct merger_strategy {
	typedef std::pair<const item_t *, memory_size_type> qi_t;
	item_t item;
	inline void store_item(qi_t & p, file_stream<item_t> *) {item = *p.first;}
	inline const item_t & fetch_item() {return item;}
};


template <typename item_t, typename comp_t, typename begin_data_t>
class sort_base: public sort_base_crtp<item_t, 
									   comp_t, 
									   merger_strategy<item_t>,
									   begin_data_t, 
									   file_stream<item_t>,
									   sort_base<item_t, comp_t, begin_data_t> 
									   > {
protected:
	item_t * buffer;
	memory_size_type bufferSize;
	memory_size_type bufferItems;
	stream_size_type size;
	typedef sort_base_crtp<item_t, 
						   comp_t, 
						   merger_strategy<item_t>,
						   begin_data_t, 
						   file_stream<item_t>,
						   sort_base<item_t, comp_t, begin_data_t> > parent_t;
	using parent_t::fileBase;
	using parent_t::m_blockFactor;
	using parent_t::memory_in;
	using parent_t::memory_out;
	using parent_t::nextFile;
	using parent_t::m_comp;
public:
	sort_base(comp_t comp=comp_t(), double blockFactor=1.0): parent_t(comp, blockFactor) {};
	
	inline void allocateBuffer(stream_size_type items) {
		memory_size_type mem = parent_t::memory_in() - parent_t::base_memory() - file_stream<item_t>::memory_usage();
		//TODO ensure that mem is less then "consecutive_memory_available"
		bufferSize = std::min( stream_size_type(mem / sizeof(item_t)), items );
		buffer = new item_t[bufferSize];
		bufferItems=0;
		size=0;
	}

	inline memory_size_type bufferMemoryUsage() {return sizeof(item_t)*bufferSize + MM_manager.space_overhead();}
	inline void compressBuffer(memory_size_type target) {
		memory_size_type u=sizeof(item_t)*bufferItems + MM_manager.space_overhead();
		if (u >= target || u >= MM_manager.memory_available()) return;
		item_t * buffer2 = new item_t[bufferItems];
		memcpy(buffer2, buffer, sizeof(item_t)*bufferItems);
		delete[] buffer;
		buffer = buffer2;
		bufferSize = bufferItems;
	}

	inline void sortRun() {
		std::sort(buffer, buffer+bufferItems, m_comp);
	}

	void flush() {
		std::cout << "Flushing " << bufferItems << " items to disk" << std::endl;
		sortRun();
		file_stream<item_t> stream(m_blockFactor);
		stream.open(name(nextFile));
		item_t * end=buffer+bufferItems;
		for (item_t * item=buffer; item != end; ++item)
			stream.write(*item);
		size += bufferItems;
		bufferItems = 0;
		++nextFile;
	}

	inline memory_size_type calculateArity(memory_size_type memory) const {
		return memory / (file_stream<item_t>::memory_usage(m_blockFactor) + MM_manager.space_overhead());
	}

	inline file_stream<item_t> * create_stream() {
		return new file_stream<item_t>(m_blockFactor);
	}
	
	inline void push(const item_t & item) {
		if (bufferItems >= bufferSize) flush();
		buffer[bufferItems++] = item;
	}

	inline merger_strategy<item_t> mergerStrategy() {return merger_strategy<item_t>();}
};


template <typename item_t,
		  typename comp_t=std::less<item_t>,
		  typename begin_data_t=empty_type,
		  typename end_data_t=empty_type,
		  typename pull_begin_data_t=empty_type,
		  typename pull_end_data_t=empty_type >
class pull_sort: public pull_sort_crtp< end_data_t, pull_begin_data_t, pull_end_data_t,
										sort_base<item_t, comp_t, begin_data_t>, 
										pull_sort<item_t, comp_t, begin_data_t, end_data_t, pull_begin_data_t, pull_end_data_t> > {
private:
	typedef pull_sort_crtp< end_data_t, pull_begin_data_t, pull_end_data_t,
							sort_base<item_t, comp_t, begin_data_t>, 
							pull_sort<item_t, comp_t, begin_data_t, end_data_t, pull_begin_data_t, pull_end_data_t> > parent_t;
	size_t curItem;
public:
	typedef item_t item_type;
	typedef item_t pull_type;
	typedef begin_data_t begin_data_type;
	typedef end_data_t end_data_type;
	typedef pull_begin_data_t pull_begin_data_type;
	typedef pull_end_data_t pull_end_data_type;

	pull_sort(comp_t comp=comp_t(), double blockFactor=1.0): parent_t(comp, blockFactor) {};

	void reset_buffer_pointer() {curItem = 0;}
	bool has_next_buffer_item() {return curItem != parent_t::bufferItems;}
	item_t & next_buffer_item() {return parent_t::buffer[curItem++];}
};


template <class dest_t,
		  class comp_t=std::less<typename dest_t::item_type> >
class sort: public sort_crtp<dest_t, 
							 sort_base<typename dest_t::item_type, comp_t, typename dest_t::begin_data_type>,
							 sort< dest_t, comp_t> > {
private:
	typedef sort_crtp<dest_t, 
					  sort_base<typename dest_t::item_type, comp_t, typename dest_t::begin_data_type>,
					  sort< dest_t, comp_t> > parent_t;
public:
	sort(dest_t & dest, comp_t comp=comp_t(), double blockFactor=1.0): parent_t(dest, comp, blockFactor) {}

	inline void pushBuffer() {
		typename dest_t::item_type * end = parent_t::buffer+ parent_t::bufferItems;
		for (typename dest_t::item_type * item=parent_t::buffer; item != end; ++item)
			parent_t::m_dest.push(*item);
	}
	
};

}
}
#endif //_TPIE_STREAMING_SORT_H
