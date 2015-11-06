

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#ifndef u8
#define u8 unsigned char
#endif

#define CLEAR_BIT(A, B)     (A &= ~(1 << B))
#define SET_BIT(A, B)       (A |= 1 << B)

#define SET_ONE_BIT         0x01

#define SPI_CHAN 0

static int pca8538_fd;

// RAM definition
// address: 0 - 203
static u8 g_PCA8538_RAM_204[204];

static void PCA8538_set_RAM_content(u8 address, u8 length, u8* content);

// 树莓派SPI总线初始化
void spiSetup (int speed)
{
    if ((pca8538_fd = wiringPiSPISetup (SPI_CHAN, speed)) < 0) {
        fprintf(stderr, "Can't open the SPI bus: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

// 初始化PCA8538
void init_PCA8538(void)
{
    int ret = 0;
    
    char init_buf[] = {0x20, 0x80, 0x3A, 0x80, 0xD8, 0x80, 0x18,
					   0x80, 0xD4, 0x80, 0xC9, 0x80, 0x45, 0x80, 0x73};
    
    char init_buf2[] = {0xC0, 0x07, 0x80, 0xD0, 0x80, 0xB3, 0x80, 0x39};
    // clear
    memset(g_PCA8538_RAM_204, 0x00, sizeof(g_PCA8538_RAM_204));
    
    if (wiringPiSPIDataRW (SPI_CHAN, init_buf, sizeof init_buf) == -1) {
        printf ("SPI failure: %s\n", strerror(errno));
        return;
    }
    
    // delay 60ms
    usleep(1000*60);
    
    if (wiringPiSPIDataRW (SPI_CHAN, init_buf, sizeof init_buf) == -1) {
        printf ("SPI failure: %s\n", strerror(errno));
        return;
    }
    
    PCA8538_set_RAM_content(0, 204, g_PCA8538_RAM_204);
}

/**
 * @brief: clear the content of PCA8538
 * @param: addr, start address(0-203 bytes)
 * @param: size, clear content length
 * @param: content, 0x00 ----> no display
 *                  0xff ----> display all
 */
static void PCA8538_set_RAM_content(u8 address, u8 length, u8* content)
{
    int i;
    u8 mask_y, mask_msb, mask_lsb;
    u8 buf[8];
    
    if (address < 102)
    {
        mask_msb = 0x80 | ((0x70 & address) >> 4);
        mask_lsb = 0x90 | (0x0f & address);
        mask_y   = 0xA0 | 0x00;
    }
    else
    {
        address  = address - 102;
        mask_msb = 0x80 | ((0x70 & address) >> 4);
        mask_lsb = 0x90 | (0x0f & address);
        mask_y   = 0xA0 | 0x01;
    }
    
    buf[0] = 0x20;
    buf[1] = 0x80;
    buf[2] = mask_msb;
    buf[3] = 0x80;
    buf[4] = mask_lsb;
    buf[5] = 0x80;
    buf[6] = mask_y;
    buf[7] = 0x20;
    
    if (wiringPiSPIDataRW(SPI_CHAN, buf, sizeof buf) == -1) {
        printf ("PCA8538_set_RAM_content SPI failure: %s\n", strerror(errno));
    }
    
    if (wiringPiSPIDataRW(SPI_CHAN, content, length) == -1) {
        printf ("PCA8538_set_RAM_content SPI failure: %s\n", strerror(errno));
    }
}

void PCA8538_read_temperature(u8* temp)
{
    char buf[2] = {0xA0, 0xA0};
    wiringPiSPIDataRW(SPI_CHAN, buf, 1);
    *temp = (u8)wiringPiSPIDataRW(SPI_CHAN, buf, 1);
}

/**
 * @brief: set centigrade icon
 * @param: value, 0 C, centigrade icon
 *                1 F, Fahrenheit icon
 */
static void PCA8538_set_T1_buffer(u8 value)
{
    switch (value)
    {
        case 0:
            CLEAR_BIT(g_PCA8538_RAM_204[96], 0x80);
            SET_BIT(g_PCA8538_RAM_204[96], 0x40);
            SET_BIT(g_PCA8538_RAM_204[97], 0x80);
            break;
        case 1:
            SET_BIT(g_PCA8538_RAM_204[96], 0x80);
            CLEAR_BIT(g_PCA8538_RAM_204[96], 0x40);
            SET_BIT(g_PCA8538_RAM_204[97], 0x80);
            break;
        default:
            break;
    }
}

/**
 * @brief: set T4 buffer
 * @param: value, -1 ----> '-1'
 *                 0 ----> '0'
 *                 1 ----> '1'
 *                 2 ----> ' ' clear display
 */
static void PCA8538_set_T4_buffer(s8 value)
{
    switch (value)
    {
        case -1:
            SET_BIT(g_PCA8538_RAM_204[99], 0x80);
            SET_BIT(g_PCA8538_RAM_204[100], 0x80);
            break;
        case 0:
            CLEAR_BIT(g_PCA8538_RAM_204[99], 0x80);
            SET_BIT(g_PCA8538_RAM_204[100], 0x80);
            break;
        case 1:
            SET_BIT(g_PCA8538_RAM_204[99], 0x80);
            CLEAR_BIT(g_PCA8538_RAM_204[100], 0x80);
            break;
        case 2:
            CLEAR_BIT(g_PCA8538_RAM_204[99], 0x80);
            CLEAR_BIT(g_PCA8538_RAM_204[100], 0x80);
            break;
        default:
            break;
    }
}

/**
 * @brief: set T2 and T3 buffer
 * @param: index, 0 ---> T2 buffer
 *                1 ---> T3 buffer
 * @param: value, 0-9
 *                0x0A, '-' display
 *                0xff, clear display
 */
static void PCA8538_set_T2T3_buffer(u8 index, u8 value)
{
    u8 seg1, seg2;
    
    switch (index)
    {
        case 0:
            seg1 = 97;
            seg2 = 98;
            break;
        case 1:
            seg1 = 99;
            seg2 = 101;
            break;
        default:
            return;
    }
    
    switch (value)
    {
        case 0:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x40);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x10);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x80);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x40);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x10);
            break;
        case 1:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x40);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x10);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x80);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x40);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x20);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x10);
            break;
        case 2:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x40);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x20);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x10);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x80);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x40);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x10);
            break;
        case 3:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x40);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x10);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x80);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x40);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x10);
            break;
        case 4:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x40);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x10);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x80);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x40);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x20);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x10);
            break;
        case 5:
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x40);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x10);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x80);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x40);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x10);
            break;
        case 6:
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x40);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x10);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x80);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x40);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x10);
            break;
        case 7:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x40);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x10);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x80);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x40);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x20);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x10);
            break;
        case 8:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x40);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x10);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x80);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x40);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x10);
            break;
        case 9:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x40);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x10);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x80);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x40);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x20);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x10);
            break;
        case 0x0A:
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x40);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x20);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x10);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x80);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x40);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x20);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x10);
            break;
        case 0xff: // no display
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x40);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x20);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x10);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x80);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x40);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x20);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x10);
            break;
        default:
            break;
    }
}

/*
 * @brief: show centigrade in PCA8538
 *
 */
void PCA8538_temperature_show(s16 temp)
{
    u8 temp_val;
    u8 val[3];
    u8 flag = 0;
    
    temp_val = abs(temp);
    val[2]   = temp_val / 100;
    temp_val = temp_val % 100;
    val[1]   = temp_val / 10;
    temp_val = temp_val % 10;
    val[0]   = temp_val;
    
    if (temp < 0) {
        if (val[2] == 0) {
            if (val[1]) {
                // '-'
                PCA8538_set_T4_buffer(0);
            } else {
                // ' '
                PCA8538_set_T4_buffer(2);
            }
        } else {
            // '-1'
            PCA8538_set_T4_buffer(-1);
            flag = 1;
        }

        if (val[1] == 0) {
            if (flag == 0) {
                if (val[0]) {
                    // T3 '-'
                    PCA8538_set_T2T3_buffer(1, 0x0a);
                } else {
                    // T3 ' '
                    PCA8538_set_T2T3_buffer(1, 0xff);
                }
            } else {
                // T3 '0'
                PCA8538_set_T2T3_buffer(1, 0);
            }
        } else {
            PCA8538_set_T2T3_buffer(1, val[1]);
        }

        PCA8538_set_T2T3_buffer(0, val[0]);
    } else {
        if (val[2] == 0) {
            // ' '
            PCA8538_set_T4_buffer(2);
        } else {
            // '1'
            PCA8538_set_T4_buffer(1);
            flag = 1;
        }
        
        if (val[1] == 0) {
            if (flag == 0) {
                // ' '
                PCA8538_set_T2T3_buffer(1, 0xff);
            } else {
                // T3 '0'
                PCA8538_set_T2T3_buffer(1, 0);
            }
        } else {
            PCA8538_set_T2T3_buffer(1, val[1]);
        }

        PCA8538_set_T2T3_buffer(0, val[0]);
    }

    // 'C'
    PCA8538_set_T1_buffer(0);

    // display temperature in COG
    PCA8538_set_RAM_content(96, 101-96+1, &g_PCA8538_RAM_204[96]);
}

/**
 * @brief: set S1 S2 S3 S4 buffer
 * @param: index, S buffer number
 *         0 ----> S1
 *         1 ----> S2
 *         2 ----> S3
 *         3 ----> S4
 *         4 ----> :
 * @param: value, the number to display
 *
 */
static void PCA8538_set_S1_S4_buffer(u8 index, u8 value)
{
    u8 seg1, seg2;

    switch (index)
    {
        case 0:             // S1
            seg1 = 99;
            seg2 = 101;
            break;
        case 1:             // S2
            seg1 = 97;
            seg2 = 98;
            break;
        case 2:             // S3
            seg1 = 95;
            seg2 = 96;
            break;
        case 3:             // S4
            seg1 = 95;
            break;
        case 4:
            seg1 = 96;      // :
            break;
        default:
            return;
    }

    // S4
    if (index == 3) {
        switch (value)
        {
            case 0:         // ' '
                CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x80);
                CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x40);
                CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x20);
                break;
            case 1:         // '1'
                SET_BIT(g_PCA8538_RAM_204[seg1], 0x80);
                CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x40);
                SET_BIT(g_PCA8538_RAM_204[seg1], 0x20);
                break;
            case 2:         // '2'
                SET_BIT(g_PCA8538_RAM_204[seg1], 0x80);
                SET_BIT(g_PCA8538_RAM_204[seg1], 0x40);
                CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x20);
                break;
            default:
                break;
        }

        return;
    } else if (index == 4) {
        // ':'
        if (value)
        {
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x01);
        }
        else
        {
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x01);
        }
        
        return;
    }

    switch (value)
    {
        case 0:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x08);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x02);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x01);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x08);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x02);
            break;
        case 1:
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x08);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x04);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x02);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x01);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x08);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x02);
            break;
        case 2:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x08);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x04);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x02);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x01);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x08);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x02);
            break;
        case 3:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x08);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x04);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x02);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x01);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x08);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x02);
            break;
        case 4:
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x08);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x02);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x01);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x08);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x02);
            break;
        case 5:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x08);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x02);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x01);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x08);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x04);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x02);
            break;
        case 6:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x08);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x02);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x01);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x08);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x04);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x02);
            break;
        case 7:
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x08);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x04);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x02);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x01);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x08);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x02);
            break;
        case 8:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x08);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x02);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x01);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x08);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x02);
            break;
        case 9:
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x08);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x02);
            SET_BIT(g_PCA8538_RAM_204[seg1], 0x01);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x08);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x04);
            SET_BIT(g_PCA8538_RAM_204[seg2], 0x02);
            break;
        case 0xff: // no display
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x08);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x04);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x02);
            CLEAR_BIT(g_PCA8538_RAM_204[seg1], 0x01);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x08);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x04);
            CLEAR_BIT(g_PCA8538_RAM_204[seg2], 0x02);
            break;
        default:
            break;
    }
}

/**
 * @brief: set symbol in RAM.
 * @param: address, RAM address
 * @param: flag, 1 ---> show
 *               0 ---> shut down
 */
static void PCA8538_set_symbol(u8 address, u8 flag)
{
    u8 mask, mask_msb, mask_lsb;
    u8 buf[9];

    // symbol @address
    mask     = address;
    mask_msb = 0x80 | ((0x70 & mask) >> 4);
    mask_lsb = 0x90 | (0x0f & mask);

    buf[0] = 0x20;          // SUBADRESS
    buf[1] = 0x80;          // control byte
    buf[2] = mask_msb;      // Set Data pointer x-MSB
    buf[3] = 0x80;          // control byte
    buf[4] = mask_lsb;      // Set Data pointer x-LSB
    buf[5] = 0x80;          // control byte
    buf[6] = 0xA1;          // Set Data pointer y = 1
    buf[7] = 0x20;          // Write to DDRAM
    if (flag == 0) {
        buf[8] = 0x00;
    } else {
        buf[8] = 0x01;
    }

    // send
    if (wiringPiSPIDataRW(SPI_CHAN, buf, sizeof buf) == -1) {
        printf("SPI failure: %s\n", strerror(errno));
        return;
    }
}

/*
 * 6 7segment
 * @param: index, 7seg display number(0-5)
 * @param: value, value of display(0-9, 0xff)
 *                0x0a, '-'
 *                0xff, ' ' clear the display number
 */
static void PCA8538_set_7seg(u8 index, u8 value)
{
    int i;

    u8 mask, mask_msb, mask_lsb;
    
    u8 buf[15];
    memset(buf, 0x00, sizeof buf);

    // D1 @24
    mask     = 24 + 7*index;
    mask_msb = 0x80 | ((0x70 & mask) >> 4);
    mask_lsb = 0x90 | (0x0f & mask);

    buf[0] = 0x20;          // SUBADRESS
    buf[1] = 0x80;          // control byte
    buf[2] = mask_msb;      // Set Data pointer x-MSB
    buf[3] = 0x80;          // control byte
    buf[4] = mask_lsb;      // Set Data pointer x-LSB
    buf[5] = 0x80;          // control byte
    buf[6] = 0xA1;          // Set Data pointer y = 1
    buf[7] = 0x20;          // Write to DDRAM

    switch (value)
    {
        case 0:                     // '0'
            for (i = 0; i < 7; i++) {
                buf[8 + i] = 0x01;
            }
            buf[12] = 0x00;
            break;
        case 1:                     // '1'
            buf[8] = 0x01;
            buf[9] = 0x01;
            break;
        case 2:                     // '2'
            buf[9] = 0x01;
            buf[10] = 0x01;
            buf[12] = 0x01;
            buf[13] = 0x01;
            buf[14] = 0x01;
            break;
        case 3:                     // '3'
            buf[8] = 0x01;
            buf[9] = 0x01;
            buf[10] = 0x01;
            buf[12] = 0x01;
            buf[14] = 0x01;
            break;
        case 4:                     // '4'
            buf[8] = 0x01;
            buf[9] = 0x01;
            buf[11] = 0x01;
            buf[12] = 0x01;
            break;
        case 5:                     // '5'
            buf[8] = 0x01;
            buf[10] = 0x01;
            buf[11] = 0x01;
            buf[12] = 0x01;
            buf[14] = 0x01;
            break;
        case 6:                     // '6'
            buf[8] = 0x01;
            buf[10] = 0x01;
            buf[11] = 0x01;
            buf[12] = 0x01;
            buf[13] = 0x01;
            buf[14] = 0x01;
            break;
        case 7:                     // '7'
            buf[8] = 0x01;
            buf[9] = 0x01;
            buf[10] = 0x01;
            break;
        case 8:                     // '8'
            for (i = 0; i < 7; i++)
            {
                buf[i + 8] = 0x01;
            }
            break;
        case 9:                     // '9'
            buf[8] = 0x01;
            buf[9] = 0x01;
            buf[10] = 0x01;
            buf[11] = 0x01;
            buf[12] = 0x01;
            buf[14] = 0x01;
            break;
        case 0x0a:                  // '-'
            buf[12] = 0x01;
            break;
        case 0xff:                  // ' '
            break;
        default:
            break;
    }

    // send
    if (wiringPiSPIDataRW(SPI_CHAN, buf, sizeof buf) == -1) {
        printf("SPI failure: %s\n", strerror(errno));
        return;
    }
}

/**
 * @brief: show symbols included wrench, LED, S
 *         and so on.
 * @param: symbols, see PCA8538_symbols_t enum
 * @param: flag, 0 ---> clear display
 *               1 ---> display symbol
 */
void PCA8538_symbol_show(PCA8538_symbols_t symbols, u8 flag)
{
    switch (symbols)
    {
        case SYMBOL_AUTO:
            // AUTO @0
            PCA8538_set_symbol(0, flag);
            break;
        case SYMBOL_W:
            // W @1
            PCA8538_set_symbol(1, flag);
            break;
        case SYMBOL_S:
            // S @2
            PCA8538_set_symbol(2, flag);
            break;
        case SYMBOL_CITY:
            // CITY @3
            PCA8538_set_symbol(3, flag);
            break;
        case SYMBOL_E:
            // E @13
            PCA8538_set_symbol(13, flag);
            break;
        case SYMBOL_mi:
            // mi @17
            PCA8538_set_symbol(17, flag);
            break;
        case SYMBOL_km_m:
            // m(km) @18
            PCA8538_set_symbol(18, flag);
            break;
        case SYMBOL_km_k:
            // k(km) @23
            PCA8538_set_symbol(23, flag);
            break;
        case SYMBOL_WRENCH:
            // wrench @70
            PCA8538_set_symbol(70, flag);
            break;
        case SYMBOL_LED:
            // LED @87
            PCA8538_set_symbol(87, flag);
            break;
        default:
            break;
    }
}

void PCA8538_7seg_number_show(s32 number)
{
    u8  flag = 0;
    u32 temp;
    u8  value[6];
    int i;
    
    temp = abs(number);
    value[5] = temp / 100000;
    temp     = temp % 100000;
    value[4] = temp / 10000;
    temp     = temp % 10000;
    value[3] = temp / 1000;
    temp     = temp % 1000;
    value[2] = temp / 100;
    temp     = temp % 100;
    value[1] = temp / 10;
    value[0] = temp % 10;

    if (number >= 0)
    {
        for (i = 0; i < 6; i++)
        {
            if (value[5 - i] == 0)
            {
                if (flag == 0)
                {
                    if (i == 5)
                    {
                        PCA8538_set_7seg(5 - i, 0);
                    }
                    else
                    {
                        PCA8538_set_7seg(5 - i, 0xff);
                    }
                    
                }
                else
                {
                    PCA8538_set_7seg(5 - i, 0);
                }
            }
            else
            {
                flag = 1;
                PCA8538_set_7seg(5 - i, value[5 - i]);
            }
        }
    }
    else
    {
        for (i = 0; i < 6; i++)
        {
            if (value[5 - i] == 0)
            {
                if (flag == 0)
                {
                    // the next value is not '0', set '-' unless
                    // this is the last value
                    if (i != 5 && value[4 - i])
                        PCA8538_set_7seg(5 - i, 0x0a);      // '-'
                    else
                        PCA8538_set_7seg(5 - i, 0xff);      // ' '
                }
                else
                {
                    PCA8538_set_7seg(5 - i, 0);
                }
            }
            else
            {
                flag = 1;
                PCA8538_set_7seg(5 - i, value[5 - i]);
            }
        }
    }
}


void PCA8538_set_H1_7seg(u8 value)
{
    u8 buf[15];
    u8 mask, mask_msb, mask_lsb;
    int i;

    memset(buf, 0x00, sizeof buf);

    // H1 @88
    mask     = 88;
    mask_msb = 0x80 | ((0x70 & mask) >> 4);
    mask_lsb = 0x90 | (0x0f & mask);

    buf[0] = 0x20;          // SUBADRESS
    buf[1] = 0x80;          // control byte
    buf[2] = mask_msb;      // Set Data pointer x-MSB
    buf[3] = 0x80;          // control byte
    buf[4] = mask_lsb;      // Set Data pointer x-LSB
    buf[5] = 0x80;          // control byte
    buf[6] = 0xA1;          // Set Data pointer y = 1
    buf[7] = 0x20;          // Write to DDRAM

    switch (value)
    {
        case 0:
            for (i = 0; i < 5; i++)
            {
                buf[i + 8] = SET_ONE_BIT;
            }
            buf[14] = SET_ONE_BIT;
            break;
        case 1:
            buf[9] = SET_ONE_BIT;
            buf[10] = SET_ONE_BIT;
            break;
        case 2:
            buf[8] = SET_ONE_BIT;
            buf[10] = SET_ONE_BIT;
            buf[11] = SET_ONE_BIT;
            buf[13] = SET_ONE_BIT;
            buf[14] = SET_ONE_BIT;
            break;
        case 3:
            buf[8] = SET_ONE_BIT;
            buf[9] = SET_ONE_BIT;
            buf[10] = SET_ONE_BIT;
            buf[11] = SET_ONE_BIT;
            buf[13] = SET_ONE_BIT;
            break;
        case 4:
            buf[9] = SET_ONE_BIT;
            buf[10] = SET_ONE_BIT;
            buf[12] = SET_ONE_BIT;
            buf[13] = SET_ONE_BIT;
            break;
        case 5:
            buf[8] = SET_ONE_BIT;
            buf[9] = SET_ONE_BIT;
            buf[11] = SET_ONE_BIT;
            buf[12] = SET_ONE_BIT;
            buf[13] = SET_ONE_BIT;
            break;
        case 6:
            buf[8] = SET_ONE_BIT;
            buf[9] = SET_ONE_BIT;
            buf[11] = SET_ONE_BIT;
            buf[12] = SET_ONE_BIT;
            buf[13] = SET_ONE_BIT;
            buf[14] = SET_ONE_BIT;
            break;
        case 7:
            buf[9] = SET_ONE_BIT;
            buf[10] = SET_ONE_BIT;
            buf[11] = SET_ONE_BIT;
            break;
        case 8:
            for (i = 0; i < 7; i++)
            {
                buf[i + 8] = SET_ONE_BIT;
            }
            break;
        case 9:
            for (i = 0; i < 6; i++)
            {
                buf[i + 8] = SET_ONE_BIT;
            }
            break;
        case 0xff:
            break;
        default:
            break;
    }
}

/**
 * @brief: set time
 * @param: hour
 * @param: minute
 * @param: second_flag, 0 ----> clear ':'
 *                      1 ----> display ':'
 */
void PCA8538_set_time(u8 hour, u8 minute, u8 second_flag)
{
    u8 value;
    u8 temp;

    // hour
    temp  = hour;
    value = temp / 10;
    temp  = temp % 10;
    PCA8538_set_S1_S4_buffer(3, value);
    PCA8538_set_S1_S4_buffer(2, temp);

    // minute
    temp  = minute;
    value = temp / 10;
    temp  = temp % 10;
    PCA8538_set_S1_S4_buffer(1, value);
    PCA8538_set_S1_S4_buffer(0, temp);

    // second ':'
    PCA8538_set_S1_S4_buffer(4, second_flag);

    // display the RAM content in COG
    PCA8538_set_RAM_content(95, 7, &g_PCA8538_RAM_204[95]);
}

int main(void)
{
    u8 temp;
    wiringPiSetup();
    spiSetup(1000000);	// 1MHz
    
    init_PCA8538();
    
    PCA8538_read_temperature(&temp);
    printf("%d\n", temp);
    
    close(pca8538_fd);
    return 0;
}



