/*****************************************************************************
* | File      	:   LCD_3in5_test.c
* | Author      :   Waveshare team
* | Function    :   3inch5 LCD test demo
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2025-12-04
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "LCD_Test.h"
#include "LCD_3in5.h"
#include "DEV_Config.h"
#include "QMI8658.h"
#include "FT6336U.h"
#include "AXP2101.h"
#include "PCF85063A.h"
#include "pico/multicore.h"  
#include "hardware/watchdog.h"

uint8_t flag = 0;
uint8_t i2c_lock = 0;
#define I2C_LOCK() i2c_lock = 1
#define I2C_UNLOCK() i2c_lock = 0

void Touch_INT_callback(uint gpio, uint32_t events);
void core1_entry();

int LCD_3in5_test(void)
{
    if(DEV_Module_Init()!=0){
        return -1;
    }

    /* PWR */
    DEV_KEY_Config(SYS_OUT_PIN);
    // multicore_launch_core1(core1_entry);

    /* LCD Init */
    printf("3.5inch LCD demo...\r\n");
    LCD_3IN5_Init(VERTICAL);
    LCD_3IN5_Clear(WHITE);
    DEV_SET_PWM(100);

    /* AXP2101 */
    AXP2101_Write(XPOWERS_AXP2101_ICC_CHG_SET, 0x0B); // Charge current 500mA
    printf("XPOWERS_AXP2101_ICC_CHG_SET: 0x%02X\n", AXP2101_Read(XPOWERS_AXP2101_ICC_CHG_SET));

    uint32_t Imagesize = LCD_3IN5_HEIGHT*LCD_3IN5_WIDTH*2;
    uint16_t *BlackImage;
    if((BlackImage = (uint16_t *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        exit(0);
    }

    /* 1.Create a new image cache named IMAGE_RGB and fill it with white */
    Paint_NewImage((uint8_t *)BlackImage, LCD_3IN5.WIDTH, LCD_3IN5.HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    Paint_Clear(WHITE);
    Paint_SetRotate(ROTATE_0);
    sleep_ms(10);
    
    /* GUI */
    printf("drawing...\r\n");
    /* 2.Drawing on the image */
#if 1
    Paint_DrawPoint(2,1, BLACK, DOT_PIXEL_1X1,  DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2,6, BLACK, DOT_PIXEL_2X2,  DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2,11, BLACK, DOT_PIXEL_3X3, DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2,16, BLACK, DOT_PIXEL_4X4, DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2,21, BLACK, DOT_PIXEL_5X5, DOT_FILL_RIGHTUP);
    Paint_DrawLine( 10,  5, 40, 35, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine( 10, 35, 40,  5, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    Paint_DrawLine( 80,  20, 110, 20, CYAN, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine( 95,   5,  95, 35, CYAN, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

    Paint_DrawRectangle(10, 5, 40, 35, RED, DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawRectangle(45, 5, 75, 35, BLUE, DOT_PIXEL_2X2,DRAW_FILL_FULL);

    Paint_DrawCircle(95, 20, 15, GREEN, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(130, 20, 15, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_DrawNum (50, 40 , 9.87654321, &Font20, 5, WHITE, BLACK);
    Paint_DrawString_EN(1, 40, "ABC", &Font20, 0x000f, 0xfff0);
    Paint_DrawString_CN(1,60, "»¶Ó­Ê¹ÓÃ",  &Font24CN, WHITE, BLUE);
    Paint_DrawString_EN(1, 100, "WaveShare", &Font16, RED, WHITE); 

    /* 3.Refresh the picture in RAM to LCD */
    LCD_3IN5_Display(BlackImage);
    DEV_Delay_ms(1000);
#endif

#if 1
    Paint_DrawImage(gImage_3inch_5,0,0,LCD_3IN5.WIDTH,LCD_3IN5.HEIGHT);
    LCD_3IN5_Display(BlackImage);
    DEV_Delay_ms(1000);
#endif
    
#if 1
    float acc[3], gyro[3];
    char str[3];
    unsigned int tim_count = 0;
    PCF85063A_Init();
    datetime_t Now_time;
    QMI8658_init();
    PCF85063A_Reset();
    sleep_ms(200);
    FT6336U_Init(FT6336U_Gesture_Mode);
    DEV_KEY_Config(Touch_INT_PIN);
    DEV_IRQ_SET(Touch_INT_PIN, GPIO_IRQ_EDGE_RISE, &Touch_INT_callback);
    
    Paint_Clear(WHITE);
    Paint_DrawRectangle(0, 00, 320, 120, 0XF410, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawRectangle(0, 120, 320, 240, 0X4F30, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawRectangle(0, 240, 320, 360, 0X2595, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawRectangle(0, 360, 320, 480, 0X8430, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    
    Paint_DrawString_EN(45, 48, "Double Click Quit", &Font20, BLACK, 0XF410);
    Paint_DrawString_EN(45, 132, "ACC_X = " , &Font20, BLACK, 0X4F30);
    Paint_DrawString_EN(45, 168, "ACC_Y = " , &Font20, BLACK, 0X4F30);
    Paint_DrawString_EN(45, 204, "ACC_Z = ", &Font20, BLACK, 0X4F30);
    Paint_DrawString_EN(45, 252, "GYR_X = ", &Font20, BLACK, 0X2595);
    Paint_DrawString_EN(45, 288, "GYR_Y = ", &Font20, BLACK, 0X2595);
    Paint_DrawString_EN(45, 324, "GYR_Z = ", &Font20, BLACK, 0X2595);
    Paint_DrawString_EN(45, 372, "RTC_H = ", &Font20, BLACK, 0X8430);
    Paint_DrawString_EN(45, 408, "RTC_M = ", &Font20, BLACK, 0X8430);
    Paint_DrawString_EN(45, 444, "RTC_S = ", &Font20, BLACK, 0X8430);
    LCD_3IN5_Display(BlackImage);
    while (true)
    {
        Paint_Clear(WHITE);
        while(i2c_lock);
        I2C_LOCK();
        QMI8658_read_xyz(acc, gyro, &tim_count);
        PCF85063A_Read_now(&Now_time);
        I2C_UNLOCK();
        // printf("acc_x   = %4.3fmg , acc_y  = %4.3fmg , acc_z  = %4.3fmg\r\n", acc[0], acc[1], acc[2]);
        // printf("gyro_x  = %4.3fdps, gyro_y = %4.3fdps, gyro_z = %4.3fdps\r\n", gyro[0], gyro[1], gyro[2]);
        // printf("tim_count = %d\r\n", tim_count);
        
        Paint_DrawRectangle(155, 130, 300, 240, 0X4F30, DOT_PIXEL_2X2, DRAW_FILL_FULL);
        Paint_DrawRectangle(155, 240, 300, 360, 0X2595, DOT_PIXEL_2X2, DRAW_FILL_FULL);
        Paint_DrawRectangle(155, 360, 300, 470, 0X8430, DOT_PIXEL_2X2, DRAW_FILL_FULL);
        Paint_DrawNum(155, 132, acc[0], &Font20, 2, BLACK , 0X4F30);
        Paint_DrawNum(155, 168, acc[1], &Font20, 2, BLACK , 0X4F30);
        Paint_DrawNum(155, 204, acc[2], &Font20, 2, BLACK, 0X4F30);
        Paint_DrawNum(155, 252, gyro[0], &Font20, 2, BLACK, 0X2595);
        Paint_DrawNum(155, 288, gyro[1], &Font20, 2, BLACK, 0X2595);
        Paint_DrawNum(155, 324, gyro[2], &Font20, 2, BLACK, 0X2595);
        sprintf(str, "%02d", Now_time.hour);
        Paint_DrawString_EN(155, 372, str, &Font20, BLACK, 0X8430);
        sprintf(str, "%02d", Now_time.min);
        Paint_DrawString_EN(155, 408, str,  &Font20, BLACK, 0X8430);
        sprintf(str, "%02d", Now_time.sec);
        Paint_DrawString_EN(155, 444, str,  &Font20, BLACK, 0X8430);
        LCD_3IN5_DisplayWindows(155, 130, 300, 470, BlackImage);
        DEV_Delay_ms(100);
        if (flag == 1)
        {
            flag = 0;
            break;
        }
    }
#endif

#if 1
    while(i2c_lock);
    I2C_LOCK();
    FT6336U_Init(FT6336U_Point_Mode);
    I2C_UNLOCK();
    Paint_Clear(WHITE);
    LCD_3IN5_Display(BlackImage);
    Paint_DrawRectangle(0, 0, 320, 60, 0X2595, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawString_EN(90, 30, "Touch test", &Font20, BLACK, 0X2595);
    LCD_3IN5_Display(BlackImage);
    while (true)
    {
        if (flag)
        {
            while(i2c_lock);
            I2C_LOCK();
            FT6336U_Get_Point();
            I2C_UNLOCK();
            if(FT6336U.touch1_x > LCD_3IN5_WIDTH - 6)
            {
                FT6336U.touch1_x = LCD_3IN5_WIDTH - 6;
            }
            if(FT6336U.touch1_y > LCD_3IN5_HEIGHT - 6)
            {
                FT6336U.touch1_y = LCD_3IN5_HEIGHT - 6;
            }
            for (int w = 0; w < 6; w++)
            {
                for (int h = 0; h < 6; h++)
                {
                    BlackImage[LCD_3IN5_WIDTH * (FT6336U.touch1_y + h) + FT6336U.touch1_x + w] = 0X00F8;
                }
            }
            LCD_3IN5_DisplayWindows(FT6336U.touch1_x, FT6336U.touch1_y, FT6336U.touch1_x + 6, FT6336U.touch1_y + 6, BlackImage);
            // printf("X:%d Y:%d\r\n", FT6336U.touch1_x, FT6336U.touch1_y);
            flag = 0;
        }
        __asm__ volatile("nop");
    }
#endif

    /* Module Exit */
    free(BlackImage);
    BlackImage = NULL;
    
    DEV_Module_Exit();
}

void Touch_INT_callback(uint gpio, uint32_t events)
{
    if(i2c_lock)return;
    if (gpio == Touch_INT_PIN)
    {
        if (FT6336U.mode == FT6336U_Gesture_Mode)
        {
            uint8_t gesture = FT6336U_Get_Gesture();
            
            if (gesture == FT6336U_Gesture_Double_Click)
            {
                flag = 1;
            }
        }
        else
        {
            flag = 1;
        }
    }
}

void core1_entry() {
    static int press_time = 0;
    while(1)
    {
        DEV_Delay_ms(5); 
        if(DEV_Digital_Read(SYS_OUT_PIN) == 1)
        {
            press_time++;
            if(press_time > 300)//shutdown  
            {
                watchdog_reboot(0,0,0);
            }
        }
        else
        {
            press_time = 0;
        }
    }
}
