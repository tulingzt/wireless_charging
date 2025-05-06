#include "QiProtocol.hpp"
#include <string.h>

size_t QiProtocol::ComputeDataLen(const uint8_t cmd)
{
    if (cmd <= 0x1F) // 0x00-0x1F 范围
        return 1 + (cmd - 0x00) / 32;
    else if (cmd <= 0x7F) // 0x20-0x7F 范围
        return 2 + (cmd - 0x20) / 16;
    else if (cmd <= 0xDF) // 0x80-0xDF 范围
        return 8 + (cmd - 0x80) / 8;
    else // 0xE0-0xFF 范围
        return 20 + (cmd - 0xE0) / 4;
}

uint8_t QiProtocol::ComputeChecksum(const QiPacket &pkt)
{
    uint8_t crc = pkt.cmd;
    size_t len = ComputeDataLen(pkt.cmd);
    for (size_t i = 0; i < len; i++)
        crc ^= pkt.data[i];
    return crc;
}

uint8_t QiProtocol::ComputeParity(DeviceType senderType, const uint8_t byte)
{
    uint8_t count = __builtin_popcount(byte);
    if (senderType == DeviceType::Transmitter)
        return (count % 2 == 1) ? 1 : 0; // 偶校验：总1数为偶，校验位补足偶数
    else
        return (count % 2 == 0) ? 1 : 0; // 奇校验：总1数为奇，校验位补足奇数
}

void QiProtocol::AddPreamble(uint8_t *buffer, size_t &bitIdx)
{
    if (type_ == DeviceType::Receiver) // 生成11-25位前导码（此处固定为全1）
    {
        const uint8_t preambleBits = 25; // WPC Qi标准最长前导码
        for (size_t i = 0; i < preambleBits; ++i)
        {
            buffer[bitIdx++] = 1;
        }
    }
}

void QiProtocol::EncodeByteToBits(const uint8_t &byte, uint8_t *buffer, size_t &bitIdx)
{
    buffer[bitIdx++] = 0;           // 开始位0
    for (int8_t j = 7; j >= 0; --j) // 数据位（MSB优先）
        buffer[bitIdx++] = (byte >> j) & 0x01;
    buffer[bitIdx++] = ComputeParity(type_, byte); // 校验位
    buffer[bitIdx++] = 1;                          // 结束位1
}

void QiProtocol::EncodePacketToBits(const QiPacket &pkt, uint8_t *buffer)
{
    size_t dataLen = ComputeDataLen(pkt.cmd);
    size_t bitIdx = 0;
    AddPreamble(buffer, bitIdx); // 添加前导码
    EncodeByteToBits(pkt.cmd, buffer, bitIdx); // 添加命令字节
    for (size_t i = 0; i < dataLen; i++)
        EncodeByteToBits(pkt.data[i], buffer, bitIdx); // 添加数据字节
    EncodeByteToBits(pkt.checksum, buffer, bitIdx); // 添加校验和字节
}

bool QiProtocol::SendData(const uint8_t cmd, uint8_t *data, size_t dataLen)
{
    if (dataLen != ComputeDataLen(cmd))
        return false;
    if (!hardware_send_function_)
        return false;
    size_t totalBits = (dataLen + 2) * 11;
    osSemaphoreWait(tx_sem_handle_, osWaitForever);

    QiPacket pkt;
    pkt.cmd = cmd;
    memcpy(pkt.data, data, dataLen);
    pkt.checksum = ComputeChecksum(pkt);
    EncodePacketToBits(pkt, tx_buffer);
    hardware_send_function_(tx_buffer, totalBits);
    return true;
}

void QiProtocol::ResetReceiver()
{
    has_half_one_ = false;
    bit_count_ = 0;
    preamble_count_ = 0;
    byte_count_ = 0;
}

bool QiProtocol::DecodeBit(uint8_t &bit)
{
    osEvent evt = osMessageGet(rx_msg_handler_, 0);
    if (evt.status != osEventMessage)
        return false;
    uint32_t delta_us = evt.value.v;
    if (delta_us > 600) // 超时，重置接收状态
        ResetReceiver();
    else if (delta_us >= 450 && delta_us <= 550) // 0位
    {
        if (has_half_one_)
            ResetReceiver();
        else
        {
            bit = 0;
            return true;
        }
    }
    else if (delta_us >= 200 && delta_us <= 300) // 1位
    {
        if (has_half_one_)
        {
            bit = 1;
            has_half_one_ = false;
            return true;
        }
        else
            has_half_one_ = true;
    }
    return false;
}

bool QiProtocol::CheckPreamble(void)
{
    if (type_ == DeviceType::Receiver)
        return preamble_count_ == 0;
    else
        return (preamble_count_ >= 11 && preamble_count_ <= 25);
}

bool QiProtocol::DecodeByte(uint8_t &byte)
{
    uint8_t bit;
    if (!DecodeBit(bit))
        return false;
    if (bit_count_ == 0) // 开始位0
    {
        if (bit == 0)
        {
            if (CheckPreamble() || byte_count_ > 0) // 前导码检测
            {
                preamble_count_ = 0;
                bit_count_++;
            }
            preamble_count_ = 0;
        }
        else
        {
            preamble_count_++;
        }
    }
    else if (bit_count_ < 8) // 数据位（MSB优先）
    {
        current_byte_ = (current_byte_ << 1) | (bit & 0x01);
        bit_count_++;
    }
    else if (bit_count_ == 8) // 校验位
    {
        parity_bit_ = bit;
        bit_count_++;
    }
    else if (bit_count_ == 9) // 结束位1
    {
        if (bit == 1 && parity_bit_ == ComputeParity(GetOppositeType(), current_byte_)) // 校验通过
        {
            byte = current_byte_;
            bit_count_ = 0;
            current_byte_ = 0;
            return true;
        }
        else
            ResetReceiver();
    }
    else
        ResetReceiver();
    return false;
}

bool QiProtocol::DecodeData(uint8_t &cmd, uint8_t *data)
{
    uint8_t byte;
    if (!DecodeByte(byte))
        return false;
    if (byte_count_ == 0) // 命令码
    {
        rx_packet_.cmd = byte;
        byte_count_++;
    }
    else if (byte_count_ <= ComputeDataLen(rx_packet_.cmd)) // 数据
    {
        memcpy(&rx_packet_.data[byte_count_ - 1], &byte, 1);
        byte_count_++;
    }
    else if (byte_count_ == ComputeDataLen(rx_packet_.cmd) + 1) // 校验
    {
        if (byte == ComputeChecksum(rx_packet_))
        {
            cmd = rx_packet_.cmd;
            memcpy(data, rx_packet_.data, ComputeDataLen(cmd));
            return true;
        }
        else
            ResetReceiver();
    }
    else
        ResetReceiver();
    return false;
}

bool QiProtocol::ReceiveData(uint8_t &cmd, uint8_t *data)
{
    while (osMessageWaiting(rx_msg_handler_))
    {
        if (DecodeData(cmd, data))
            return true;
    }
    return false;
}