// Project Hover_Debug_Display  19.06.2026
//  Copyright (c) 2022, Aleksey Sedyshev
//  https://github.com/AlekseySedyshev

// LCD:      SCL-PC2, SDA- PC1,
// Button:   PC4

// Prog:    PD1 - SWIO
// USART RX -  PD6

// not using: PA2

#include "main.h"
#include "SSD1306_DMA.h"

// #define IWDG_ON

extern uint32_t SystemCoreClock;

extern volatile u8 lcd_buf[1025]; // на один байт больше чтобы отправить 0x40 в самом начале.
extern volatile uint16_t pointer;

volatile uint16_t TimingDelay;
//-------------LCD-------------------

volatile I2C_stat i2c_ack_stat;

volatile uint16_t count_lcd = 0, but_press = 0, blink = 0;
volatile uint8_t word_flag = 0;
const u16 wheel_d[] = {165, 203, 215, 254, 267}; // cтандартные диаметры колес гироскутеров( 6.5", 8, 8.5, 10, 10.5)
uint16_t *EEP_REG = (uint16_t *)(EEP_ADDR);

char RX_BUF[RXBUF_SIZE] = {0};

uint16_t timer = 0;
u8 hour = 0, min = 0, sec = 0;
int16_t vel_temp = 0;
uint32_t odometr = 0;
bool od_tick = false;
uint32_t amph = 0;
u8 wheel; // диаметр колеса

//-----------Определения-----------------
volatile usart_data _data;
volatile int16_t cur = 0; // полный ток
volatile uint8_t work_page = 1, data_timer = 10;
volatile uint16_t max_cur = 0;
volatile int16_t rec_cur = 0;

volatile uint32_t speed = 0;
volatile u8 max_speed = 0;

bool lcd_out_enable = false;

//--------------------------------------------------------
void DelayMs(uint16_t Delay_time)
{
    TimingDelay = Delay_time;
    while (TimingDelay > 0x00 && BUTTON())
    {
    };
}

void init(void)
{
    { //---------Systic init---------------
        SysTick->SR &= ~(1 << 0);
        SysTick->CMP = (SystemCoreClock / 1000) - 1; // 1ms
        SysTick->CNT = 0;
        SysTick->CTLR = 0xF;

        NVIC_SetPriority(SysTicK_IRQn, 1);
        NVIC_EnableIRQ(SysTicK_IRQn);
    }
    { //---------Clock On------------------
        RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;
        RCC->APB2PCENR |= RCC_APB2Periph_USART1;
        RCC->APB1PCENR |= RCC_APB1Periph_I2C1;
        RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;
    }
    { // --------Usart Rx PD6 -

        GPIOD->CFGLR &= (~GPIO_CFGLR_CNF6); // Reset mode
        GPIOD->CFGLR |= GPIO_CFGLR_CNF6_0;  // PD6 - Output mode 50 MHZ

        USART1->BRR = SystemCoreClock / 115200;                                // 0x1A0; //((48000000 /115200))
        USART1->CTLR1 |= USART_CTLR1_RE | USART_CTLR1_UE | USART_CTLR1_IDLEIE; // USART_CTLR1_RXNEIE; //
        USART1->CTLR3 |= USART_CTLR3_DMAR;

        NVIC_EnableIRQ(USART1_IRQn); //
        NVIC_SetPriority(USART1_IRQn, 0x02);

        DMA1_Channel5->PADDR = (uint32_t)&USART1->DATAR;
        DMA1_Channel5->MADDR = (uint32_t)&RX_BUF;
        DMA1_Channel5->CNTR = RXBUF_SIZE;

        DMA1_Channel5->CFGR |= DMA_CFGR1_MINC | DMA_CFGR1_PL_1;
        DMA1_Channel5->CFGR |= DMA_CFG5_EN | DMA_CFG5_TCIE;

        NVIC_EnableIRQ(DMA1_Channel5_IRQn);
        NVIC_SetPriority(DMA1_Channel5_IRQn, 2);
    }
    { //---------I2C-----------------

        GPIOC->CFGLR |= GPIO_CFGLR_CNF1 | GPIO_CFGLR_CNF2;   // Set Alternative OD mode
        GPIOC->CFGLR |= GPIO_CFGLR_MODE1 | GPIO_CFGLR_MODE2; // Speed 50 MHz

        I2C1->CTLR2 = SystemCoreClock / 1000000; // I2C_CTLR2_FREQ_1;//I2C_CTLR2_FREQ_3; // 8 MHz SystemCoreClock/1000000;

        I2C1->CKCFGR = SystemCoreClock / (I2C_SPEED << 1); // Normal mode 100kHz
        I2C1->CTLR1 |= I2C_CTLR1_PE;
        I2C1->CTLR2 |= I2C_CTLR2_DMAEN; // I2C_CTLR2_LAST |

        DMA1_Channel6->PADDR = (uint32_t)&I2C1->DATAR;
        DMA1_Channel6->MADDR = (uint32_t)&lcd_buf;
        DMA1_Channel6->CNTR = 1025;

        DMA1_Channel6->CFGR |= DMA_CFGR1_DIR | DMA_CFGR1_MINC | DMA_CFGR1_PL_1;
        DMA1_Channel6->CFGR |= DMA_CFG6_TCIE;

        NVIC_EnableIRQ(DMA1_Channel6_IRQn);
        NVIC_SetPriority(DMA1_Channel6_IRQn, 2);
    }
    { // --------Button   PC4

        GPIOC->CFGLR &= (~GPIO_CFGLR_CNF4); // Set in Analog Input Mode
        GPIOC->CFGLR |= GPIO_CFGLR_CNF4_0;  // PC4-Input Pull_Up
    }
}
//---------------------------i2c -------------------------------------

void key_scan(void)
{
    if (!BUTTON() && but_press > 50 && but_press < 100 && work_page < 5)
    {
        work_page++;
        if (work_page > 4)
            work_page = 0;
        BUF_Clear();
        but_press = 100;
    }
    if (!BUTTON() && but_press > 3000 && work_page < 5) // вход в режим установок колеса
    {
        work_page = 5;
        BUF_Clear();
        but_press = 100;
    }
    if (!BUTTON() && but_press > 50 && but_press < 100 && work_page == 5)
    {

        if (wheel < 4)
            wheel++;
        else
            wheel = 0;
        but_press = 100;
    }

    if (!BUTTON() && but_press > 3000 && work_page == 5) // вход в режим установок колеса
    {

        FLASH_Unlock_Fast();
        FLASH_ErasePage_Fast(EEP_ADDR);
        FLASH_ProgramHalfWord(EEP_ADDR, wheel);
        FLASH_Lock_Fast();

        wheel = (u8)EEP_REG[0];

        work_page = 1;
        BUF_Clear();
        but_press = 100;
    }
}
void lcd_routine(void)
{
    if (lcd_out_enable == 1)
    {
        if (work_page == 0) // Батарея, ток, скорость
        {
            BUF_Gotoxy(0, 0);
            BUF_PrintDig24(_data.BatV, 2);
            BUF_24Char(15); // буква V
            BUF_cut_tail24();

            BUF_Gotoxy(0, 3);
            BUF_PrintDig24(cur, 1);
            BUF_24Char(14); // буква A
            BUF_cut_tail24();

            BUF_Gotoxy(0, 6);
            BUF_cut_tail12();
            BUF_Gotoxy(0, 6);
            BUF_PrintDig12(odometr, 3);
            BUF_PrintStr(" m", 0);
            BUF_cut_tail12();
            //-------------------------------------
            BUF_Gotoxy(15 * 6, 4);
            BUF_PrintStr("Speed ", 0);
            BUF_Gotoxy(15 * 6, 5);
            BUF_PrintDig24(speed, 0);
            BUF_cut_tail24();
            BUF_Gotoxy(17 * 6, 3);
            if (wheel == 0)
            {
                BUF_PrintStr("6.5  ", 0);
            }

            if (wheel == 1)
            {
                BUF_PrintStr("8.0  ", 0);
            }
            if (wheel == 2)
            {
                BUF_PrintStr("8.5  ", 0);
            }
            if (wheel == 3)
            {
                BUF_PrintStr("10.0", 0);
            }
            if (wheel == 4)
            {
                BUF_PrintStr("10.5", 0);
            }
        }
        if (work_page == 1) // время, максимальная скорость, максимальный ток
        {
            hour = timer / 3600;
            min = (timer % 3600) / 60;
            sec = (timer % 3600) % 60;
            BUF_Gotoxy(0, 0);
            if (hour < 10)
            {
                BUF_24Char(0);
            }
            BUF_PrintDig24(hour, 0);

            BUF_24Char(11);
            if (min < 10)
            {
                BUF_24Char(0);
            }
            BUF_PrintDig24(min, 0);

            BUF_24Char(11);
            if (sec < 10)
            {
                BUF_24Char(0);
            }
            BUF_PrintDig24(sec, 0);

            //-----------------------------------------------------------

            BUF_Gotoxy(0, 4);
            BUF_PrintStr("Max Cur. ", 0);
            BUF_Gotoxy(0, 5);
            BUF_PrintDig24(max_cur, 1);

            BUF_cut_tail24();
            BUF_Gotoxy(12 * 6, 4);
            BUF_PrintStr("Max Speed", 0);
            BUF_Gotoxy(14 * 6, 5);
            BUF_PrintDig24(max_speed, 0);
        }
        if (work_page == 2) // VelL, VelR, Максимальный ток рекуперации
        {
            BUF_Gotoxy(0, 0);
            BUF_cut_tail24();
            BUF_Gotoxy(0, 0);
            BUF_PrintStr("Spent: ", 0);
            BUF_Gotoxy(6 * 7, 0);
            BUF_PrintDig24(amph / 36000, 0);
            BUF_24Char(10); // точка
            BUF_PrintDig24((amph % 36000) / 3600, 0);
            BUF_Gotoxy(0, 1);
            BUF_PrintStr(" Ah", 0);
            //--------Vel--------------
            BUF_Gotoxy(0, 4);
            BUF_PrintStr("velL: ", 0);
            BUF_Gotoxy(6 * 7, 4);
            BUF_PrintDig12(_data.velL, 0);
            BUF_cut_tail12();
            BUF_Gotoxy(0, 6);
            BUF_PrintStr("velR: ", 0);
            BUF_Gotoxy(6 * 7, 6);
            BUF_PrintDig12(_data.velR, 0);
            BUF_cut_tail12();
        }
        if (work_page == 3) // In1, In2, Cmdl, cmdr
        {
            //--------in--------------
            BUF_Gotoxy(0, 0);
            BUF_PrintStr("in1: ", 0);
            BUF_Gotoxy(6 * 8, 0);
            BUF_PrintDig12(_data.in1, 0); //, 6 * 8, 0);
            BUF_cut_tail12();

            BUF_Gotoxy(0, 2);
            BUF_PrintStr("in2: ", 0);
            BUF_Gotoxy(6 * 8, 2);
            BUF_PrintDig12(_data.in2, 0); //, 6 * 8, 2);
            BUF_cut_tail12();
            //--------cmd-------------
            BUF_Gotoxy(0, 4);
            BUF_PrintStr("cmdL: ", 0);
            BUF_Gotoxy(6 * 8, 4);
            BUF_PrintDig12(_data.cmdL, 0); //, 6 * 8, 4);
            BUF_cut_tail12();
            BUF_Gotoxy(0, 6);
            BUF_PrintStr("cmdR: ", 0);
            BUF_Gotoxy(6 * 8, 6);
            BUF_PrintDig12(_data.cmdR, 0); //, 6 * 8, 6);
            BUF_cut_tail12();
        }
        if (work_page == 4) // Напряжение, напряжение АЦП, Температура, температура АЦП)
        {
            //-------Напряжение-----------
            BUF_Gotoxy(0, 0);
            BUF_PrintStr("BatV ", 0);
            BUF_Gotoxy(6 * 8, 0);
            BUF_PrintDig12(_data.BatV, 2); //, 6 * 7, 0);
            BUF_cut_tail12();

            BUF_Gotoxy(0, 2);
            BUF_PrintStr("BatADC ", 0);
            BUF_Gotoxy(6 * 8, 2);
            BUF_PrintDig12(_data.BatADC, 0); //, 6 * 8, 2);
            BUF_cut_tail12();
            //--------Температура-------------
            BUF_Gotoxy(0, 4);
            BUF_PrintStr("Temp ", 0);
            BUF_Gotoxy(6 * 8, 4);
            BUF_PrintDig12(_data.Temp, 1); //, 6 * 8, 4);
            BUF_cut_tail12();
            BUF_Gotoxy(0, 6);
            BUF_PrintStr("TempADC ", 0);
            BUF_Gotoxy(6 * 8, 6);
            BUF_PrintDig12(_data.TempADC, 0); //, 6 * 8, 6);
            BUF_cut_tail12();
        }
        if (work_page == 5)
        {
            BUF_Gotoxy(0, 2);
            BUF_PrintStr("Wheel ", 0);

            BUF_Gotoxy(16 * 3, 2);
            if (wheel == 0)
            {
                BUF_PrintDig24(65, 1);
            }

            if (wheel == 1)
            {
                BUF_PrintDig24(80, 1);
            }
            if (wheel == 2)
            {
                BUF_PrintDig24(85, 1);
            }
            if (wheel == 3)
            {
                BUF_PrintDig24(100, 1);
            }
            if (wheel == 4)
            {
                BUF_PrintDig24(105, 1);
            }
            BUF_24Char(12);
        }
        LCD_update();
        lcd_out_enable = 0;
    }
}

int32_t parse_value(const char *source_str, const char *token, uint8_t *success) // Функция для поиска метки и извлечения числа, идущего сразу за ней
{
    // Ищем указатель на начало метки
    char *ptr = strstr(source_str, token);

    if (ptr != NULL)
    {
        // Сдвигаем указатель на длину самой метки, чтобы он встал на начало числа
        ptr += strlen(token);
        // atoi автоматически остановится, как только встретит пробел после числа
        return atoi(ptr);
    }

    // Если метка не найдена, взводим флаг ошибки
    *success = 0;
    return 0;
}
void parse_telemetry_manual(const char *buffer)
{
    uint8_t is_valid = 1; // Флаг успешного парсинга всей строки

    // Извлекаем каждое значение по его уникальному "ключу"
    _data.in1 = parse_value(buffer, "in1:", &is_valid);
    _data.in2 = parse_value(buffer, "in2:", &is_valid);
    _data.cmdL = parse_value(buffer, "cmdL:", &is_valid);
    _data.cmdR = parse_value(buffer, "cmdR:", &is_valid);
    _data.BatADC = parse_value(buffer, "BatADC:", &is_valid);
    _data.BatV = parse_value(buffer, "BatV:", &is_valid);
    _data.TempADC = parse_value(buffer, "TempADC:", &is_valid);
    _data.Temp = parse_value(buffer, "Temp:", &is_valid);
    _data.velL = parse_value(buffer, "velL:", &is_valid);
    _data.velR = parse_value(buffer, "velR:", &is_valid);
    _data.curL = parse_value(buffer, "curL:", &is_valid);
    _data.curR = parse_value(buffer, "curR:", &is_valid);

    if (is_valid)
    {
        // Все метки найдены, данные успешно обновлены в переменных
    }
    else
    {
        // Какая-то из меток отсутствовала (строка побилась при передаче)
    }
}

int main(void)
{
    init();
    DelayMs(200);
    LCD_Init();
    BUF_Clear();

    BUF_Gotoxy(5 * 6, 3);
    BUF_PrintStr("Ver.: ", 0);
    BUF_PrintStr(SW_REVISION, 0);
    BUF_Gotoxy(5 * 6, 4);
    BUF_PrintStr(SW_DATE, 0);
    LCD_update();

    DelayMs(2000);
    BUF_Clear();
    wheel = (u8)EEP_REG[0];
    if (wheel > 4)
        wheel = 4;
    while (1)

    {

        key_scan();
        lcd_routine();
        if (data_timer > 3 && data_timer < 10)
        {
            BUF_Clear();
            _data.in1 = 0;
            _data.in2 = 0;
            _data.cmdL = 0;
            _data.cmdR = 0;
            _data.BatADC = 0;
            _data.BatV = 0;
            _data.TempADC = 0;
            _data.Temp = 0;
            _data.velL = 0;
            _data.velR = 0;
            _data.curL = 0;
            _data.curR = 0;
            data_timer = 10;

            work_page = 1;
        }
        if (word_flag == true)
        {
            if (data_timer > 3)
            {
                data_timer = 0;
                work_page = 0;
                BUF_Clear();
            }

            if ((RX_BUF[0] == 'i') && (RX_BUF[1] == 'n') && (RX_BUF[2] == '1') && (RX_BUF[3] == ':'))
            {
                parse_telemetry_manual(RX_BUF);
                data_timer = 0;

                if ((_data.curL > 0 && _data.curR > 0) && ((_data.curL + _data.curR) / 10) >= 10)
                {
                    cur = (_data.curL + _data.curR) / 10;
                }
                if (((_data.curL + _data.curR) / 10) < 10)
                    cur = 0;

                if (_data.velL < 0)
                {
                    vel_temp = (-1) * _data.velL;
                    if (vel_temp > _data.velR)
                        vel_temp = _data.velR;
                    speed = (uint32_t)(Pi * wheel_d[wheel] * vel_temp * 36) / (uint32_t)60000000;
                }
                if (_data.velR < 0)
                {
                    vel_temp = (-1) * _data.velR;
                    if (vel_temp > _data.velL)
                        vel_temp = _data.velL;
                    speed = (uint32_t)(Pi * wheel_d[wheel] * vel_temp * 36) / (uint32_t)60000000;
                }
                if (speed > max_speed && speed < (max_speed + 10))
                    max_speed = speed;
                if (_data.velL == 0 && _data.velR == 0)
                {
                    speed = 0;
                }
                if (cur > max_cur)
                {
                    max_cur = cur;
                }
                if (od_tick && speed > 0)
                {
                    odometr = odometr + (Pi * wheel_d[wheel] * vel_temp) / 6000000; // метры
                    amph = amph + cur;
                }
                od_tick = false;
                lcd_out_enable = true;
                count_lcd = 0;
            }
        }
        word_flag = 0;
    }
}

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    if (TimingDelay > 0)
    {
        TimingDelay--;
    }
    if (blink < 1000)
    {
        blink++;
    }
    else
    {
        blink = 0;
        if (data_timer < 10)
            data_timer++;
        timer++;
        od_tick = true;
    }
    if (count_lcd < LCD_RESCAN)
    {
        count_lcd++;
    }
    else
    {
        count_lcd = 0;
        lcd_out_enable = true;
    }

    if (!BUTTON())
    {
        if (but_press < 0xffff)
            but_press++;
    }
    else
    {
        but_press = 0;
    }
    // scan_on = true;
    SysTick->SR = 0;
}
void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART1_IRQHandler(void)
{ // USART1 IRQ HANDLER

    if (USART1->STATR & USART_STATR_IDLE) // Idle - message receidved
    {
        // dma_qnt = (RXBUF_SIZE - DMA1_Channel5->CNTR);

        DMA1_Channel5->CFGR &= (~DMA_CFG5_EN); // RX channel Off
        DMA1_Channel5->CNTR = RXBUF_SIZE;      // Upload settings for next Packet
        DMA1_Channel5->CFGR |= DMA_CFG5_EN;    // Enable DMA Rx
        USART1->DATAR;                         // Clear Flag

        word_flag = 1;
    }
}
void DMA1_Channel5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel5_IRQHandler(void)
{
    if (DMA1->INTFR & DMA_TCIF5)
    {                                          // Rx Transfer complete - Error Data > 256 byte
        DMA1_Channel5->CFGR &= (~DMA_CFG5_EN); // DISABLE DMA RX Channel
        DMA1->INTFCR |= DMA_TCIF5;             // clear TC Dma flags
        DMA1_Channel5->CNTR = RXBUF_SIZE;
        DMA1_Channel5->CFGR |= DMA_CFG5_EN;
    }
}
void DMA1_Channel6_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel6_IRQHandler(void)
{
    if (DMA1->INTFR & DMA_TCIF6)
    {
        DMA1_Channel6->CFGR &= (~DMA_CFG6_EN); // DISABLE DMA
        DMA1->INTFCR |= DMA_TCIF6;             // clear TC Dma flags
        I2C_Stop();
    }
}
void EXTI0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI0_IRQHandler(void)
{
}
