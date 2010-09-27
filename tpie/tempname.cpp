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

#include <tpie/config.h>
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <cstring>
#include <tpie/tempname.h>
#include <tpie/tpie_log.h>
#include <string>
#include <tpie/portability.h>
#include <boost/filesystem.hpp>
#include <stdexcept>
#include <tpie/util.h>

using namespace tpie;

std::string tempname::default_path;
std::string tempname::default_base_name; 
std::string tempname::default_extension;

std::string tempname::get_system_path() {
#ifdef WIN32
	//set temporary path
	CHAR temp_path[MAX_PATH];
		
	if (GetTempPath(MAX_PATH,temp_path) != 0) {
		return std::string(temp_path);
	} else {
		TP_LOG_WARNING_ID("Could not get default system path, using current working dir.\n");
		return ".";
	}
#else
	return "/var/tmp";
#endif
}

std::string tempname::tpie_name(const std::string& post_base, const std::string& dir, const std::string& ext) 
{	std::string extension;
	std::string base_name;	
	std::string base_dir;
	
	extension = ext;
	if(extension.empty()) { 
		extension = default_extension;
		if(extension.empty())
			extension = "tpie";
	}
	
	base_name = default_base_name;
	if(base_name.empty())
		base_name = "TPIE";
	
	if(!dir.empty())
		base_dir = dir;
	else 
		base_dir = tempname::get_actual_path();

	std::string path;	
	for(int i=0; i < 42; ++i) {
		if(post_base.empty())
			path = base_dir + directory_delimiter + base_name + "_" + tpie_mktemp() + "." + extension;
		else 
			path = base_dir + directory_delimiter + base_name + "_" + post_base + "_" + tpie_mktemp() + "." + extension;
		if ( !file_exists(path) )
			return path;
	}
	throw std::runtime_error("Unable to find free name for temporary file");
}

std::string tempname::get_actual_path() {
	//information about the search order is in the header
	std::string dir;

	if(!default_path.empty()) 
		dir = default_path; //user specified path
	else if(getenv(AMI_SINGLE_DEVICE_ENV) != NULL)  //TPIE env variable
		dir = getenv(AMI_SINGLE_DEVICE_ENV);
	else if(getenv(TMPDIR_ENV) != NULL)  
		dir = getenv(TMPDIR_ENV); //OS env variable (from portability.h)
	else  
		dir = get_system_path(); //OS path

	return dir;
}

std::string tempname::tpie_mktemp()
{
	const std::string chars[] = 
	{ "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", 
	"N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", 
	"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", 
	"n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", 
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
	const int chars_count = 62;
	static TPIE_OS_TIME_T counter = time(NULL) % (chars_count * chars_count); 

	std::string result = "";
	result +=
		chars[counter/chars_count] +
		chars[counter%chars_count] +
		chars[random() % chars_count] +
		chars[random() % chars_count] +
		chars[random() % chars_count] +
		chars[random() % chars_count] +
		chars[random() % chars_count] +
		chars[random() % chars_count];

	counter = (counter + 1) % (chars_count * chars_count);

	return result;
}


void tempname::set_default_path(const std::string&  path, const std::string& subdir) {
	if (subdir=="") {
		default_path = path;
		return;
	}
	std::string p = path+"/"+subdir;
	try {
		if (!boost::filesystem::exists(p)) {
			boost::filesystem::create_directory(p);
		}
		if (!boost::filesystem::is_directory(p)) {	
			default_path = path;
			TP_LOG_WARNING_ID("Could not use " << p << " as directory for temporary files, trying " << path);
		}
		default_path = p;
	} catch (...) { 
		TP_LOG_WARNING_ID("Could not use " << p << " as directory for temporary files, trying " << path);
		default_path = path; 
	}	
}

void tempname::set_default_base_name(const std::string& name) {
	default_base_name = name;
}

void tempname::set_default_extension(const std::string& ext) {
	default_extension = ext;
}


const std::string& tempname::get_default_path() {
	return default_path;
}

const std::string& tempname::get_default_base_name() {
	return default_base_name;
}

const std::string& tempname::get_default_extension() {
	return default_extension;
}

temp_file::temp_file(): m_persist(false) {}

temp_file::temp_file(const std::string & path): m_path(path), m_persist(false) {}

const std::string & temp_file::path() {
	if (m_path.empty())
		m_path = tempname::tpie_name();
	std::cout << "Temp " << m_path << std::endl;
	return m_path;
}

temp_file::~temp_file() {
	if (!m_path.empty() && !m_persist && file_exists(m_path)) 
		remove(m_path);
}
