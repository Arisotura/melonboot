#include <stdio.h>
#include <string.h>

#include <wup/wup.h>


u32 GetCP15Reg(int cn, int cm, int cp);





// FPGA debug output

#if 0
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
    int len = strlen(str);

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
#endif



void main()
{
	for(;;);
}



