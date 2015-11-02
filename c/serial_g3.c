
/*
 * G3 PM2.5传感器C语言实现版本
 * bruce.zhu@2015.11.02
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "serial_helper.h"

#define SERIAL_PORT "/dev/ttyAMA0"
#define PACKAGE_LEN 24

static void dump_package(char *buf, int len);

static void
handle_package(char *package)
{
    printf("-----------------\n");
    dump_package(package, 24);

    // check data package length, should be 20
    int package_length = package[2] * 256 + package[3];
    if (package_length != 20) {
        printf("RECV data package length error[20, %d]\n", package_length);
        return;
    }

    // check CRC
    int crc = 0;
    int i;
    for (i = 0; i < PACKAGE_LEN - 2; i++) {
        crc += package[i];
    }
    crc = crc % (256*256);
    int package_crc = package[22] * 256 + package[23];
    if (package_crc != crc) {
        printf("data package crc error[%d, %d]\n", package_crc, crc);
        return;
    }

    // all is OK, let's get real value
    int index = 4;
    if (package[0] == 0x42 && package[1] == 0x4d) {
        // PM1.0(CF=1)
        int pm1_0 = package[4] * 256 + package[5];
        index = 6;
        // PM2.5(CF=1)i
        int pm2_5 = package[6] * 256 + package[7];
        // PM10(CF=1)
        int pm10 = package[8] * 256 + package[9];
        printf("(CF=1) -> [%d, %d, %d]\n", pm1_0, pm2_5, pm10);

        // PM1.0(大气环境下)
        int pm_air_1_0 = package[10] * 256 + package[11];
        // PM2.5(大气环境下)
        int pm_air_2_5 = package[12] * 256 + package[13];
        // PM10(大气环境下)
        int pm_air_10 = package[14] * 256 + package[15];

        printf("大气环境 -> [%d, %d, %d]\n", pm_air_1_0, pm_air_2_5, pm_air_10);

        // 数据7,8,9保留
    } else {
        printf("package length error\n");
    }
}

static int
serial_callback(int fd, char* buf, int len)
{
    static int package_index = 0;
    static char whole_package[PACKAGE_LEN];

    printf("package len = %d\n", len);
    dump_package(buf, len);

    int i;
    for (i = 0; i < len; i++) {
        if (package_index == 0) {
            if (buf[i] == 0x42 && buf[i+1] == 0x4d) {
                whole_package[package_index++] = buf[i];
            }
        } else if (package_index < PACKAGE_LEN) {
            whole_package[package_index++] = buf[i];
        }

        if (package_index == PACKAGE_LEN) {
            handle_package(whole_package);
            package_index = 0;
        }
    }

    return 0;
}

static void
dump_package(char *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%x ", buf[i]);
    }

    printf("\n");
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



