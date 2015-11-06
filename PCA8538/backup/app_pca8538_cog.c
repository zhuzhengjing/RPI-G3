/******************** (C) COPYRIGHT 2013 VINY **********************************
 * File Name          : app_pca8538_cog.c
 * Author             : Bruce.zhu
 * Date First Issued  : 2013/11/16
 * Description        : This file contains the software implementation for the
 *                      PCA8538 display unit
 *******************************************************************************
 * History:
 *------------------------------------------------------------------------------
 *    DATE    |  VER  |   AUTOR    | Description
 *------------------------------------------------------------------------------
 * 2013/11/16 | v1.0  | Bruce.zhu  | initial released
 *******************************************************************************/
#include "app_pca8538_cog.h"
#include "pca8538.h"
#include "trace.h"
#include "encoder.h"
#include "app_dc_motor.h"

#define PCA8538_PWM_LOAD_AREA1   0
#define PCA8538_PWM_LOAD_AREA2   71
#define PCA8538_LOGO_POS         28


static OS_STK pca8538_cog_task_stk[PCA8538_COG_TASK_STK_SIZE];

static void app_PCA8538_COG_task(void *p_arg);




void init_PCA8538_COG_task(void)
{
    INT8U os_err;

    os_err = OSTaskCreateExt((void (*)(void *)) app_PCA8538_COG_task,
                            (void 		 * ) 0,
                            (OS_STK		 * )&pca8538_cog_task_stk[PCA8538_COG_TASK_STK_SIZE - 1],
                            (INT8U		   ) PCA8538_COG_TASK_PRIO,
                            (INT16U		   ) PCA8538_COG_TASK_PRIO,
                            (OS_STK		 * )&pca8538_cog_task_stk[0],
                            (INT32U		   ) PCA8538_COG_TASK_STK_SIZE,
                            (void 		 * ) 0,
                            (INT16U		   )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
    assert_param(OS_ERR_NONE == os_err);
    OSTaskNameSet(PCA8538_COG_TASK_PRIO, (INT8U *)"PCA8538", &os_err);
}



/**
  * @brief: Enter auto test mode
  *
  */
void PCA8538_AUTO_test(void)
{
    int i;
    u8  second_flag = 1;

    APP_TRACE("Enter PCA8538 AUTO test mode...\r\n");
    // symbol
    for (i = 0; i < SYMBOL_NUM; i++)
    {
        PCA8538_symbol_show((PCA8538_symbols_t)i, 1);
        OSTimeDlyHMSM(0, 0, 0, 500);
        PCA8538_symbol_show((PCA8538_symbols_t)i, 0);
    }

    // time and H1 7segment
    for (i = 0; i < 10; i++)
    {
        PCA8538_set_time(24, 0 + i, second_flag);
        if (second_flag)
        {
            second_flag = 0;
        }
        else
        {
            second_flag = 1;
        }

        PCA8538_set_H1_7seg((u8)i);

        OSTimeDlyHMSM(0, 0, 0, 500);
    }

    APP_TRACE("PCA8538 AUTO test complete!\r\n");

}



static void get_ucos_run_time(u32* hour, u32* minute, u32* second)
{
    INT32U tick_time;

    tick_time = OSTimeGet();
    *hour   = (u32)(tick_time/(1000*3600));
    tick_time -= *hour*(1000*3600);
    *minute = (u32)(tick_time/(1000*60));
    tick_time -= *minute*(1000*60);
    *second = (u32)(tick_time/1000);
}



static void app_PCA8538_COG_task(void *p_arg)
{
    u8      temp = 0;
    double  temp_val;
    u32     hour, minute, second, second_flag = 1, sp_flag = 1;
    s32     dc_motor_sp[2];
    u32     dc_motor_adjust_value[2];
    u32     run_count = 0, sp_run_count = 0;

    init_PCA8538();
    APP_TRACE("PCA8538 start...\r\n");

    PCA8538_AUTO_test();
    PCA8538_VINY_logo(PCA8538_LOGO_POS);
    PCA8538_set_top_RAM_area(PCA8538_PWM_LOAD_AREA2, 0);
    PCA8538_set_top_RAM_area(PCA8538_PWM_LOAD_AREA1, 0);

    for (;;)
    {
        OSTimeDlyHMSM(0, 0, 0, 100);

        // DC motor speed and PWM dirve value
        dc_motor_sp[0] = get_current_dc_motor_speed(ENCODER_NUM1);
        dc_motor_sp[1] = get_current_dc_motor_speed(ENCODER_NUM2);

        // 7 seg number show DC motor speed
        if (sp_flag)
        {
            // No.1 DC motor
            PCA8538_set_H1_7seg(1);
            PCA8538_7seg_number_show(dc_motor_sp[0]);
        }
        else
        {
            // No.2 DC motor
            PCA8538_set_H1_7seg(2);
            PCA8538_7seg_number_show(dc_motor_sp[1]);
        }

        dc_motor_adjust_value[0] = get_adjust_pwm_percent(MOTOR_NUM1);
        dc_motor_adjust_value[1] = get_adjust_pwm_percent(MOTOR_NUM2);
        // DC motor PWM reload(0%-100%)
        PCA8538_set_top_RAM_area(PCA8538_PWM_LOAD_AREA2, (u8)dc_motor_adjust_value[1]);
        PCA8538_set_top_RAM_area(PCA8538_PWM_LOAD_AREA1, (u8)dc_motor_adjust_value[0]);

        // temperature
        PCA8538_read_temperature(&temp);
        // see <PCA8538.pdf>
        // 8.10.4 Temperature measurement and temperature compensation of VLCD
        temp_val = 0.6275 * temp - 40.0;
        PCA8538_temperature_show(temp_val);

        // show uC/OS run time
        get_ucos_run_time(&hour, &minute, &second);
        hour %= 23;

        PCA8538_set_time(hour, minute, second_flag);

        // run time flag
        if (++run_count == 5)
        {
            if (second_flag == 0)
                second_flag = 1;
            else
                second_flag = 0;

            run_count = 0;
        }

        if (++sp_run_count == 20)
        {
            if (sp_flag == 0)
                sp_flag = 1;
            else
                sp_flag = 0;

            sp_run_count = 0;
        }
    }
}





