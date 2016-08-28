/**
 * @file        lcd.c
 * @author      Weyne
 * @version     V01
 * @date        2016.08.23
 * @brief       LCD驱动部分
 * @note
 * @attention   COYPRIGHT WEYNE
 */
#include "lcd.h" 
#include "sysconfig.h"
#include "power.h"
#include "ssd2828.h"

void SetLcdPower(StateTypeDef state)
{
	PowerTypeDef power_list[POWER_AMOUT] = {POWER_1V8,POWER_2V8,POWER_3V3,POWER_P5V,POWER_N5V};
	uint8_t i = 0;	
	
	if(state == ON)
	{
		//顺序开启电源
		for(i = 0;i<POWER_AMOUT;i++)
		{
			if(((SystemConfig.PowerSettings>>i) & 0x01) == 0x01)
			{
				Power_SetState(power_list[i],ON);
				HAL_Delay(50);
			}
		}
	}
	else
	{
		//倒序关闭电源
		for(i = POWER_AMOUT ; i > 0 ; i--)
		{
			if(((SystemConfig.PowerSettings>>(i - 1)) & 0x01) == 0x01)
			{
				Power_SetState(power_list[i-1],OFF);
				HAL_Delay(50);
			}
		}
	}
}

void SetLcdTiming(void)
{
}

void SetMipiPara(void)
{
}
/**
	* @brief  设定LCD初始化
	* @note   头两个字节为整个initcode的长度
						后面为MIPI_TYPE + LEN + DATA
	* @param  None
	* @retval None
  */
void SetLcdInitCode(void)
{
	uint16_t initCodeSize = (SystemConfig.LCDInitCode[0]<<8) | SystemConfig.LCDInitCode[1];
	uint8_t *p = &SystemConfig.LCDInitCode[2];
	uint8_t para_amount = 0;
	uint8_t para;
	MipiPackageDef package = NO_PACKAGE;
	uint16_t i = 0 , j = 0;
	uint16_t delay_time;
	MIPI_ReadTypeDef result;
	
	while(i<initCodeSize)
	{
		switch((MipiTypeDef)(*(p+i++)))
		{
			case MIPI_DCS:
				package = DCS;
				break;
			
			case MIPI_GP:
				package = GP;
				break;
			
			case MIPI_DELAY:
				delay_time = *(p+i++);
				delay_time = (delay_time<<8) | *(p+i++);
				HAL_Delay(delay_time);
				break;
			
			case MIPI_WRITE:
				if(package == DCS)
				{
					para_amount = *(p+i++);
					if(para_amount == 1)
					{
						SSD2828_DcsShortWrite(1);
						SSD2828WriteData(*(p+i++));						
					}
					else
					{
						SSD2828_DcsShortWrite(para_amount);
						for (j = 0; j < para_amount; j++)
						{
							SSD2828WriteData(*(p+i++));
						}					
					}
				}
				else if(package == GP)
				{
					para_amount = *(p+i++);
					if(para_amount == 1)
					{
						SSD2828_GenericShortWrite(1);
						SSD2828WriteData(*(p+i++));						
					}
					else
					{
						SSD2828_GenericShortWrite(para_amount);
						for (j = 0; j < para_amount; j++)
						{
							SSD2828WriteData(*(p+i++));
						}					
					}					
				}
				else
				{
					para_amount = *(p+i++);
					for (j = 0; j < para_amount; j++)
					{
						i++;
					}
					printf("error: not specified package\n");
				}
				break;
			
			case MIPI_READ:
				para = *(p+i++);
				para_amount = *(p+i++);
				if (package == DCS)
				{
					result = SSD2828_DcsReadDT06(para, para_amount, SystemBuf);
				}
				if (package == GP)
				{
					result = SSD2828_GenericReadDT14(para, para_amount, SystemBuf);
				}

				if (result == MIPI_READ_SUCCEED)
				{
					printf("Info:read 0x%x\n", para);
					for (j = 0; j < para_amount; j++)
					{
						printf("para%d = 0x%x\n", j + 1, SystemBuf[j]);
					}
				}
				else
				{
					printf("Waring:MIPI read failed!\n");
				}				
					break;
			
			
			default:
				break;
		}
	}
}

void SetPattern(void)
{
}


void Lcd_ReInit(void)
{
	
}	
/************************ (C) COPYRIGHT WEYNE *****END OF FILE****/

