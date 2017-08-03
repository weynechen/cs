/**
* @file         flickersensor.c
* @author       Weyne Chen
* @version      V01
* @date         2017.06.25
* @brief         
* @note          
* @attention    COPYRIGHT WEYNE
**/

#include "flickersensor.h"
#include "usart.h"

uint8_t FlickerDataReady = 0;
static uint16_t FlickerValue;

void FS_Callback(PproTypeDef *data)
{
    switch (data->PackageID)
    {
    case FLICKER_VALUE:
        FlickerDataReady = 1;
        FlickerValue = data->Data;
        break;

    default:
        break;
    }
}

uint8_t GetFlickerValue(float *flicker)
{
    uint32_t timeout = 0xffffff;

    while (FlickerDataReady != 1)
    {
        timeout--;
        if (timeout == 0)
            return FLICKER_TIMEOUT;
    }

    FlickerDataReady = 0;

    *flicker = (float)FlickerValue / 10.0;

    return 1;
}

static void FS_SendData(uint8_t pid, uint16_t data)
{
    Ppro_SendData(FLCIKER_SENSOR, pid, data);
    HAL_Delay(50);
}

void SendVcomToFlickerSensor(uint16_t vcom)
{
    FS_SendData(VCOM_VALUE, vcom);
}

void SendIdToFlickerSensor(uint16_t id)
{
    FS_SendData(ID_VALUE, id);
}

/********************* (C) COPYRIGHT WEYNE CHEN *******END OF FILE ********/
