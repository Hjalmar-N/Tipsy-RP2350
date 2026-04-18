/*****************************************************************************
* | File      	:   LCD_3in5.h
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*                Used to shield the underlying layers of each master 
*                and enhance portability
*----------------
* |	This version:   V1.0
* | Date        :   2025-03-13
* | Info        :   Basic version
*
******************************************************************************/
#ifndef __LCD_3IN5_H
#define __LCD_3IN5_H

#include "DEV_Config.h"
#include <stdint.h>

#include <stdlib.h>     //itoa()
#include <stdio.h>

#define LCD_3IN5_HEIGHT 480
#define LCD_3IN5_WIDTH 320

#define HORIZONTAL 0
#define VERTICAL   1

#define LCD_3IN5_SetBacklight(Value) ; 

typedef struct{
    uint16_t WIDTH;
    uint16_t HEIGHT;
    uint8_t SCAN_DIR;
}LCD_3IN5_ATTRIBUTES;
extern LCD_3IN5_ATTRIBUTES LCD_3IN5;

/********************************************************************************
function:	
			Macro definition variable name
********************************************************************************/
void LCD_3IN5_Init(uint8_t Scan_dir);
void LCD_3IN5_Clear(uint16_t Color);
void LCD_3IN5_Display(uint16_t *Image);
void LCD_3IN5_SetWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend);
void LCD_3IN5_DisplayWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t *Image);
void LCD_3IN5_DisplayPoint(uint16_t X, uint16_t Y, uint16_t Color);
void Handler_3IN5_LCD(int signo);
#endif
