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
#ifndef _TPIE_VAR_STREAM_H
#define _TPIE_VAR_STREAM_H

#include <tpie/types.h>
#include <tpie/file.h>

#include <algorithm>

#include <cstring>
#include <cassert>

namespace tpie {

template<typename T>
class exponential_allocator {
public:
  exponential_allocator()
    : m_data(0), m_size(0) {
  }
  
  inline ~exponential_allocator() {
    delete [] m_data;
  }
  
  T &bla(int size) {
  }
  
  T &allocate(memory_size_type size) {
    if (size > m_size) {
      char *old_data = m_data;
      memory_size_type old_size = m_size;
      m_size = std::max((memory_size_type)1, m_size);
      while (size > m_size) {
        m_size *= 2;
      }
      m_data = new char[m_size];
      if (old_size > 0) {
        memcpy(m_data, old_data, old_size);
        delete [] old_data;
      }
    }
    return *reinterpret_cast<T *>(m_data);
  }
  
private:
  char *m_data;
  memory_size_type m_size;
};

template<typename T>
class fixed_allocator {
public:
  fixed_allocator(memory_size_type size = sizeof(T))
    : m_data(new char[size]), m_size(size) {
  }
  
  inline ~fixed_allocator() {
    delete [] m_data;
  }
  
  T &allocate(memory_size_type size) {
    assert(size <= m_size);
    return *reinterpret_cast<T *>(m_data);
  }
  
  friend void std::swap<T>(fixed_allocator<T> &l, fixed_allocator<T> &r);
private:
  char *m_data;
  memory_size_type m_size;
};

template<typename T, typename size_extractor_type, typename allocator_type=exponential_allocator<T> >
class var_stream {
public:
 	typedef T item_type;
private:
  typename file<char>::stream &m_byteStream;
  size_extractor_type m_size_extractor;
  allocator_type m_allocator;
  
public:
	inline static memory_size_type memory_usage(memory_size_type count=1, double blockFactor=1.0) {
    assert(count == 1);
		return sizeof(var_stream) - sizeof(file<char>::stream) + file<char>::stream::memory_usage(count, blockFactor);
	}

	var_stream(file<char>::stream &byteStream, size_extractor_type size_extractor, allocator_type allocator = exponential_allocator<T>()) :
		m_byteStream(byteStream), m_size_extractor(size_extractor), m_allocator(allocator) {}

	void free() {
    m_byteStream.free();
  }
  
	inline void byte_seek(stream_offset_type offset, offset_type whence=file_base::beginning) throw(stream_exception) {
		m_byteStream.seek(offset, whence);
	}

 	inline stream_size_type byte_size() const throw() {
		return m_byteStream.size();
	}

 	inline stream_size_type byte_offset() const throw() {
		return m_byteStream.offset();
 	}

 	inline bool can_read() const throw() {
		return m_byteStream.can_read();
 	}

 	inline const item_type &read() {
    memory_size_type headerSize = sizeof(size_extractor_type::header_t);
    size_extractor_type::header_t &header = reinterpret_cast<size_extractor_type::header_t &>m_allocator.allocate(headerSize);
    m_byteStream->read(reinterpret_cast<char *>(&header), reinterpret_cast<char *>(&header + 1));
    memory_size_type size = m_size_extractor.size(header);
    T &item = m_allocator.allocate(size);
    m_byteStream->read(reinterpret_cast<char *>(&item) + headerSize, reinterpret_cast<char *>(&item) + size);
    return item;
	}

 	inline void write(const item_type& item) throw(stream_exception) {
    m_byteStream->write(reinterpret_cast<char *>)(&item),
                        reinterpret_cast<char *>)(&item) + m_size_extractor.size(reinterpret_cast<size_extractor_type::header_t &>item));
	}

  template<typename IT>
 	inline void byte_write(const IT & start, const IT & end) throw(stream_exception) {
    m_byteStream->write(begin, end);
	}
  
  inline allocator_type &allocator() {
    return m_allocator;
  }
};

}

namespace std {
template<class T>
void swap(fixed_allocator<T> &l, fixed_allocator<T> &r) {
  std::swap(l.m_data, r.m_data);
  std::swap(l.m_size, r.m_size);
}
}

#endif //_TPIE_VAR_STREAM_H
