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

#ifndef _TPIE_MM_H
#define _TPIE_MM_H

#include <tpie/config.h>

// Get the base class, enums, etc...
#include <tpie/mm_base.h>

///////////////////////////////////////////////////////////////////////////
/// \file mm.h Provides means to choose and set a specific memory
/// management to use within TPIE.
/// For now only single address space memory management is supported
/// (through the class MM_Register).
///////////////////////////////////////////////////////////////////////////

// Get an implementation definition..

#ifdef MM_IMP_REGISTER
#include <tpie/mm_manager.h>
#else
#error No MM implementation selected.
#endif

#endif // _TPIE_MM_H 
