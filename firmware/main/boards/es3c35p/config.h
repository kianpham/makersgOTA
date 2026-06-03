#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

// Board LCDWIKI ES3C35P — ESP32-S3, LCD ST77922 QSPI 320x480, touch ST77922 TDDI (I2C 0x55, cùng chip).
#include <driver/gpio.h>
#include <hal/lcd_types.h>

// ---- LCD ST77922 QSPI ----
#define DISPLAY_WIDTH        320
#define DISPLAY_HEIGHT       480
// Thứ tự chân QSPI theo bảng phân chân CHÍNH THỨC LCDWIKI ES3C35P:
//   IO11=D0, IO13=D1, IO14=D2, IO9=D3, IO10=CS, IO12=CLK, IO42=DC, EN=reset.
#define DISPLAY_CS_PIN       GPIO_NUM_10
#define DISPLAY_CLK_PIN      GPIO_NUM_12
#define DISPLAY_DATA0_PIN    GPIO_NUM_11
#define DISPLAY_DATA1_PIN    GPIO_NUM_13
#define DISPLAY_DATA2_PIN    GPIO_NUM_14
#define DISPLAY_DATA3_PIN    GPIO_NUM_9
#define DISPLAY_PCLK_HZ      (80 * 1000 * 1000)  // theo BSP LCDWIKI

#define DISPLAY_MIRROR_X     false
#define DISPLAY_MIRROR_Y     false
#define DISPLAY_SWAP_XY      false
#define DISPLAY_RGB_ORDER    LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_INVERT_COLOR false
#define DISPLAY_OFFSET_X     0
#define DISPLAY_OFFSET_Y     0

// Backlight PWM (mức cao = sáng).
#define DISPLAY_BACKLIGHT_PIN           GPIO_NUM_41
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

// ---- Touch ST77922 TDDI (I2C 0x55) — theo BSP LCDWIKI bsp_touch.c/bsp_i2c.h ----
// I2C port 0, RST=IO48 (output, xung reset), INT=IO47 (input pull-up, KHÔNG drive).
#define TOUCH_I2C_SDA_PIN GPIO_NUM_38
#define TOUCH_I2C_SCL_PIN GPIO_NUM_39
#define TOUCH_INT_PIN     GPIO_NUM_47
#define TOUCH_RST_PIN     GPIO_NUM_48
#define TOUCH_I2C_ADDR    0x55

#define BOOT_BUTTON_GPIO GPIO_NUM_0

#endif  // _BOARD_CONFIG_H_
