#include "microsecond_delay.h"

void MicrosecondDelay_Initialize(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void MicrosecondDelay_Wait(uint32_t delayTimeInMicroseconds)
{
  uint32_t delayStartCycleCounterValue = DWT->CYCCNT;
  uint32_t numberOfCpuCyclesPerMicrosecond = SystemCoreClock / 1000000U;
  uint32_t requestedDelayInCpuCycles = delayTimeInMicroseconds * numberOfCpuCyclesPerMicrosecond;

  while ((DWT->CYCCNT - delayStartCycleCounterValue) < requestedDelayInCpuCycles)
  {
    __NOP();
  }
}
