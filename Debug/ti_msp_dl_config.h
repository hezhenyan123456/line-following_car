/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define CPUCLK_FREQ                                                     32000000



/* Defines for PWM_MOTOR */
#define PWM_MOTOR_INST                                                     TIMA1
#define PWM_MOTOR_INST_IRQHandler                               TIMA1_IRQHandler
#define PWM_MOTOR_INST_INT_IRQN                                 (TIMA1_INT_IRQn)
#define PWM_MOTOR_INST_CLK_FREQ                                         32000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_MOTOR_C0_PORT                                             GPIOB
#define GPIO_PWM_MOTOR_C0_PIN                                      DL_GPIO_PIN_4
#define GPIO_PWM_MOTOR_C0_IOMUX                                  (IOMUX_PINCM17)
#define GPIO_PWM_MOTOR_C0_IOMUX_FUNC                 IOMUX_PINCM17_PF_TIMA1_CCP0
#define GPIO_PWM_MOTOR_C0_IDX                                DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_MOTOR_C1_PORT                                             GPIOB
#define GPIO_PWM_MOTOR_C1_PIN                                      DL_GPIO_PIN_1
#define GPIO_PWM_MOTOR_C1_IOMUX                                  (IOMUX_PINCM13)
#define GPIO_PWM_MOTOR_C1_IOMUX_FUNC                 IOMUX_PINCM13_PF_TIMA1_CCP1
#define GPIO_PWM_MOTOR_C1_IDX                                DL_TIMER_CC_1_INDEX



/* Defines for TIMER_SYSTICK */
#define TIMER_SYSTICK_INST                                              (TIMG12)
#define TIMER_SYSTICK_INST_IRQHandler                          TIMG12_IRQHandler
#define TIMER_SYSTICK_INST_INT_IRQN                            (TIMG12_INT_IRQn)
#define TIMER_SYSTICK_INST_LOAD_VALUE                                   (31999U)



/* Defines for UART_OPENMV */
#define UART_OPENMV_INST                                                   UART1
#define UART_OPENMV_INST_FREQUENCY                                      32000000
#define UART_OPENMV_INST_IRQHandler                             UART1_IRQHandler
#define UART_OPENMV_INST_INT_IRQN                                 UART1_INT_IRQn
#define GPIO_UART_OPENMV_RX_PORT                                           GPIOA
#define GPIO_UART_OPENMV_TX_PORT                                           GPIOA
#define GPIO_UART_OPENMV_RX_PIN                                    DL_GPIO_PIN_9
#define GPIO_UART_OPENMV_TX_PIN                                    DL_GPIO_PIN_8
#define GPIO_UART_OPENMV_IOMUX_RX                                (IOMUX_PINCM20)
#define GPIO_UART_OPENMV_IOMUX_TX                                (IOMUX_PINCM19)
#define GPIO_UART_OPENMV_IOMUX_RX_FUNC                 IOMUX_PINCM20_PF_UART1_RX
#define GPIO_UART_OPENMV_IOMUX_TX_FUNC                 IOMUX_PINCM19_PF_UART1_TX
#define UART_OPENMV_BAUD_RATE                                           (115200)
#define UART_OPENMV_IBRD_32_MHZ_115200_BAUD                                 (17)
#define UART_OPENMV_FBRD_32_MHZ_115200_BAUD                                 (23)





/* Port definition for Pin Group GPIO_MOTOR */
#define GPIO_MOTOR_PORT                                                  (GPIOB)

/* Defines for AIN1: GPIOB.12 with pinCMx 29 on package pin 64 */
#define GPIO_MOTOR_AIN1_PIN                                     (DL_GPIO_PIN_12)
#define GPIO_MOTOR_AIN1_IOMUX                                    (IOMUX_PINCM29)
/* Defines for AIN2: GPIOB.17 with pinCMx 43 on package pin 14 */
#define GPIO_MOTOR_AIN2_PIN                                     (DL_GPIO_PIN_17)
#define GPIO_MOTOR_AIN2_IOMUX                                    (IOMUX_PINCM43)
/* Defines for BIN1: GPIOB.15 with pinCMx 32 on package pin 3 */
#define GPIO_MOTOR_BIN1_PIN                                     (DL_GPIO_PIN_15)
#define GPIO_MOTOR_BIN1_IOMUX                                    (IOMUX_PINCM32)
/* Defines for BIN2: GPIOB.16 with pinCMx 33 on package pin 4 */
#define GPIO_MOTOR_BIN2_PIN                                     (DL_GPIO_PIN_16)
#define GPIO_MOTOR_BIN2_IOMUX                                    (IOMUX_PINCM33)
/* Defines for STBY: GPIOB.13 with pinCMx 30 on package pin 1 */
#define GPIO_MOTOR_STBY_PIN                                     (DL_GPIO_PIN_13)
#define GPIO_MOTOR_STBY_IOMUX                                    (IOMUX_PINCM30)
/* Port definition for Pin Group GPIO_ENCODER_LEFT */
#define GPIO_ENCODER_LEFT_PORT                                           (GPIOB)

/* Defines for ENC_L_A: GPIOB.0 with pinCMx 12 on package pin 47 */
// groups represented: ["GPIO_ENCODER_RIGHT","GPIO_ENCODER_LEFT"]
// pins affected: ["ENC_R_A","ENC_L_A"]
#define GPIO_MULTIPLE_GPIOB_INT_IRQN                            (GPIOB_INT_IRQn)
#define GPIO_MULTIPLE_GPIOB_INT_IIDX            (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define GPIO_ENCODER_LEFT_ENC_L_A_IIDX                       (DL_GPIO_IIDX_DIO0)
#define GPIO_ENCODER_LEFT_ENC_L_A_PIN                            (DL_GPIO_PIN_0)
#define GPIO_ENCODER_LEFT_ENC_L_A_IOMUX                          (IOMUX_PINCM12)
/* Defines for ENC_L_B: GPIOB.6 with pinCMx 23 on package pin 58 */
#define GPIO_ENCODER_LEFT_ENC_L_B_PIN                            (DL_GPIO_PIN_6)
#define GPIO_ENCODER_LEFT_ENC_L_B_IOMUX                          (IOMUX_PINCM23)
/* Port definition for Pin Group GPIO_ENCODER_RIGHT */
#define GPIO_ENCODER_RIGHT_PORT                                          (GPIOB)

/* Defines for ENC_R_A: GPIOB.7 with pinCMx 24 on package pin 59 */
#define GPIO_ENCODER_RIGHT_ENC_R_A_IIDX                      (DL_GPIO_IIDX_DIO7)
#define GPIO_ENCODER_RIGHT_ENC_R_A_PIN                           (DL_GPIO_PIN_7)
#define GPIO_ENCODER_RIGHT_ENC_R_A_IOMUX                         (IOMUX_PINCM24)
/* Defines for ENC_R_B: GPIOB.8 with pinCMx 25 on package pin 60 */
#define GPIO_ENCODER_RIGHT_ENC_R_B_PIN                           (DL_GPIO_PIN_8)
#define GPIO_ENCODER_RIGHT_ENC_R_B_IOMUX                         (IOMUX_PINCM25)
/* Defines for S1: GPIOA.18 with pinCMx 40 on package pin 11 */
#define GPIO_KEYS_S1_PORT                                                (GPIOA)
#define GPIO_KEYS_S1_PIN                                        (DL_GPIO_PIN_18)
#define GPIO_KEYS_S1_IOMUX                                       (IOMUX_PINCM40)
/* Defines for S2: GPIOB.21 with pinCMx 49 on package pin 20 */
#define GPIO_KEYS_S2_PORT                                                (GPIOB)
#define GPIO_KEYS_S2_PIN                                        (DL_GPIO_PIN_21)
#define GPIO_KEYS_S2_IOMUX                                       (IOMUX_PINCM49)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_MOTOR_init(void);
void SYSCFG_DL_TIMER_SYSTICK_init(void);
void SYSCFG_DL_UART_OPENMV_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
