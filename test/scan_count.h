#ifndef _SCAN_COUNT_H
#define _SCAN_COUNT_H

#include <tpie/portability.h>
#include <tpie/scan.h>

using namespace tpie;

class scan_count : ami::scan_object {
    
private:
    TPIE_OS_OFFSET maximum;
    
public:
    TPIE_OS_OFFSET ii;
    TPIE_OS_OFFSET called;
    
    scan_count(TPIE_OS_OFFSET max = 1000);
    ami::err initialize(void);
    ami::err operate(TPIE_OS_OFFSET *out1, ami::SCAN_FLAG *sf);
};


#endif // _SCAN_COUNT_H 
