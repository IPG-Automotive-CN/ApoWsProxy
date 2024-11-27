#ifndef TESTRUNMGR_H
#define TESTRUNMGR_H

#include "DataDefine.h"


extern TestrunInfo tsInfo;


int loadTestrun(const char* projectdir, const char *testrun);
int isTestRunLoaded();

char * getOpenDrivePath();
#endif
