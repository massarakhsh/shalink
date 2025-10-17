#ifndef _TMS_H
#define _TMS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

typedef int64_t MS;

MS GetNow();
MS GetDuration();
void Sleep(MS delay);

#endif
