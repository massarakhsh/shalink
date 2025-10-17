#include "tms.h"
#include "interop.h"

MS startMS = 0;

MS GetNow() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t now = (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if (startMS == 0) startMS = now;
    return now;
}

MS GetDuration() {
    MS now = GetNow();
    return now - startMS;
}

void Sleep(MS delay) {
    usleep(delay * 1000);
}
