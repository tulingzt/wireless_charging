#include "cmsis_os.h"
#include "tim.h"
#include "QiProtocol.hpp"

uint32_t tim4_channel1_value[1024];
QiProtocol qi(QiProtocol::DeviceType::Receiver);

extern "C"
{
    uint8_t qi_oc_dma_output(uint8_t *data, size_t len)
    {
        int index1, index2, change_value;
        for (index1 = 0, index2 = 0, change_value = 0; index1 < len; index1++)
        {
            if (data[index1]) // 1码
            {
                change_value += 50;
                tim4_channel1_value[index2++] = change_value;
                change_value += 50;
                tim4_channel1_value[index2++] = change_value;
            }
            else // 0码
            {
                change_value += 100;
                tim4_channel1_value[index2++] = change_value;
            }
        }
        return HAL_TIM_OC_Start_DMA(&htim4, TIM_CHANNEL_1, tim4_channel1_value, index2);
    }
    
    void Comm_Task(void const * argument)
    {
        qi.RegisterHardwareSendFunction(qi_oc_dma_output);
        for (;;)
        {
            uint8_t data = 10;
            qi.SendData(0x16, &data, 1);
            HAL_Delay(1000);
        }
    }
    
    void qi_send_complete_callback(void)
    {
        qi.SendCompleteCallback();
    }
}