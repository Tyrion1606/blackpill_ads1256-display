#ifndef MICROSECOND_DELAY_H
#define MICROSECOND_DELAY_H

#include "main.h"
#include <stdint.h>

void MicrosecondDelay_Initialize(void);
void MicrosecondDelay_Wait(uint32_t delayTimeInMicroseconds);
uint32_t MicrosecondDelay_GetTimestampMicroseconds(void);

#endif /* MICROSECOND_DELAY_H */
