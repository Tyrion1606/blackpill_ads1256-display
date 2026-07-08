#include "microsecond_delay.h"

static uint32_t previousCycleCounterValue = 0;
static uint64_t extendedCycleCounterHighWord = 0;

void MicrosecondDelay_Initialize(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  previousCycleCounterValue = 0;
  extendedCycleCounterHighWord = 0;
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

uint32_t MicrosecondDelay_GetTimestampMicroseconds(void)
{
  uint32_t currentCycleCounterValue = DWT->CYCCNT;
  uint32_t numberOfCpuCyclesPerMicrosecond = SystemCoreClock / 1000000U;

  if (currentCycleCounterValue < previousCycleCounterValue)
  {
    extendedCycleCounterHighWord += (1ULL << 32);
  }

  previousCycleCounterValue = currentCycleCounterValue;

  uint64_t extendedCycleCounterValue =
      extendedCycleCounterHighWord | currentCycleCounterValue;

  return (uint32_t)(extendedCycleCounterValue /
                    numberOfCpuCyclesPerMicrosecond);
}
