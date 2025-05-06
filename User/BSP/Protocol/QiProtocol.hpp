#ifndef __QI_PROTOCOL_HPP__
#define __QI_PROTOCOL_HPP__

#include "cmsis_os.h"

class QiProtocol
{
public:
    using HardwareSendFunction = uint8_t (*)(uint8_t *data, size_t len);
    enum class DeviceType
    {
        Transmitter,
        Receiver
    };
    // 发射端命令定义 (WPC Qi标准)
    enum class TransmitterCommand : uint8_t
    {
        NDATA = 0x00,
        ADT_1e = 0x16,
        ADT_1o = 0x17,
        PROP_1e = 0x1E,
        PROP_1o = 0x1F,
        ADC = 0x25,
        ADT_2e = 0x26,
        ADT_2o = 0x27,
        PROP_2e = 0x2E,
        PROP_2o = 0x2F,
        ID = 0x30,
        CAP = 0x31,
        XCAP = 0x32,
        ADT_3e = 0x36,
        ADT_3o = 0x37,
        PROP_3 = 0x3F,
        ADT_4e = 0x46,
        ADT_4o = 0x47,
        PROP_4 = 0x4F,
        ADT_5e = 0x56,
        ADT_5o = 0x57,
        PROP_5 = 0x5F,
        ADT_6e = 0x66,
        ADT_6o = 0x67,
        PROP_6 = 0x6F,
        ADT_7e = 0x76,
        ADT_7o = 0x77,
        PROP_7 = 0x7F,
        PROP_9 = 0x8F,
    };
    // 接收端命令定义 (WPC Qi标准)
    enum class ReceiverCommand : uint8_t
    {
        SIG = 0x01,
        EPT = 0x02,
        CE = 0x03,
        RP8 = 0x04,
        CHS = 0x05,
        PCH = 0x06,
        GPQ = 0x07,
        NEGO = 0x09,
        DSP = 0x15,
        ADT_1e = 0x16,
        ADT_1o = 0x17,
        PROP_1e = 0x18,
        PROP_1o = 0x19,
        SRQ = 0x20,
        FOD = 0x22,
        ADC = 0x25,
        ADT_2e = 0x26,
        ADT_2o = 0x27,
        PROP_2e = 0x28,
        PROP_2o = 0x29,
        RP = 0x31,
        ADT_3e = 0x36,
        ADT_3o = 0x37,
        PROP_3 = 0x38,
        ADT_4e = 0x46,
        ADT_4o = 0x47,
        PROP_4 = 0x48,
        CFG = 0x51,
        WPID_H = 0X54,
        WPID_L = 0x55,
        ADT_5e = 0x56,
        ADT_5o = 0x57,
        PROP_5 = 0x58,
        ADT_6e = 0x66,
        ADT_6o = 0x67,
        PROP_6 = 0x68,
        ID = 0x71,
        ADT_7e = 0x76,
        ADT_7o = 0x77,
        PROP_7 = 0x78,
        XID = 0x81,
        PROP_8 = 0x84,
        PROP_12 = 0xA4,
        PROP_16 = 0xC4,
        PROP_20 = 0xE2,
    };
    // 数据包结构
    struct QiPacket
    {
        uint8_t cmd;      // 命令字
        uint8_t data[27]; // 数据载荷（1 ~ 27字节）
        uint8_t checksum; // 校验
    };

    explicit QiProtocol(DeviceType type) : type_(type)
    {
        osSemaphoreDef(qiTxSem);
        tx_sem_handle_ = osSemaphoreCreate(osSemaphore(qiTxSem), 1);
        osMessageQDef(qiRxQueue, 512, uint32_t);
        rx_msg_handler_ = osMessageCreate(osMessageQ(qiRxQueue), NULL);
    };
    // 发送相关函数
    inline void RegisterHardwareSendFunction(HardwareSendFunction func) { hardware_send_function_ = func; }
    bool SendData(const uint8_t cmd, uint8_t *data, size_t dataLen);
    inline void SendCompleteCallback() { osSemaphoreRelease(tx_sem_handle_); }
    // 接收相关函数
    inline void RecieveDeltaTime(uint32_t data) { osMessagePut(rx_msg_handler_, data, 0); } // 接收跳变沿时间差 us
    bool ReceiveData(uint8_t &cmd, uint8_t *data);

private:
    DeviceType GetOppositeType() const
    {
        return type_ == DeviceType::Transmitter ? DeviceType::Receiver : DeviceType::Transmitter;
    }
    size_t ComputeDataLen(const uint8_t cmd);
    uint8_t ComputeChecksum(const QiPacket &pkt);
    uint8_t ComputeParity(DeviceType senderType, const uint8_t byte);
    // 编码相关函数
    void AddPreamble(uint8_t *buffer, size_t &bitIdx);
    void EncodeByteToBits(const uint8_t &byte, uint8_t *buffer, size_t &bitIdx);
    void EncodePacketToBits(const QiPacket &pkt, uint8_t *buffer); // 将数据包编码为比特流
    // 解码相关函数
    void ResetReceiver(void);
    bool CheckPreamble(void);
    bool DecodeBit(uint8_t &bit);
    bool DecodeByte(uint8_t &byte);
    bool DecodeData(uint8_t &cmd, uint8_t *data);
    // 发送相关变量
    HardwareSendFunction hardware_send_function_ = nullptr; // 硬件发送函数指针
    osSemaphoreId tx_sem_handle_;                           // DMA发送完成信号量
    uint8_t tx_buffer[11 * 29 + 25];                        // 发送缓冲区
    // 接收相关变量
    osMessageQId rx_msg_handler_; // 接收消息队列
    bool has_half_one_ = false;   // 是否已经接收到半个1
    uint8_t bit_count_ = 0, preamble_count_ = 0, current_byte_ = 0, parity_bit_ = 0;
    uint8_t byte_count_ = 0;
    QiPacket rx_packet_;

    DeviceType type_;
};

#endif // __QI_PROTOCOL_HPP__