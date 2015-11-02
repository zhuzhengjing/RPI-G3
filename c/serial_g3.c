
/*
 * G3 PM2.5传感器C语言实现版本
 * bruce.zhu@2015.11.02
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "serial_helper.h"

#define SERIAL_PORT "/dev/ttyAMA0"

static int 
serial_callback(int fd, char* buf, int len) 
{
    printf("fd = %d, len = %d\n", fd, len);
    int i = 0;
    for (i = 0; i < len; i++) {
        
    }
    return 0;
}

static struct serial_helper_t serial = {
    .dev_path = SERIAL_PORT,
    .baud_rate = 9600,
    .data_bits = 8,
    .serial_notification = serial_callback
};


int main()
{
    int ret = serial_helper_register(&serial);
    if (ret) printf("serial open failed\n");
    else printf("serial open success\n");

    for (;;) {
        sleep(1);
    }
}



