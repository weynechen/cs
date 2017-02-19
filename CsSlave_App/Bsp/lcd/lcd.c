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
#include "string.h"
#include "in_img.h"
#include "res.h"
#include "cdce913.h"
#include "fpga.h"
#include "tim.h"
#include "ack.h"
#include "rgbif.h"
#include "font.h"

FontColorTypeDef FontColor = {0xffffff,0};

uint8_t ReadBackAmount = 0;
uint8_t ReadBackTemp[32];

static uint8_t ShowIDPattern = 0xff;

void SetLcdPower(StateTypeDef state)
{
  PowerTypeDef power_list[POWER_AMOUT] = { POWER_1V8, POWER_2V8, POWER_3V3, POWER_VSP, POWER_VSN,POWER_OUT5V,POWER_MTP };
  uint8_t i = 0;

  if (state == ON)
  {
    //顺序开启电源
    for (i = 0; i < POWER_AMOUT; i++)
    {
      if (((SystemConfig.PowerSettings >> i) & 0x01) == 0x01)
      {
        Power_SetState(power_list[i], ON);
        HAL_Delay(10);
      }
    }
  }
  else
  {
    //倒序关闭电源
    for (i = POWER_AMOUT; i > 0; i--)
    {
      if (((SystemConfig.PowerSettings >> (i - 1)) & 0x01) == 0x01)
      {
        Power_SetState(power_list[i - 1], OFF);
        HAL_Delay(50);
      }
    }
  }
}


void SetLcdTiming(void)
{
  memcpy((uint16_t *)&LCDTiming, (uint16_t *)SystemConfig.LCDTimingPara, sizeof(SystemConfig.LCDTimingPara));
  CDCE_Init(LCDTiming.DCLK);
  LcdDrvSetTiming();
}

void SetMipiPara(void)
{
  uint8_t size = *(uint8_t *)SystemConfig.MIPIConfig;
  uint16_t i;
	
	if(size%3!=0)
		return;
	
	if(size == 3)
	{
		uint8_t lane = SystemConfig.MIPIConfig[3];
		uint16_t speed = (SystemConfig.MIPIConfig[1] << 8) | SystemConfig.MIPIConfig[2];
		
		SSD2828_Init(lane, speed);
	}
	else
	{
		SSD2828_Reset();
		
		for(i=1;i<size;i+=3)
		{
			SSD2828WriteReg(SystemConfig.MIPIConfig[i],SystemConfig.MIPIConfig[i+1],SystemConfig.MIPIConfig[i+2]);
		}
	}
}



void ResetRGBLcd(void)
{
  HAL_GPIO_WritePin(LS245_OE_GPIO_Port, LS245_OE_Pin, GPIO_PIN_RESET);
  HAL_Delay(10);
//  HAL_GPIO_WritePin(MIPIRESET_GPIO_Port, MIPIRESET_Pin, GPIO_PIN_SET);
//  HAL_Delay(10);
	
  RGB_RESET = 0;
  HAL_Delay(120);
  RGB_RESET = 1;
  HAL_Delay(120);	
}

void SetRGBSPI8Or9BitLcdInitCode(void)
{
  uint16_t code_size = (SystemConfig.LCDInitCode[0] << 8) | SystemConfig.LCDInitCode[1];
  uint8_t *p = &SystemConfig.LCDInitCode[2];
  uint8_t para_amount = 0;

//  SPIEdgeTypeDef package;
  uint16_t i = 0, j = 0;
  uint16_t delay_time;
  //uint8_t buffer[32];
	//  uint8_t para;
	//ReadBackAmount = 0;
	
  while (i < code_size)
  {
    switch ((RGBTypeDef)(*(p + i++)))
    {
    case RGB_SPI_RISING:
  //    package = SPI_RISING;
      break;

    case RGB_SPI_FALLING:
    //  package = SPI_FALLING;
      break;

    case RGB_DELAY:
      delay_time = *(p + i++);
      delay_time = (delay_time << 8) | *(p + i++);
      HAL_Delay(delay_time);
      break;

    case RGB_WRITE:
			para_amount = *(p + i++);
			if (SystemConfig.LcdType == RGB_SPI8BIT)
      {
				RGB_SPIWrite8Bit(*(p + i++),SPI_COMMAND);

				for (j = 0; j < para_amount-1; j++)
				{
					RGB_SPIWrite8Bit(*(p + i++),SPI_DATA);
				}
			}
      else
      {
				RGB_SPIWrite9Bit(*(p + i++),SPI_COMMAND);

				for (j = 0; j < para_amount-1; j++)
				{
					RGB_SPIWrite9Bit(*(p + i++),SPI_DATA);
				}
      }
      break;

    case RGB_READ:
			UserPrintf("Waring:This function will be supported nexe version!\n");
      break;


    default:
      break;
    }
  }	
}

void SetRGBSPI16BitLcdInitCode(void)
{
  uint16_t code_size = (SystemConfig.LCDInitCode[0] << 8) | SystemConfig.LCDInitCode[1];
  uint8_t *p = &SystemConfig.LCDInitCode[2];
  uint16_t reg = 0;
  uint8_t para;
  SPIEdgeTypeDef package = SPI_RISING;
  uint16_t delay_time;
	uint16_t i =0;
	ReadBackAmount = 0;
	
  while (i < code_size)
  {
    switch ((RGBTypeDef)(*(p + i++)))
    {
    case RGB_SPI_RISING:
      package = SPI_RISING;
      break;

    case RGB_SPI_FALLING:
      package = SPI_FALLING;
      break;

    case RGB_DELAY:
      delay_time = *(p + i++);
      delay_time = (delay_time << 8) | *(p + i++);
      HAL_Delay(delay_time);
      break;

    case RGB_WRITE:
			reg = 0;
			reg = *(p + i++);
		  reg = *(p + i++)<<8 | reg;
		  para = *(p + i++);
			RGB_SPIWrite_16Bit(reg,para,package);	
      break;

    case RGB_READ:
			reg = 0;
			reg = *(p + i++);
		  reg = *(p + i++)<<8 | reg;

	
			UserPrintf("Info:read 0x%04X\n", reg);
			para = RGB_SPIRead_16Bit(reg,package);
			if(sizeof(ReadBackTemp) - ReadBackAmount >  + 2)
			{
				ReadBackTemp[ReadBackAmount++] = 2; //len
				ReadBackTemp[ReadBackAmount++] = reg; //register
				ReadBackTemp[ReadBackAmount++] = para;
			}

			UserPrintf("para = 0x%02X\n", para);

      break;


    default:
      break;
    }
  }
}


/**
 * @brief  解析并设定LCD初始化
 * @note   头两个字节为整个initcode的长度
 *          后面为MIPI_TYPE + LEN + DATA
 * @param  None
 * @retval None
 */
void SetMipiLcdInitCode(void)
{
  uint16_t code_size = (SystemConfig.LCDInitCode[0] << 8) | SystemConfig.LCDInitCode[1];
  uint8_t *p = &SystemConfig.LCDInitCode[2];
  uint8_t para_amount = 0;
  uint8_t para;
  MipiPackageDef package = NO_PACKAGE;
  uint16_t i = 0, j = 0;
  uint16_t delay_time;
  MIPI_ReadTypeDef result;
  uint8_t buffer[32];

	ReadBackAmount = 0;
	SSD2828_SetMode(LP);
	
  while (i < code_size)
  {
    switch ((MipiTypeDef)(*(p + i++)))
    {
    case MIPI_DCS:
      package = DCS;
      break;

    case MIPI_GP:
      package = GP;
      break;

    case MIPI_DELAY:
      delay_time = *(p + i++);
      delay_time = (delay_time << 8) | *(p + i++);
      HAL_Delay(delay_time);
      break;

    case MIPI_WRITE:
      if (package == DCS)
      {
        para_amount = *(p + i++);
        if (para_amount == 1)
        {
          SSD2828_DcsShortWrite(1);
          SSD2828WriteData(*(p + i++));
        }
        else
        {
          SSD2828_DcsLongWrite(para_amount);
          for (j = 0; j < para_amount; j++)
          {
            SSD2828WriteData(*(p + i++));
          }
        }
      }
      else if (package == GP)
      {
        para_amount = *(p + i++);
        if (para_amount == 1)
        {
          SSD2828_GenericShortWrite(1);
          SSD2828WriteData(*(p + i++));
        }
        else
        {
          SSD2828_GenericLongWrite(para_amount);
          for (j = 0; j < para_amount; j++)
          {
            SSD2828WriteData(*(p + i++));
          }
        }
      }
      else
      {
        para_amount = *(p + i++);
        for (j = 0; j < para_amount; j++)
        {
          i++;
        }
        UserPrintf("Error: not specified MIPI package\n");
        return;
      }
      break;

    case MIPI_READ:
      para = *(p + i++);
      para_amount = *(p + i++);
      if (package == DCS)
      {
        result = SSD2828_DcsReadDT06(para, para_amount, buffer);
      }
      if (package == GP)
      {
        result = SSD2828_GenericReadDT14(para, para_amount, buffer);
      }

      if (result == MIPI_READ_SUCCEED)
      {
        UserPrintf("Info:read 0x%02X\n", para);
				
				if(sizeof(ReadBackTemp) - ReadBackAmount > para_amount + 2)
				{
					ReadBackTemp[ReadBackAmount++] = para_amount + 1; //len
					ReadBackTemp[ReadBackAmount++] = para; //register
					memcpy(ReadBackTemp + ReadBackAmount,buffer,para_amount);
					ReadBackAmount++;
				}
				
				for (j = 0; j < para_amount; j++)
        {
					HAL_Delay(10);
          UserPrintf("para%d = 0x%02X\n", j + 1, buffer[j]);
        }
				
      }
      else
      {
        UserPrintf("Waring:MIPI read failed!\n");
      }
      break;


    default:
      break;
    }
  }
}

static void ShowID(void)
{
	uint16_t j = 0,x = 0, y = 0;
	char temp[16];
	
	LcdDrvSetCharIndex(ShowIDPattern);
	
	while(j<ReadBackAmount)
	{
		uint8_t len = ReadBackTemp[j++];
		uint8_t n;
		
		for(n = 0 ; n<len ; n++)
		{
			memset(temp,0,sizeof(temp));
			sprintf(temp,"0x%02X ",ReadBackTemp[j++]);
			x += 16*n*5;
			LCD_ShowString(x,y,temp);
		}
		x= 0;
		y+=32;
	}	
}

void SetPattern(void)
{
  uint16_t size = (SystemConfig.Pattern[0] << 8) | SystemConfig.Pattern[1];
  uint8_t *p = &SystemConfig.Pattern[2];
  uint16_t i = 0;
  uint8_t r, g, b;
  uint16_t stay_time;


  memset(&PatternProperty, 0, sizeof(PatternProperty));

  LcdDrvSetPattern();
  while (i < size)
  {
		uint8_t is_pattern = 1;
    if (PatternProperty.Counter > PATTERN_AMOUNT)
    {
      break;
    }

    switch ((PatternTypeDef) * (p + i++))
    {
    case PATTERN_START:
			is_pattern = 0;
      break;

    case RGB:
      r = *(p + i++);
      g = *(p + i++);
      b = *(p + i++);
      Img_Full(r, g, b);
      sprintf(PatternProperty.Name[PatternProperty.Counter], "%d,RGB(0x%02X,%02X,%02X)", PatternProperty.Counter, r, g, b);
      PatternProperty.Counter++;
      break;
		
		case SHOW_ID:
			if(PatternProperty.Counter!=0)
				ShowIDPattern =PatternProperty.Counter -1;
			else
				ShowIDPattern =0;
			
			ShowID();
			break;

    case FLICKERV:
      Flicker();
      sprintf(PatternProperty.Name[PatternProperty.Counter], "%d,flicker", PatternProperty.Counter);
      PatternProperty.Counter++;
      break;

    case FLICKERH:
      Flicker();
      sprintf(PatternProperty.Name[PatternProperty.Counter], "%d,flicker", PatternProperty.Counter);
      PatternProperty.Counter++;
      break;

    case COLORBARV:
      Img_ColorBarV();
      sprintf(PatternProperty.Name[PatternProperty.Counter], "%d,vertical colorbar", PatternProperty.Counter);
      PatternProperty.Counter++;
      break;

    case COLORBARH:
      Img_ColorBar();
      sprintf(PatternProperty.Name[PatternProperty.Counter], "%d,horizontal colorbar", PatternProperty.Counter);
      PatternProperty.Counter++;
      break;

    case GRAYLEVEL_V:
      Img_Gray256_V();
      sprintf(PatternProperty.Name[PatternProperty.Counter], "%d,vertical gray level", PatternProperty.Counter);
      PatternProperty.Counter++;
      break;

    case GRAYLEVEL_H:
      Img_Gray256_H();
      sprintf(PatternProperty.Name[PatternProperty.Counter], "%d,horizontal gray level", PatternProperty.Counter);
      PatternProperty.Counter++;
      break;

    case CROSSTALK:
      Img_CT();
      sprintf(PatternProperty.Name[PatternProperty.Counter], "%d,crosstalk", PatternProperty.Counter);
      PatternProperty.Counter++;
      break;

    case CHESSBOARD:
      Img_Chcker58();
      sprintf(PatternProperty.Name[PatternProperty.Counter], "%d,chessboard", PatternProperty.Counter);
      PatternProperty.Counter++;
      break;

    case RGBBAR:
      RGBBar();
      sprintf(PatternProperty.Name[PatternProperty.Counter], "%d,rgbbar", PatternProperty.Counter);
      PatternProperty.Counter++;
      break;

    case RGBLEVEL:
      RGBLevel();
      sprintf(PatternProperty.Name[PatternProperty.Counter], "%d,rgblevel", PatternProperty.Counter);
      PatternProperty.Counter++;
      break;

    case FRAME:
      Img_Box();
      sprintf(PatternProperty.Name[PatternProperty.Counter], "%d,frame", PatternProperty.Counter);
      PatternProperty.Counter++;
      break;

    case PIC:
      Res_ReadPic((char *)(p + i));

      snprintf(PatternProperty.Name[PatternProperty.Counter], PATTERN_NAME_LEN,"%d,PIC %s", PatternProperty.Counter, (char *)(p + i));
      PatternProperty.Counter++;
      i += strlen((char *)p + i);
			i++; // 跳过结束符
      break;


    case PATTERN_END:
			is_pattern = 0;
      break;

    case PATTERN_STAY:
      stay_time = *(p + i++);
      stay_time = (stay_time << 8) | *(p + i++);
      if (PatternProperty.Counter != 0)
      {
        PatternProperty.StayTime[PatternProperty.Counter - 1] = stay_time;
      }
			is_pattern = 0;
      break;

    default:
      UserPrintf("Error:pattern syntax error\n");
      break;
    }

    if (TaskID != ACTION_NULL)
    {
      break;
    }

		if(is_pattern)
			HAL_Delay(500);
  }

  LcdDrvShowPattern(0);
}


uint8_t IsStayTimeOver(uint8_t frame)
{
  if (PatternProperty.StayTime[frame] == 0)
  {
    return 0;
  }

  if (htim3.Instance->CNT > PatternProperty.StayTime[frame] * 10)
  {
    htim3.Instance->CNT = 0;
    return 1;
  }

  return 0;
}


void ResetStayTimeCounter(void)
{
  htim3.Instance->CNT = 0;
}


void ResetMipiLcd(void)
{
  HAL_GPIO_WritePin(LS245_OE_GPIO_Port, LS245_OE_Pin, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(MIPIRESET_GPIO_Port, MIPIRESET_Pin, GPIO_PIN_SET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(MIPIRESET_GPIO_Port, MIPIRESET_Pin, GPIO_PIN_RESET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(MIPIRESET_GPIO_Port, MIPIRESET_Pin, GPIO_PIN_SET);
  HAL_Delay(120);
}


void Lcd_ReInit(void)
{
  memset(&PatternProperty, 0, sizeof(PatternProperty));
	HAL_GPIO_WritePin(MIPIRESET_GPIO_Port, MIPIRESET_Pin, GPIO_PIN_SET);
  SetLcdTiming();
	
	HAL_Delay(10);
  SetLcdPower(OFF);
	HAL_Delay(10);
  SetLcdPower(ON);
	
	if(SystemConfig.LcdType == MIPI_LCD)
	{
		SetMipiPara();
		ResetMipiLcd();
		SetMipiLcdInitCode();
		SSD2828_SetMode(VD);
  }
	else if(SystemConfig.LcdType == RGB_SPI16BIT)
	{
		ResetRGBLcd();
		SetRGBSPI16BitLcdInitCode();
	}
	else if((SystemConfig.LcdType == RGB_SPI8BIT) || (SystemConfig.LcdType == RGB_SPI9BIT))
	{
		ResetRGBLcd();
		SetRGBSPI8Or9BitLcdInitCode();
	}
	else
		UserPrintf("Error:LCD type definition error!\n");
	
	Power_SetBLCurrent(SystemConfig.Backlight);
  LcdDrvOpenRGB();
	//RGB_SPI_Test();
  SetPattern();

//	LcdDrvShowPattern(1);
}

void Lcd_LightOn(void)
{
  SetLcdPower(ON);
	if(SystemConfig.LcdType == MIPI_LCD)
	{
		SetMipiPara();
		ResetMipiLcd();
		SetMipiLcdInitCode();
		SSD2828_SetMode(VD);
  }
	else if(SystemConfig.LcdType == RGB_SPI16BIT)
	{
		ResetRGBLcd();
		SetRGBSPI16BitLcdInitCode();
	}
	else if((SystemConfig.LcdType == RGB_SPI8BIT) || (SystemConfig.LcdType == RGB_SPI9BIT))
	{
		ResetRGBLcd();
		SetRGBSPI8Or9BitLcdInitCode();
	}
	if(ShowIDPattern != 0xff)
		ShowID();
  Power_SetBLCurrent(SystemConfig.Backlight);
  LcdDrvOpenRGB();
}


void MipiLcdSleep(void)
{
  SSD2828_DcsShortWrite(1);
  SSD2828WriteData(0x11);
}






/*
static void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
  LcdDrvSetXY(x, y);
  LcdDrvWriteData(color >> 8);
  LcdDrvWriteData(color);
}
*/

void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t chars)
{
  uint8_t temp = 0x01;
  uint8_t pos, t;

  chars = chars - ' ';

  for (pos = 0; pos < 64; pos += 2)
  {
    LcdDrvSetXY(x, y + (pos >> 1));
		
    temp = asc2_3216[chars][pos];
    for (t = 0; t < 8; t++)
    {
      if (temp & 0x01)
      {
				LcdDrvWriteData(FontColor.Fore >> 16);
        LcdDrvWriteData(FontColor.Fore >> 8);
        LcdDrvWriteData(FontColor.Fore);
      }
      else
      {
				LcdDrvWriteData(FontColor.Background >> 16);
        LcdDrvWriteData(FontColor.Background >> 8);
        LcdDrvWriteData(FontColor.Background);
      }
      temp >>= 1;
    }
    temp = asc2_3216[chars][pos + 1];
    for (t = 8; t < 16; t++)
    {
      if (temp & 0x01)
      {
				LcdDrvWriteData(FontColor.Fore >> 16);
        LcdDrvWriteData(FontColor.Fore >> 8);
        LcdDrvWriteData(FontColor.Fore);
      }
      else
      {
				LcdDrvWriteData(FontColor.Background >> 16);
        LcdDrvWriteData(FontColor.Background >> 8);
        LcdDrvWriteData(FontColor.Background);
      }
      temp >>= 1;
    }
  }
}

void LCD_ShowString(uint16_t x,uint16_t y,const char *p)
{         
	

    while(*p!='\0')
    {       
        LCD_ShowChar(x,y,*p);
				x=x+16;
        p++;
    }  
}

void PageTuning(PageTuningTypeDef page)
{
	if(page == PAGE_UP)
	{
	
      if (PatternProperty.CurrentPattern == PatternProperty.Counter - 1)
      {
        PatternProperty.CurrentPattern = 0;
      }
      else
      {
        PatternProperty.CurrentPattern++;
      }
      LcdDrvShowPattern(PatternProperty.CurrentPattern);	
	}
	else
	{
			if (PatternProperty.CurrentPattern == 0)
      {
        PatternProperty.CurrentPattern = PatternProperty.Counter - 1;
      }
      else
      {
        PatternProperty.CurrentPattern--;
      }
      LcdDrvShowPattern(PatternProperty.CurrentPattern);
	}
}




/************************ (C) COPYRIGHT WEYNE *****END OF FILE****/
