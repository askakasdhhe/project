#include "gt911.h"
#include "lcd_touch.h"
#include "../LCD/delay.h"
#include "../LCD/lcd.h"
#include "string.h"
#include <stdio.h>

#include "main.h"
#include "i2c.h"
#include "gpio.h"

uint8_t touch_buf[4];
uint8_t touch_event = 0;

extern __IO uint32_t I2CTimeout;

void init_gt911(uint16_t slave_addr)
{
	  Set_GPIO_MODER(LCD_INT_GPIO_Port, MODE_OUTPUT, 2);

	  HAL_GPIO_WritePin(LCD_NRST_GPIO_Port, LCD_NRST_Pin, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(LCD_INT_GPIO_Port, LCD_INT_Pin, GPIO_PIN_RESET);

	  HAL_Delay(5);
	  HAL_GPIO_WritePin(LCD_INT_GPIO_Port, LCD_INT_Pin, (slave_addr == 0x14) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	  HAL_Delay(1);
	  HAL_GPIO_WritePin(LCD_NRST_GPIO_Port, LCD_NRST_Pin, GPIO_PIN_SET);
	  HAL_Delay(10);

	  Set_GPIO_MODER(LCD_INT_GPIO_Port, MODE_INPUT, 2);

	  uint8_t ctrl_val = 0x02;
	  gt911_send_data(GT_CTRL_REG, &ctrl_val, 1);
}

int gt911_send_data(uint16_t cmd, uint8_t* data, int len)
{
	return HAL_I2C_Mem_Write(&hi2c1, ((GT911_I2C_SLAVE<<1)|I2C_WRITE_BIT), cmd, I2C_MEMADD_SIZE_16BIT, data, len, I2CTimeout);
}

int gt911_read_data(uint16_t cmd, uint8_t* data, int len)
{
	return HAL_I2C_Mem_Read(&hi2c1, ((GT911_I2C_SLAVE<<1)|I2C_READ_BIT), cmd, I2C_MEMADD_SIZE_16BIT, data, len, I2CTimeout);
}


void gt911_scanning_event(void)
{
	uint8_t temp;
	uint8_t tempsta;
	uint8_t mode;
	uint8_t  value = 0;

	if (gt911_read_data(GT_GSTID_REG, &mode, 1) != 0) {
	        return;   // 通信失败，直接返回，避免执行错误逻辑
	}
	if ((mode & 0x80) == 0) {
		if (tp_dev.sta & TP_PRES_DOWN) {
			tp_dev.sta &= ~(1 << 7);
		}
		return;
	}

	gt911_read_data(GT_TP1_REG, touch_buf, 4);

	temp=0XFF<<(mode&0XF);
	tempsta=tp_dev.sta;
	tp_dev.sta=(~temp)|TP_PRES_DOWN|TP_CATH_PRES;
	tp_dev.x[4]=tp_dev.x[0];
	tp_dev.y[4]=tp_dev.y[0];

	if(tp_dev.touchtype&0X01)
	{
		tp_dev.y[0]=((uint16_t)touch_buf[1]<<8)+touch_buf[0];
		tp_dev.x[0]=800-(((uint16_t)touch_buf[3]<<8)+touch_buf[2]);
	}else
	{
		tp_dev.x[0]=((uint16_t)touch_buf[1]<<8)+touch_buf[0];
		tp_dev.y[0]=((uint16_t)touch_buf[3]<<8)+touch_buf[2];
	}


	if(tp_dev.x[0]>lcddev.width||tp_dev.y[0]>lcddev.height)
	{
		tp_dev.x[0]=tp_dev.x[4];
		tp_dev.y[0]=tp_dev.y[4];
		mode=0X80;
		tp_dev.sta=tempsta;
	}

	if((mode&0X8F)==0X80)
	{
		if(tp_dev.sta&TP_PRES_DOWN)
			{
				tp_dev.sta&=~(1<<7);
			}else
		{
			tp_dev.x[0]=0xffff;
			tp_dev.y[0]=0xffff;
			tp_dev.sta&=0XE0;
		}
	}
	gt911_send_data(GT_GSTID_REG, &value, 0x01);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == LCD_INT_Pin) {
		touch_event = 1;
	}
}



