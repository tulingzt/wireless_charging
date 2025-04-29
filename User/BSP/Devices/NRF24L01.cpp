#include "NRF24L01.hpp"
#include <string.h>

#define NRF_READ_REG        0x00//读寄存器命令需要或上寄存器地址
#define NRF_WRITE_REG       0x20//写寄存器命令需要或上寄存器地址 
#define RD_RX_PLOAD         0x61//从FIFO中读收到的数据，1-32字节，读出后FIFO数据被删除。适用于接收模式
#define WR_TX_PLOAD         0xA0//写发射负载数据，大小为1-32字节，适用于发射模式。
#define RD_RX_PLOAD_WID     0x60//读取收到的数据字节数。
#define WR_ACK_PLOAD        0xA8//适用于接收方，通过PIPE PPP将数据通过ACK的形式发出去，最多允许三帧数据存于FIFO中
#define WR_TX_PLOAD_NOACK   0xB0//适用于发射模式，使用这个命令同时需要将AUTOACK位置1。
#define FLUSH_TX            0xE1//清空TX FIFO，适用于发射模式。
#define FLUSH_RX            0xE2//清空RX FIFO，适用于接收模式。
#define REUSE_TX_PL         0xE3//适用于发送方，清空TX FIFO或对FIFO写入新的数据后不能使用该命令。
#define NOP                 0xFF//无操作。可用于返回STATUS值。

#define CONFIG          0x00
                            
#define EN_AA           0x01
#define EN_RXADDR       0x02
#define SETUP_AW        0x03
#define SETUP_RETR      0x04
#define RF_CH           0x05
#define RF_SETUP        0x06
#define STATUS          0x07
              
#define MAX_TX  		0x10
#define TX_OK   		0x20
#define RX_OK   		0x40

#define OBSERVE_TX      0x08
#define CD              0x09
#define RX_ADDR_P0      0x0A
#define RX_ADDR_P1      0x0B
#define RX_ADDR_P2      0x0C
#define RX_ADDR_P3      0x0D
#define RX_ADDR_P4      0x0E
#define RX_ADDR_P5      0x0F
#define TX_ADDR         0x10
#define RX_PW_P0        0x11
#define RX_PW_P1        0x12
#define RX_PW_P2        0x13
#define RX_PW_P3        0x14
#define RX_PW_P4        0x15
#define RX_PW_P5        0x16
#define NRF_FIFO_STATUS 0x17

#define DYNPD           0x1C
#define FEATURE         0x1D

#define TX_ADR_WIDTH    5
#define RX_ADR_WIDTH    5
#define TX_PLOAD_WIDTH  32
#define RX_PLOAD_WIDTH  32

const uint8_t TX_ADDRESS[TX_ADR_WIDTH]={0x34,0x43,0x10,0x10,0x01};
const uint8_t RX_ADDRESS[RX_ADR_WIDTH]={0x34,0x43,0x10,0x10,0x01};

uint8_t NRF24L01::WriteReg(uint8_t reg, uint8_t value)
{
    tx_frame_.head = reg;
    tx_frame_.data[0] = value;
    TransmitDMA(2);
    return rx_frame_.head;
}

uint8_t NRF24L01::ReadReg(uint8_t reg)
{
    tx_frame_.head = reg;
    TransmitDMA(2);
    return rx_frame_.data[0];
}

uint8_t NRF24L01::WriteBuf(uint8_t reg, uint8_t *pBuf, size_t len)
{
    tx_frame_.head = reg;
    memcpy(tx_frame_.data, pBuf, len);
    TransmitDMA(len + 1);
    return 0;
}

uint8_t NRF24L01::ReadBuf(uint8_t reg, uint8_t *pBuf, size_t len)
{
    tx_frame_.head = reg;
    TransmitDMA(len + 1);
    memcpy(pBuf, rx_frame_.data, len);
    return 0;
}

uint8_t NRF24L01::Check(void)
{
    uint8_t buf[5]={0xb5, 0xb5, 0xb5, 0xb5, 0xb5};
    WriteBuf(NRF_WRITE_REG | TX_ADDR, buf, 5);
	ReadBuf(TX_ADDR, buf, 5);
    for (int i = 0; i < 5; i++)
        if (buf[i] != 0xb5)
            return 0;
    return 1;
}

void NRF24L01::SetTxMode(void)
{
    ResetCE();
    WriteBuf(NRF_WRITE_REG | TX_ADDR, (uint8_t*)TX_ADDRESS, TX_ADR_WIDTH);
	WriteBuf(NRF_WRITE_REG | RX_ADDR_P0, (uint8_t*)TX_ADDRESS, TX_ADR_WIDTH);
//	WriteReg(NRF_WRITE_REG | EN_AA, 0x01);
//	WriteReg(NRF_WRITE_REG | EN_RXADDR, 0x01);
	WriteReg(NRF_WRITE_REG | SETUP_RETR, 0xff);//0xff 0x1a
	WriteReg(NRF_WRITE_REG | RF_CH, 40);
	WriteReg(NRF_WRITE_REG | RF_SETUP, 0x0f);//0x03
    WriteReg(NRF_WRITE_REG | FEATURE, 0x06);
    WriteReg(NRF_WRITE_REG | DYNPD, 0x01);
	WriteReg(NRF_WRITE_REG | CONFIG, 0x0e);
    SetCE();
}

void NRF24L01::SetRxMode(void)
{
    ResetCE();
    WriteBuf(NRF_WRITE_REG | RX_ADDR_P0, (uint8_t*)RX_ADDRESS, 5);
//  	WriteReg(NRF_WRITE_REG | EN_AA, 0x01);
//  	WriteReg(NRF_WRITE_REG | EN_RXADDR, 0x01);
  	WriteReg(NRF_WRITE_REG | RF_CH, 40);
  	WriteReg(NRF_WRITE_REG | RX_PW_P0, RX_PLOAD_WIDTH);
  	WriteReg(NRF_WRITE_REG | RF_SETUP, 0x0f);//0x03
    WriteReg(NRF_WRITE_REG | FEATURE, 0x06);
    WriteReg(NRF_WRITE_REG | DYNPD, 0x01);
  	WriteReg(NRF_WRITE_REG | CONFIG, 0x0f);
    SetCE();
}

uint8_t NRF24L01::TxPacket(uint8_t *txBuf, size_t txLen, uint8_t *rxBuf, size_t rxLen)
{
    ResetCE();
    WriteBuf(WR_TX_PLOAD, txBuf, txLen);
    SetCE();
    osSemaphoreWait(irq_sem_handle_, osWaitForever);
    status = ReadReg(NRF_READ_REG | STATUS);
    WriteReg(NRF_WRITE_REG | STATUS, status);
    observe_tx = ReadReg(NRF_READ_REG | OBSERVE_TX);
    if (status & MAX_TX)
    {
        WriteReg(FLUSH_TX, 0xff);
        return MAX_TX;
    }
    if (status & TX_OK)
    {
        ReadBuf(RD_RX_PLOAD, rxBuf, rxLen);
        WriteReg(FLUSH_RX, 0xff);
        return TX_OK;
    }
    return 0xff;
}

uint8_t NRF24L01::RxPacket(uint8_t *txBuf, size_t txLen, uint8_t *rxBuf, size_t rxLen)
{
    osSemaphoreWait(irq_sem_handle_, osWaitForever);
    status = ReadReg(NRF_READ_REG | STATUS);
    WriteReg(NRF_WRITE_REG | STATUS, status);
    if (status & RX_OK)
    {
        WriteBuf(WR_ACK_PLOAD | 0x00, txBuf, txLen);
        ReadBuf(RD_RX_PLOAD, rxBuf, rxLen);
        WriteReg(FLUSH_RX, 0xff);
        return 0;
    }
    return 1;
}

//uint8_t NRF24L01::TxPacket(uint8_t *txBuf, size_t len)
//{
//    ResetCE();
//    WriteBuf(WR_TX_PLOAD, txBuf, len);
//    SetCE();
//    osSemaphoreWait(irq_sem_handle_, osWaitForever);
//    status = ReadReg(STATUS);
//    WriteReg(NRF_WRITE_REG | STATUS, status);
//    if (status & MAX_TX)
//    {
//        WriteReg(FLUSH_TX, 0xff);
//        return MAX_TX;
//    }
//    if (status & TX_OK)
//    {
//        return TX_OK;
//    }
//    return 0xff;
//}

//uint8_t NRF24L01::RxPacket(uint8_t *rxBuf, size_t len)
//{
//    status = ReadReg(STATUS);
//    WriteReg(NRF_WRITE_REG | STATUS, status);
//    if (status & RX_OK)
//    {
//        ReadBuf(RD_RX_PLOAD, rxBuf, len);
//        WriteReg(FLUSH_RX, 0xff);
//        return 0;
//    }
//    return 1;
//}
