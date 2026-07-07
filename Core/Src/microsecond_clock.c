#include "microsecond_clock.h"

static uint32_t previousCycleCounterValue = 0U;
static uint64_t accumulatedCycleCount = 0ULL;
static uint32_t cyclesPerMicrosecond = 1U;
static uint8_t microsecondClockInitialized = 0U;

void MicrosecondClock_Initialize(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    cyclesPerMicrosecond = SystemCoreClock / 1000000U;
    if (cyclesPerMicrosecond == 0U)
    {
        cyclesPerMicrosecond = 1U;
    }

    previousCycleCounterValue = DWT->CYCCNT;
    accumulatedCycleCount = 0ULL;
    microsecondClockInitialized = 1U;
}

uint64_t MicrosecondClock_Now(void)
{
    if (microsecondClockInitialized == 0U)
    {
        MicrosecondClock_Initialize();
    }

    uint32_t currentCycleCounterValue = DWT->CYCCNT;

    uint32_t elapsedCycleCount =
        currentCycleCounterValue - previousCycleCounterValue;

    previousCycleCounterValue = currentCycleCounterValue;
    accumulatedCycleCount += (uint64_t)elapsedCycleCount;

    return accumulatedCycleCount / (uint64_t)cyclesPerMicrosecond;
}
