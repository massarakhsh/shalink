#include "tms.h"
#include "interop.h"

MCS startMCS = 0;

MCS GetNow() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t now = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    if (startMCS == 0) startMCS = now;
    return now;
}

MCS GetSience(MCS from) {
    MCS now = GetNow();
    return now - from;
}

MCS GetDuration() {
    MCS now = GetNow();
    return now - startMCS;
}

void Sleep(MCS delay) {
    usleep(delay);
}
