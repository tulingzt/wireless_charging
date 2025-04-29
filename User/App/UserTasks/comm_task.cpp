#include "cmsis_os.h"
#include "NRF24L01.hpp"
#include "spi.h"

//#define NRF_IS_TX

NRF24L01 nrf24l01(&hspi1, NRF24L01_CE_GPIO_Port, NRF24L01_CE_Pin, NRF24L01_CSN_GPIO_Port, NRF24L01_CSN_Pin);
uint8_t data = 0;
uint8_t tx_data[32];
uint8_t rx_data[32];

void change(uint8_t data)
{
    for (int i = 0; i < 32; i++)
        tx_data[i] = data;
}

extern "C"
{
    void Comm_Task(void const * argument)
    {
        while (!nrf24l01.Check())
            HAL_Delay(100);
        #ifdef NRF_IS_TX
        nrf24l01.SetRxMode();
        for (;;)
        {
            change(data);
            data += 2;
            nrf24l01.RxPacket(tx_data, sizeof(tx_data), rx_data, sizeof(rx_data));
//            HAL_Delay(1000);
        }
        #else
        nrf24l01.SetTxMode();
        for (;;)
        {
            change(data++);
            nrf24l01.TxPacket(tx_data, sizeof(tx_data), rx_data, sizeof(rx_data));
            HAL_Delay(1000);
        }
        #endif
    }

    void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
    {
        if (hspi == &hspi1)
        {
            nrf24l01.TransferCompleteCallback();
        }
    }
    
    void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
    {
        if (GPIO_Pin == NRF24L01_IRQ_Pin)
        {
            nrf24l01.IRQCallback();
        }
    }
}