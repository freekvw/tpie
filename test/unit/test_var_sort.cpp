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
#include <tpie/streaming/util.h>
#include <tpie/types.h>
#include <tpie/portability.h>
#include <tpie/streaming/var_sort.h>
#include <stdlib.h>

using namespace tpie;
using namespace tpie::streaming;

struct item_t {
  size_t size;
  char str[];
};

struct comp_t {
  bool operator()(const item_t & a, const item_t & b) const {
    return strcmp(a.str, b.str) < 0;
  }
};

struct size_t_size_extractor_t {
  typedef size_t header_t;
  memory_size_type size(const size_t & header) const {
    return sizeof(header_t) + header;
  };
};

struct string_sink: public memory_single {
	typedef item_t item_type;
	typedef empty_type begin_data_type;
	typedef empty_type end_data_type;
	
	std::vector<std::string> items;
	void begin(stream_size_type size=0, empty_type * e=0) {unused(size); unused(e);}
	void push(const item_t & item) {items.push_back(item.str);}
	void end(empty_type * e = 0) {unused(e);}
};

bool basic_test() {
	const char * strings[] = {
		"William Kahan",
		"Robert E. Kahn",
		"Avinash Kak",
		"Alan Kay",
		"Richard Karp",
		"Narendra Karmarkar",
		"Marek Karpinski",
		"John George Kemeny",
		"Ken Kennedy",
		"Brian Kernighan",
		"Carl Kesselman",
		"Gregor Kiczales",
		"Stephen Cole Kleene",
		"Leonard Kleinrock",
		"Donald Knuth",
		"Andrew Koenig",
		"Michael Kölling",
		"Janet L. Kolodner",
		"David Korn",
		"Kees Koster",
		"John Koza",
		"Andrey Nikolaevich Kolmogorov",
		"Robert Kowalski",
		"John Krogstie",
		"Joseph Kruskal",
		"Thomas E. Kurtz", 0};

	string_sink sink;
	comp_t comp;
	size_t_size_extractor_t size_extractor;
	var_sort<string_sink, size_t_size_extractor_t, comp_t> sorter(sink, size_extractor, comp);
  
	item_t * item = (item_t*)new char[1024*1024];
	std::vector<std::string> items;
	sorter.set_memory_in(sorter.minimum_memory_in()*10);
	sorter.set_memory_out(sorter.minimum_memory_out()*2);
		
	sorter.begin();
	// for(size_t i=0; strings[i]; ++i) {
	// 	strcpy(item->str, strings[i]);
	while(fgets(item->str, 1024*1023, stdin)) {
		item->size = strlen(item->str)+1;
		items.push_back(item->str);
		sorter.push(*item);
	}
	sorter.end();
	sort(items.begin(), items.end());
	if (items != sink.items) {
		std::cout << "Invalid output" << std::endl;
		return false;
	}
	return true;
}

int main() {
	if (basic_test()) exit(EXIT_SUCCESS);
	exit(EXIT_FAILURE);
}
