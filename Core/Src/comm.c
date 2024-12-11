#include "comm.h"
#include <tim.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t UARTBuf[BYTE_NUM] = {0};
STATE_MACHINE_TYPE CommState = Init;
STATE_MACHINE_ERROR_TYPE ErrType = NONE;
uint8_t response[100];
Frame_Structure frame;
REPORT_TYPE reportType = VOUT;
int CommunicationStatus = 0;


unsigned int calculateChecksum(const unsigned char* frame)
{
    unsigned int calculatedChecksum = 0;
    for (int i = 0; i < 6 - 2; i++)
    {
        calculatedChecksum += frame[i];
    }
    return calculatedChecksum & 0xFFFF;
}

void CommInit()
{
    HAL_TIM_Base_Start_IT(&htim14);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, UARTBuf, BYTE_NUM);
    CommState = Idle;
}

void CommIdle()
{
    // Function body can be added if needed
}

void CommParse()
{
    frame.FunctionCode = ((UARTBuf[1] << 8) | UARTBuf[0]);
    frame.Value = ((UARTBuf[3] << 8) | UARTBuf[2]);
    frame.CheckValue = ((UARTBuf[5] << 8) | UARTBuf[4]);
    if (calculateChecksum(UARTBuf) == frame.CheckValue)
    {
        CommState = (frame.FunctionCode < 8) ? Process : Error;
        ErrType = (frame.FunctionCode < 8) ? NONE : FUN;
    }
    else
    {
        CommState = Error;
        ErrType = CCR;
    }
}

int checkDataRange(const int min, const int max)
{
    if (frame.Value < min || frame.Value > max)
    {
        CommState = Error;
        ErrType = DATA;
        return 1;
    }
    return 0;
}

uint8_t* wrapFrameData(Frame_Structure reportFrame)
{
    static uint8_t data[BYTE_NUM] = {0};
    data[0] = reportFrame.FunctionCode & 0x00FF;
    data[1] = (reportFrame.FunctionCode >> 8) & 0x00FF;
    data[2] = reportFrame.Value & 0x00FF;
    data[3] = (reportFrame.Value >> 8) & 0x00FF;
    reportFrame.CheckValue = calculateChecksum(data);
    data[4] = reportFrame.CheckValue & 0x00FF;
    data[5] = (reportFrame.CheckValue >> 8) & 0x00FF;
    return data;
}

void CommProcess()
{
    switch (frame.FunctionCode)
    {
    case EN_:
        if (checkDataRange(0, 1)) break;
        break;
    case VREF:
        if (checkDataRange(0, 8000)) break;
        break;
    case IREF:
        if (checkDataRange(0, 50)) break;
        break;
    case OUT_MODE_:
        if (checkDataRange(0, 1)) break;
        break;
    default:
        break;
    }
    if (CommState != Error)
    {
        CommState = Idle;
    }
}

void CommError()
{
    switch (ErrType)
    {
    case FUN:
        // sprintf((char*)response, "Error:Function code error\n");
        break;
    case CCR:
        // sprintf((char*)response, "Error:Checksum error\n");
        break;
    case DATA:
        // sprintf((char*)response, "Error:Data out of range\n");
        break;
    case NONE:
        break;
    }
    // HAL_UART_Transmit(&huart1, response, strlen(response), HAL_MAX_DELAY);
    CommState = Idle;
}

void CommReport()
{
    Frame_Structure reportFrame = {};
    int randomValue;
    switch (reportType)
    {
    case VIN:
        reportFrame.FunctionCode = VIN;
        randomValue = rand() % 8000;
        reportFrame.Value = randomValue & 0xFFFF;
        reportType = IIN;
        break;
    case IIN:
        reportFrame.FunctionCode = IIN;
        randomValue = rand() % 500;
        reportFrame.Value = randomValue & 0xFFFF;
        reportType = VOUT;
        break;
    case VOUT:
        reportFrame.FunctionCode = VOUT;
        randomValue = rand() % 8000;
        reportFrame.Value = randomValue & 0xFFFF;
        reportType = IOUT;
        break;
    case IOUT:
        reportFrame.FunctionCode = IOUT;
        randomValue = rand() % 500;
        reportFrame.Value = randomValue & 0xFFFF;
        reportType = RUN_ERROR_TYPE;
        break;
    case RUN_ERROR_TYPE:
        reportFrame.FunctionCode = RUN_ERROR_TYPE;
        randomValue = rand() % 3;
        reportFrame.Value = randomValue;
        reportType = RUN_MODE;
        break;
    case RUN_MODE:
        reportFrame.FunctionCode = RUN_MODE;
        randomValue = rand() % 3;
        reportFrame.Value = randomValue;
        reportType = OUT_MODE;
        break;
    case OUT_MODE:
        reportFrame.FunctionCode = OUT_MODE;
        reportFrame.Value = 0;
        reportType = EN;
        break;
    case EN:
        reportFrame.FunctionCode = EN;
        reportFrame.Value = 1;
        reportType = VIN;
        break;
    }
    const uint8_t* data = wrapFrameData(reportFrame);
    HAL_UART_Transmit(&huart1, data, BYTE_NUM, HAL_MAX_DELAY);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == TIM14)
    {
        switch (CommState)
        {
        case Parse:
            CommParse();
            break;
        case Process:
            CommProcess();
            break;
        case Error:
            CommError();
            break;
        default:
            CommReport();
            break;
        }
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size)
{
    if (huart->Instance == USART1)
    {
        __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, UARTBuf, BYTE_NUM);
        if (CommState == Idle)
        {
            CommState = Parse;
        }
    }
}