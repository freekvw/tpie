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
#include "common.h"
#include <tpie/internal_priority_queue.h>
#include <vector>
#include "priority_queue.h"

using namespace tpie;

bool basic_test() {
	size_t z = 104729;
	internal_priority_queue<boost::uint64_t, bit_pertume_compare<std::greater<boost::uint64_t> > > pq(z);
	return basic_pq_test(pq, z);
}

bool large_cycle(){
	size_t x = 524*1024*102;
	internal_priority_queue<boost::uint64_t, bit_pertume_compare<std::greater<boost::uint64_t> > > pq(x);
	return cyclic_pq_test(pq, x, 20000000);
}

class my_memory_test: public memory_test {
public:
	internal_priority_queue<int> * a;
	virtual void alloc() {a = new internal_priority_queue<int>(123456);}
	virtual void free() {delete a;}
	virtual size_type claimed_size() {return internal_priority_queue<int>::memory_usage(123456);}
};

int main(int argc, char **argv) {
	if(argc != 2) return 1;
	std::string test(argv[1]);
	if (test == "basic")
		return basic_test()?EXIT_SUCCESS:EXIT_FAILURE;
	else if (test == "large_cycle")
		return large_cycle()?EXIT_SUCCESS:EXIT_FAILURE;
	else if (test == "memory") 
		return my_memory_test()()?EXIT_SUCCESS:EXIT_FAILURE;
	return EXIT_FAILURE;
}
