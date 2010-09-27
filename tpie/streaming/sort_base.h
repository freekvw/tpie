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
#ifndef __TPIE_STREAMING_SORT_BASE_H__
#define  __TPIE_STREAMING_SORT_BASE_H__

#include <tpie/mm_manager.h>
#include <tpie/streaming/memory.h>
#include <tpie/streaming/stream.h>
#include <tpie/array.h>
#include <memory>

namespace tpie {
namespace streaming {
	
template <typename item_t, 
		  typename comp_t, 
		  typename merger_strat_t,
		  typename begin_data_t, 
		  typename file_stream_t,
		  typename child_t>
class sort_base_crtp: public memory_split {
public:
	typedef item_t item_type;
	typedef begin_data_t begin_data_type;
	typedef comp_t comp_type;
protected:
	comp_t m_comp;
	std::string fileBase;
	memory_size_type firstFile;
	memory_size_type nextFile;
	begin_data_type * beginData;
	double m_blockFactor;
private:
	sort_base_crtp(const sort_base_crtp &);
	child_t & child() {return *reinterpret_cast<child_t*>(this);}
public:
	typedef file_stream_t file_stream_type;

	sort_base_crtp(comp_t comp=comp_t(), double blockFactor=1.0):
		m_comp(comp), m_blockFactor(blockFactor) {
		fileBase = tempname::tpie_name("ssort");
	}
	
	virtual memory_size_type minimum_memory_in() {
		return base_memory() + //For our selves
			file_stream_t::memory_usage() + //To write output
			2*MM_manager.space_overhead() + //Overhead on buffer and stream allocation
			file_stream_t::memory_usage();//Questimate one the minimal buffersize needed for resonable performance
	}

	virtual memory_size_type minimum_memory_out() {
		return base_memory() + 3*file_stream_t::memory_usage() + 3*MM_manager.space_overhead();
	}

	virtual memory_size_type base_memory() {
		return fileBase.capacity() + MM_manager.space_overhead();
	}

	void begin(stream_size_type items=max_items, begin_data_type * data=0) {
		assert(memory_out() >= minimum_memory_out());
		assert(memory_in() >= minimum_memory_in());
		beginData=data;
		firstFile=0;
		nextFile=0;
		child().allocateBuffer(items);
	}

	std::string name(memory_size_type number) {
		std::ostringstream ss;
		ss << fileBase << "_" << number;
		return ss.str();
	}
	
	class Merger {
	private:
		//TODO should we use reference for fixed_item_sort??
		typedef std::pair<const item_t *, memory_size_type> qi_t;	
		merger_strat_t strat;
	public:
		struct qcomp_t: public std::binary_function<qi_t, qi_t, bool> {
			comp_t comp;
			qcomp_t (const comp_t & _): comp(_) {}
			inline bool operator()(const qi_t & a, const qi_t & b) const {
				return comp(*b.first, *a.first);
			}
		};
		
		std::string name(memory_size_type number) {
			std::ostringstream ss;
			ss << fileBase << "_" << number;
			return ss.str();
		}
		
		qcomp_t comp;
		std::string fileBase;
		memory_size_type first;
		memory_size_type last;
		child_t & sortBase;  
									 
		tpie::array<std::auto_ptr<file_stream_t> > streams;
		//TODO change to a tpie internal queue of fixed size		
		std::priority_queue<qi_t, std::vector<qi_t>, qcomp_t> queue;
		
		Merger(const std::string & fb,
			   const comp_t & c,
			   child_t & sb,  
			   memory_size_type f,
			   memory_size_type l
			): strat(sb.mergerStrategy()), comp(c), fileBase(fb), first(f), last(l), sortBase(sb), queue(comp) {
			streams.resize(last-first);
			assert(last-first >= 2);
			for (memory_size_type i=0; i < last-first; ++i) {
				streams[i].reset(sb.create_stream());
				streams[i]->open(name(i+first));
				if (streams[i]->can_read()) queue.push(std::make_pair(&streams[i]->read(), i));
			}
		}
		
		~Merger() {
			for (memory_size_type i=0; i < last-first; ++i)
				remove(name(i+first).c_str());
		}
		
		inline const item_t & pull() {
			qi_t p = queue.top();
			strat.store_item(p, streams[p.second].get());
			queue.pop();
			if (streams[p.second]->can_read())
				queue.push(std::make_pair(&streams[p.second]->read(), p.second));;
			return strat.fetch_item();
		}
		
		inline bool can_pull() {return !queue.empty();}
	};

	void baseMerge(memory_size_type arity) {
		memory_size_type avail = MM_manager.memory_available();
		avail -= file_stream_t::memory_usage(m_blockFactor) - MM_manager.space_overhead();
		memory_size_type highArity = std::min(child().calculateArity(avail), available_files()-1);
		while (nextFile - firstFile > arity ) {
			memory_size_type count = std::min(nextFile - firstFile - arity+1, highArity);
			std::cout << "Internal merge: arity=" << count << std::endl;
			file_stream_t * stream = child().create_stream();
			stream->open(name(nextFile));
			{
				Merger merger(fileBase, m_comp, child(), firstFile, firstFile+count);
				while (merger.can_pull()) stream->write(merger.pull());
			}
			firstFile += count;
			nextFile += 1;
			delete stream;
		}
	}
};

template <typename end_data_type, typename pull_begin_data_type,
		  typename pull_end_data_type, typename parent_t, typename child_t>
class pull_sort_crtp: public parent_t {
private:
	typename parent_t::Merger * merger;
	end_data_type * endData;
	memory_size_type index;

	inline child_t & child() {return *reinterpret_cast<child_t*>(this);}
public:
	pull_sort_crtp(typename parent_t::comp_type comp, double blockFactor): parent_t(comp, blockFactor) {}

	virtual memory_size_type base_memory() {
		return parent_t::base_memory() + sizeof(child_t);
	}

	void end(end_data_type * data=0) {
		endData=data;
		merger=0;
		if (parent_t::nextFile == 0) {
			parent_t::sortRun();
			index=0;
			return;
		}
		parent_t::flush();
		delete[] parent_t::buffer;
		parent_t::buffer=0;
		memory_size_type arity = child().mergeArity();
		parent_t::baseMerge(arity);
	}
	
	inline void pull_begin(stream_size_type * items=0, pull_begin_data_type * data=0) {
		unused(data);
		if (parent_t::nextFile != 0) {
			if (items) *items=parent_t::size;
			merger = new typename parent_t::Merger(
				parent_t::fileBase, parent_t::m_comp, child(), 
				parent_t::firstFile, parent_t::nextFile);
		} else {
			if (items) *items=parent_t::bufferItems;
			child().reset_buffer_pointer();
		}
	}
	
	inline const typename parent_t::item_type & pull() {
		if (parent_t::nextFile == 0)
			return child().next_buffer_item();
		else
			return merger->pull();
	}

	inline bool can_pull() {
		if (parent_t::nextFile == 0)
			return child().has_next_buffer_item();
		else
			return merger->can_pull();
	}
	
	inline void pull_end(pull_end_data_type * data=0) {
		unused(data);
		if (parent_t::buffer) delete[] parent_t::buffer;
		parent_t::buffer = 0;
		if (merger) delete merger;
		merger = 0;
	}
};

template <typename dest_t, typename parent_t, typename child_t>
class sort_crtp: public parent_t {
protected:
	dest_t & m_dest;
	inline child_t & child() {return *reinterpret_cast<child_t*>(this);}
public:	
	sort_crtp(dest_t & d, typename parent_t::comp_type comp, double blockFactor): parent_t(comp, blockFactor), m_dest(d) {}
	typedef typename dest_t::item_type item_type;
	typedef typename dest_t::begin_data_type begin_data_type;
	typedef typename dest_t::end_data_type end_data_type;

	virtual memory_size_type base_memory() {
		return parent_t::base_memory() + sizeof(child_t);
	}

	void end(end_data_type * endData=0) {	   
		if (parent_t::nextFile == 0) {
			if (child().bufferMemoryUsage() >= parent_t::memory_out())
				child().compressBuffer(parent_t::memory_out());

			if (child().bufferMemoryUsage() <= parent_t::memory_out()) {
				parent_t::sortRun();
				m_dest.begin(parent_t::bufferItems, parent_t::beginData);
				child().pushBuffer();
				delete[] parent_t::buffer;
				parent_t::buffer = 0;
				m_dest.end(endData);
				return;
			}
		}
		parent_t::flush();
		delete[] parent_t::buffer;
		parent_t::buffer = 0;
		
		if (parent_t::nextFile == 1) {
			typename parent_t::file_stream_type * stream = child().create_stream();
			stream->open(parent_t::name(0));
			std::cout << "Final merge replaced by a scan" << std::endl;
			m_dest.begin(parent_t::size, parent_t::beginData);
			while(stream->can_read())
				m_dest.push(stream->read());
			m_dest.end(endData);
			return;
			
		}
		
		memory_size_type avail = parent_t::memory_out();
		memory_size_type arity = std::min(child().calculateArity(avail), available_files()-1);
		parent_t::baseMerge(arity);
		std::cout << "Final merge: arity=" << std::min(arity, parent_t::nextFile) << std::endl;
		m_dest.begin(parent_t::size, parent_t::beginData);
		{
			typename parent_t::Merger merger(parent_t::fileBase, parent_t::m_comp, child(),
											 parent_t::firstFile, parent_t::nextFile);
			while(merger.can_pull()) m_dest.push(merger.pull());
		}		
		m_dest.end(endData);
	}
	
	virtual void memory_next(std::vector<memory_base *> & next) {
		next.push_back(&m_dest);
	}
};


}
}
#endif //__TPIE_STREAMING_SORT_BASE_H__


