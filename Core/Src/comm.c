//
// Created by xiaow on 24-12-7.
//
#include "comm.h"
#include <tim.h>
#include <stdio.h>
#include <string.h>

uint8_t UARTBuf[BYTE_NUM] = {0};
STATE_MACHINE_TYPE CommState = Init;
void CommInit()
{
    HAL_TIM_Base_Start_IT(&htim14);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, UARTBuf, BYTE_NUM);
    CommState = Idle;
}

void CommIdle()
{
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == TIM14)
    {
        uint16_t a[30];
        switch (CommState)
        {
        case Process:
            sprintf(a, "%s", UARTBuf);
            CommState = Ack;
            break;
        case Ack:
            HAL_UART_Transmit(&huart1, a, strlen(a), 1000);
            CommState = Idle;
            break;
        case Error:
            sprintf(a, "Error\r\n");
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
        // HAL_UART_Receive_DMA(&huart1, UARTBuf, Size);
        // 关闭DMA半中断
        __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, UARTBuf, BYTE_NUM);
        if (CommState == Idle)
        {
            CommState = Process;
        }
    }
}
