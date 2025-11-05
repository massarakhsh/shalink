#ifndef _TMS_H
#define _TMS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

typedef int64_t MCS;

#define ShaSec ((MCS)1000000)
#define ShaMSec ((MCS)1000)
#define ShaMCSec ((MCS)1)

MCS GetNow();
MCS GetDuration();
MCS GetSience(MCS from);
void Sleep(MCS delay);

#endif
