#include "comm.h"
#include <tim.h>
#include <stdio.h>
#include <string.h>

uint8_t UARTBuf[BYTE_NUM] = {0};
STATE_MACHINE_TYPE CommState = Init;
Error_Type ErrType = NONE;
uint8_t response[100];
Frame_Structure frame;

// Function to check if the checksum is correct
int calculateChecksum(const unsigned char* frame, const int16_t check_value)
{
    unsigned int calculatedChecksum = 0;
    // Exclude the last two bytes of the checksum
    for (int i = 0; i < 6 - 2; i++)
    {
        calculatedChecksum += frame[i];
    }
    calculatedChecksum &= 0xFFFF; // Keep only the lowest 16 bits
    // Compare the calculated checksum with the received checksum
    return calculatedChecksum == check_value;
}

void CommInit()
{
    HAL_TIM_Base_Start_IT(&htim14);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, UARTBuf, BYTE_NUM);
    CommState = Idle;
    sprintf((char*)response, "Inited\n");
    HAL_UART_Transmit_IT(&huart1, response, strlen(response));
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
    if (calculateChecksum(UARTBuf, frame.CheckValue))
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
        sprintf((char*)response, "EN:%d\n", frame.Value);
        if (frame.Value < 0 || frame.Value > 1)
        {
            CommState = Error;
            ErrType = DATA;
        }
        break;
    case VREF:
        sprintf((char*)response, "VREF:%dV\n", frame.Value);
        if (frame.Value < 0 || frame.Value > 80)
        {
            CommState = Error;
            ErrType = DATA;
        }
        break;
    case IREF:
        sprintf((char*)response, "IREF:%dA\n", frame.Value);
        if (frame.Value < 0 || frame.Value > 5)
        {
            CommState = Error;
            ErrType = DATA;
        }
        break;
    default:
        break;
    }
    HAL_UART_Transmit_IT(&huart1, response, strlen(response));

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
    HAL_UART_Transmit(&huart1, response, strlen(response), HAL_MAX_DELAY);
    CommState = Idle;
}

void CommError()
{
    sprintf((char*)response, "Error:%s\r\n",
            ErrType == FUN ? "FUN" : ErrType == CCR ? "CCR" : ErrType == DATA ? "DATA" : "NONE");
    HAL_UART_Transmit(&huart1, response, strlen(response), HAL_MAX_DELAY);
    CommState = Idle;
}

void CommReport()
{
    // Function body can be added if needed
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
