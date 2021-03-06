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
#include <tpie/hash_map.h>
#include <map>
#include <boost/random/linear_congruential.hpp>
#include <boost/unordered_map.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace tpie;
using namespace std;
//using namespace std::tr1;
using namespace boost::posix_time;


template <template <typename value_t, typename hash_t, typename equal_t> class table_t>
bool basic_test() {
	hash_map<int, char, hash<int>, std::equal_to<int>,  table_t> q1(200);
	map<int, char> q2;
	boost::rand48 prng(42);
	for(int i=0; i < 100; ++i) {
		int k = (prng()*2) % 250;
		char v = static_cast<char>(prng() % 265);
		q1[k] = v;
		q2[k] = v;
	}
	while (!q2.empty()) {
		if (q1.size() != q2.size()) {
			std::cerr << "Size differs " << q1.size() << " " << q2.size() << std::endl;
			return false;
		}
		for (map<int, char>::iterator i=q2.begin(); i != q2.end(); ++i) {
			if (q1.find((*i).first) == q1.end()) {
				std::cerr << "Element too much" << std::endl;
				return false;
			}
			if (q1[(*i).first] != (*i).second) {
				std::cerr << "Value differs" << std::endl;
				return false;
			}
			if (q1.find((*i).first+1) != q1.end()) {
				std::cerr << "Element too much" << std::endl;
				return false;
			}

		}
		int x=(*q2.begin()).first;
		q1.erase(x);
		q2.erase(x);
	}
	return true;
}

struct charm_gen {
	static inline int key(int i) {
		return (i*21467) % 0x7FFFFFFF;
	}
	static inline int value(int i) {
		return (i*41983)%128;
	}
	static inline int cnt() {return 1000000;}
};

struct identity_gen {
	static inline int key(int i) {
		return i;
	}
	static inline int value(int i) {
		return i%128;
	}
	static inline int cnt() {return 1000000;}
};


template <typename gen_t, 
		  template <typename value_t, typename hash_t, typename equal_t> class table_t>
void test_speed() {
	ptime s1 = microsec_clock::universal_time();
	hash_map<int, char, hash<size_t>, std::equal_to<size_t>,  table_t> q1(gen_t::cnt());
	{
		for(int i=0; i < gen_t::cnt();++i) {
			q1[gen_t::key(i)] = gen_t::value(i);
		}
	}
	ptime s2 = microsec_clock::universal_time();
	boost::unordered_map<int, char> q2;
	{
		for(int i=0; i < gen_t::cnt();++i)
			q2[gen_t::key(i)] = gen_t::value(i);
	}
	ptime s3 = microsec_clock::universal_time();
	std::cout << "Insert speedup: " << (double)(s3 - s2).total_milliseconds() / (double)(s2 - s1).total_milliseconds() << std::endl;
	int x=42;
	s1 = microsec_clock::universal_time();
	{
		for(int i=0; i < gen_t::cnt();++i)
			x ^= q1.find(gen_t::key(i))->second;
	}
	s2 = microsec_clock::universal_time();
	{
		for(int i=0; i < gen_t::cnt();++i)
			x ^= q2.find(gen_t::key(i))->second;
	}
	s3 = microsec_clock::universal_time();

	std::cout << "Find speedup: " << (double)(s3 - s2).total_milliseconds() / (double)(s2 - s1).total_milliseconds() << std::endl;

	s1 = microsec_clock::universal_time();
	{
		for(int i=0; i < gen_t::cnt();++i)
			q1.erase(gen_t::key(i));
	}
	s2 = microsec_clock::universal_time();
	{
		for(int i=0; i < gen_t::cnt();++i)
			q2.erase(gen_t::key(i));
	}
	s3 = microsec_clock::universal_time();
	if (x + q1.size() + q2.size() != 42) std::cout << "Orly" << std::endl;
	std::cout << "Delete speedup: " << (double)(s3 - s2).total_milliseconds() / (double)(s2 - s1).total_milliseconds() << std::endl;
}

class hashmap_memory_test: public memory_test {
public:
	hash_map<int, char> * a;
	virtual void alloc() {a = new hash_map<int, char>(123456);}
	virtual void free() {delete a;}
	virtual size_type claimed_size() {return hash_map<int, char>::memory_usage(123456);}
};

int main(int argc, char **argv) {

	if(argc != 2) return 1;
	std::string test(argv[1]);
	if (test == "chaining")
		return basic_test<chaining_hash_table>()?EXIT_SUCCESS:EXIT_FAILURE;
	else if (test == "linear_probing")
		return basic_test<linear_probing_hash_table>()?EXIT_SUCCESS:EXIT_FAILURE;
	else if (test == "speed") {
		std::cout << "=====================> Linear Probing, Charm Dataset <========================" << std::endl;
		test_speed<charm_gen, linear_probing_hash_table>();
		std::cout << "========================> Chaining, Charm Dataset <===========================" << std::endl;
		test_speed<charm_gen, chaining_hash_table>();
		std::cout << "===================> Linear Probing, Identity Dataset <=======================" << std::endl;
		test_speed<identity_gen, linear_probing_hash_table>();
		std::cout << "=======================> Chaining, Identity Dataset <=========================" << std::endl;
		test_speed<identity_gen, chaining_hash_table>();
		exit(EXIT_SUCCESS);
	}
	//else if (test == "iterators") 
	//	return iterator_test()?EXIT_SUCCESS:EXIT_FAILURE;
	else if (test == "memory") 
		return hashmap_memory_test()()?EXIT_SUCCESS:EXIT_FAILURE;
	std::cerr << "No such test" << std::endl;
	return EXIT_FAILURE;
}
