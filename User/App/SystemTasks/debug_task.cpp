#include "cmsis_os.h"
#include "usart.h"

// #include "arm_math.h"
// #include "arm_const_structs.h"

//#define FFT_LENGTH 1024
//#define ADC1_DMA_Size 1024
//int SAM_FRE = 2000000;

using namespace std;

extern "C"
{
    void Debug_Task(void const *argument)
    {
//        float fft_inputbuf[FFT_LENGTH * 2];
//        float fft_outputbuf[FFT_LENGTH];
//        uint32_t ADC1_ConvertedValue[ADC1_DMA_Size];
//        arm_cfft_radix2_instance_f32 scfft;
        for (;;)
        {
            HAL_Delay(100);
//            arm_cfft_f32(&arm_cfft_sR_f32_len1024, fft_inputbuf, 0, 1);
//            arm_cmplx_mag_f32(fft_inputbuf, fft_outputbuf, FFT_LENGTH);
        }
    }
}