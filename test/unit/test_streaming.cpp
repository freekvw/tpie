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
#include <cstring>
#include <tpie/bte/err.h>
#include <tpie/portability.h>
#include <tpie/streaming.h>
#include <tpie/streaming/concepts.h>
#include <tpie/streaming/util.h>

using namespace tpie;
using namespace std;
using namespace tpie::streaming;
namespace sc = tpie::streaming::concepts;

struct X {};
struct Y{};

const memory_size_type the_test_size=229;
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

#define ERR(x) {cerr << x << endl; exit(1);}
//================================> push test <=================================
struct push_test_sink: public memory_single {
	typedef int item_type;
	typedef empty_type begin_data_type;
	typedef empty_type end_data_type;
	bool b;
	bool e;
	int c;
	bool o;
	push_test_sink(): b(false), e(false), c(0), o(false) {}

	void begin(stream_size_type items=max_items, empty_type * data=0) {
		if (items != the_test_size) ERR("begin() wrong item count");
		unused(data);
		if (e || b) ERR("begin()");
		b=true;
	}

	void ok() {o=true;}

	void end(empty_type * data=0) {
		unused(data);
		if (!b || e) ERR("end()");
		e=true;
	}

	void push(const int & x) {
		if (!o) ERR("Push to early");
		if (test[c++] != x) ERR("push() pushed wrong item");
	}

	void final() {
		std::cout << e << " " << c << " " << test[c] << std::endl;
		if (!e || test[c] != 0) ERR("final()");
	};
};

template <typename T>
struct push_test_source: public push_single<push_test_source<T>, T> {
	typedef push_single<push_test_source<T>, T> parent_t;
	using parent_t::dest;
	push_test_source(T & d): parent_t(d, 0.0) {};

	template <typename I>
	void run(push_test_sink & sink, const I & start, const I & end, bool split) {
		dest().begin(the_test_size);
		if (!split) sink.ok();
		for (I i=start; i != end; ++i)
			dest().push(*i);
		if (split) sink.ok();
		dest().end();
		sink.final();
	}
};

//================================> pull_test <=================================

template <typename T>
struct pull_test_source: public memory_single {
	typedef int pull_type;
	typedef empty_type pull_begin_data_type;
	typedef empty_type pull_end_data_type;

	T iter;
	const T & end;
	bool beginCalled;
	bool endCalled;
	pull_test_source(const T & start, const T & e): iter(start), end(e),
													beginCalled(false), endCalled(false) {}
	int d;
	bool can_pull() {return iter != end;}
	const int & pull() {
		if (endCalled) ERR("pull_end() called before pull()");
		if (!beginCalled) ERR("pull_begin() must be called before pull()");
		d = *iter;
		iter++;
		return d;
	};
	void pull_begin(stream_size_type * size=0, pull_begin_data_type * data=0) {
		unused(size);
		unused(data);
		if (beginCalled) ERR("pull_begin() called twice");
		if (endCalled) ERR("pull_end() called before pull_begin()");
		beginCalled = true;
	};
	void pull_end(pull_end_data_type *data=0) {
		unused(data);
		if (!beginCalled) ERR("pull_end() called before pull_begin()");
		if (endCalled) ERR("pull_end() called twice");
		endCalled = true;
	};
};

template <typename T>
struct pull_test_sink: public pull_single<pull_test_sink<T>, T> {
	typedef pull_single<pull_test_sink<T>, T> parent_t;
	using parent_t::source;

	pull_test_sink(T & src): parent_t(src, 0.0) {};

	void run() {
		source().pull_begin();
		for (memory_size_type i=0; i < the_test_size; ++i) {
			if (!source().can_pull()) ERR("can_pull() returned false when it should return true");
			if (test[i] != source().pull()) ERR("pull() returned wrong item");
		}
		if (source().can_pull()) ERR("can_pull() returned true when it should return false");
		source().pull_end();
	}
};

//==================================> memory <==================================
struct memory_monitor {
	memory_size_type base;
	memory_size_type used;
	inline void begin() {
		base = MM_manager.memory_used();
		used = base;
	}
	inline void sample() {
		used = max(used,  MM_manager.memory_used());
	}
	inline void clear() {
		used = MM_manager.memory_used();
	}
	inline void empty() {
		used = base;
	}
	inline memory_size_type usage(int allocations) {
		return used-base - allocations*MM_manager.space_overhead();
	}
};

struct memory_test_sink: public memory_single {
	typedef int item_type;
	typedef empty_type begin_data_type;
	typedef empty_type end_data_type;
	int cnt;
	memory_monitor * monitor;
	memory_test_sink(memory_monitor * m=0): monitor(m) {};

	void begin(stream_size_type items=max_items, empty_type * data=0) {
		unused(items);
		unused(data);
		cnt=0;
	}
	void push(const int & x) {
		cnt^=x;
		monitor->sample();
	}
	void end(empty_type * data=0) {
		unused(data);
		if (cnt == 42) std::cout << "";
	}
};

struct memory_test_source: public memory_single {
	typedef int pull_type;
	typedef empty_type pull_begin_data_type;
	typedef empty_type pull_end_data_type;
	int cnt;
	memory_monitor * monitor;
	memory_test_source(memory_monitor * m=0): monitor(m) {};

	void pull_begin(stream_size_type * items=0, empty_type * data=0) {
		unused(items);
		unused(data);
		cnt=0;
	}

	inline const int & pull() {
		cnt++;
		return cnt;
	}
	void pull_end(empty_type * data=0) {
		unused(data);
	}
	inline bool can_pull() const {
		return cnt<42;
	}
};

template <typename T>
struct test_push_single_memory_limit {
	virtual T * construct(memory_monitor & monitor) = 0;
	virtual void destruct(T * elm) {delete elm;}
	void operator() (memory_size_type memory) {
		memory_monitor monitor;
		monitor.begin();
		T * t = construct(monitor);
		monitor.sample();
		memory = max(memory, t->minimum_memory());
		t->set_memory(memory);
		monitor.sample();
		t->begin();
		monitor.sample();
		for (int i=0; i < 42; ++i) {
			t->push(i);
			monitor.sample();
		}
		t->end();
		monitor.sample();
		memory_size_type usage = monitor.usage(2);
		destruct(t);
		monitor.clear();
		memory_size_type aa = monitor.usage(0);
		std::cout << "Memory used: " << usage << "; Allowed: " << memory
				  << "; After dealocation: " << aa << ";" << std::endl;
		if (usage > memory) ERR("Used more memory then allocated");
		if (aa > 0) ERR("Did not deallocate all its memory");
	}
};

template <typename T>
struct test_pull_single_memory_limit {
	virtual T * construct(memory_monitor & monitor) = 0;
	virtual void destruct(T * elm) {delete elm;}
	void operator() (memory_size_type memory ) {
		memory_monitor monitor;
		monitor.begin();
		T * t = construct(monitor);
		monitor.sample();
		memory = max(memory, t->minimum_memory());
		t->set_memory(memory);
		monitor.sample();
		t->pull_begin();
		monitor.sample();
		int x=0;
		while (t->can_pull()) {
			x ^= t->pull();
			monitor.sample();
		}
		t->pull_end();
		monitor.sample();
		memory_size_type usage = monitor.usage(2);
		destruct(t);
		monitor.clear();
		memory_size_type aa = monitor.usage(0);
		std::cout << "memory used: " << usage << "; Allowed: " << memory
				  << "; After dealocation: " << aa << ";" << std::endl;
		if (usage > memory) ERR("Used more memory then allocated");
		if (aa > 0) ERR("Did not deallocate all its memory");
	}
};

template <typename T>
struct test_split_memory_limit {
	virtual T * construct(memory_monitor & monitor) = 0;
	virtual void destruct(T * elm) {delete elm;}
	void operator() (memory_size_type in_memory, memory_size_type out_memory ) {
		memory_monitor monitor;
		monitor.begin();
		T * t = construct(monitor);
		monitor.sample();
		in_memory = max(in_memory, t->minimum_memory_in());
		out_memory = max(out_memory, t->minimum_memory_out());
		t->set_memory_in(in_memory);
		t->set_memory_out(out_memory);
		monitor.sample();
		t->begin();
		monitor.sample();
		for (int i=0; i < 42; ++i) {
			t->push(i);
			monitor.sample();
		}
		memory_size_type in_usage = monitor.usage(2);
		monitor.empty();
		t->end();
		monitor.sample();
		memory_size_type out_usage = monitor.usage(2);
		destruct(t);
		monitor.clear();
		memory_size_type aa = monitor.usage(0);
		std::cout << "In memory used: " << in_usage << "; In memory allowed: " << in_memory
				  << "; Out memory used: " << out_usage << "; Out memory allowed: " << out_memory
				  << "; After dealocation: " << aa << ";" << std::endl;
		if (in_usage > in_memory) ERR("Used more memory then allocated");
		if (out_usage > out_memory) ERR("Used more memory then allocated");
		if (aa > 0) ERR("Did not deallocate all its memory");
	}
};

template <typename T>
struct test_pull_split_memory_limit {
	virtual T * construct(memory_monitor & monitor) = 0;
	virtual void destruct(T * elm) {delete elm;}
	void operator() (memory_size_type in_memory, memory_size_type out_memory ) {
		memory_monitor monitor;
		monitor.begin();
		T * t = construct(monitor);
		monitor.sample();
		in_memory = max(in_memory, t->minimum_memory_in());
		out_memory = max(out_memory, t->minimum_memory_out());
		t->set_memory_in(in_memory);
		t->set_memory_out(out_memory);
		monitor.sample();
		t->begin();
		monitor.sample();
		for (int i=0; i < 42; ++i) {
			t->push(i);
			monitor.sample();
		}
		memory_size_type in_usage = monitor.usage(2);
		monitor.empty();
		t->end();
		monitor.empty();
		t->pull_begin();
		monitor.empty();
		int x=0;
		for (int i=0; i < 42; ++i) {
			x ^= t->pull();
			monitor.empty();
		}
		if (x == 42) std::cout << "" << std::endl;
		t->pull_end();
		monitor.sample();
		memory_size_type out_usage = monitor.usage(2);
		destruct(t);
		monitor.clear();
		memory_size_type aa = monitor.usage(0);
		std::cout << "In memory used: " << in_usage << "; In memory allowed: " << in_memory
				  << "; Out memory used: " << out_usage << "; Out memory allowed: " << out_memory
				  << "; After dealocation: " << aa << ";" << std::endl;
		if (in_usage > in_memory) ERR("Used more memory then allocated");
		if (out_usage > out_memory) ERR("Used more memory then allocated");
		if (aa > 0) ERR("Did not deallocate all its memory");
	}
};

double blockFactor;
void memory_test_single(memory_base * elm,
						memory_base * next,
						memory_base * prev,
						memory_size_type minSize,
						double priority) {
	std::vector<memory_base *> n;
	elm->memory_next(n);
	std::vector<memory_base *> p;
	elm->memory_prev(p);
	if (n.size() != (next==0?0:1) ) ERR("memory_next() returned the wrong size");
	if (next != 0 && n[0] != next) ERR("memory_next() returned the wrong value");
	if (p.size() != (prev==0?0:1) ) ERR("memory_prev() returned the wrong size");
	if (prev != 0 && p[0] != prev) ERR("memory_prev() returned the wrong value");
	if (elm->base_memory() < minSize) ERR("base_memory() return to small a value");
	//if (elm->memory_type() != memory_base::SINGLE) ERR("memory_type() should be SINGLE");
	if (static_cast<memory_single*>(elm)->memory_priority() != priority) ERR("memory_priority() returned unexpected value");
}

void memory_test_split(memory_base * elm,
					   memory_base * next,
					   memory_size_type minSize,
					   double inPrio,
					   double outPrio) {
	std::vector<memory_base *> n;
	elm->memory_next(n);
	if (n.size() != (next==0?0:1) ) ERR("memory_next() returned the wrong size");
	if (next != 0 && n[0] != next) ERR("memory_next() returned the wrong value");
	if (elm->base_memory() < minSize) ERR("base_memory() return to small a value");
	//if (elm->memory_type() == memory_base::SPLIT) ERR("memory_type() should be SPLIT");
	if (static_cast<memory_split*>(elm)->memory_in_priority() != inPrio) ERR("memory_in_priority() returned unexpected value");
	if (static_cast<memory_split*>(elm)->memory_out_priority() != outPrio) ERR("memory_out_priority() returned unexpected value");
}

//==============================> stream_source <===============================
void test_stream_source(char * testName) {
	BOOST_CONCEPT_ASSERT((sc::memory_managable< tpie::streaming::stream_source<push_test_sink> >));
	file_stream<int> s(blockFactor);
	push_test_sink sink;
	//TODO test if begin and end data are proccess correctly
	if (!strcmp(testName, "memory")) {
		stream_source<push_test_sink> ss(s, sink);
		memory_test_single(&ss, &sink, 0, sizeof(s), 0);
	} else if (!strcmp(testName, "minimum_memory")) {
		s.open();
		for (memory_size_type i=0; test[i]; ++i)
			s.write(test[i]);
		s.seek(0);
		sink.ok();

		memory_monitor monitor;
		monitor.begin();
		memory_test_sink mysink(&monitor);
		stream_source<memory_test_sink> * ss = new stream_source<memory_test_sink>(s, mysink);
		memory_size_type memory = ss->minimum_memory();
		ss->set_memory(memory);
		monitor.sample();
		ss->process();
		monitor.sample();
		memory_size_type usage = monitor.usage(1);
		delete ss;
		monitor.clear();
		memory_size_type aa = monitor.usage(0);
		std::cout << "Memory used: " << usage << "; Allowed: " << memory << "; After dealocation: " << aa << ";" << std::endl;
		if (usage > memory) ERR("Used more memory then allocated");
		if (aa > 0) ERR("Did not deallocate all its memory");
	} else if (!strcmp(testName, "process")) {
		s.open();
		for (memory_size_type i=0; test[i]; ++i)
			s.write(test[i]);
		s.seek(14);
		sink.ok();
		stream_source<push_test_sink> ss(s, sink);
		ss.process();
		sink.final();
	} else if (!strcmp(testName, "process_back")) {
		s.open();
		for (memory_size_type i=the_test_size-1; i != memory_size_type(-1); --i)
			s.write(test[i]);
		s.seek(14);
		push_test_sink sink;
		sink.ok();
		stream_source<push_test_sink> ss(s, sink);
		ss.process_back();
		sink.final();
	} else
		ERR("No such test");
}

//===============================> stream_sink <================================
struct test_stream_sink_memory_limit: public test_push_single_memory_limit< stream_sink<int> > {
	file_stream<int> fs;
	test_stream_sink_memory_limit() {
		fs.open();
	}

	virtual stream_sink<int> * construct(memory_monitor & mm) {
		unused(mm);
		return new stream_sink<int>(fs);
	}
};

void test_stream_sink(char * testName) {
	BOOST_CONCEPT_ASSERT((sc::pushable< tpie::streaming::stream_sink<int, X, Y> >));
	BOOST_CONCEPT_ASSERT((sc::memory_managable< tpie::streaming::stream_sink<int, X, Y> >));
	BOOST_CONCEPT_ASSERT((sc::memory_managable< tpie::streaming::stream_source<push_test_sink> >));
	file_stream<int> s(blockFactor);
	push_test_sink sink;
	//TODO test if begin and end data are proccess correctly
	if (!strcmp(testName, "memory")) {
		stream_sink<int> ss(s);
		memory_test_single(&ss, 0, 0, sizeof(ss), 0);
	} else if (!strcmp(testName, "minimum_memory")) {
		test_stream_sink_memory_limit test;
		test(0);
	} else if (!strcmp(testName, "process")) {
		file_stream<int> s(blockFactor);
		s.open();
		stream_sink<int> sink(s);
		sink.begin();
		for (memory_size_type i=0; test[i]; ++i)
			sink.push(test[i]);
		sink.end();

		s.seek(0);
		memory_size_type i=0;
		while (s.can_read())
			if (s.read() != test[i++] ) ERR("sink");
		if (test[i] != 0) ERR("sink");
	} else
		ERR("No such test");
}

//===================================> sort <===================================
struct test_sort_memory_limit: public test_split_memory_limit< tpie::streaming::sort<memory_test_sink> > {
	memory_test_sink sink;

	virtual tpie::streaming::sort<memory_test_sink> * construct(memory_monitor & mm) {
		sink.monitor = &mm;
		return new tpie::streaming::sort<memory_test_sink>(sink);
	}
};

void test_sort(char * testName) {
	BOOST_CONCEPT_ASSERT((sc::pushable< tpie::streaming::sort<push_test_sink> >));
	BOOST_CONCEPT_ASSERT((sc::memory_managable< tpie::streaming::sort<push_test_sink> >));
	if (!strcmp(testName, "memory")) {
		push_test_sink sink;
		tpie::streaming::sort<push_test_sink> sort(sink, std::less<int>(), blockFactor);
		memory_test_split(&sort, &sink, sizeof(sort), 1.0, 1.0);
	} else if (!strcmp(testName, "minimum_memory")) {
		test_sort_memory_limit test;
		test(0, 0);
	} else if (!strcmp(testName, "process")) {
		vector<int> t2;
		for (memory_size_type i=0; test[i]; ++i)
			t2.push_back(test[i]);
		std::random_shuffle(t2.begin(), t2.end());

		push_test_sink sink;
		tpie::streaming::sort<push_test_sink> sort(sink, std::less<int>(), blockFactor);
		push_test_source<tpie::streaming::sort<push_test_sink> > source(sort);
		sort.set_memory_in(sort.minimum_memory_in()+sizeof(int)*3);
		sort.set_memory_out(sort.minimum_memory_out()+sizeof(int)*3);

		source.run(sink, t2.begin(), t2.end(), true);
	}
}

//================================> pull_sort <=================================
struct test_pull_sort_memory_limit: public test_pull_split_memory_limit< tpie::streaming::pull_sort<int> > {
	virtual tpie::streaming::pull_sort<int> * construct(memory_monitor & mm) {
		unused(mm);
		return new tpie::streaming::pull_sort<int>();
	}
};

void test_pull_sort(char * testName) {
	BOOST_CONCEPT_ASSERT((sc::pushable< tpie::streaming::pull_sort<int, std::less<int>, X, Y> >));
	BOOST_CONCEPT_ASSERT((sc::pullable< tpie::streaming::pull_sort<int, std::less<int>, X, Y> >));
	BOOST_CONCEPT_ASSERT((sc::memory_managable< tpie::streaming::pull_sort<int> >));

	if (!strcmp(testName, "memory")) {
		tpie::streaming::pull_sort<int> sort(std::less<int>(), blockFactor);
		memory_test_split(&sort, 0, sizeof(sort), 1.0, 1.0);
	} else if (!strcmp(testName, "minimum_memory")) {
		test_pull_sort_memory_limit test;
		test(0, 0);
	} else if (!strcmp(testName, "process")) {
		vector<int> t2;
		for (memory_size_type i=0; test[i]; ++i)
			t2.push_back(test[i]);
		std::random_shuffle(t2.begin(), t2.end());

		tpie::streaming::pull_sort<int> sort(std::less<int>(), blockFactor);
		sort.set_memory_in(sort.minimum_memory_in()+sizeof(int)*3);
		sort.set_memory_out(sort.minimum_memory_out()+sizeof(int)*3);

		sort.begin();
		for (memory_size_type i=0; test[i]; ++i)
			sort.push(t2[i]);
		sort.end();

		sort.pull_begin();
		for (memory_size_type i=0; test[i]; ++i) {
			if (!sort.can_pull() ) ERR("can_pull()");
			if (sort.pull() != test[i]) ERR("pull()");
		}
		if (sort.can_pull() ) ERR("can_pull");
		sort.pull_end();
	} else
		ERR("No such test");
}

//==================================> buffer <==================================
struct test_buffer_memory_limit: public test_split_memory_limit< tpie::streaming::buffer<memory_test_sink> > {
	memory_test_sink sink;
	virtual tpie::streaming::buffer<memory_test_sink> * construct(memory_monitor & mm) {
		sink.monitor = &mm;
		return new tpie::streaming::buffer<memory_test_sink>(sink);
	}
};

void test_buffer(char * testName) {
	BOOST_CONCEPT_ASSERT((sc::pushable< tpie::streaming::buffer<push_test_sink> >));
	BOOST_CONCEPT_ASSERT((sc::memory_managable< tpie::streaming::buffer<push_test_sink> >));
	if (!strcmp(testName, "memory")) {
		push_test_sink sink;
		tpie::streaming::buffer<push_test_sink> buffer(sink, blockFactor);
		memory_test_split(&buffer, &sink, sizeof(buffer), 1.0, 1.0);
	} else if (!strcmp(testName, "minimum_memory")) {
		test_buffer_memory_limit test;
		test(0, 0);
	} else if (!strcmp(testName, "process")) {
		push_test_sink sink;
		tpie::streaming::buffer<push_test_sink> buffer(sink, blockFactor);
		push_test_source<tpie::streaming::buffer<push_test_sink> > source(buffer);
		buffer.set_memory_in(buffer.minimum_memory_in()+sizeof(int)*3);
		buffer.set_memory_out(buffer.minimum_memory_out()+sizeof(int)*3);

		source.run(sink, (int*)test, test+the_test_size, true);
	} else
		ERR("No such test");
}

//===============================> pull_buffer <================================
struct test_pull_buffer_memory_limit: public test_pull_split_memory_limit< tpie::streaming::pull_buffer<int> > {
	virtual tpie::streaming::pull_buffer<int> * construct(memory_monitor & mm) {
		unused(mm);
		return new tpie::streaming::pull_buffer<int>();
	}
};

void test_pull_buffer(char * testName) {
	BOOST_CONCEPT_ASSERT((sc::pushable< tpie::streaming::pull_buffer<int, false, X, Y> >));
	BOOST_CONCEPT_ASSERT((sc::pullable< tpie::streaming::pull_buffer<int, false, X, Y> >));
	BOOST_CONCEPT_ASSERT((sc::memory_managable< tpie::streaming::pull_buffer<int, false, X, Y> >));
	if (!strcmp(testName, "memory")) {
		tpie::streaming::pull_buffer<int> buffer(blockFactor);
		memory_test_split(&buffer, 0, sizeof(buffer), 1.0, 1.0);
	} else if (!strcmp(testName, "minimum_memory")) {
		test_pull_buffer_memory_limit test;
		test(0, 0);
	} else if (!strcmp(testName, "process")) {
		tpie::streaming::pull_buffer<int> buffer(blockFactor);
		buffer.set_memory_in(buffer.minimum_memory_in()+sizeof(int)*3);
		buffer.set_memory_out(buffer.minimum_memory_out()+sizeof(int)*3);

		buffer.begin();
		for (int i=0; test[i]; ++i)
			buffer.push(test[i]);
		buffer.end();

		buffer.pull_begin();
		for (int i=0; test[i]; ++i) {
			if (!buffer.can_pull() ) ERR("can_pull");
			if (buffer.pull() != test[i]) ERR("pull()");
		}
		if (buffer.can_pull() ) ERR("can_pull 2");
		buffer.pull_end();
	} else
		ERR("No such test");
}

//=================================> virtual <==================================
void test_virtual(char * testName) {
	BOOST_CONCEPT_ASSERT((sc::pushable< tpie::streaming::virtual_sink_impl<int, X, Y> >));
	BOOST_CONCEPT_ASSERT((sc::memory_managable< tpie::streaming::virtual_sink_impl<int, X, Y> >));
	BOOST_CONCEPT_ASSERT((sc::memory_managable< tpie::streaming::virtual_source_impl<push_test_sink> >));
	if (!strcmp(testName, "memory")) {
		push_test_sink c;
		tpie::streaming::virtual_source_impl<push_test_sink> b(c);
		tpie::streaming::virtual_sink_impl<int> a(&b);
		memory_test_single(&b, &c, 0, sizeof(b), 0.0);
		memory_test_single(&a, &b, 0, sizeof(a), 0.0);
	} else if (!strcmp(testName, "minimum_memory")) {
		memory_monitor monitor;
		memory_test_sink c(&monitor);
		monitor.begin();
		tpie::streaming::virtual_source_impl<memory_test_sink> * b  = new tpie::streaming::virtual_source_impl<memory_test_sink>(c);
		monitor.sample();
		tpie::streaming::virtual_sink_impl<int> * a = new tpie::streaming::virtual_sink_impl<int>(b);
		monitor.sample();
		memory_size_type b_memory = b->minimum_memory();
		memory_size_type a_memory = a->minimum_memory();
		b->set_memory(b_memory);
		monitor.sample();
		a->set_memory(a_memory);
		monitor.sample();
		a->begin();
		monitor.sample();
		for (int i=0; i < 42; ++i) {
			a->push(i);
			monitor.sample();
		}
		a->end();
		monitor.sample();
		memory_size_type usage = monitor.usage(2);
		delete a;
		delete b;
		monitor.clear();
		memory_size_type aa = monitor.usage(0);
		std::cout << "Memory used: " << usage << "; Allowed: " << b_memory + a_memory << "; After dealocation: " << aa << ";" << std::endl;
		if (usage > b_memory+a_memory) ERR("Used more memory then allocated");
		if (aa > 0) ERR("Did not deallocate all its memory");
	} else if (!strcmp(testName, "process")) {
		push_test_sink c;
		tpie::streaming::virtual_source_impl<push_test_sink> b(c);
		tpie::streaming::virtual_sink_impl<int> a(&b);
		push_test_source<tpie::streaming::virtual_sink_impl<int> > z(a);
		z.run(c, (int*)test, test+the_test_size, false);
	} else
		ERR("No such test");
}

//============================> push_block_buffer <=============================
struct test_push_block_buffer_memory_limit: public test_push_single_memory_limit< tpie::streaming::push_block_buffer<memory_test_sink> > {
	memory_test_sink sink;
	virtual tpie::streaming::push_block_buffer<memory_test_sink> * construct(memory_monitor & mm) {
		sink.monitor = &mm;
		return new tpie::streaming::push_block_buffer<memory_test_sink>(sink);
	}
};

void test_push_block_buffer(char * testName) {
	BOOST_CONCEPT_ASSERT((sc::pushable< tpie::streaming::push_block_buffer<push_test_sink> >));
	BOOST_CONCEPT_ASSERT((sc::memory_managable< tpie::streaming::push_block_buffer<push_test_sink> >));
	if (!strcmp(testName, "memory")) {
		push_test_sink sink;
		tpie::streaming::push_block_buffer<push_test_sink> buffer(sink, blockFactor);
		memory_test_single(&buffer, &sink, 0, sizeof(buffer), 0.0);
	} else if (!strcmp(testName, "minimum_memory")) {
		test_push_block_buffer_memory_limit test;
		test(0);
	} else if (!strcmp(testName, "process")) {
		push_test_sink sink;
		push_block_buffer<push_test_sink> push_buffer(sink, blockFactor);
		push_test_source<push_block_buffer<push_test_sink> > source(push_buffer);
		source.run(sink, (int*)test, test+the_test_size, false);
	} else
		ERR("No such test");
}

//============================> pull_block_buffer <=============================
struct test_pull_block_buffer_memory_limit: public test_pull_single_memory_limit< tpie::streaming::pull_block_buffer<memory_test_source> > {
	memory_test_source source;
	virtual tpie::streaming::pull_block_buffer<memory_test_source> * construct(memory_monitor & mm) {
		source.monitor = &mm;
		return new tpie::streaming::pull_block_buffer<memory_test_source>(source);
	}
};

void test_pull_block_buffer(char * testName) {
	BOOST_CONCEPT_ASSERT((sc::pullable< tpie::streaming::pull_block_buffer<pull_test_source<int*> > >));
 	BOOST_CONCEPT_ASSERT((sc::memory_managable< tpie::streaming::pull_block_buffer<pull_test_source<int*> > >));
	if (!strcmp(testName, "memory")) {
		pull_test_source<int*> source((int*)test, test+the_test_size);
		pull_block_buffer< pull_test_source<int*> > buffer(source, blockFactor);
		memory_test_single(&buffer, 0, &source, sizeof(buffer), 0.0);
	} else if (!strcmp(testName, "minimum_memory")) {
		test_pull_block_buffer_memory_limit test;
		test(0);
	} else if (!strcmp(testName, "process")) {
		pull_test_source<int*> source((int*)test, test+the_test_size);
		pull_block_buffer< pull_test_source<int*> > buffer(source, blockFactor);
		pull_test_sink< pull_block_buffer< pull_test_source<int*> > > sink(buffer);
		sink.run();
	} else
		ERR("No such test");
}

//===================================> main <===================================
int main(int argc, char ** argv) {
	MM_manager.set_memory_limit(128*1024*1024);
	blockFactor=file_base::calculate_block_factor(32*sizeof(int));
	if (argc != 3) return 1;
	remove("/tmp/stream");
	if (!strcmp(argv[1], "stream_source"))
		test_stream_source(argv[2]);
	else if (!strcmp(argv[1], "stream_sink"))
		test_stream_sink(argv[2]);
	else if (!strcmp(argv[1], "sort"))
		test_sort(argv[2]);
	else if (!strcmp(argv[1], "pull_sort"))
		test_pull_sort(argv[2]);
	else if (!strcmp(argv[1], "buffer"))
		test_buffer(argv[2]);
	else if (!strcmp(argv[1], "pull_buffer"))
		test_pull_buffer(argv[2]);
	else if (!strcmp(argv[1], "pull_block_buffer"))
		test_pull_block_buffer(argv[2]);
	else if (!strcmp(argv[1], "push_block_buffer"))
		test_push_block_buffer(argv[2]);
	else if (!strcmp(argv[1], "virtual"))
		test_virtual(argv[2]);
	else
		ERR("no such test");
	return 0;
}