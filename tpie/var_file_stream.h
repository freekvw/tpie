// -*- mode: c++; tab-width: 4; indent-tabs-mode: t; eval: (progn (c-set-style "stroustrup") (c-set-offset 'innamespace 0)); -*-
// vi:set ts=4 sts=4 sw=4 noet :
// Copyright 2009, The TPIE development team
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
#ifndef __TPIE_VAR_FILE_STREAM_H__
#define __TPIE_VAR_FILE_STREAM_H__
#include <tpie/tempname.h>
#include <tpie/var_file.h>
////////////////////////////////////////////////////////////////////////////
/// \file var_file_stream.h
/// \brief Implement simple class aggregating both a var_file and a var_file::stream
////////////////////////////////////////////////////////////////////////////

namespace tpie {

////////////////////////////////////////////////////////////////////////////
/// \brief Simple class aggregating both as var_file and a var_file::stream
///
/// A file stream basicly supports every operation a var_file or
/// a var_file::stream stream supports. This is used to access a file
/// io-efficiently, and is the direct replacement of the old
/// ami::stream
///
/// \tparam T The type of items stored in the stream
////////////////////////////////////////////////////////////////////////////
template <typename T, typename size_extractor_type, typename allocator_type=exponential_allocator<T> >
class var_file_stream {
public:
	/** The type of the items stored in the stream */
	typedef T item_type;
	/** The type of file object that is used */
	typedef var_file<T, size_extractor_type, allocator_type> file_type;
	/** The type of file::stream object that is used */
	typedef typename var_file<T, size_extractor_type, allocator_type>::stream stream_type;
	
	/////////////////////////////////////////////////////////////////////////
	/// \brief Construct a new var_file_stream
	/// 
	/// \param blockFactor The relative size of a block compared to the 
	/// default
	/// \param fileAccessor The file accessor to use, if none is supplied a
	/// default will be used
	/////////////////////////////////////////////////////////////////////////
	inline var_file_stream(size_extractor_type &size_extractor,
				allocator_type allocator = exponential_allocator<T>(),
				double blockFactor=1.0, 
				file_accessor::file_accessor * fileAccessor = new default_file_accessor)
		throw() : 
		m_file(size_extractor, allocator, blockFactor, fileAccessor), m_stream(m_file)  {};

	/////////////////////////////////////////////////////////////////////////
	/// \copydoc file_base::open
	/// \sa file_base::open
	/////////////////////////////////////////////////////////////////////////
	inline void open(const std::string & path,
					 file_base::access_type accessType=file_base::read_write,
					 memory_size_type user_data_size=0) throw (stream_exception) {
		m_file.open(path, accessType, user_data_size);
		m_stream.byte_seek(0);
	}

	/////////////////////////////////////////////////////////////////////////
	/// Open a new temporary file
	/// \sa file_base::open
	/////////////////////////////////////////////////////////////////////////
	inline void open(file_base::access_type accessType=file_base::read_write,
					 memory_size_type user_data_size=0) throw (stream_exception) {
		m_file.open(m_temp.path(), accessType, user_data_size);
	}

	/////////////////////////////////////////////////////////////////////////
	/// \copydoc file_base::read_user_data
	/// \sa file_base::read_user_data
	/////////////////////////////////////////////////////////////////////////
	template <typename TT>
	void read_user_data(TT & data) throw (stream_exception) {
		m_file.read_user_data(data);
	}

	/////////////////////////////////////////////////////////////////////////
	/// \copydoc file_base::write_user_data
	/// \sa file_base::write_user_data
	/////////////////////////////////////////////////////////////////////////
	template <typename TT>
	void write_user_data(const TT & data) {
		m_file.write_user_data(data);
	}

	/////////////////////////////////////////////////////////////////////////
	/// \brief Close the fileand release resources
	///
	/// This will close the file and resources used by buffers and such
	/////////////////////////////////////////////////////////////////////////
	inline void close() throw(stream_exception) {
		m_stream.free();
		m_file.close();
	}
	
	/////////////////////////////////////////////////////////////////////////
	/// \copydoc file<T>::stream::write(const item_type & item)
	/// \sa file<T>::stream::write(const item_type & item)
	/////////////////////////////////////////////////////////////////////////
	inline void write(const item_type & item) throw(stream_exception) {
		m_stream.write(item);
	}

	/////////////////////////////////////////////////////////////////////////
	/// \copydoc file<T>::stream::write(const IT & start, const IT & end)
	/// \sa file<T>::stream::write(const IT & start, const IT & end)
	/////////////////////////////////////////////////////////////////////////
	template <typename IT>
	inline void byte_write(const IT & start, const IT & end, memory_size_type count) throw(stream_exception) {
		m_stream.byte_write(start, end, count);
	}

	/////////////////////////////////////////////////////////////////////////
	/// \copydoc file<T>::stream::read()
	/// \sa file<T>::stream::read()
	/////////////////////////////////////////////////////////////////////////
	inline const item_type & read() throw(stream_exception) {
		return m_stream.read();
	}

	/////////////////////////////////////////////////////////////////////////
	/// \copydoc var_file::stream::byte_offset()
	/// \sa var_file::stream::byte_offset()
	/////////////////////////////////////////////////////////////////////////
	inline stream_size_type byte_offset() const throw() {
		return m_stream.byte_offset();
	}

	/////////////////////////////////////////////////////////////////////////
	/// \copydoc file_base::path()
	/// \sa file_base::path()
	/////////////////////////////////////////////////////////////////////////
	inline const std::string & path() const throw() {
		return m_file.path();
	}

	/////////////////////////////////////////////////////////////////////////
	/// \copydoc file_base::size()
	/// \sa file_base::size()
	/////////////////////////////////////////////////////////////////////////
	inline stream_size_type size() const throw() {
		return m_file.size();
	}

	/////////////////////////////////////////////////////////////////////////
	/// \copydoc var_file::stream::byte_size()
	/// \sa var_file::stream::byte_size()
	/////////////////////////////////////////////////////////////////////////
	inline stream_size_type byte_size() const throw() {
		return m_stream.byte_size();
	}

	/////////////////////////////////////////////////////////////////////////
	/// \copydoc file_base::stream::can_read()
	/// \sa file_base::stream::can_read()
	/////////////////////////////////////////////////////////////////////////
	inline bool can_read() const throw() {
		return m_stream.can_read();
	}

	/////////////////////////////////////////////////////////////////////////
	/// \copydoc var_file::stream::byte_seek()
	/// \sa var_file::stream::byte_seek()
	/////////////////////////////////////////////////////////////////////////
	inline void byte_seek(stream_offset_type offset, 
					 file_base::offset_type whence=file_base::beginning) 
		throw (stream_exception) {
		return m_stream.byte_seek(offset, whence);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// Check if we can read from the file
	///
	/// \returns True if we can read from the file
	////////////////////////////////////////////////////////////////////////////////
	bool is_readable() const throw() {
		return m_file.is_readable();
	}

	////////////////////////////////////////////////////////////////////////////////
	/// Check if we can write to the file
	///
	/// \returns True if we can write to the file
	////////////////////////////////////////////////////////////////////////////////
	bool is_writable() const throw() {
		return m_file.is_writable();
	}

	/////////////////////////////////////////////////////////////////////////
	/// \brief Calculate the amount of memory used by a var_file_stream
	///
	/// \param blockFactor The block factor you promice to pass to open
	/// \param includeDefaultFileAccessor Unless you are supplieng your own
	/// file accessor to open leave this to be true
	/// \returns The amount of memory maximaly used by the var_file_stream
	/////////////////////////////////////////////////////////////////////////
	inline static memory_size_type memory_usage(
		float blockFactor=1.0,
		bool includeDefaultFileAccessor=true) throw() {
		return file_type::memory_usage(includeDefaultFileAccessor) 
			+ stream_type::memory_usage(blockFactor) 
			+ sizeof(var_file_stream);
	}

	//////////////////////////////////////////////////////////////////////////
	/// \brief Return the underlying file
	///
	/// \returns The underlying file
	//////////////////////////////////////////////////////////////////////////
	inline file_type & get_file() throw() {
		return m_file;
	}

	//////////////////////////////////////////////////////////////////////////
	/// \brief Return the underlying stream
	///
	/// \returns The underlying stream
	//////////////////////////////////////////////////////////////////////////
	inline stream_type & get_stream() throw() {
		return m_stream;
	}
private:
	file_type m_file;
	stream_type m_stream;
	temp_file m_temp;
};
}

#endif //__TPIE_VAR_FILE_STREAM_H__
