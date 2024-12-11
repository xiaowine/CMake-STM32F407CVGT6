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
    Parse, //帧内容处理
    Process, //帧信息处理
    Error //信息错误
} STATE_MACHINE_TYPE;

//状态机枚举
typedef enum
{
    NONE = 0,
    FUN = 1,
    CCR = 2,
    DATA = 3,
} STATE_MACHINE_ERROR_TYPE;


typedef enum
{
    EN_ = 0,
    VREF = 1,
    IREF = 2,
    OUT_MODE_ = 3,
} FUN_TYPE;

typedef enum
{
    VIN = 0,
    IIN = 1,
    VOUT = 2,
    IOUT = 3,
    RUN_ERROR_TYPE = 4,
    RUN_MODE = 5,
    OUT_MODE = 6,
    EN = 7,
} REPORT_TYPE;

typedef enum
{
    BUCK = 0,
    BOOST = 1,
    MIXED = 2,
} RUN_MODE_TYPE;

typedef enum
{
    CC = 0,
    CV = 1
} OUT_MODE_TYPE;

//一帧数据的相关结构体
typedef struct _frame_structure
{
    __IO int16_t FunctionCode; //功能码
    __IO int16_t Value; //接收数值
    __IO int16_t CheckValue; //校验值
} Frame_Structure;


void CommInit();

#endif //COMM_H
