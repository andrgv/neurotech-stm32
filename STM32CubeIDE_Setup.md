# STM32F411RE TIM1 Complementary PWM Setup

This repo now contains a self-contained firmware implementation in [Src/main.c](C:\Users\andre\Downloads\neurotech\Src\main.c) that drives TIM1 complementary PWM on `PA8` and `PB13`.

For the equivalent STM32CubeIDE / CubeMX project:

1. Install `STM32CubeIDE`.
2. Create a new project for `NUCLEO-F411RE` or `STM32F411RETx`.
3. In CubeMX pinout:
   `PA8` -> `TIM1_CH1`
   `PB13` -> `TIM1_CH1N`
4. In Clock Configuration:
   Set `SYSCLK = 100 MHz`.
5. In `TIM1` PWM configuration:
   Prescaler `PSC = 99`
   Counter Period `ARR = 39999`
   Pulse `CCR1 = 300`
   This yields `25 Hz` with a `300 us` pulse width.
6. After code generation, add these two lines after `MX_TIM1_Init();`

```c
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
```

Scope expectation:
`PA8` shows the main PWM pulse.
`PB13` shows the complementary output from `TIM1_CH1N`.
