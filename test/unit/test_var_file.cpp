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
#include "../app_config.h"

#include <tpie/portability.h>
#include <tpie/var_file.h>

int test[] = {
	2, 	3, 	5, 	7,   11,   13,   17,   19,   23,   29,
	31,   37,   41,   43,   47,   53,   59,   61,   67,   71,
	73,   79,   83,   89,   97,  101,  103,  107,  109,  113,
	127,  131,  137,  139,  149,  151,  157,  163,  167,  173,
	179,  181,  191,  193,  197,  199,  211,  223,  227,  229,
	233,  239,  241,  251,  257,  263,  269,  271,  277,  281,
	283,  293,  307,  311,  313,  317,  331,  337,  347,  349,
	353,  359,  367,  373,  379,  383,  389,  397,  401,  409,
	419,  421,  431,  433,  439,  443,  449,  457,  461,  463,
	467,  479,  487,  491,  499,  503,  509,  521,  523,  541,
	547,  557,  563,  569,  571,  577,  587,  593,  599,  601,
	607,  613,  617,  619,  631,  641,  643,  647,  653,  659,
	661,  673,  677,  683,  691,  701,  709,  719,  727,  733,
	739,  743,  751,  757,  761,  769,  773,  787,  797,  809,
	811,  821,  823,  827,  829,  839,  853,  857,  859,  863,
	877,  881,  883,  887,  907,  911,  919,  929,  937,  941,
	947,  953,  967,  971,  977,  983,  991,  997, 1009, 1013,
	1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069,
	1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151,
	1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223,
	1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291,
	1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373,
	1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 0};


struct int_size_extractor_t {
	typedef int header_t;

	memory_size_type size(int header) {
		// return total item size in bytes
		return sizeof(int) + header * sizeof(short);
	}
};

struct item_type {
	int count;
	short data[0];
};

int main(void) {
	MM_manager.set_memory_limit(128*1024*1024);
	double blockFactor = file_base::calculate_block_factor(32*sizeof(int));
	remove("/tmp/stream");

	int_size_extractor_t se;
	exponential_allocator<item_type> alloc;
	typedef tpie::var_file<item_type, int_size_extractor_t> varfile;

	{
		varfile f(se, alloc, blockFactor);
		varfile::stream s(f);

		// Test write, read, open and close
		std::cout << "Writing." << std::endl;
		f.open("/tmp/stream");
		s.byte_seek(0);
		for (memory_size_type i = 0; test[i]; ++i) {
			item_type *item = reinterpret_cast<item_type *>(new char[sizeof(int)+sizeof(short)*test[i]]);
			item->count = test[i];
			for (int j = 0; j < test[i]; ++j) {
				item->data[j] = test[i];
			}
			s.write(*item);
			delete item;
		}
		f.close();
		std::cout << "Reading " << s.size() << " elements." << std::endl;
		f.open("/tmp/stream");
		s.byte_seek(0);
		memory_size_type i = 0;
		while (s.can_read()) {
			const item_type &item = s.read();
			assert(item.count == test[i]);
			for (int j = 0; j < test[i]; ++j) {
				assert(item.data[j] == test[i]);
			}
			++i;
		}
		f.close();
	}

	{
		varfile f(se, alloc, blockFactor);
		varfile::stream s(f);

		// Test byte_write and byte_read
		int multiplier = 3;
		std::cout << "Writing." << std::endl;
		remove("/tmp/stream");
		f.open("/tmp/stream");
		s.byte_seek(0);
		for (memory_size_type i = 0; test[i]; i += multiplier) {
			int byteSum = 0;
			int nItems = 0;
			for (memory_size_type j = i; test[j] && j < i + 3; ++j) {
				byteSum += sizeof(int)+sizeof(short)*test[j];
				nItems++;
			}
			char *data = new char[byteSum], *dp = data;
			for (memory_size_type j = i; test[j] && j < i + 3; ++j) {
				item_type *item = reinterpret_cast<item_type *>(dp);
				item->count = test[j];
				for (int k = 0; k < test[j]; ++k) {
					item->data[k] = test[j];
				}
				dp += sizeof(int)+sizeof(short)*test[j];
			}
			s.byte_write(data, dp, nItems);
		}
		f.close();
		std::cout << "Reading " << s.size() << " elements." << std::endl;
		f.open("/tmp/stream");
		s.byte_seek(0);
		memory_size_type i = 0;
		while (s.can_read()) {
			const item_type &item = s.read();
			assert(item.count == test[i]);
			for (int j = 0; j < test[i]; ++j) {
				assert(item.data[j] == test[i]);
			}
			++i;
		}
		f.close();
	}
}

