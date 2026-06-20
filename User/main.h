// Project wire cut started 01/10/2024
//  Copyright (c) 2022, Aleksey Sedyshev
//  https://github.com/AlekseySedyshev
#ifndef __MAIN_H
#define __MAIN_H

#include <ch32v00x.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define I2C_READ 1u
#define I2C_WRITE 0u
#define I2C_SPEED 100000u

#define SW_REVISION "1.02"
#define SW_DATE "16/06/2025"

#define RXBUF_SIZE 150u

#define BUTTON() (GPIOC->INDR & GPIO_INDR_IDR4)
#define LCD_RESCAN 125u
#define SHOW_PERIOD 6u

#define Pi 314

#define EEP_ADDR 0x08003FC0 // 0x0800FFC0
typedef enum
{
    I2C_ACK_ERR,
    I2C_ACK_OK,
    SENS_BASE = 0
} I2C_stat;

typedef struct
{
    int16_t in1;
    int16_t in2;
    int16_t cmdL;
    int16_t cmdR;
    uint16_t BatADC;  // напряжение в попунаях АЦП
    uint16_t BatV;    // напряжеение на батарее
    uint16_t TempADC; // Температура АЦП
    uint16_t Temp;    // Teмпература реальная
    int16_t velL;
    int16_t velR;
    int16_t curL; // Ток левого двигателя
    int16_t curR; // Ток правого двигателя
} usart_data;

void DelayMs(uint16_t Delay_time);
void I2C_write(uint8_t byte_send);
void I2C_Start(uint8_t i2c_addr);
void I2C_Stop(void);

void lcd_routine(void);

#endif