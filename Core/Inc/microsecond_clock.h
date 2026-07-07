#ifndef MICROSECOND_CLOCK_H
#define MICROSECOND_CLOCK_H

#include "main.h"
#include <stdint.h>

void MicrosecondClock_Initialize(void);
uint64_t MicrosecondClock_Now(void);

#endif /* MICROSECOND_CLOCK_H */
