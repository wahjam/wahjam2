#pragma once

#include <stdio.h>

extern FILE *logfp;

bool globalInit();
void globalCleanup();
