// Copyright (c) 1995 Darren Erik Vengroff
//
// File: ami_key.cpp
// Author: Darren Erik Vengroff <dev@cs.duke.edu>
// Created: 3/12/95
//

#include <key.h>

key_range::key_range() : m_min(0), m_max(0) {
}

key_range::key_range(kb_key min_key, kb_key max_key) : m_min(min_key), m_max(max_key) {
}


