//
// Created by xiaow on 24-12-7.
//

#ifndef COMM_H
#define COMM_H
#include "stm32f4xx.h"
#include <stdint.h>
#include <usart.h>
#include <dma.h>
#include <stm32f4xx_it.h>

#define BYTE_NUM 6//缓冲最大字节数
extern uint8_t UARTBuf[BYTE_NUM]; //数据缓存

//状态机枚举
typedef enum
{
    Init, //初始化
    Idle, //等待接收
    Process, //帧内容处理
    Ack, //回复
    Error //信息错误
} STATE_MACHINE_TYPE;


//一帧数据的相关结构体
typedef struct _frame_structure
{
    __IO int16_t TxFunctionCode; //功能码
    __IO int16_t TxValue; //接收数值
    __IO int16_t TxCheckValue; //校验值
} Frame_Structure;


void CommInit();

#endif //COMM_H
