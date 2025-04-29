#include "cmsis_os.h"
#include "HBridge.hpp"
#include "hrtim.h"

HBridgeController hbridge(&hhrtim1);

extern "C"
{
    void Charging_Task(void const * argument)
    {
        hbridge.Init(135e3, 0.5f, 0.5f, 0.5f);
        hbridge.Open();
        for (;;)
        {
            // hbridge.SetFrequency(10e3); // 设置频率10kHz
            HAL_Delay(100);
        }
    }
}