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
#ifndef _TPIE_FILE_H
#define _TPIE_FILE_H

#include <limits>
#include <tpie/exception.h>
#include <tpie/file_accessor/file_accessor.h>
#include <tpie/file_accessor/stdio.h>
#include <tpie/file_accessor/posix.h>
#include <tpie/mm_base.h>
#include <tpie/mm_manager.h>

namespace tpie {

typedef tpie::file_accessor::posix default_file_accessor;

class file_base {
protected:
	struct block_t {
		memory_size_type size;
		memory_size_type usage;
		stream_size_type number;
		bool dirty;
 		block_t * next;
		char data[0];
	};
public:

	/** Type describing how the offset supplied when seeking should be interpeted */
	enum offset_type {
		beginning,
		end,
		current
	};

	/** Type describing how we want to access a file */
	enum access_type {
		read,
		write,
		read_write
	};

	////////////////////////////////////////////////////////////////////////////////
	/// Check if we can read from the file
	///
	/// \returns True if we can read from the file
	////////////////////////////////////////////////////////////////////////////////
	bool is_readable() const throw() {
		return m_canRead;
	}

	////////////////////////////////////////////////////////////////////////////////
	/// Check if we can write to the file
	///
	/// \returns True if we can write to the file
	////////////////////////////////////////////////////////////////////////////////
	bool is_writable() const throw() {
		return m_canWrite;
	}

	////////////////////////////////////////////////////////////////////////////////
	/// Calculate the block size in bytes used by a stream.
	///
	/// \param blockFactor Factor of the global block size to use
	/// \returns Size in Bytes
	////////////////////////////////////////////////////////////////////////////////
	static inline memory_size_type block_size(double blockFactor) throw () {
		return 2 * 1024*1024 * blockFactor;
	}

	static inline double calculate_block_factor(memory_size_type blockSize) throw () {
		return (double)blockSize / (double)block_size(1.0);
	}


	/////////////////////////////////////////////////////////////////////////
	/// \brief Read the user data associated with the file
	///
	/// \param data Where to store the user data
	/// \tparam TT The type of user data. This must be the same size as the
	/// user_data_size supplied to the open call.
	/////////////////////////////////////////////////////////////////////////
	template <typename TT>
	void read_user_data(TT & data) throw(stream_exception) {
		assert(m_open);
		if (sizeof(TT) != m_fileAccessor->user_data_size()) throw io_exception("Wrong user data size");
		m_fileAccessor->read_user_data(reinterpret_cast<void*>(&data));
	}

	/////////////////////////////////////////////////////////////////////////
	/// \brief Write user data to the stream
	///
	/// \param data The user data to store in the stream
	/// \tparam TT The type of user data. This must be the same size as the
	/// user_data_size supplied to the open call.
	/////////////////////////////////////////////////////////////////////////
	template <typename TT>
	void write_user_data(const TT & data) throw(stream_exception) {
		assert(m_open);
		if (sizeof(TT) != m_fileAccessor->user_data_size()) throw io_exception("Wrong user data size");
		m_fileAccessor->write_user_data(reinterpret_cast<const void*>(&data));
	}

	/////////////////////////////////////////////////////////////////////////
	/// \brief Close the file
	///
	/// Note all streams into the will must be freed, before you call close
	/////////////////////////////////////////////////////////////////////////
	inline void close() throw(stream_exception) {
		m_open = false;
		m_fileAccessor->close();
	}

	/////////////////////////////////////////////////////////////////////////
	/// \brief Open a file
	///
	/// \param path The path of the file to open
	/// \param accessType The way in which we want the file to be opened
	/// \param userDataSize The size of the userdata we want to store in the file
	/////////////////////////////////////////////////////////////////////////
	inline void open(const std::string & path,
					 access_type accessType=read_write,
					 memory_size_type userDataSize=0) throw(stream_exception) {
		close();
		m_canRead = accessType == read || accessType == read_write;
		m_canWrite = accessType == write || accessType == read_write;
		m_fileAccessor->open(path, m_canRead, m_canWrite, m_itemSize, userDataSize);
		m_size = m_fileAccessor->size();
		m_open = true;
	}

	/////////////////////////////////////////////////////////////////////////
	/// \brief Calculate the size of the file in items.
	///
	/// \returns The number of items in the file
	/////////////////////////////////////////////////////////////////////////
	inline stream_size_type size() const throw() {
		return m_size;
	}

	/////////////////////////////////////////////////////////////////////////
	/// \brief Check whether the file is open.
	///
	/// \returns True if the file is open, otherwise false.
	/////////////////////////////////////////////////////////////////////////
	inline bool is_open() const throw() {
		return m_open;
	}

	/////////////////////////////////////////////////////////////////////////
	/// \brief The path of the file opened or the empty string
	///
	/// \returns The path of the currently opened file
	/////////////////////////////////////////////////////////////////////////
	inline const std::string & path() const throw() {
		assert(m_open);
		return m_fileAccessor->path();
	}

	class stream {
	protected:
		file_base & m_file;
		memory_size_type m_index;
		stream_size_type m_nextBlock;
		memory_size_type m_nextIndex;
		block_t * m_block;
		inline void update_vars() throw() {
			if (m_index != std::numeric_limits<memory_size_type>::max())
				m_block->size = std::max(m_block->size, m_index);
			if (m_index != std::numeric_limits<memory_size_type>::max() &&
				m_block->number != std::numeric_limits<stream_size_type>::max())
				m_file.m_size = std::max(m_file.m_size, m_block->number * static_cast<stream_size_type>(m_file.m_blockItems) + static_cast<stream_size_type>(m_index));
		}

		void update_block();
		inline memory_size_type block_items() const {return m_file.m_blockItems;}
		inline void write_update() {
			m_block->dirty = true;
			m_block->size = std::max(m_block->size, m_index);
			m_file.m_size = std::max(m_file.m_size, static_cast<stream_size_type>(m_index)+m_block->number*static_cast<stream_size_type>(m_file.m_blockItems));
		}
	public:
		stream(file_base & file, stream_size_type offset=0);
		void free();
		inline ~stream() {free();}

		/////////////////////////////////////////////////////////////////////////
		/// \brief Moves the logical offset in the stream
		///
		/// \param offset Where to move the logical offset to
		/// \param whence Move the offset relative to what
		/////////////////////////////////////////////////////////////////////////
		inline void seek(stream_offset_type offset, offset_type whence=beginning) throw(stream_exception) {
			assert(m_file.m_open);
			if (whence == end)
				offset += size();
			else if (whence == current)
				offset += this->offset();
			if (0 > offset || (stream_size_type)offset > size())
				throw io_exception("Tried to seek out of file");
			update_vars();
			stream_size_type b = static_cast<stream_size_type>(offset) / m_file.m_blockItems;
			m_index = offset - b*m_file.m_blockItems;
			if (b == m_block->number) {
				m_nextBlock = std::numeric_limits<stream_size_type>::max();
				m_nextIndex = std::numeric_limits<memory_size_type>::max();
				assert(this->offset() == (stream_size_type)offset);
				return;
			}
			m_nextBlock = b;
			m_nextIndex = m_index;
			m_index = std::numeric_limits<memory_size_type>::max();
			assert(this->offset() == (stream_size_type)offset);
		}

 		inline stream_size_type size() const throw() {
			assert(m_file.m_open);
			return m_file.size();
		}

		/////////////////////////////////////////////////////////////////////////
		/// \brief Calculate the current offset in the stream.
		///
		/// \returns The current offset in the stream
		/////////////////////////////////////////////////////////////////////////
 		inline stream_size_type offset() const throw() {
			assert(m_file.m_open);
 			if (m_nextBlock == std::numeric_limits<stream_size_type>::max())
 				return m_index + m_block->number * m_file.m_blockItems;
 			return m_nextIndex + m_nextBlock * m_file.m_blockItems;
 		}

		/////////////////////////////////////////////////////////////////////////
		/// \brief Check if we can read an item with read()
		///
		/// This is logicaly equivalent to:
		/// \code
		/// return offset() < size();
		/// \endcode
		/// But it might be faster
		///
		/// \returns Whether or not we can read more items
		/////////////////////////////////////////////////////////////////////////
 		inline bool can_read() const throw() {
			assert(m_file.m_open);
 			if (m_index < m_block->size) return true;
 			return offset() < size();
 		}

		/////////////////////////////////////////////////////////////////////////
		/// \brief Check if we can read an item with read_back()
		///
		/// \returns Whether or not we can an item with read_back()
		/////////////////////////////////////////////////////////////////////////
		inline bool can_read_back() const throw() {
			assert(m_file.m_open);
 			if (m_index < m_block->size) return true;
			if (m_nextBlock == std::numeric_limits<stream_size_type>::max())
				return m_block->number != 0;
			else
				return true;
		}
	};

	void truncate(stream_size_type s) throw(stream_exception) {
		assert(m_open);
		m_size = s;
		m_fileAccessor->truncate(s);
	}

	~file_base();

	memory_size_type blockItems() {
		return m_blockItems;
	}

	bool m_open;
protected:
	memory_size_type m_blockItems;
	stream_size_type m_size;
	bool m_canRead;
	bool m_canWrite;
	static block_t m_emptyBlock;

	file_base(memory_size_type item_size,
			  double blockFactor=1.0,
			  file_accessor::file_accessor * fileAccessor=NULL);

	void create_block();
	void delete_block();
	block_t * get_block(stream_size_type block);
	void free_block(block_t * block);

	memory_size_type m_itemSize;
private:
	//TODO this should really be a hash map
	block_t * m_firstUsed;
	block_t* m_firstFree;
	file_accessor::file_accessor * m_fileAccessor;
};

template <typename T>
class file: public file_base {
public:
 	typedef T item_type;

	static inline memory_size_type memory_usage(bool includeDefaultFileAccessor=true) {
		memory_size_type x = sizeof(file);
		if (includeDefaultFileAccessor)
			x += MM_manager.space_overhead() + default_file_accessor::memory_usage();
		return x;
	}

	file(double blockFactor=1.0,
		 file_accessor::file_accessor * fileAccessor=NULL):
		file_base(sizeof(T), blockFactor, fileAccessor) {};

 	class stream: public file_base::stream {
	public:
		typedef T item_type;
		typedef file file_type;
	private:
		typedef typename file::block_t block_t;
	public:
		inline static memory_size_type memory_usage(double blockFactor=1.0) {
			return sizeof(stream) + block_size(blockFactor) +  MM_manager.space_overhead() + sizeof(block_t);
		}

		stream(file_type & file, stream_size_type offset=0):
			file_base::stream(file, offset) {}


		/////////////////////////////////////////////////////////////////////////
		/// \brief Read a mutable item from the stream.
		///
		/// Read current item from the stream, and increment the offset
		/// by one item.
		/// This will throw an end_of_stream_exception if there are no more items
		/// left in the stream.  This can also be checkout with can_read.
		/// \returns The item read from the stream
		/////////////////////////////////////////////////////////////////////////
 		inline item_type & read_mutable() {
			assert(m_file.m_open);
			if (m_index >= m_block->size) {
				if (offset() >= m_file.size())
					throw end_of_stream_exception();
				update_block();
			}
			return reinterpret_cast<T*>(m_block->data)[m_index++];
		}

		/////////////////////////////////////////////////////////////////////////
		/// \brief Read an item from the stream.
		///
		/// Read current item from the stream, and increment the offset
		/// by one item.
		/// This will throw an end_of_stream_exception if there are no more items
		/// left in the stream.  This can also be checkout with can_read.
		/// \returns The item read from the stream
		/////////////////////////////////////////////////////////////////////////
 		inline const item_type & read() {
			return read_mutable();
		}

		/////////////////////////////////////////////////////////////////////////
		/// \brief Read an item from the stream.
		///
		/// Read current item from the stream, and decrement the offset
		/// by one item.
		/// This will throw an end_of_stream_exception if there are no more items
		/// left in the stream.  This can also be checkout with can_read_back.
		/// \returns The item read from the stream
		/////////////////////////////////////////////////////////////////////////
		inline item_type & read_back() {
			assert(m_file.m_open);
			//The first index in a block is 0, when that is read
			// m_index will underflow and become max int
			if (m_index >= m_block->size) {
				if (m_nextBlock == std::numeric_limits<stream_size_type>::max()) {
					if (m_block == 0)
						throw end_of_stream_exception();
 					m_nextBlock = m_block->number-1;
					m_nextIndex = m_file.blockItems()-1;
				}
				update_block();
			}
			return reinterpret_cast<T*>(m_block->data)[m_index--];
		}

		/////////////////////////////////////////////////////////////////////////
		/// \brief Write an item to the stream
		///
		/// \param item The item to write to the stream
		/////////////////////////////////////////////////////////////////////////
 		inline void write(const item_type& item) throw(stream_exception) {
			assert(m_file.m_open);
#ifndef NDEBUG
			if (!m_file.is_writable())
				throw io_exception("Cannot write to read only stream");
#endif
			if (m_index >= block_items()) update_block();
			reinterpret_cast<T*>(m_block->data)[m_index++] = item;
			write_update();
		}

		/////////////////////////////////////////////////////////////////////////
		/// \brief Write several itmes to the stream
		///
		/// \tparam IT The type of Random Access Iteractors used to supplie the
		/// items
		/// \param start Iterator to the first item to write
		/// \param end Iterator parst the last item to write
		/////////////////////////////////////////////////////////////////////////
		template <typename IT>
		inline void write(const IT & start, const IT & end) {
			assert(m_file.m_open);
			// FIXME implement more efficiently
			for (IT i=start; i != end; ++i)
				write(*i);
		}

		/////////////////////////////////////////////////////////////////////////
		/// \brief Reads several items from the stream
		///
		/// \tparam IT The type of Random Access Iteractors used to supplie
		/// storage
		/// \param start Iterator pointing to the first place to store an item
		/// \param end Iterator pointing past the last place to store an item
		/////////////////////////////////////////////////////////////////////////
		template <typename IT>
		inline void read(const IT & start, const IT & end) {
			assert(m_file.m_open);
			for (IT i=start; i != end; ++i)
				*i = read();
		}
 	};
};
}
#endif //_TPIE_FILE_H
