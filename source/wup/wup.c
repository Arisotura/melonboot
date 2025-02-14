#include <wup/wup.h>


static void fill32(u32 a, u32 b, u32 v)
{
    u32 i;
    for (i = a; i <= b; i+=4) *(vu32*)i = v;
}

static void fill16(u32 a, u32 b, u32 v)
{
    u32 i;
    for (i = a; i <= b; i+=4) *(vu16*)i = v;
}


typedef struct
{
    fnIRQHandler handler;
    void* userdata;
    int priority;

} sIRQHandlerEntry;

sIRQHandlerEntry IRQTable[64];


volatile u8 Timer0Flag;
void Timer0IRQ(int irq, void* userdata);


void WUP_Init()
{
    // IRQ init

    for (int i = 0; i < 64; i++)
    {
        IRQTable[i].handler = NULL;
        IRQTable[i].userdata = NULL;
        IRQTable[i].priority = 0;
    }

    REG_IRQ_ACK = 0xF;
    *(vu8*)0xF00019D8 = 1;

    for (int i = 0x20; i < 0x28; i++) REG_IRQ_TRIGGER(i) = 1;
    for (int i = 0x00; i < 0x1F; i++) REG_IRQ_TRIGGER(i) = 1;

    for (int i = 0x20; i < 0x28; i++) REG_IRQ_ENABLE(i) = 0x40;
    for (int i = 0x00; i < 0x1F; i++) REG_IRQ_ENABLE(i) = 0x4E;

    // hardware init

    *(vu32*)0xF0000814 = 0xE;
    *(vu32*)0xF0000800 = 0x1011;
    *(vu32*)0xF0000800 = 0x1001;
    *(vu32*)0xF0000008 &= ~0x1;
    *(vu32*)0xF0000030 = 0;

    u32 reg0C;
    u32 clktype;
    if (REG_HARDWARE_ID & (1<<16))
    {
        // 24MHz base clock
        REG_TIMER_PRESCALER = 23;
        reg0C = 0x14;
        clktype = 0;
    }
    else
    {
        // 16MHz base clock
        REG_TIMER_PRESCALER = 15;
        reg0C = 0xD;
        clktype = 1;
    }

    REG_TIMER_TARGET(0) = 0xFFFFFFFF;
    *(vu32*)0xF0000008 = ((*(vu32*)0xF0000008) & ~6) + 2;

    if (!(REG_HARDWARE_ID & (1<<19)))
    {
        REG_TIMER_CNT(0) = TIMER_ENABLE;
        while (REG_TIMER_VALUE(0) < 5013);
        REG_TIMER_CNT(0) = 0;
    }

    // PLL init

    *(vu32*)0xF000000C = reg0C;
    *(vu32*)0xF0000010 = 0x5E8000;
    *(vu32*)0xF0000014 = 5;
    *(vu32*)0xF0000018 = 0x1FE;
    *(vu32*)0xF000001C = 0x6C;
    *(vu32*)0xF0000020 = 0x1FE;
    *(vu32*)0xF0000024 = 0xBC;
    *(vu32*)0xF0000028 = 0x24;
    *(vu32*)0xF000002C = 0x3A1;

    if (clktype)
        *(vu32*)0xF0000008 |= 0x8;
    else
        *(vu32*)0xF0000008 &= ~0x8;

    *(vu32*)0xF0000008 &= ~0x2;

    REG_TIMER_CNT(0) = TIMER_ENABLE;
    while (REG_TIMER_VALUE(0) < 5000);
    REG_TIMER_CNT(0) = 0;

    *(vu32*)0xF0000030 = 3;
    *(vu32*)0xF0000008 |= 0x1;
    *(vu32*)0xF0000814 = 0x5C;
    *(vu32*)0xF0000800 = 0x1011;
    *(vu32*)0xF0000800 = 0x1001;

    EnableIRQ();

    *(vu32*)0xF0000410 = 0;
    WUP_SetIRQHandler(IRQ_TIMER0, Timer0IRQ, NULL, 0);
    Timer0Flag = 0;

    DMA_Init();
    SPI_Init();

    Flash_Init();

    *(vu32*)0x3FFFFC = 0;
    if (clktype == 1)
        UIC_Init();

    Input_Init();
}


void WUP_SetIRQHandler(u8 irq, fnIRQHandler handler, void* userdata, int prio)
{
    if (irq >= 64) return;

    int irqen = DisableIRQ();

    IRQTable[irq].handler = handler;
    IRQTable[irq].userdata = userdata;
    IRQTable[irq].priority = prio;

    if (handler)
        WUP_EnableIRQ(irq);
    else
        WUP_DisableIRQ(irq);

    RestoreIRQ(irqen);
}

void WUP_EnableIRQ(u8 irq)
{
    if (irq >= 64) return;

    int irqen = DisableIRQ();

    int prio = IRQTable[irq].priority & 0xF;
    REG_IRQ_ENABLE(irq) = (REG_IRQ_ENABLE(irq) & ~0x4F) | prio;

    RestoreIRQ(irqen);
}

void WUP_DisableIRQ(u8 irq)
{
    if (irq >= 64) return;

    int irqen = DisableIRQ();

    REG_IRQ_ENABLE(irq) |= 0x40;

    RestoreIRQ(irqen);
}
u32 irqlog[16];
void IRQHandler()
{
    irqlog[12] = *(vu32*)0xF00013FC;
    irqlog[10] = *(vu32*)0xF00013F8;
    irqlog[11] = *(vu32*)0xF00019F8;
    u32 irqnum = REG_IRQ_CURRENT;
    irqlog[13] = *(vu32*)0xF00013FC;
    irqlog[7] = *(vu32*)0xF00013F8;
    irqlog[8] = *(vu32*)0xF00019F8;
    u32 ack = REG_IRQ_ACK_KEY;
    irqlog[0] = ack;
    irqlog[1] = REG_IRQ_ACK_KEY;
    irqlog[2] = *(vu32*)0xF00013F8;
    irqlog[9] = *(vu32*)0xF00019F8;
    irqlog[3] = *(vu32*)0xF00013FC;
    irqlog[4] = *(vu32*)0xF00019F8;
    irqlog[5] = *(vu32*)0xF00019FC;
    irqlog[6] = REG_IRQ_ACK_KEY;

    if (irqnum < 64)
    {
        sIRQHandlerEntry* entry = &IRQTable[irqnum];
        if (entry->handler)
            entry->handler(irqnum, entry->userdata);
    }
    else
    {
        // ???
    }

    REG_IRQ_ACK = ack;
}


void Timer0IRQ(int irq, void* userdata)
{
    Timer0Flag = 1;
}

void WUP_DelayUS(int us)
{
    // TODO: consider halving the timer prescaler, so we can actually
    // have microsecond precision here
    if (us < 2) us = 2;

    *(vu32*)0xF0000410 = 0;
    Timer0Flag = 0;
    *(vu32*)0xF0000414 = 0;
    *(vu32*)0xF0000418 = (us >> 1) - 1;
    *(vu32*)0xF0000410 = 2;

    while (!Timer0Flag)
        WaitForIRQ();

    *(vu32*)0xF0000410 = 0;
}

void WUP_DelayMS(int ms)
{
    *(vu32*)0xF0000410 = 0;
    Timer0Flag = 0;
    *(vu32*)0xF0000414 = 0;
    *(vu32*)0xF0000418 = (ms * 500) - 1;
    *(vu32*)0xF0000410 = 2;

    while (!Timer0Flag)
        WaitForIRQ();

    *(vu32*)0xF0000410 = 0;
}
