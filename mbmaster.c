#include "mbmaster.h"

#define highByte(dat) ((uint8_t)((dat) >> 8))
#define lowByte(dat) ((uint8_t)((dat) & 0xFF))
#define lowWord(ww)  ((uint16_t)((ww) & 0xFFFF))
#define highWord(ww) ((uint16_t)((ww) >> 16))
#define word(hdat,ldat) (((uint16_t)((hdat) << 8)) | (ldat))



typedef enum
{
    MBReadCoils = 0x01,
    MBReadDiscreteInputs = 0x02,
    MBWriteSingleCoil = 0x05,
    MBWriteMultipleCoils = 0x0F,
    MBReadHoldingRegisters = 0x03,
    MBReadInputRegisters = 0x04,
    MBWriteSingleRegister = 0x06,
    MBWriteMultipleRegisters = 0x10,
    MBMaskWriteRegister = 0x16,
    MBReadWriteMultipleRegisters = 0x17,

} FunctionCodes;

static uint16_t crc16_update(uint16_t crc, uint8_t a);
static MBMasterStatus MBMasterTransaction(MBMaster* cb, FunctionCodes MBFunction);


MBMasterStatus MBMaster_init(MBMaster* cb,
                             uint16_t u16MBResponseTimeout,
                             uint8_t (*available)(void),
                             uint8_t (*readByte)(void),
                             void (*flush)(void),
                             uint32_t (*millis)(void),
                             void (*writeByte)(uint8_t),
                             void (*idle)(void),
                             void (*preTransmission)(void),
                             void (*postTransmission)(void)
                            )
{
    MBMasterStatus MBStatus = MBSuccess;

    if (available && readByte && writeByte && millis && flush)
    {
        cb->flush = flush;
        cb->available = available;
        cb->readByte = readByte;
        cb->writeByte = writeByte;
        cb->millis = millis;
    }
    else
    {
        MBStatus = MBIllegalDataValue;
    }

    cb->u16MBResponseTimeout = u16MBResponseTimeout;
    cb->idle = idle;
    cb->preTransmission = preTransmission;
    cb->postTransmission = postTransmission;

    return MBStatus;
}

uint16_t ModbusMaster_getResponseBuffer(MBMaster* cb, uint8_t u8Index)
{
    if (u8Index < MaxBufferSize)
    {
        return cb->u16ResponseBuffer[u8Index];
    }
    else
    {
        return 0xFFFF;
    }
}

void ModbusMaster_clearResponseBuffer(MBMaster* cb)
{
    uint8_t i;

    for (i = 0; i < MaxBufferSize; i++)
    {
        cb->u16ResponseBuffer[i] = 0;
    }
}

MBMasterStatus ModbusMaster_setTransmitBuffer(MBMaster* cb,
        uint8_t u8Index,
        uint16_t u16Value)
{
    if (u8Index < MaxBufferSize)
    {
        cb->u16TransmitBuffer[u8Index] = u16Value;
        return MBSuccess;
    }
    else
    {
        return MBIllegalDataAddress;
    }
}

void ModbusMaster_clearTransmitBuffer(MBMaster* cb)
{
    uint8_t i;

    for (i = 0; i < MaxBufferSize; i++)
    {
        cb->u16TransmitBuffer[i] = 0;
    }
}

MBMasterStatus MBMaster_readCoils(MBMaster* cb,
                                  uint8_t u8MBSlave,
                                  uint16_t u16ReadAddress,
                                  uint16_t u16BitQty)
{
    cb->u8MBSlave = u8MBSlave;
    cb->u16ReadAddress = u16ReadAddress;
    cb->u16ReadQty = u16BitQty;
    return MBMasterTransaction(cb, MBReadCoils);
}

MBMasterStatus MBMaster_readDiscreteInputs(MBMaster* cb,
        uint8_t u8MBSlave,
        uint16_t u16ReadAddress,
        uint16_t u16BitQty)
{
    cb->u8MBSlave = u8MBSlave;
    cb->u16ReadAddress = u16ReadAddress;
    cb->u16ReadQty = u16BitQty;
    return MBMasterTransaction(cb, MBReadDiscreteInputs);
}

MBMasterStatus MBMaster_readHoldingRegisters(MBMaster* cb,
        uint8_t u8MBSlave,
        uint16_t u16ReadAddress,
        uint16_t u16ReadQty)
{
    cb->u8MBSlave = u8MBSlave;
    cb->u16ReadAddress = u16ReadAddress;
    cb->u16ReadQty = u16ReadQty;
    return MBMasterTransaction(cb, MBReadHoldingRegisters);
}

MBMasterStatus MBMaster_readInputRegisters(MBMaster* cb,
        uint8_t u8MBSlave,
        uint16_t u16ReadAddress,
        uint8_t u16ReadQty)
{
    cb->u8MBSlave = u8MBSlave;
    cb->u16ReadAddress = u16ReadAddress;
    cb->u16ReadQty = u16ReadQty;
    return MBMasterTransaction(cb, MBReadInputRegisters);
}

MBMasterStatus MBMaster_writeSingleCoil(MBMaster* cb,
                                        uint8_t u8MBSlave,
                                        uint16_t u16WriteAddress,
                                        uint8_t u8State)
{
    cb->u8MBSlave = u8MBSlave;
    cb->u16WriteAddress = u16WriteAddress;
    cb->u16WriteQty = (u8State ? 0xFF00 : 0x0000);
    return MBMasterTransaction(cb, MBWriteSingleCoil);
}

MBMasterStatus MBMaster_writeSingleRegister(MBMaster* cb,
        uint8_t u8MBSlave,
        uint16_t u16WriteAddress,
        uint16_t u16WriteValue)
{
    cb->u8MBSlave = u8MBSlave;
    cb->u16WriteAddress = u16WriteAddress;
    cb->u16WriteQty = 0;
    cb->u16TransmitBuffer[0] = u16WriteValue;
    return MBMasterTransaction(cb, MBWriteSingleRegister);
}

MBMasterStatus MBMaster_writeMultipleCoils(MBMaster* cb,
        uint8_t u8MBSlave,
        uint16_t u16WriteAddress,
        uint16_t u16BitQty)
{
    cb->u8MBSlave = u8MBSlave;
    cb->u16WriteAddress = u16WriteAddress;
    cb->u16WriteQty = u16BitQty;
    return MBMasterTransaction(cb, MBWriteMultipleCoils);
}


MBMasterStatus MBMaster_writeMultipleRegisters(MBMaster* cb,
        uint8_t u8MBSlave,
        uint16_t u16WriteAddress,
        uint16_t u16WriteQty)
{
    cb->u8MBSlave = u8MBSlave;
    cb->u16WriteAddress = u16WriteAddress;
    cb->u16WriteQty = u16WriteQty;
    return MBMasterTransaction(cb, MBWriteMultipleRegisters);
}


MBMasterStatus MBMaster_maskWriteRegister(MBMaster* cb,
        uint8_t u8MBSlave,
        uint16_t u16WriteAddress,
        uint16_t u16AndMask,
        uint16_t u16OrMask)
{
    cb->u8MBSlave = u8MBSlave;
    cb->u16WriteAddress = u16WriteAddress;
    cb->u16TransmitBuffer[0] = u16AndMask;
    cb->u16TransmitBuffer[1] = u16OrMask;
    return MBMasterTransaction(cb, MBMaskWriteRegister);
}

MBMasterStatus MBMaster_readWriteMultipleRegisters(MBMaster* cb,
        uint8_t u8MBSlave,
        uint16_t u16ReadAddress,
        uint16_t u16ReadQty,
        uint16_t u16WriteAddress,
        uint16_t u16WriteQty)
{
    cb->u8MBSlave = u8MBSlave;
    cb->u16ReadAddress = u16ReadAddress;
    cb->u16ReadQty = u16ReadQty;
    cb->u16WriteAddress = u16WriteAddress;
    cb->u16WriteQty = u16WriteQty;
    return MBMasterTransaction(cb, MBReadWriteMultipleRegisters);
}


static MBMasterStatus MBMasterTransaction(MBMaster* cb, FunctionCodes MBFunction)
{
    uint8_t u8ModbusADU[256];
    uint8_t u8ModbusADUSize = 0;
    uint8_t i, u8Qty;
    uint16_t u16CRC;
    uint32_t u32StartTime;
    uint8_t u8BytesLeft = 8;
    MBMasterStatus MBStatus = MBSuccess;

    // assemble Modbus Request Application Data Unit
    u8ModbusADU[u8ModbusADUSize++] = cb->u8MBSlave;
    u8ModbusADU[u8ModbusADUSize++] = (uint8_t)MBFunction;

    switch (MBFunction)
    {
        case MBReadCoils:
        case MBReadDiscreteInputs:
        case MBReadInputRegisters:
        case MBReadHoldingRegisters:
        case MBReadWriteMultipleRegisters:
            u8ModbusADU[u8ModbusADUSize++] = highByte(cb->u16ReadAddress);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(cb->u16ReadAddress);
            u8ModbusADU[u8ModbusADUSize++] = highByte(cb->u16ReadQty);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(cb->u16ReadQty);
            break;
    }

    switch (MBFunction)
    {
        case MBWriteSingleCoil:
        case MBMaskWriteRegister:
        case MBWriteMultipleCoils:
        case MBWriteSingleRegister:
        case MBWriteMultipleRegisters:
        case MBReadWriteMultipleRegisters:
            u8ModbusADU[u8ModbusADUSize++] = highByte(cb->u16WriteAddress);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(cb->u16WriteAddress);
            break;
    }

    switch (MBFunction)
    {
        case MBWriteSingleCoil:
            u8ModbusADU[u8ModbusADUSize++] = highByte(cb->u16WriteQty);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(cb->u16WriteQty);
            break;

        case MBWriteSingleRegister:
            u8ModbusADU[u8ModbusADUSize++] = highByte(cb->u16TransmitBuffer[0]);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(cb->u16TransmitBuffer[0]);
            break;

        case MBWriteMultipleCoils:
            u8ModbusADU[u8ModbusADUSize++] = highByte(cb->u16WriteQty);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(cb->u16WriteQty);
            u8Qty = (cb->u16WriteQty % 8) ? ((cb->u16WriteQty >> 3) + 1) : (cb->u16WriteQty >> 3);
            u8ModbusADU[u8ModbusADUSize++] = u8Qty;

            for (i = 0; i < u8Qty; i++)
            {
                switch (i % 2)
                {
                    case 0: // i is even
                        u8ModbusADU[u8ModbusADUSize++] = lowByte(cb->u16TransmitBuffer[i >> 1]);
                        break;

                    case 1: // i is odd
                        u8ModbusADU[u8ModbusADUSize++] = highByte(cb->u16TransmitBuffer[i >> 1]);
                        break;
                }
            }

            break;

        case MBWriteMultipleRegisters:
        case MBReadWriteMultipleRegisters:
            u8ModbusADU[u8ModbusADUSize++] = highByte(cb->u16WriteQty);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(cb->u16WriteQty);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(cb->u16WriteQty << 1);

            for (i = 0; i < lowByte(cb->u16WriteQty); i++)
            {
                u8ModbusADU[u8ModbusADUSize++] = highByte(cb->u16TransmitBuffer[i]);
                u8ModbusADU[u8ModbusADUSize++] = lowByte(cb->u16TransmitBuffer[i]);
            }

            break;

        case MBMaskWriteRegister:
            u8ModbusADU[u8ModbusADUSize++] = highByte(cb->u16TransmitBuffer[0]);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(cb->u16TransmitBuffer[0]);
            u8ModbusADU[u8ModbusADUSize++] = highByte(cb->u16TransmitBuffer[1]);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(cb->u16TransmitBuffer[1]);
            break;
    }

    // append CRC
    u16CRC = 0xFFFF;

    for (i = 0; i < u8ModbusADUSize; i++)
    {
        u16CRC = crc16_update(u16CRC, u8ModbusADU[i]);
    }

    u8ModbusADU[u8ModbusADUSize++] = lowByte(u16CRC);
    u8ModbusADU[u8ModbusADUSize++] = highByte(u16CRC);
    u8ModbusADU[u8ModbusADUSize] = 0;

    // flush receive buffer before transmitting request
    //while (_serial->read() != -1);

    // transmit request
    if (cb->preTransmission)
    {
        cb->preTransmission();
    }

    for (i = 0; i < u8ModbusADUSize; i++)
    {
        cb->writeByte(u8ModbusADU[i]);
    }

    u8ModbusADUSize = 0;
    cb->flush();    // flush transmit buffer

    if (cb->postTransmission)
    {
        cb->postTransmission();
    }

    // loop until we run out of time or bytes, or an error occurs
    u32StartTime = cb->millis();

    while (u8BytesLeft && !MBStatus)
    {
        if (cb->available())
        {
            u8ModbusADU[u8ModbusADUSize++] = cb->readByte();
            u8BytesLeft--;
        }
        else
        {
            if (cb->idle)
            {
                cb->idle();
            }
        }

        // evaluate slave ID, function code once enough bytes have been read
        if (u8ModbusADUSize == 5)
        {
            // verify response is for correct Modbus slave
            if (u8ModbusADU[0] != cb->u8MBSlave)
            {
                MBStatus = MBInvalidSlaveID;
                break;
            }

            // verify response is for correct Modbus function code (mask exception bit 7)
            if ((u8ModbusADU[1] & 0x7F) != (uint8_t)MBFunction)
            {
                MBStatus = MBInvalidFunction;
                break;
            }

            // check whether Modbus exception occurred; return Modbus Exception Code
            if (u8ModbusADU[1] & 0x80)
            {
                MBStatus = u8ModbusADU[2];
                break;
            }

            // evaluate returned Modbus function code
            switch (u8ModbusADU[1])
            {
                case MBReadCoils:
                case MBReadDiscreteInputs:
                case MBReadInputRegisters:
                case MBReadHoldingRegisters:
                case MBReadWriteMultipleRegisters:
                    u8BytesLeft = u8ModbusADU[2];
                    break;

                case MBWriteSingleCoil:
                case MBWriteMultipleCoils:
                case MBWriteSingleRegister:
                case MBWriteMultipleRegisters:
                    u8BytesLeft = 3;
                    break;

                case MBMaskWriteRegister:
                    u8BytesLeft = 5;
                    break;
            }
        }

        if ((cb->millis() - u32StartTime) > cb->u16MBResponseTimeout)
        {
            MBStatus = MBResponseTimedOut;
        }
    }

    // verify response is large enough to inspect further
    if (!MBStatus && u8ModbusADUSize >= 5)
    {
        // calculate CRC
        u16CRC = 0xFFFF;

        for (i = 0; i < (u8ModbusADUSize - 2); i++)
        {
            u16CRC = crc16_update(u16CRC, u8ModbusADU[i]);
        }

        // verify CRC
        if (!MBStatus && (lowByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 2] ||
                          highByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 1]))
        {
            MBStatus = MBInvalidCRC;
        }
    }

    // disassemble ADU into words
    if (!MBStatus)
    {
        // evaluate returned Modbus function code
        switch (u8ModbusADU[1])
        {
            case MBReadCoils:
            case MBReadDiscreteInputs:

                // load bytes into word; response bytes are ordered L, H, L, H, ...
                for (i = 0; i < (u8ModbusADU[2] >> 1); i++)
                {
                    if (i < MaxBufferSize)
                    {
                        cb->u16ResponseBuffer[i] = word(u8ModbusADU[2 * i + 4], u8ModbusADU[2 * i + 3]);
                    }

                    cb->u8ResponseBufferLength = i;
                }

                // in the event of an odd number of bytes, load last byte into zero-padded word
                if (u8ModbusADU[2] % 2)
                {
                    if (i < MaxBufferSize)
                    {
                        cb->u16ResponseBuffer[i] = word(0, u8ModbusADU[2 * i + 3]);
                    }

                    cb->u8ResponseBufferLength = i + 1;
                }

                break;

            case MBReadInputRegisters:
            case MBReadHoldingRegisters:
            case MBReadWriteMultipleRegisters:

                // load bytes into word; response bytes are ordered H, L, H, L, ...
                for (i = 0; i < (u8ModbusADU[2] >> 1); i++)
                {
                    if (i < MaxBufferSize)
                    {
                        cb->u16ResponseBuffer[i] = word(u8ModbusADU[2 * i + 3], u8ModbusADU[2 * i + 4]);
                    }

                    cb->u8ResponseBufferLength = i;
                }

                break;
        }
    }

    return MBStatus;
}

static uint16_t crc16_update(uint16_t crc, uint8_t a)
{
    int i;

    crc ^= a;

    for (i = 0; i < 8; ++i)
    {
        if (crc & 1)
        {
            crc = (crc >> 1) ^ 0xA001;
        }
        else
        {
            crc = (crc >> 1);
        }
    }

    return crc;
}
