#include "HBridge.hpp"
#include <algorithm>

using namespace std;

#define fHRTIM 170e6 // HRTIM时钟频率

void HBridgeController::Open()
{
    HAL_HRTIM_WaveformOutputStart(hrtim_, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
    HAL_HRTIM_WaveformCounterStart(hrtim_, HRTIM_TIMERID_MASTER | HRTIM_TIMERID_TIMER_A | HRTIM_TIMERID_TIMER_B);
}

void HBridgeController::Close()
{
    HAL_HRTIM_WaveformCounterStop(hrtim_, HRTIM_TIMERID_MASTER | HRTIM_TIMERID_TIMER_A | HRTIM_TIMERID_TIMER_B);
    HAL_HRTIM_WaveformOutputStop(hrtim_, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
}

float HBridgeController::GetFrequency()
{
    return fHRTIM * GetPrescalerFactor(GgetClockPrescaler()) / GetPeriod();
}

float HBridgeController::GetPhase()
{
    return static_cast<float>(__HAL_HRTIM_GETCOMPARE(hrtim_, HRTIM_TIMERINDEX_MASTER, HRTIM_COMPAREUNIT_1) / GetPeriod());
}

float HBridgeController::GetDutyCycleA()
{
    return static_cast<float>(__HAL_HRTIM_GETCOMPARE(hrtim_, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1) / GetPeriod());
}

float HBridgeController::GetDutyCycleB()
{
    return static_cast<float>(__HAL_HRTIM_GETCOMPARE(hrtim_, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_1) / GetPeriod());
}

bool HBridgeController::SetFrequency(float targetFreq)
{
    // 分频比优先级：DIV4 < DIV2 < DIV1 < MUL2 < ... < MUL32 采用分辨率最高方案 也可自定义调整优先级
    const uint32_t prescalers[] = {
        HRTIM_PRESCALERRATIO_MUL32,  /*!< fHRCK: fHRTIM x 32U = 4.608 GHz - Resolution: 217 ps - Min PWM frequency: 70.3 kHz (fHRTIM=144MHz) */
        HRTIM_PRESCALERRATIO_MUL16,  /*!< fHRCK: fHRTIM x 16U = 2.304 GHz - Resolution: 434 ps - Min PWM frequency: 35.1 KHz (fHRTIM=144MHz) */
        HRTIM_PRESCALERRATIO_MUL8 ,  /*!< fHRCK: fHRTIM x 8U = 1.152 GHz - Resolution: 868 ps - Min PWM frequency: 17.6 kHz (fHRTIM=144MHz)  */
        HRTIM_PRESCALERRATIO_MUL4 ,  /*!< fHRCK: fHRTIM x 4U = 576 MHz - Resolution: 1.73 ns - Min PWM frequency: 8.8 kHz (fHRTIM=144MHz)    */
        HRTIM_PRESCALERRATIO_MUL2 ,  /*!< fHRCK: fHRTIM x 2U = 288 MHz - Resolution: 3.47 ns - Min PWM frequency: 4.4 kHz (fHRTIM=144MHz)    */
        HRTIM_PRESCALERRATIO_DIV1 ,  /*!< fHRCK: fHRTIM = 144 MHz - Resolution: 6.95 ns - Min PWM frequency: 2.2 kHz (fHRTIM=144MHz)         */
        HRTIM_PRESCALERRATIO_DIV2 ,  /*!< fHRCK: fHRTIM / 2U = 72 MHz - Resolution: 13.88 ns- Min PWM frequency: 1.1 kHz (fHRTIM=144MHz)     */
        HRTIM_PRESCALERRATIO_DIV4 ,  /*!< fHRCK: fHRTIM / 4U = 36 MHz - Resolution: 27.7 ns- Min PWM frequency: 550Hz (fHRTIM=144MHz)        */
    };
    for (auto ratio : prescalers)
    {
        float fHRCK = fHRTIM * GetPrescalerFactor(ratio); // 计算实际时钟频率
        uint32_t period = static_cast<uint32_t>(fHRCK / targetFreq);
        if (period >= 100 && period <= 0xFFFF) // 周期有效范围检查
        {
            float phase = GetPhase();
            float dutyA = GetDutyCycleA();
            float dutyB = GetDutyCycleB();
            // 配置Master/TimerA/TimerB的周期和分频比
            __HAL_HRTIM_SETPERIOD(hrtim_, HRTIM_TIMERINDEX_MASTER, period);
            __HAL_HRTIM_SETPERIOD(hrtim_, HRTIM_TIMERINDEX_TIMER_A, period);
            __HAL_HRTIM_SETPERIOD(hrtim_, HRTIM_TIMERINDEX_TIMER_B, period);
            __HAL_HRTIM_SETCLOCKPRESCALER(hrtim_, HRTIM_TIMERINDEX_MASTER, ratio);
            __HAL_HRTIM_SETCLOCKPRESCALER(hrtim_, HRTIM_TIMERINDEX_TIMER_A, ratio);
            __HAL_HRTIM_SETCLOCKPRESCALER(hrtim_, HRTIM_TIMERINDEX_TIMER_B, ratio);
            // 调整频率后需重新配置相位差和占空比
            SetPhase(phase);
            SetDutyCycle(dutyA, dutyB);
            return true;
        }
    }
    return false; // 频率超出支持范围，保持原先配置
}

void HBridgeController::SetPhase(float phase)
{
    phase = max(0.0f, min(1.0f, phase)); // 限制占空比
    uint32_t phaseOffset = static_cast<uint32_t>(GetPeriod() * phase);
    __HAL_HRTIM_SETCOMPARE(hrtim_, HRTIM_TIMERINDEX_MASTER, HRTIM_COMPAREUNIT_1, phaseOffset);
}

void HBridgeController::SetDutyCycle(float dutyA, float dutyB)
{
    dutyA = max(0.0f, min(1.0f, dutyA)); // 限制占空比
    dutyB = max(0.0f, min(1.0f, dutyB)); // 限制占空比
    uint32_t cmpA = static_cast<uint32_t>(GetPeriod() * dutyA);
    uint32_t cmpB = static_cast<uint32_t>(GetPeriod() * dutyB);
    __HAL_HRTIM_SETCOMPARE(hrtim_, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1, cmpA);
    __HAL_HRTIM_SETCOMPARE(hrtim_, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_1, cmpB);
}