/******************** (C) COPYRIGHT 2013 VINY **********************************
 * File Name          : pca8538.h
 * Author             : Bruce.zhu
 * Date First Issued  : 2013/11/14
 * Description        : This file contains the software implementation for the
 *                      PCA8538 NXP COG driver unit
 *******************************************************************************
 * History:
 *------------------------------------------------------------------------------
 *    DATE    |  VER  |   AUTOR    | Description
 *------------------------------------------------------------------------------
 * 2013/11/14 | v1.0  | Bruce.zhu  | initial released
 *******************************************************************************/
#ifndef _PCA8538_H_
#define _PCA8538_H_

#include "stm32f2xx.h"


typedef enum
{
    SYMBOL_AUTO,
    SYMBOL_W,
    SYMBOL_S,
    SYMBOL_CITY,
    SYMBOL_E,
    SYMBOL_mi,
    SYMBOL_km_m,
    SYMBOL_km_k,
    SYMBOL_WRENCH,
    SYMBOL_LED,
    SYMBOL_NUM
}PCA8538_symbols_t;


void init_PCA8538(void);
void PCA8538_temperature_show(s16 temp);
void PCA8538_read_temperature(u8* temp);
void PCA8538_7seg_number_show(s32 number);
void PCA8538_symbol_show(PCA8538_symbols_t symbols, u8 flag);
void PCA8538_set_H1_7seg(u8 value);
void PCA8538_set_time(u8 hour, u8 minute, u8 second_flag);
void PCA8538_set_top_RAM_area(u8 start, u8 load);
void PCA8538_VINY_logo(u8 start);






#endif /* _PCA8538_H_ */

