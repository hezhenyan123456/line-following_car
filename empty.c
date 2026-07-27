/*
 * Copyright (c) 2021, Texas Instruments Incorporated
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

#include "ti_msp_dl_config.h"

#include <stdbool.h>
#include <stdint.h>

#define CONTROL_PERIOD_MS                 (10U)
#define VISION_TIMEOUT_MS                 (500U)
#define PWM_PERIOD_COUNTS                 (1600U)
#define BASE_DUTY_PERCENT                 (40)
#define MAX_DUTY_PERCENT                  (70)
#define MAX_STEER_PERCENT                 (30)
#define VISION_LOST_GRACE_MS              (800U)
#define SEARCH_DUTY_PERCENT               (28)
/*
 * Give the motors enough time and duty to overcome static friction while
 * waiting for the first valid OpenMV frame.
 */
#define STARTUP_DRIVE_MS                  (3000U)
#define STARTUP_DUTY_PERCENT              (35)
#define KEY_DEBOUNCE_MS                   (30U)
#define UART_LINE_BUFFER_SIZE             (32U)
#define UART_RX_RING_SIZE                 (256U)
#define PWM_SCOPE_TEST                    (0)
/* Normal line-following mode: motor control uses OpenMV vision frames. */
#define MOTOR_OPEN_LOOP_TEST              (0)
#define MOTOR_TEST_DUTY_PERCENT           (40)

typedef struct {
    bool lineSeen;
    int16_t error;
    uint32_t lastUpdateMs;
    bool valid;
} VisionFrame;

typedef struct {
    int32_t leftCount;
    int32_t rightCount;
    int16_t leftDelta;
    int16_t rightDelta;
} EncoderSample;

static volatile uint32_t g_msTicks;
static volatile bool g_controlDue;
static volatile int32_t g_leftEncoderCount;
static volatile int32_t g_rightEncoderCount;

static volatile uint8_t g_uartRxRing[UART_RX_RING_SIZE];
static volatile uint16_t g_uartRxHead;
static volatile uint16_t g_uartRxTail;

static VisionFrame g_vision;
static volatile EncoderSample g_encoder;
static volatile bool g_carRunning;
static volatile uint8_t g_debugMode;
static volatile int16_t g_lastLineError;
static volatile uint32_t g_startupDriveUntilMs;
static volatile uint32_t g_lastLineSeenMs;

static bool parseVisionLine(const char *line, VisionFrame *frame);
static bool parseSignedInt(const char **text, int16_t *value);
static void servicePendingVisionFrame(void);
static bool uartRingPop(uint8_t *value);
static void appInit(void);
static void controlTask10ms(void);
static void encoderSample10ms(void);
static void keysScan1ms(void);
static void motorsStop(void);
static void motorSetLeft(int16_t dutyPercent);
static void motorSetRight(int16_t dutyPercent);
static void motorSetPwm(uint32_t ccIndex, int16_t dutyPercent);
static void motorSetStandby(bool enabled);
static int16_t clampInt16(int16_t value, int16_t minValue, int16_t maxValue);
static bool keyS1IsPressed(void);
static bool keyS2IsPressed(void);

int main(void)
{
    SYSCFG_DL_init();
    appInit();

    while (1) {
        servicePendingVisionFrame();

        if (g_controlDue) {
            g_controlDue = false;
            controlTask10ms();
        }

        __WFI();
    }
}

static void appInit(void)
{
    motorsStop();

    NVIC_ClearPendingIRQ(GPIO_MULTIPLE_GPIOB_INT_IRQN);
    NVIC_ClearPendingIRQ(TIMER_SYSTICK_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(UART_OPENMV_INST_INT_IRQN);
    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOB_INT_IRQN);
    NVIC_EnableIRQ(TIMER_SYSTICK_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_OPENMV_INST_INT_IRQN);

    DL_TimerA_startCounter(PWM_MOTOR_INST);
    DL_TimerG_startCounter(TIMER_SYSTICK_INST);

    __enable_irq();
}

static void controlTask10ms(void)
{
    int16_t leftDuty;
    int16_t rightDuty;
    int16_t steer;
    int16_t errorDelta;
    uint32_t nowMs = g_msTicks;

    encoderSample10ms();

#if PWM_SCOPE_TEST
    motorSetLeft(BASE_DUTY_PERCENT);
    motorSetRight(BASE_DUTY_PERCENT);
    return;
#endif

#if MOTOR_OPEN_LOOP_TEST
    if (g_carRunning) {
        motorSetLeft(MOTOR_TEST_DUTY_PERCENT);
        motorSetRight(MOTOR_TEST_DUTY_PERCENT);
    } else {
        motorsStop();
    }
    return;
#endif

    if (!g_carRunning) {
        motorsStop();
        return;
    }

    if ((!g_vision.valid) || (!g_vision.lineSeen) ||
        ((nowMs - g_vision.lastUpdateMs) > VISION_TIMEOUT_MS)) {
        if (nowMs < g_startupDriveUntilMs) {
            motorSetLeft(STARTUP_DUTY_PERCENT);
            motorSetRight(STARTUP_DUTY_PERCENT);
            return;
        }

        if (g_lastLineSeenMs != 0U) {
            uint32_t lostMs = nowMs - g_lastLineSeenMs;
            if (lostMs < VISION_LOST_GRACE_MS) {
                int16_t searchDuty = SEARCH_DUTY_PERCENT;

                if (g_lastLineError > 0) {
                    motorSetLeft(searchDuty / 2);
                    motorSetRight(searchDuty);
                } else if (g_lastLineError < 0) {
                    motorSetLeft(searchDuty);
                    motorSetRight(searchDuty / 2);
                } else {
                    motorSetLeft(searchDuty);
                    motorSetRight(searchDuty);
                }
                return;
            }
        }

        motorsStop();
        return;
    }

    g_startupDriveUntilMs = 0;
    g_lastLineSeenMs = nowMs;

    errorDelta = g_vision.error - g_lastLineError;
    g_lastLineError = g_vision.error;

    /* Positive error means the line is on the right. */
    steer = (g_vision.error / 4) + (errorDelta / 10);
    steer = clampInt16(steer, -MAX_STEER_PERCENT, MAX_STEER_PERCENT);

    leftDuty  = clampInt16(BASE_DUTY_PERCENT - steer, 0, MAX_DUTY_PERCENT);
    rightDuty = clampInt16(BASE_DUTY_PERCENT + steer, 0, MAX_DUTY_PERCENT);

    motorSetLeft(leftDuty);
    motorSetRight(rightDuty);
}

static void encoderSample10ms(void)
{
    static int32_t lastLeftCount;
    static int32_t lastRightCount;
    int32_t leftCount;
    int32_t rightCount;

    __disable_irq();
    leftCount  = g_leftEncoderCount;
    rightCount = g_rightEncoderCount;
    __enable_irq();

    g_encoder.leftDelta  = (int16_t)(leftCount - lastLeftCount);
    g_encoder.rightDelta = (int16_t)(rightCount - lastRightCount);
    g_encoder.leftCount  = leftCount;
    g_encoder.rightCount = rightCount;

    lastLeftCount  = leftCount;
    lastRightCount = rightCount;
}

static void keysScan1ms(void)
{
    static bool initialized;
    static bool lastS1Raw;
    static bool lastS2Raw;
    static bool stableS1;
    static bool stableS2;
    static uint8_t s1StableMs;
    static uint8_t s2StableMs;

    bool s1Raw = keyS1IsPressed();
    bool s2Raw = keyS2IsPressed();

    if (!initialized) {
        initialized = true;
        lastS1Raw = s1Raw;
        lastS2Raw = s2Raw;
        stableS1 = s1Raw;
        stableS2 = s2Raw;
        return;
    }

    if (s1Raw == lastS1Raw) {
        if (s1StableMs < KEY_DEBOUNCE_MS) {
            s1StableMs++;
        }
    } else {
        s1StableMs = 0;
        lastS1Raw  = s1Raw;
    }

    if ((s1StableMs >= KEY_DEBOUNCE_MS) && (stableS1 != s1Raw)) {
        stableS1 = s1Raw;
        if (stableS1) {
            g_carRunning = !g_carRunning;
            g_lastLineError = 0;
            if (g_carRunning) {
                g_startupDriveUntilMs = g_msTicks + STARTUP_DRIVE_MS;
            } else {
                g_startupDriveUntilMs = 0;
            }
        }
    }

    if (s2Raw == lastS2Raw) {
        if (s2StableMs < KEY_DEBOUNCE_MS) {
            s2StableMs++;
        }
    } else {
        s2StableMs = 0;
        lastS2Raw  = s2Raw;
    }

    if ((s2StableMs >= KEY_DEBOUNCE_MS) && (stableS2 != s2Raw)) {
        stableS2 = s2Raw;
        if (stableS2) {
            g_debugMode++;
        }
    }
}

static void servicePendingVisionFrame(void)
{
    static char line[UART_LINE_BUFFER_SIZE];
    static uint8_t lineIndex;
    static bool lineOverflow;
    uint8_t rxData;

    while (uartRingPop(&rxData)) {
        if (rxData == '\r') {
            continue;
        }

        if (rxData == '\n') {
            if (!lineOverflow) {
                VisionFrame parsed;

                line[lineIndex] = '\0';
                if (parseVisionLine(line, &parsed)) {
                    parsed.lastUpdateMs = g_msTicks;
                    parsed.valid = true;
                    g_vision = parsed;
                }
            }
            lineIndex = 0;
            lineOverflow = false;
        } else if (!lineOverflow) {
            if (lineIndex < (UART_LINE_BUFFER_SIZE - 1U)) {
                line[lineIndex++] = (char)rxData;
            } else {
                lineIndex = 0;
                lineOverflow = true;
            }
        }
    }
}

static bool uartRingPop(uint8_t *value)
{
    uint16_t tail;

    __disable_irq();
    if (g_uartRxTail == g_uartRxHead) {
        __enable_irq();
        return false;
    }

    tail = g_uartRxTail;
    *value = g_uartRxRing[tail];
    g_uartRxTail = (uint16_t)((tail + 1U) % UART_RX_RING_SIZE);
    __enable_irq();

    return true;
}

static bool parseVisionLine(const char *line, VisionFrame *frame)
{
    int16_t errorValue;
    int16_t steeringValue;
    int16_t visibleCount;

    /*
     * Current OpenMV output:
     *   LINE,error,steering,visible_count
     *   LOST,lost_time_ms
     */
    if ((line[0] == 'L') && (line[1] == 'I') &&
        (line[2] == 'N') && (line[3] == 'E') &&
        (line[4] == ',')) {
        line += 5;
        if (!parseSignedInt(&line, &errorValue)) {
            return false;
        }
        if (*line != ',') {
            return false;
        }
        line++;
        if (!parseSignedInt(&line, &steeringValue)) {
            return false;
        }
        if (*line != ',') {
            return false;
        }
        line++;
        if (!parseSignedInt(&line, &visibleCount)) {
            return false;
        }
        if (visibleCount <= 0) {
            return false;
        }

        frame->lineSeen = true;
        frame->error = errorValue;
    } else if ((line[0] == 'L') && (line[1] == 'O') &&
               (line[2] == 'S') && (line[3] == 'T') &&
               (line[4] == ',')) {
        frame->lineSeen = false;
        frame->error = 0;
    } else if ((line[0] == 'L') && (line[1] == ':')) {
        /* Backward-compatible support for L:1,E:-20 frames. */
        int16_t lineSeenValue;

        line += 2;
        if (!parseSignedInt(&line, &lineSeenValue)) {
            return false;
        }
        if ((line[0] != ',') || (line[1] != 'E') || (line[2] != ':')) {
            return false;
        }
        line += 3;
        if (!parseSignedInt(&line, &errorValue)) {
            return false;
        }

        frame->lineSeen = (lineSeenValue != 0);
        frame->error = errorValue;
    } else {
        return false;
    }

    frame->lastUpdateMs = 0;
    frame->valid = true;

    return true;
}

static bool parseSignedInt(const char **text, int16_t *value)
{
    const char *p = *text;
    int32_t result = 0;
    int32_t sign = 1;
    bool hasDigit = false;

    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    while ((*p >= '0') && (*p <= '9')) {
        hasDigit = true;
        result = (result * 10) + (*p - '0');
        if (result > 32767) {
            return false;
        }
        p++;
    }

    if (!hasDigit) {
        return false;
    }

    result *= sign;
    if ((result < -32768) || (result > 32767)) {
        return false;
    }

    *value = (int16_t)result;
    *text = p;
    return true;
}

static void motorsStop(void)
{
    motorSetLeft(0);
    motorSetRight(0);
    motorSetStandby(false);
}

static void motorSetLeft(int16_t dutyPercent)
{
    if (dutyPercent > 0) {
        motorSetStandby(true);
        DL_GPIO_setPins(GPIO_MOTOR_PORT, GPIO_MOTOR_AIN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_AIN2_PIN);
        motorSetPwm(GPIO_PWM_MOTOR_C1_IDX, dutyPercent);
    } else if (dutyPercent < 0) {
        motorSetStandby(true);
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_AIN1_PIN);
        DL_GPIO_setPins(GPIO_MOTOR_PORT, GPIO_MOTOR_AIN2_PIN);
        motorSetPwm(GPIO_PWM_MOTOR_C1_IDX, -dutyPercent);
    } else {
        DL_GPIO_clearPins(GPIO_MOTOR_PORT,
            GPIO_MOTOR_AIN1_PIN | GPIO_MOTOR_AIN2_PIN);
        motorSetPwm(GPIO_PWM_MOTOR_C1_IDX, 0);
    }
}

static void motorSetRight(int16_t dutyPercent)
{
    if (dutyPercent > 0) {
        motorSetStandby(true);
        DL_GPIO_setPins(GPIO_MOTOR_PORT, GPIO_MOTOR_BIN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_BIN2_PIN);
        motorSetPwm(GPIO_PWM_MOTOR_C0_IDX, dutyPercent);
    } else if (dutyPercent < 0) {
        motorSetStandby(true);
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_BIN1_PIN);
        DL_GPIO_setPins(GPIO_MOTOR_PORT, GPIO_MOTOR_BIN2_PIN);
        motorSetPwm(GPIO_PWM_MOTOR_C0_IDX, -dutyPercent);
    } else {
        DL_GPIO_clearPins(GPIO_MOTOR_PORT,
            GPIO_MOTOR_BIN1_PIN | GPIO_MOTOR_BIN2_PIN);
        motorSetPwm(GPIO_PWM_MOTOR_C0_IDX, 0);
    }
}

static void motorSetPwm(uint32_t ccIndex, int16_t dutyPercent)
{
    uint32_t compareValue;
    int16_t limitedDuty = clampInt16(dutyPercent, 0, 100);

    compareValue = PWM_PERIOD_COUNTS -
        ((PWM_PERIOD_COUNTS * (uint32_t)limitedDuty) / 100U);
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, compareValue, ccIndex);
}

static void motorSetStandby(bool enabled)
{
    if (enabled) {
        DL_GPIO_setPins(GPIO_MOTOR_PORT, GPIO_MOTOR_STBY_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_STBY_PIN);
    }
}

static int16_t clampInt16(int16_t value, int16_t minValue, int16_t maxValue)
{
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

static bool keyS1IsPressed(void)
{
    /* The LaunchPad S1 button drives PA18 high when pressed. */
    return ((DL_GPIO_readPins(GPIO_KEYS_S1_PORT, GPIO_KEYS_S1_PIN) &
             GPIO_KEYS_S1_PIN) != 0U);
}

static bool keyS2IsPressed(void)
{
    return ((DL_GPIO_readPins(GPIO_KEYS_S2_PORT, GPIO_KEYS_S2_PIN) &
             GPIO_KEYS_S2_PIN) == 0U);
}

void TIMER_SYSTICK_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_SYSTICK_INST)) {
        case DL_TIMER_IIDX_ZERO:
            g_msTicks++;
            keysScan1ms();
            if ((g_msTicks % CONTROL_PERIOD_MS) == 0U) {
                g_controlDue = true;
            }
            break;
        default:
            break;
    }
}

void UART_OPENMV_INST_IRQHandler(void)
{
    uint8_t rxData;

    switch (DL_UART_Main_getPendingInterrupt(UART_OPENMV_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            /*
             * Drain the whole FIFO. OpenMV sends short frames continuously;
             * consuming only one byte per IRQ can leave stale bytes queued
             * and eventually break frame alignment.
             */
            while (!DL_UART_Main_isRXFIFOEmpty(UART_OPENMV_INST)) {
                rxData = DL_UART_Main_receiveData(UART_OPENMV_INST);

                uint16_t nextHead = (uint16_t)(
                    (g_uartRxHead + 1U) % UART_RX_RING_SIZE);
                if (nextHead != g_uartRxTail) {
                    g_uartRxRing[g_uartRxHead] = rxData;
                    g_uartRxHead = nextHead;
                }
            }
            break;
        default:
            break;
    }
}

void GROUP1_IRQHandler(void)
{
    DL_GPIO_IIDX pendingGpio;

    switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)) {
        case GPIO_MULTIPLE_GPIOB_INT_IIDX:
            pendingGpio = DL_GPIO_getPendingInterrupt(GPIOB);
            if (pendingGpio == GPIO_ENCODER_LEFT_ENC_L_A_IIDX) {
                if ((DL_GPIO_readPins(GPIO_ENCODER_LEFT_PORT,
                         GPIO_ENCODER_LEFT_ENC_L_B_PIN) &
                        GPIO_ENCODER_LEFT_ENC_L_B_PIN) != 0U) {
                    g_leftEncoderCount++;
                } else {
                    g_leftEncoderCount--;
                }
            } else if (pendingGpio == GPIO_ENCODER_RIGHT_ENC_R_A_IIDX) {
                if ((DL_GPIO_readPins(GPIO_ENCODER_RIGHT_PORT,
                         GPIO_ENCODER_RIGHT_ENC_R_B_PIN) &
                        GPIO_ENCODER_RIGHT_ENC_R_B_PIN) != 0U) {
                    g_rightEncoderCount--;
                } else {
                    g_rightEncoderCount++;
                }
            }
            break;
        default:
            break;
    }
}
