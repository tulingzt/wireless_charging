#ifndef __HBRIGHT_HPP__
#define __HBRIGHT_HPP__

#include "hrtim.h"

class HBridgeController
{
public:
    HBridgeController(HRTIM_HandleTypeDef *hrtim) : hrtim_(hrtim) {}
    // 初始化H桥控制器
    inline void Init(float targetFreq, float phase, float dutyA, float dutyB)
    {
        SetFrequency(targetFreq);
        SetPhase(phase);
        SetDutyCycle(dutyA, dutyB);
    }
    // 启动H桥控制器
    void Open();
    // 关闭H桥控制器
    void Close();

    // 获取当前频率（Hz）
    float GetFrequency();
    // 获取当前相位差
    float GetPhase();
    // 获取当前占空比
    float GetDutyCycleA();
    float GetDutyCycleB();

    // 设置频率（理论支持范围：650Hz ~ 5.44GHz，最好设置在1kHz ~ 50MHz）
    bool SetFrequency(float targetFreq);
    // 设置相位差（0.0 ~ 1.0，对应0°~360°）
    void SetPhase(float phase);
    // 设置占空比（0.0 ~ 1.0）
    void SetDutyCycle(float dutyA, float dutyB);

private:
    HRTIM_HandleTypeDef *hrtim_;
    // 分频比转换因子（例如DIV4对应1/4）
    inline float GetPrescalerFactor(uint32_t ratio)
    {
        static const float factors[] = {32.0f, 16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f};
        return factors[ratio];
    }
    // 获取周期值
    inline uint32_t GetPeriod() { return __HAL_HRTIM_GETPERIOD(hrtim_, HRTIM_TIMERINDEX_MASTER); }
    // 获取时钟分频比
    inline uint32_t GgetClockPrescaler() { return __HAL_HRTIM_GETCLOCKPRESCALER(hrtim_, HRTIM_TIMERINDEX_MASTER); }
};

#endif // __HBRIGHT_HPP__