#include <stdio.h>
#include <string.h>

#include <wup/wup.h>


u32 GetCP15Reg(int cn, int cm, int cp);





// FPGA debug output

int _strlen(char* str)
{
    int ret = 0;
    while (str[ret]) ret++;
    return ret;
}

void send_binary(u8* data, int len)
{
#ifdef FPGA_LOG
    len &= 0x3FFF;
    u16 header = len;

    u8 buf[3];
    buf[0] = 0xF2;
    buf[1] = header >> 8;
    buf[2] = header & 0xFF;

    SPI_Start(SPI_DEVICE_FLASH, 0x8400);//SPI_SPEED_FLASH);
    SPI_Write(buf, 3);
    SPI_Write(data, len);
    SPI_Finish();
#endif
}

void send_string(char* str)
{
#ifdef FPGA_LOG
    int len = _strlen(str);

    len &= 0x3FFF;
    u16 header = 0x8000 | len;

    u8 buf[3];
    buf[0] = 0xF2;
    buf[1] = header >> 8;
    buf[2] = header & 0xFF;

    SPI_Start(SPI_DEVICE_FLASH, SPI_SPEED_FLASH);
    SPI_Write(buf, 3);
    SPI_Write((u8*)str, len);
    SPI_Finish();
#endif
}

void send_hex(u32 hex)
{
#ifdef FPGA_LOG
    /*int len = _strlen(str);

    len &= 0x3FFF;
    u16 header = 0x8000 | len;

    u8 buf[3];
    buf[0] = 0xF2;
    buf[1] = header >> 8;
    buf[2] = header & 0xFF;

    SPI_Start(SPI_DEVICE_FLASH, SPI_SPEED_FLASH);
    SPI_Write(buf, 3);
    SPI_Write((u8*)str, len);
    SPI_Finish();*/
#endif
}

void dump_data(u8* data, int len)
{
#ifdef FPGA_LOG
    len &= 0x3FFF;
    u16 header = 0x4000 | len;

    u8 buf[3];
    buf[0] = 0xF2;
    buf[1] = header >> 8;
    buf[2] = header & 0xFF;

    SPI_Start(SPI_DEVICE_FLASH, 0x8400);//SPI_SPEED_FLASH);
    SPI_Write(buf, 3);
    SPI_Write(data, len);
    SPI_Finish();
#endif
}



void main()
{
    send_string("melonboot\n");
    send_binary((u8*)0x3FFFFC, 4);

    u32 loadaddr = 0;

    if (UIC_IsGood())
    {
        // TODO: check UIC EEPROM data for alternate loading
        // data required:
        // * direct boot flag (always boot to menu or require key combo)
        // * temporary boot ID?
        // * default boot ID
        // * CRC

        u8 state = UIC_CurState();
        if (state == 7) // service mode
            loadaddr = 0x1C00000;
    }

    if (!loadaddr)
    {
        // load partition data
        u8 partsel = 0;
        Flash_Read(0xF000, &partsel, 1);
        if (partsel == 1)
            loadaddr = 0x500000; // TODO: for 24m clock, it is 0x480000
        else
            loadaddr = 0x100000;
    }

    u32 codeaddr, codelen;
    if (!Flash_GetCodeAddr(loadaddr, &codeaddr, &codelen))
    {
        // TODO handle this gracefully
        for(;;);
    }

    u8 excepvectors[64];
    Flash_Read(codeaddr, excepvectors, 64);

    // load bulk of code blob
    Flash_Read(codeaddr + 64, (u8*)64, codelen - 64);
    /*codelen -= 64;
    for (u32 i = 0; i < codelen; i += 0x100000)
    {
        u32 chunklen = 0x100000;
        if ((i + chunklen) > codelen)
            chunklen = codelen - i;

        Flash_Read(codeaddr + 64 + i, (u8*)(64 + i), chunklen);
    }*/

    // load exception vectors
    DisableIRQ();
    for (int i = 0; i < 64; i++)
        *(u8*)i = excepvectors[i];

    // trigger soft reset
    *(vu32*)0xF0000058 = 0xFFFFFFFB;
    *(vu32*)0xF0000058 = 0;
    *(vu32*)0xF0000004 = 0;
    *(vu32*)0xF0000004 = 1;

	for(;;);
}



