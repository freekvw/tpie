// Copyright (c) 1995 Darren Vengroff
//
// File: scan_uniform_sm.cpp
// Author: Darren Vengroff <darrenv@eecs.umich.edu>
// Created: 3/6/95
//


// Get information on the configuration to test.
#include "app_config.h"

// Define it all.
#include <ami.h>
VERSION(scan_uniform_sm_cpp,"$Id: scan_uniform_sm.cpp,v 1.4 2000-01-11 02:12:01 hutchins Exp $");

#include "scan_uniform_sm.h"

scan_uniform_sm::scan_uniform_sm(unsigned int rows, unsigned int cols,
                                 double density, int seed) :
        rmax(rows), cmax(cols), d(density)
{
    LOG_APP_DEBUG("scan_uniform_sm seed = ");
    LOG_APP_DEBUG(seed);
    LOG_APP_DEBUG('\n');

    srandom(seed);
}

scan_uniform_sm::~scan_uniform_sm(void)
{
}

AMI_err scan_uniform_sm::initialize(void)
{
    r = rmax - 1;
    c = cmax - 1;
    return AMI_ERROR_NO_ERROR;
}

AMI_err scan_uniform_sm::operate(AMI_sm_elem<double> *out, AMI_SCAN_FLAG *sf)
{
    double dr = double(random() & 0xFFF) / double(0x1000);

    if ((*sf = (dr < d))) {
        out->er = r;
        out->ec = c;
        out->val = 1.0;
    } 
        
    if (!(r--)) {
        r = rmax - 1;
        if (!(c--)) {
            return AMI_SCAN_DONE;
        }
    }

    return AMI_SCAN_CONTINUE;
}