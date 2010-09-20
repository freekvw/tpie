// -*- mode: c++; tab-width: 4; indent-tabs-mode: t; c-file-style: "stroustrup"; -*-
// vi:set ts=4 sts=4 sw=4 noet :
// Copyright 2010, The TPIE development team
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
#ifndef _TPIE_STREAMING_STREAM_H
#define _TPIE_STREAMING_STREAM_H
#include <tpie/file.h>
#include <tpie/stream.h>
#include <tpie/file_stream.h>
#include <tpie/streaming/memory.h>
#include <tpie/streaming/util.h>

namespace tpie {
namespace streaming {

template <typename item_t,
		  typename begin_data_t=empty_type,
		  typename end_data_t=empty_type>
class stream_sink: public memory_single {
public:
	typedef item_t item_type;
	typedef begin_data_t begin_data_type;
	typedef end_data_t end_data_type;
	typedef typename file<item_type>::stream stream_type;
private:
	stream_type & m_stream;
	begin_data_type * m_beginData;
	end_data_type * m_endData;
public:
	static memory_size_type memory() {
		return sizeof(stream_sink);
	}

	inline stream_sink(stream_type & stream) throw() :
		m_stream(stream),
		m_beginData(0),
		m_endData(0) {
		set_memory_priority(0.0);
	}

	inline stream_sink(file_stream<item_type> & stream) throw() :
		m_stream(stream.get_stream()),
		m_beginData(0),
		m_endData(0) {
		set_memory_priority(0.0);
	}

	inline void begin(stream_size_type items=max_items,
					  begin_data_type * data=0) throw()
	{
		unused(items);
		m_beginData=data;
	}

	inline void push(const item_t & item) throw(stream_exception) {
		m_stream.write(item);
	}

	inline void end(end_data_t * data=0) throw() {
		m_endData=data;
	}

	inline begin_data_type * begin_data() throw () {
		return m_beginData;
	}

	inline end_data_type * end_data() throw () {
		return m_endData;
	}

	virtual memory_size_type base_memory() {
		return memory();
	}
};

template <class dest_t>
class stream_source: public push_single<stream_source<dest_t>, dest_t> {
private:
	typedef push_single<stream_source<dest_t>, dest_t> parent_t;
public:
	typedef typename parent_t::begin_data_type begin_data_type;
	typedef typename parent_t::end_data_type end_data_type;
	typedef typename dest_t::item_type item_type;
	typedef typename file<item_type>::stream stream_type;
private:
	stream_type & m_stream;
	using parent_t::dest;
public:
	static memory_size_type memory() {
		return sizeof(stream_source);
	}

	inline stream_source(stream_type & stream, dest_t & dest) throw():
		parent_t(dest, 0),
		m_stream(stream) {}

	inline stream_source(file_stream<item_type> & stream, dest_t & dest) throw():
		parent_t(dest, 0),
		m_stream(stream.get_stream()) {}

	inline void process(bool seek=true,
						begin_data_type * beginData=0,
						end_data_type * endData=0) throw(stream_exception) {
		if (seek) m_stream.seek(0);
		dest().begin(m_stream.size() - m_stream.offset(), beginData);
		while (m_stream.can_read())
			dest().push(m_stream.read());
		dest().end(endData);
	}

	inline void process_back(bool seek=true,
							 begin_data_type * beginData=0,
							 end_data_type * endData=0) throw(stream_exception) {
		if (seek) m_stream.seek(-1, file_base::end);
		dest().begin(m_stream.offset()+1, beginData);
		while (m_stream.can_read_back())
			dest().push(m_stream.read_back());
		dest().end(endData);
	}

};

template <typename item_t,
		  bool backwards=false,
		  typename pull_begin_data_t=empty_type,
		  typename pull_end_data_t=empty_type>
class pull_stream_source: public memory_single {
public:
	typedef item_t item_type;
	typedef pull_begin_data_t pull_begin_data_type;
	typedef pull_end_data_t pull_end_data_type;
	typedef typename file<item_type>::stream stream_type;
private:
	stream_type & m_stream;
	bool m_seek;
public:
	static memory_size_type memory() {
		return sizeof(pull_stream_source);
	}

	inline pull_stream_source(stream_type & stream, bool seek=true) throw():
		m_stream(stream),
		m_seek(seek) {}

	inline pull_stream_source(file_stream<item_t> & stream, bool seek=true) throw():
		m_stream(stream.get_stream()),
		m_seek(seek) {}

	inline void pull_begin(stream_size_type * size=0,
						   pull_begin_data_type * data=0) throw(stream_exception) {
		unused(data);
		if (m_seek) {
			if (backwards) m_stream.seek(-1, file_base::end);
			else m_stream.seek(0, file_base::beginning);
		}
		if (size)
			*size = backwards?m_stream.offset()+1:m_stream.size()-m_stream.offset();
	}

	inline bool can_pull() const throw() {
		return backwards?m_stream.can_read_back():m_stream.can_read();
	}

	inline const item_type & pull() throw(stream_exception) {
		return backwards?(const item_type&)m_stream.read_back():(const item_type&)m_stream.read();
	}

	inline void pull_end(pull_end_data_type * data=0) const throw() {
		unused(data);
	}

	virtual memory_size_type base_memory() {
		return memory();
	}
};


}
}
#endif //_TPIE_STREAMING_STREAM_H
