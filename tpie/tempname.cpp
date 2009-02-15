#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <cstring>
#include "lib_config.h"
#include <tpie/tempname.h>
#include <string>

using namespace tpie;

std::string tempname::default_path;
std::string tempname::default_base_name; 
std::string tempname::default_extension;


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

	if(post_base.empty())
		return base_dir + TPIE_OS_DIR_DELIMITER + base_name + "_" + tpie_mktemp() + "." + extension;
	else 
		return base_dir + TPIE_OS_DIR_DELIMITER + base_name + "_" + post_base + "_" + tpie_mktemp() + "." + extension;
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
		dir = TMP_DIR; //OS hardcoded path (from portability.h)

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
		chars[TPIE_OS_RANDOM() % chars_count] +
		chars[TPIE_OS_RANDOM() % chars_count] +
		chars[TPIE_OS_RANDOM() % chars_count] +
		chars[TPIE_OS_RANDOM() % chars_count];

	counter = (counter + 1) % (chars_count * chars_count);

	return result;
}


void tempname::set_default_path(const std::string&  path) {
	default_path = path;
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
