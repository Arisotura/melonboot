#include <wup/wup.h>


static u8 UICGood = 0;

// This table defines which UIC state transitions are possible.
// The UIC firmware uses a similar table.
const u8 StateTransitions[15][15] = {
    {0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1},
    {1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1},
    {1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1},
    {1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1},
    {1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1},
    {1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1},
    {0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    {1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0}
};

static u8 UICState = 0;


u16 CRC16(u8* data, u32 len)
{
    u16 crc = 0xFFFF;

    for (u32 i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (u32 j  = 0; j < 8; j++)
        {
            if (crc & 0x1)
                crc = (crc >> 1) ^ 0x8408;
            else
                crc >>= 1;
        }
    }

    return crc;
}

int CheckCRC16(u8* data, u32 len)
{
    u16 crc_calc = CRC16(data, len);
    u16 crc_data = data[len] | (data[len+1] << 8);
    return crc_calc == crc_data;
}


void UIC_Init()
{
    WUP_DelayMS(300);
    u8 sync = UIC_Sync();
    if (sync != 0x2F && sync != 0x3F)
        return;

    UICState = UIC_GetState();
    *(u32*)0x3FFFFC = sync;
    UICGood = 1;
}

u8 UIC_IsGood()
{
    return UICGood;
}

u8 UIC_CurState() // TODO better naming
{
    return UICState;
}


u8 UIC_Sync()
{
    u8 buf = 0x7F;
    u8 ret = 0;

    SPI_Start(SPI_DEVICE_UIC, SPI_SPEED_UIC);
    WUP_DelayMS(1);
    SPI_Write(&buf, 1);
    WUP_DelayMS(1);
    do
    {
        REG_SPI_CNT |= 0x40;
        WUP_DelayUS(60);
        REG_SPI_CNT &= ~0x40;
        ret = 0;
        SPI_Read(&ret, 1);
    }
    while (ret == 0);

    SPI_Finish();

    return ret;
}


void UIC_SendCommand(u8 cmd, u8* in_data, int in_len, u8* out_data, int out_len)
{
    SPI_Start(SPI_DEVICE_UIC, 0x835C);

    SPI_Write(&cmd, 1);
    if (in_data)
    {
        WUP_DelayUS(60);
        SPI_Write(in_data, in_len);
    }

    if (out_data)
    {
        // TODO: figure out what REG_SPI_CNT bit6 does
        REG_SPI_CNT |= 0x40;
        WUP_DelayUS(60);
        REG_SPI_CNT &= ~0x40;
        SPI_Read(out_data, out_len);
    }

    SPI_Finish();
}


#if 0
// FIRMWARE UPLOAD CODE. DANGER
// this is untested, and a firmware upload gone wrong can and will brick the UIC.

u8 UIC_WaitForReply(u8 unwanted)
{
    u8 ret = unwanted;
    for (int i = 0; i < 2000; i++)
    {
        SPI_Read(&ret, 1);
        REG_SPI_CNT |= 0x40;
        REG_SPI_CNT &= ~0x40;
        if (ret != unwanted) break;
    }
    return ret;
}

int UIC_UploadFirmware(u8* data, int len)
{
    u8 buf[129];

    if (UICGood)
    {
        buf[0] = 0;

        UIC_SendCommand(0x09, buf, 1, NULL, 0);
        WUP_DelayMS(1000);

        /*buf[0] = 0x7F;

        SPI_Start(SPI_DEVICE_UIC, SPI_SPEED_UIC);
        SPI_Write(buf, 1);
        if (UIC_WaitForReply(0x00) != 0x79) return 0;
        SPI_Finish();*/
    }

    buf[0] = 0x00;
    buf[1] = 0xFF;

    SPI_Start(SPI_DEVICE_UIC, SPI_SPEED_UIC);
    SPI_Write(buf, 2);
    if (UIC_WaitForReply(0x00) != 0x79) return -1;
    SPI_Finish();
    WUP_DelayUS(60);

    SPI_Start(SPI_DEVICE_UIC, SPI_SPEED_UIC);
    REG_SPI_CNT |= 0x40;
    WUP_DelayUS(60);
    REG_SPI_CNT &= ~0x40;
    SPI_Read(buf, 7);
    SPI_Finish();
    WUP_DelayUS(60);

    u32 addr = 0x9000;
    for (int i = 0; i < len; )
    {
        int chunk = 128;
        if ((i + chunk) > len)
            chunk = len - i;

        buf[0] = 0x31;
        buf[1] = 0xCE;

        SPI_Start(SPI_DEVICE_UIC, SPI_SPEED_UIC);
        SPI_Write(buf, 2);
        if (UIC_WaitForReply(0x00) != 0x79) return -2;
        SPI_Finish();
        WUP_DelayUS(60);

        buf[0] = (addr >> 24) & 0xFF;
        buf[1] = (addr >> 16) & 0xFF;
        buf[2] = (addr >> 8) & 0xFF;
        buf[3] = addr & 0xFF;
        buf[4] = buf[0] ^ buf[1] ^ buf[2] ^ buf[3];

        SPI_Start(SPI_DEVICE_UIC, SPI_SPEED_UIC);
        SPI_Write(buf, 5);
        if (UIC_WaitForReply(0x00) != 0x79) return -3;
        SPI_Finish();
        WUP_DelayUS(60);

        buf[0] = chunk;

        SPI_Start(SPI_DEVICE_UIC, SPI_SPEED_UIC);
        SPI_Write(buf, 1);
        if (UIC_WaitForReply(0x00) != 0x79) return -4;
        SPI_Finish();
        WUP_DelayUS(60);

        buf[chunk] = chunk;
        for (int j = 0; j < chunk; j++)
        {
            buf[j] = data[i + j];
            buf[chunk] ^= data[i + j];
        }

        SPI_Start(SPI_DEVICE_UIC, SPI_SPEED_UIC);
        SPI_Write(buf, chunk+1);
        if (UIC_WaitForReply(0x00) != 0x79) return -5;
        SPI_Finish();
        WUP_DelayUS(60);

        i += chunk;
        addr += chunk;
    }

    buf[0] = 0x21;
    buf[1] = 0xDE;

    SPI_Start(SPI_DEVICE_UIC, SPI_SPEED_UIC);
    SPI_Write(buf, 2);
    if (UIC_WaitForReply(0x79) != 0x51) return -6;
    SPI_Finish();

    WUP_DelayMS(100);
    return 0;
}
#endif


u32 UIC_GetFirmwareVersion()
{
    if (!UICGood) return 0;

    u8 buf[4];
    UIC_SendCommand(0x0B, NULL, 0, buf, 4);
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}


u8 UIC_GetState()
{
    if (!UICGood) return 0xFF;

    u8 ret;
    UIC_SendCommand(0x05, NULL, 0, &ret, 1);
    return ret;
}

int UIC_SetState(u8 state)
{
    if (!UICGood) return 0;

    if (state > 14) return 0;
    if (!StateTransitions[UICState][state]) return 0;

    UIC_SendCommand(0x01, &state, 1, NULL, 0);
    UICState = state;
    return 1;
}


void UIC_GetInputData(u8* data)
{
    if (!UICGood) return;

    UIC_SendCommand(0x07, NULL, 0, data, 128);
}


void UIC_WriteEnable()
{
    if (!UICGood) return;

    UIC_SendCommand(0x06, NULL, 0, NULL, 0);
}

void UIC_WriteDisable()
{
    if (!UICGood) return;

    UIC_SendCommand(0x04, NULL, 0, NULL, 0);
    WUP_DelayMS(140);
}

int UIC_ReadEEPROM(u32 offset, u8* data, int length)
{
    if (!UICGood) return 0;

    offset += 0x1100;
    if (offset >= 0x1800) return 0;
    if (length < 1) return 0;
    if (length >= 256) return 0;

    u8 buf[3];
    buf[0] = (offset >> 8) & 0xFF;
    buf[1] = offset & 0xFF;
    buf[2] = length;

    UIC_SendCommand(0x03, buf, 3, data, length);
    return 1;
}

int UIC_WriteEEPROM(u32 offset, u8* data, int length)
{
    if (!UICGood) return 0;

    offset += 0x1100;
    if (offset >= 0x1800) return 0;
    if (length < 1) return 0;
    if (length >= 256) return 0;

    u8 buf[3];
    buf[0] = (offset >> 8) & 0xFF;
    buf[1] = offset & 0xFF;
    buf[2] = length;

    SPI_Start(SPI_DEVICE_UIC, 0x8018);

    u8 cmd = 0x02;
    SPI_Write(&cmd, 1);
    WUP_DelayUS(60);
    SPI_Write(buf, 3);
    WUP_DelayUS(60);
    SPI_Write(data, length);

    SPI_Finish();
    return 1;
}

void UIC_SetBacklight(int enable)
{
    if (!UICGood) return;

    u8 buf = enable ? 1 : 0;
    UIC_SendCommand(0x12, &buf, 1, NULL, 0);
}
