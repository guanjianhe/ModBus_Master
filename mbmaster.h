#ifndef MBMASTER_H
#define MBMASTER_H

#include <stdint.h>

#define MaxBufferSize 64


typedef enum
{
    MBSuccess                    = 0x00,
    MBIllegalFunction            = 0x01,
    MBIllegalDataAddress         = 0x02,
    MBIllegalDataValue           = 0x03,
    MBSlaveDeviceFailure         = 0x04,
    MBInvalidSlaveID             = 0xE0,
    MBInvalidFunction            = 0xE1,
    MBResponseTimedOut           = 0xE2,
    MBInvalidCRC                 = 0xE3,
} MBMasterStatus;




typedef struct
{
    uint8_t  u8MBSlave;
    uint16_t u16ReadAddress;
    uint16_t u16WriteAddress;
    uint16_t u16ReadQty;
    uint16_t u16WriteQty;
    uint16_t u16MBResponseTimeout;
    uint16_t u16ResponseBuffer[MaxBufferSize];
    uint16_t u16TransmitBuffer[MaxBufferSize];
    uint8_t  u8ResponseBufferLength;

    uint8_t (*available)(void);
    uint8_t (*readByte)(void);
    void (*writeByte)(uint8_t);
    void (*flush)(void);

    uint32_t (*millis)(void);

    // idle callback function; gets called during idle time between TX and RX
    void (*idle)(void);
    // preTransmission callback function; gets called before writing a Modbus message
    void (*preTransmission)(void);
    // postTransmission callback function; gets called after a Modbus message has been sent
    void (*postTransmission)(void);

} MBMaster;

#endif
