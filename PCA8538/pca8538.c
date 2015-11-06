
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

#define SPI_CHAN 0

static int pca8538_fd;

// RAM definition
// address: 0 - 203
static u8 g_PCA8538_RAM_204[204];

static void PCA8538_set_RAM_content(u8 address, u8 length, u8* content);


void spiSetup (int speed)
{
  if ((pca8538_fd = wiringPiSPISetup (SPI_CHAN, speed)) < 0)
  {
    fprintf (stderr, "Can't open the SPI bus: %s\n", strerror (errno)) ;
    exit (EXIT_FAILURE);
  }
}


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

