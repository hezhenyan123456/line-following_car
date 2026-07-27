# Project Handoff

This file is the persistent context anchor for future Codex sessions.

## Project

- CCS Theia / TI DriverLib project: `empty_LP_MSPM0G3507_nortos_ticlang`
- Device: MSPM0G3507, package LQFP-64(PM)
- SDK product: `mspm0_sdk@2.11.00.07`
- SysConfig source: `empty.syscfg`
- Main editable application source: `empty.c`
- Generated/build output: `Debug/` is inspection-only. Do not hand-edit generated files such as `Debug/ti_msp_dl_config.c`, `Debug/ti_msp_dl_config.h`, linker files, objects, maps, or `.out`.

## Current Firmware Shape

The firmware is a line-following car skeleton using:

- OpenMV UART input on `UART1`, 115200 8N1, RX interrupt enabled.
- Motor driver GPIO direction pins and standby pin.
- TIMA1 edge-aligned PWM with period count 1600 for two motor channels.
- TIMG12 1 ms periodic interrupt as the system tick.
- Two quadrature encoder A-channel rising-edge GPIO interrupts, sampling B level for direction.
- Two keys: S1 toggles run/stop, S2 increments `g_debugMode`.

The main loop:

1. Calls `SYSCFG_DL_init()` and `appInit()`.
2. Services completed OpenMV UART lines.
3. Runs `controlTask10ms()` when the 1 ms timer marks a 10 ms control period.
4. Sleeps with `__WFI()`.

## Vision UART Protocol

`empty.c` currently parses newline-terminated ASCII frames in this exact form:

```text
L:<line_seen>,E:<error>
```

Examples:

```text
L:1,E:0
L:1,E:-80
L:0,E:0
```

`line_seen` is treated as true when nonzero. `error` is signed int16. Frames older than 200 ms, invalid frames, or line-lost frames cause motor stop.

## Pin Assignments

Motor driver:

| Signal | Pin |
| --- | --- |
| AIN1 | PB12 |
| AIN2 | PB17 |
| BIN1 | PB15 |
| BIN2 | PB16 |
| STBY | PB13 |
| PWMB / TIMA1 CCP0 | PB4 |
| PWMA / TIMA1 CCP1 | PB1 |

Encoders:

| Signal | Pin |
| --- | --- |
| ENC_L_A interrupt | PB0 |
| ENC_L_B | PB6 |
| ENC_R_A interrupt | PB7 |
| ENC_R_B | PB8 |

Keys and UART:

| Signal | Pin |
| --- | --- |
| S1 | PA18 |
| S2 | PB21 |
| UART1 TX | PA8 |
| UART1 RX | PA9 |

Debug:

| Signal | Pin |
| --- | --- |
| SWCLK | PA20 |
| SWDIO | PA19 |

## Control Constants

Defined in `empty.c`:

| Constant | Value | Meaning |
| --- | --- | --- |
| `CONTROL_PERIOD_MS` | 10 | Control loop period |
| `VISION_TIMEOUT_MS` | 200 | Stop if vision data is stale |
| `PWM_PERIOD_COUNTS` | 1600 | Must match SysConfig PWM timer count |
| `BASE_DUTY_PERCENT` | 25 | Nominal forward duty |
| `MAX_DUTY_PERCENT` | 60 | Motor duty clamp |
| `MAX_STEER_PERCENT` | 25 | Steering correction clamp |
| `KEY_DEBOUNCE_MS` | 30 | Key debounce time |

Current steering law:

```c
steer = (error / 8) + (errorDelta / 16);
leftDuty  = BASE_DUTY_PERCENT + steer;
rightDuty = BASE_DUTY_PERCENT - steer;
```

## Validation Notes

- SysConfig CLI validation was run successfully with SysConfig 1.28.0 against SDK 2.11.00.07.
- SysConfig emitted one informational warning: PWM registers are not retained in STOP/STANDBY modes. The current firmware does not intentionally enter those modes.
- `targetConfigs/MSPM0G3507.ccxml` is configured for a Texas Instruments XDS110 USB Debug Probe.
- The local `.git` directory is currently empty, so this folder does not have usable Git history yet.

## Safe Development Rules

- Treat `empty.syscfg` as the source of truth for peripheral, pin, interrupt, clock, and generated-name configuration.
- Read `Debug/ti_msp_dl_config.h` before using generated macro or IRQ names.
- Keep application logic edits in `empty.c` unless a larger structure is deliberately introduced.
- Regenerate SysConfig output after changing `empty.syscfg`.
- Ask before changing the MCU, package, SDK version, debug probe, or important hardware pins.
- For hardware flashing/debugging, detect or confirm the physical probe first.
