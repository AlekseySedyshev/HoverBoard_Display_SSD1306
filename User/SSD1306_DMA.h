#ifndef SSD1306_h
#define SSD1306_h

#include "main.h"

void CMD(unsigned char byte);
void LCD_Init(void);
void BUF_Gotoxy(u8 x, u8 y);
void LCD_mode(unsigned char mode);
void BUF_Char(unsigned char flash_o, unsigned char mode); // Print Symbol
void BUF_PrintStr(char *s, unsigned char mode);           // Print String
void BUF_Clear(void);                                     // Clear LCD
void BUF_ClearStr(unsigned char y, unsigned char qnt);
void BUF_PrintDec(long val, unsigned char mode);            // Print Dec
void BUF_PrintHex(long val, unsigned char mode);            // Print Hex
void BUF_PrintBin(unsigned char value, unsigned char mode); // Print Bin
void LCD_update(void);
void BUF_PrintDig12(long val, uint8_t dp);
void BUF_12Char(unsigned char symb);
void BUF_PrintDig24(long val, uint8_t dp);
void BUF_24Char(unsigned char symb);
void BUF_cut_tail12(void);
void BUF_cut_tail24(void);
#endif
