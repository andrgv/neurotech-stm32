#include <stdint.h>

typedef struct
{
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
    uint32_t RESERVED3[2];
    volatile uint32_t AHB1LPENR;
    volatile uint32_t AHB2LPENR;
    volatile uint32_t AHB3LPENR;
    uint32_t RESERVED4;
    volatile uint32_t APB1LPENR;
    volatile uint32_t APB2LPENR;
    uint32_t RESERVED5[2];
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
    uint32_t RESERVED6[2];
    volatile uint32_t SSCGR;
    volatile uint32_t PLLI2SCFGR;
    volatile uint32_t DCKCFGR;
} RCC_TypeDef;

typedef struct
{
    volatile uint32_t ACR;
    volatile uint32_t KEYR;
    volatile uint32_t OPTKEYR;
    volatile uint32_t SR;
    volatile uint32_t CR;
    volatile uint32_t OPTCR;
} FLASH_TypeDef;

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t CSR;
} PWR_TypeDef;

typedef struct
{
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t RCR;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    volatile uint32_t BDTR;
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
    volatile uint32_t OR;
} TIM_TypeDef;

typedef struct
{
    TIM_TypeDef *Instance;
} TIM_HandleTypeDef;

#define PERIPH_BASE              0x40000000UL
#define APB1PERIPH_BASE          PERIPH_BASE
#define APB2PERIPH_BASE          (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE          (PERIPH_BASE + 0x00020000UL)
#define RCC_BASE                 (AHB1PERIPH_BASE + 0x3800UL)
#define GPIOA_BASE               (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE               (AHB1PERIPH_BASE + 0x0400UL)
#define FLASH_R_BASE             0x40023C00UL
#define PWR_BASE                 (APB1PERIPH_BASE + 0x7000UL)
#define TIM1_BASE                APB2PERIPH_BASE

#define GPIOA                    ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB                    ((GPIO_TypeDef *)GPIOB_BASE)
#define RCC                      ((RCC_TypeDef *)RCC_BASE)
#define FLASH                    ((FLASH_TypeDef *)FLASH_R_BASE)
#define PWR                      ((PWR_TypeDef *)PWR_BASE)
#define TIM1                     ((TIM_TypeDef *)TIM1_BASE)

#define RCC_CR_HSION             (1UL << 0)
#define RCC_CR_HSIRDY            (1UL << 1)
#define RCC_CR_PLLON             (1UL << 24)
#define RCC_CR_PLLRDY            (1UL << 25)

#define RCC_CFGR_SW_PLL          (2UL << 0)
#define RCC_CFGR_SWS_PLL         (2UL << 2)
#define RCC_CFGR_PPRE1_DIV2      (4UL << 10)

#define RCC_AHB1ENR_GPIOAEN      (1UL << 0)
#define RCC_AHB1ENR_GPIOBEN      (1UL << 1)
#define RCC_APB1ENR_PWREN        (1UL << 28)
#define RCC_APB2ENR_TIM1EN       (1UL << 0)

#define FLASH_ACR_LATENCY_3WS    3UL
#define FLASH_ACR_PRFTEN         (1UL << 8)
#define FLASH_ACR_ICEN           (1UL << 9)
#define FLASH_ACR_DCEN           (1UL << 10)

#define PWR_CR_VOS_SCALE1        (3UL << 14)

#define TIM_CR1_CEN              (1UL << 0)
#define TIM_CR1_ARPE             (1UL << 7)
#define TIM_EGR_UG               (1UL << 0)
#define TIM_CCMR1_OC1PE          (1UL << 3)
#define TIM_CCMR1_OC1M_PWM1      (6UL << 4)
#define TIM_CCER_CC1E            (1UL << 0)
#define TIM_CCER_CC1NE           (1UL << 2)
#define TIM_BDTR_MOE             (1UL << 15)

#define HAL_OK                   0U
#define TIM_CHANNEL_1            0U

static TIM_HandleTypeDef htim1 = { TIM1 };

static void ConfigureSystemClock100MHz(void);
static void ConfigurePwmPins(void);
static void ConfigureTim1ForComplementaryPwm(void);

void SystemInit(void)
{
    ConfigureSystemClock100MHz();
}

static uint32_t HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t channel)
{
    (void)channel;
    htim->Instance->CCER |= TIM_CCER_CC1E;
    htim->Instance->CR1 |= TIM_CR1_CEN;
    return HAL_OK;
}

static uint32_t HAL_TIMEx_PWMN_Start(TIM_HandleTypeDef *htim, uint32_t channel)
{
    (void)channel;
    htim->Instance->CCER |= TIM_CCER_CC1NE;
    htim->Instance->CR1 |= TIM_CR1_CEN;
    return HAL_OK;
}

static void ConfigureSystemClock100MHz(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS_SCALE1;

    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U)
    {
    }

    FLASH->ACR = FLASH_ACR_LATENCY_3WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    /* HSI = 16 MHz, PLLM = 16, PLLN = 200, PLLP = 2 -> SYSCLK = 100 MHz. */
    RCC->PLLCFGR = 16UL | (200UL << 6) | (4UL << 24);
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U)
    {
    }

    RCC->CFGR = RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_PLL) != RCC_CFGR_SWS_PLL)
    {
    }
}

static void ConfigurePwmPins(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;

    /* PA8 -> TIM1_CH1, PB13 -> TIM1_CH1N, both on AF1. */
    GPIOA->MODER &= ~(3UL << (8U * 2U));
    GPIOA->MODER |= (2UL << (8U * 2U));
    GPIOA->OSPEEDR |= (3UL << (8U * 2U));
    GPIOA->AFR[1] &= ~(0xFUL << ((8U - 8U) * 4U));
    GPIOA->AFR[1] |= (1UL << ((8U - 8U) * 4U));

    GPIOB->MODER &= ~(3UL << (13U * 2U));
    GPIOB->MODER |= (2UL << (13U * 2U));
    GPIOB->OSPEEDR |= (3UL << (13U * 2U));
    GPIOB->AFR[1] &= ~(0xFUL << ((13U - 8U) * 4U));
    GPIOB->AFR[1] |= (1UL << ((13U - 8U) * 4U));
}

static void ConfigureTim1ForComplementaryPwm(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    TIM1->CR1 = TIM_CR1_ARPE;
    TIM1->PSC = 99U;
    TIM1->ARR = 39999U;
    TIM1->CCR1 = 300U;
    TIM1->RCR = 0U;
    TIM1->CCMR1 = TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_PWM1;
    TIM1->CCER = 0U;
    TIM1->BDTR = TIM_BDTR_MOE;
    TIM1->EGR = TIM_EGR_UG;
}

int main(void)
{
    ConfigurePwmPins();
    ConfigureTim1ForComplementaryPwm();

    /*
     * Timer math at 100 MHz TIM1 clock:
     *  PSC  = 99    -> 1 MHz timer tick (1 us)
     *  ARR  = 39999 -> 40 ms period      (25 Hz)
     *  CCR1 = 300   -> 300 us pulse width
     */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);

    for (;;)
    {
        __asm volatile ("wfi");
    }
}
