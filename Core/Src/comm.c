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

// Function to check if the checksum is correct
unsigned int calculateChecksum(const unsigned char* frame)
{
    unsigned int calculatedChecksum = 0;
    // Exclude the last two bytes of the checksum
    for (int i = 0; i < 6 - 2; i++)
    {
        calculatedChecksum += frame[i];
    }
    calculatedChecksum &= 0xFFFF; // Keep only the lowest 16 bits
    // Compare the calculated checksum with the received checksum
    return calculatedChecksum;
}

void CommInit()
{
    HAL_TIM_Base_Start_IT(&htim14);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, UARTBuf, BYTE_NUM);
    CommState = Idle;
    // sprintf((char*)response, "Inited\n");
    // HAL_UART_Transmit_IT(&huart1, response, strlen(response));
}

void CommIdle()
{
    // Function body can be added if needed
}

void CommParse()
{
    frame.FunctionCode = ((UARTBuf[1] << 8) | UARTBuf[0]); // NOLINT(*-narrowing-conversions)
    frame.Value = ((UARTBuf[3] << 8) | UARTBuf[2]); // NOLINT(*-narrowing-conversions)
    frame.CheckValue = ((UARTBuf[5] << 8) | UARTBuf[4]); // NOLINT(*-narrowing-conversions)
    if (calculateChecksum(UARTBuf) == frame.CheckValue)
    {
        if (frame.FunctionCode < 8)
        {
            CommState = Process;
        }
        else
        {
            CommState = Error;
            ErrType = FUN;
        }
    }
    else
    {
        CommState = Error;
        ErrType = CCR;
    }
}

void CommProcess()
{
    switch (frame.FunctionCode)
    {
    case EN:
        // sprintf((char*)response, "EN:%d\n", frame.Value);
        if (frame.Value < 0 || frame.Value > 1)
        {
            CommState = Idle;
            ErrType = DATA;
        }
        break;
    case VREF:
        // sprintf((char*)response, "VREF:%dV\n", frame.Value);
        if (frame.Value < 0 || frame.Value > 80)
        {
            CommState = Idle;
            ErrType = DATA;
        }
        break;
    case IREF:
        // sprintf((char*)response, "IREF:%dA\n", frame.Value);
        if (frame.Value < 0 || frame.Value > 5)
        {
            CommState = Idle;
            ErrType = DATA;
        }
        break;
    default:
        break;
    }
    // HAL_UART_Transmit_IT(&huart1, response, strlen(response));

    // sprintf((char*)response, "0x%04x 0x%04x 0x%02x\n%02X %02X %02X %02X %02X %02X\n",
    //         frame.FunctionCode, frame.Value, frame.CheckValue,
    //         UARTBuf[0], UARTBuf[1], UARTBuf[2], UARTBuf[3], UARTBuf[4], UARTBuf[5]);
    // HAL_UART_Transmit(&huart1, response, strlen(response), HAL_MAX_DELAY);
    if (CommState != Error)
    {
        CommState = Idle;
    }
}

void CommAck()
{
    // HAL_UART_Transmit(&huart1, response, strlen(response), HAL_MAX_DELAY);
    CommState = Idle;
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
    Frame_Structure reportFrame;
    switch (reportType)
    {
    case VIN:
        reportFrame.FunctionCode = VIN;
        int eb = rand() % 8000;
        reportFrame.Value = eb & 0xFFFF;
        reportType = IIN;
        break;
    case IIN:
        reportFrame.FunctionCode = IIN;
        int aa = rand() % 500;
        reportFrame.Value = aa & 0xFFFF;
        reportType = VOUT;
        break;
    case VOUT:
        reportFrame.FunctionCode = VOUT;
        int b = rand() % 8000;
        reportFrame.Value = b & 0xFFFF;
        reportType = IOUT;
        break;
    case IOUT:
        reportFrame.FunctionCode = IOUT;
        int a = rand() % 500;
        reportFrame.Value = a & 0xFFFF;
        reportType = RUN_ERROR_TYPE;
        break;
    case RUN_ERROR_TYPE:
        reportFrame.FunctionCode = RUN_ERROR_TYPE;
        int c = rand() % 3;
        reportFrame.Value = c;
        reportType = RUN_MODE;
        break;
    case RUN_MODE:
        reportFrame.FunctionCode = RUN_MODE;
        int d = rand() % 3;
        reportFrame.Value = d;
        reportType = OUT_MODE;
        break;
    case OUT_MODE:
        reportFrame.FunctionCode = OUT_MODE;
        reportFrame.Value = 1;
        reportType = VIN;
        break;
    }
    uint8_t data[BYTE_NUM] = {0}; // 发送输出-一帧数据6个字节

    // 赋值发送的数组
    data[0] = reportFrame.FunctionCode & 0x00FF;
    data[1] = (reportFrame.FunctionCode >> 8) & 0x00FF;
    data[2] = reportFrame.Value & 0x00FF;
    data[3] = (reportFrame.Value >> 8) & 0x00FF;
    reportFrame.CheckValue = calculateChecksum(data); // NOLINT(*-narrowing-conversions)
    data[4] = reportFrame.CheckValue & 0x00FF;
    data[5] = (reportFrame.CheckValue >> 8) & 0x00FF;

    // 使用data数组进行传输
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
        case Ack:
            CommAck();
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
