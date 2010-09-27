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
#ifndef _TPIE_VAR_FILE_H
#define _TPIE_VAR_FILE_H

#include <tpie/file.h>
#include <tpie/types.h>

#include <algorithm>

#include <cassert>
#include <cstring>

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

	exponential_allocator(const exponential_allocator & o): m_data(new char[o.m_size]), m_size(o.m_size) {
		memcpy(m_data, o.m_data, m_size);
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
class fixed_allocator: public linear_memory_base<fixed_allocator<T> > {
public:
	inline static double memory_coefficient() {return 1.0;}
	inline static double memory_overhead() {return sizeof(fixed_allocator) + MM_manager.space_overhead();}
	
	fixed_allocator(memory_size_type size)
		: m_data(size?new char[size]:0), m_size(size) {
	}

	fixed_allocator(const fixed_allocator & o): m_data(o.m_size?new char[o.m_size]:0), m_size(o.m_size) {
		memcpy(m_data, o.m_data, m_size);
	}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
	fixed_allocator(fixed_allocator && other): m_data(other.m_data), m_size(other.m_size) {
		other.m_data = 0;
		other.m_size = 0;
	}
#endif

	inline ~fixed_allocator() {
		delete [] m_data;
	}

	T &allocate(memory_size_type size) {
		assert(size <= m_size);
		return *reinterpret_cast<T *>(m_data);
	}

	void swap(fixed_allocator<T> & other) {
		std::swap(m_data, other.m_data);
		std::swap(m_size, other.m_size);
	}
private:
	char *m_data;
	memory_size_type m_size;
};

template <typename T, typename size_extractor_type, typename allocator_type=exponential_allocator<T> >
class var_file {
private:
	typedef typename size_extractor_type::header_t header_t;
	file_accessor::file_accessor *m_fileAccessor;
	file<char> m_byteFile;
	size_extractor_type &m_size_extractor;
	allocator_type m_allocator;
	stream_size_type m_itemCount;
	memory_size_type m_userDataSize;

public:
 	typedef T item_type;
	typedef typename file_base::offset_type offset_type;
	typedef typename file_base::access_type access_type;

	var_file(size_extractor_type &size_extractor,
			 allocator_type allocator = exponential_allocator<T>(),
			 double blockFactor=1.0,
			 file_accessor::file_accessor *fileAccessor = new default_file_accessor) :
		m_fileAccessor(fileAccessor), // NOTE: deleted by m_byteFile
		m_byteFile(blockFactor, m_fileAccessor),
		m_size_extractor(size_extractor),
		m_allocator(allocator) {}

	bool is_readable() const {
		return m_byteFile.is_readable();
	}

	bool is_writable() const {
		return m_byteFile.is_writable();
	}

	static inline memory_size_type block_size(double blockFactor) {
		return file<char>::block_size(blockFactor);
	}

	static inline double calculate_block_factor(memory_size_type blockSize) {
		return file<char>::calculate_block_factor(blockSize);
	}

	inline void open(const std::string & path,
					 access_type accessType=file_base::read_write,
					 memory_size_type userDataSize=0) {
		m_byteFile.open(path, accessType, userDataSize + sizeof(stream_size_type));
		m_userDataSize = userDataSize;

		if (m_byteFile.size() == 0) {
			m_itemCount = 0;
		} else {
			char data[userDataSize + sizeof(stream_size_type)];
			m_fileAccessor->read_user_data(reinterpret_cast<void*>(&data));
			::memcpy(&m_itemCount, data, sizeof(stream_size_type));
		}
	}

	inline void close() {
		if (m_byteFile.is_open() && m_byteFile.is_writable()) {
			char data[m_userDataSize + sizeof(stream_size_type)];
			m_fileAccessor->read_user_data(data);
			::memcpy(data, &m_itemCount, sizeof(stream_size_type));
			m_fileAccessor->write_user_data(data);
		}
		m_byteFile.close();
	}

	inline const std::string & path() const {
		return m_byteFile.path();
	}

	static inline memory_size_type memory_usage(bool includeDefaultFileAccessor=true) {
		return sizeof(var_file) - sizeof(file<char>) + file<char>::memory_usage(includeDefaultFileAccessor);
	}

	template <typename TT>
	void read_user_data(TT & data) throw(stream_exception) {
		struct { stream_size_type itemCount; TT user_data; } m_userData;
		assert(m_byteFile.is_open());
		if (sizeof(TT) + sizeof(stream_size_type) != m_fileAccessor->user_data_size()) throw io_exception("Wrong user data size");
		m_fileAccessor->read_user_data(reinterpret_cast<void*>(&m_userData));
		::memcpy(&data, &m_userData.user_data, sizeof(TT));
	}

	template <typename TT>
	void write_user_data(const TT & data) throw(stream_exception) {
		struct { stream_size_type itemCount; TT user_data; } m_userData;
		m_userData.itemCount = m_itemCount;
		::memcpy(&m_userData.user_data, &data, sizeof(TT));
		assert(m_byteFile.is_open());
		if (sizeof(TT) + sizeof(stream_size_type) != m_fileAccessor->user_data_size()) throw io_exception("Wrong user data size");
		m_fileAccessor->write_user_data(reinterpret_cast<const void*>(&m_userData));
	}

	inline stream_size_type size() const throw() {
		return m_itemCount;
	}

	inline allocator_type &allocator() {
		return m_allocator;
	}

	inline file<char> &byte_file() {
		return m_byteFile;
	}

 	class stream {
	public:
		typedef T item_type;
		typedef var_file file_type;

	private:
		file_type &m_file;
		file<char>::stream m_byteStream;

	public:
		inline static memory_size_type memory_usage(double blockFactor=1.0) {
			return sizeof(stream) - sizeof(file<char>::stream) + file<char>::stream::memory_usage(blockFactor);
		}

		stream(file_type &file) :
			m_file(file), m_byteStream(m_file.byte_file()) {}

		void free() {
			m_byteStream.free();
		}

		inline void byte_seek(stream_offset_type offset, offset_type whence=file_base::beginning) throw(stream_exception) {
			m_byteStream.seek(offset, whence);
		}

		inline stream_size_type byte_size() const throw() {
			return m_byteStream.size();
		}

		// Returns item count
		inline stream_size_type size() const throw() {
			return m_file.size();
		}

		inline stream_size_type byte_offset() const throw() {
			return m_byteStream.offset();
		}

		inline bool can_read() const throw() {
			return m_byteStream.can_read();
		}

		inline const item_type &read() {
			memory_size_type headerSize = sizeof(typename size_extractor_type::header_t);
			header_t &header = reinterpret_cast<typename size_extractor_type::header_t &>(m_file.m_allocator.allocate(headerSize));
			m_byteStream.read(reinterpret_cast<char *>(&header), reinterpret_cast<char *>(&header + 1));
			memory_size_type size = m_file.m_size_extractor.size(header);
			T &item = m_file.m_allocator.allocate(size);
			m_byteStream.read(reinterpret_cast<char *>(&item) + headerSize, reinterpret_cast<char *>(&item) + size);
			return item;
		}

		inline void write(const item_type& item) throw(stream_exception) {
			assert(m_byteStream.offset() == m_byteStream.size());
			m_byteStream.write(reinterpret_cast<const char *>(&item),
							   reinterpret_cast<const char *>(&item) + m_file.m_size_extractor.size(reinterpret_cast<const header_t &>(item)));
			++m_file.m_itemCount;
		}

		template<typename IT>
		inline void byte_write(const IT & begin, const IT & end, memory_size_type count) throw(stream_exception) {
			assert(m_byteStream.offset() == m_byteStream.size());
			m_byteStream.write(begin, end);
			m_file.m_itemCount += count;
		}
	};
};

}

namespace std {
template<class T>
void swap(fixed_allocator<T> &l, fixed_allocator<T> &r) {
	std::swap(l.m_data, r.m_data);
	std::swap(l.m_size, r.m_size);
}
}

#endif //_TPIE_VAR_FILE_H
