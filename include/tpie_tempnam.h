//
// File: tpie_tempnam.h
// Author: 
// Created: 02/02/02
//
// $Id: tpie_tempnam.h,v 1.1 2002-02-02 18:10:12 tavi Exp $
//
//
#ifndef _TPIE_TEMPNAM_H
#define _TPIE_TEMPNAM_H

// The name of the environment variable pointing to a tmp directory.
#define TMP_DIR_ENV "TMPDIR"

// The name of a tmp directory to use if the env variable is not set.
#define TMP_DIR "/var/tmp"

// The name of the environment variable to consult for default device
// descriptions.
#define AMI_SINGLE_DEVICE_ENV "AMI_SINGLE_DEVICE"

char *tpie_tempnam(char *);

#endif // _TPIE_TEMPNAM_H 