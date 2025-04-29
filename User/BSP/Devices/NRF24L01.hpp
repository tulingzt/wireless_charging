#ifndef __NRF24L01_HPP__
#define __NRF24L01_HPP__

#include "cmsis_os.h"
#include "spi.h"
#include "gpio.h"
//不要使用硬件NSS，硬件NSS会每个字节抬高一个时钟时间
class NRF24L01
{
public:
    NRF24L01(SPI_HandleTypeDef *hspi, GPIO_TypeDef *ce_gpio_port, uint16_t ce_pin,
    GPIO_TypeDef *csn_gpio_port, uint16_t csn_pin) : hspi_(hspi), ce_gpio_port_(ce_gpio_port),
    ce_pin_(ce_pin), csn_gpio_port_(csn_gpio_port), csn_pin_(csn_pin)
    {
        osSemaphoreDef(irqSem);
        irq_sem_handle_ = osSemaphoreCreate(osSemaphore(irqSem), 1);
        osSemaphoreDef(spiSem);
        dma_sem_handle_ = osSemaphoreCreate(osSemaphore(spiSem), 1);
    }
    uint8_t Check(void);
    void SetTxMode(void);
    void SetRxMode(void);
    uint8_t TxPacket(uint8_t *txBuf, size_t txLen, uint8_t *rxBuf, size_t rxLen);
    uint8_t RxPacket(uint8_t *txBuf, size_t txLen, uint8_t *rxBuf, size_t rxLen);
    void TransferCompleteCallback()
    {
        SetCSN();
        osSemaphoreRelease(dma_sem_handle_);
    }
    void IRQCallback()
    {
        osSemaphoreRelease(irq_sem_handle_);
    }
private:
    SPI_HandleTypeDef *hspi_;
    GPIO_TypeDef *ce_gpio_port_, *csn_gpio_port_;
    uint16_t ce_pin_, csn_pin_;
    // 信号量
    osSemaphoreId dma_sem_handle_, irq_sem_handle_;
    // 寄存器
    uint8_t status;
    uint8_t observe_tx;
    #pragma pack(push, 1)
    struct SPI_Frame {
        uint8_t head;
        uint8_t data[32];
    };
    #pragma pack(pop)
    SPI_Frame tx_frame_, rx_frame_;
    inline void SetCE() { HAL_GPIO_WritePin(ce_gpio_port_, ce_pin_, GPIO_PIN_SET); }
    inline void ResetCE() { HAL_GPIO_WritePin(ce_gpio_port_, ce_pin_, GPIO_PIN_RESET); }
    inline void SetCSN() { HAL_GPIO_WritePin(csn_gpio_port_, csn_pin_, GPIO_PIN_SET); }
    inline void ResetCSN() { HAL_GPIO_WritePin(csn_gpio_port_, csn_pin_, GPIO_PIN_RESET); }

    uint8_t TransmitDMA(size_t len)
    {
        HAL_StatusTypeDef result;
        ResetCSN();
        result = HAL_SPI_TransmitReceive_DMA(hspi_, reinterpret_cast<uint8_t*>(&tx_frame_), reinterpret_cast<uint8_t*>(&rx_frame_), len);
        osSemaphoreWait(dma_sem_handle_, osWaitForever);
        return result;
    }
    uint8_t WriteReg(uint8_t reg, uint8_t value);
    uint8_t ReadReg(uint8_t reg);
    uint8_t WriteBuf(uint8_t reg, uint8_t *pBuf, size_t len);
    uint8_t ReadBuf(uint8_t reg, uint8_t *pBuf, size_t len);
};

#endif // __NRF24L01_HPP__