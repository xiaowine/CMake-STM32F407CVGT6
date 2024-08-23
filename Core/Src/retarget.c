// All credit to Carmine Noviello for this code
// https://github.com/cnoviello/mastering-stm32/blob/master/nucleo-f030R8/system/src/retarget/retarget.c

#include <_ansi.h>
#include <errno.h>
#include <sys/times.h>
#include <signal.h>
#include <retarget.h>
#include <stdint.h>
#include <stdio.h>
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

UART_HandleTypeDef* gHuart;

void RetargetInit(UART_HandleTypeDef* huart)
{
    gHuart = huart;

    /* Disable I/O buffering for STDOUT stream, so that
     * chars are sent out as soon as they are printed. */
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Hello World\n");
    printf("Clock: %lu MHz\n", HAL_RCC_GetSysClockFreq() / 1000000);
    printf("Unique ID: %08lX%08lX%08lX\n", *(uint32_t*)0x1FFF7A10, *(uint32_t*)(0x1FFF7A10 + 4),
           *(uint32_t*)(0x1FFF7A10 + 8));
}


int _write(int fd, char* ptr, int len)
{
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO)
    {
        const HAL_StatusTypeDef hstatus = HAL_UART_Transmit(gHuart, (uint8_t*)ptr, len, HAL_MAX_DELAY);
        if (hstatus == HAL_OK)
        {
            return len;
        }
        return EIO;
    }
    errno = EBADF;
    return -1;
}

int _read(int fd, char* ptr, int len)
{
    if (fd == STDIN_FILENO)
    {
        const HAL_StatusTypeDef hstatus = HAL_UART_Receive(gHuart, (uint8_t*)ptr, 1, HAL_MAX_DELAY);
        if (hstatus == HAL_OK)
        {
            return 1;
        }

        return EIO;
    }
    errno = EBADF;
    return -1;
}
