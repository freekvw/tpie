#include <tpie/portability.h>

//Needed for windows only

#ifdef _WIN32

std::ostream& operator<<(std::ostream& s, const TPIE_OS_OFFSET x){
  char buf[30];
  sprintf(buf,"%I64d",x);
  return s << buf;
}

#endif
