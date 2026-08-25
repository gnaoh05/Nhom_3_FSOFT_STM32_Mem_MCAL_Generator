#ifndef STM32F401_BAREMETAL_H
#define STM32F401_BAREMETAL_H

#include "Platform_Types.h"

typedef struct
{
    volatile uint32 MODER;
    volatile uint32 OTYPER;
    volatile uint32 OSPEEDR;
    volatile uint32 PUPDR;
    volatile uint32 IDR;
    volatile uint32 ODR;
    volatile uint32 BSRR;
    volatile uint32 LCKR;
    volatile uint32 AFR[2];
} Stm32_GpioRegistersType;

typedef struct
{
    volatile uint32 SR;
    volatile uint32 DR;
    volatile uint32 BRR;
    volatile uint32 CR1;
    volatile uint32 CR2;
    volatile uint32 CR3;
    volatile uint32 GTPR;
} Stm32_UsartRegistersType;

typedef struct
{
    volatile uint32 CR;
    volatile uint32 PLLCFGR;
    volatile uint32 CFGR;
    volatile uint32 CIR;
    volatile uint32 AHB1RSTR;
    volatile uint32 AHB2RSTR;
    uint32 RESERVED0[2];
    volatile uint32 APB1RSTR;
    volatile uint32 APB2RSTR;
    uint32 RESERVED1[2];
    volatile uint32 AHB1ENR;
    volatile uint32 AHB2ENR;
    uint32 RESERVED2[2];
    volatile uint32 APB1ENR;
    volatile uint32 APB2ENR;
} Stm32_RccRegistersType;

typedef struct
{
    volatile uint32 ACR;
    volatile uint32 KEYR;
    volatile uint32 OPTKEYR;
    volatile uint32 SR;
    volatile uint32 CR;
    volatile uint32 OPTCR;
} Stm32_FlashRegistersType;

#define STM32_PERIPH_BASE                 0x40000000UL
#define STM32_APB1PERIPH_BASE             STM32_PERIPH_BASE
#define STM32_AHB1PERIPH_BASE             0x40020000UL

#define STM32_GPIOA_BASE                  (STM32_AHB1PERIPH_BASE + 0x0000UL)
#define STM32_GPIOB_BASE                  (STM32_AHB1PERIPH_BASE + 0x0400UL)
#define STM32_GPIOC_BASE                  (STM32_AHB1PERIPH_BASE + 0x0800UL)
#define STM32_RCC_BASE                    (STM32_AHB1PERIPH_BASE + 0x3800UL)
#define STM32_FLASH_REG_BASE              (STM32_AHB1PERIPH_BASE + 0x3C00UL)
#define STM32_USART2_BASE                 (STM32_APB1PERIPH_BASE + 0x4400UL)

#define STM32_GPIOA                       ((Stm32_GpioRegistersType*)STM32_GPIOA_BASE)
#define STM32_GPIOB                       ((Stm32_GpioRegistersType*)STM32_GPIOB_BASE)
#define STM32_GPIOC                       ((Stm32_GpioRegistersType*)STM32_GPIOC_BASE)
#define STM32_RCC                         ((Stm32_RccRegistersType*)STM32_RCC_BASE)
#define STM32_FLASH                       ((Stm32_FlashRegistersType*)STM32_FLASH_REG_BASE)
#define STM32_USART2                      ((Stm32_UsartRegistersType*)STM32_USART2_BASE)

#define STM32_RCC_CR_HSION                (1UL << 0)
#define STM32_RCC_CR_HSEON                (1UL << 16)
#define STM32_RCC_CR_HSEBYP               (1UL << 18)
#define STM32_RCC_CR_CSSON                (1UL << 19)
#define STM32_RCC_CR_PLLON                (1UL << 24)
#define STM32_RCC_CFGR_SWS                (0x3UL << 2)
#define STM32_RCC_CFGR_SWS_HSI            (0x0UL << 2)
#define STM32_RCC_CFGR_SWS_HSE            (0x1UL << 2)
#define STM32_RCC_CFGR_SWS_PLL            (0x2UL << 2)
#define STM32_RCC_CFGR_HPRE_POS           4u
#define STM32_RCC_PLLCFGR_PLLM            0x3FUL
#define STM32_RCC_PLLCFGR_PLLN_POS        6u
#define STM32_RCC_PLLCFGR_PLLN            (0x1FFUL << STM32_RCC_PLLCFGR_PLLN_POS)
#define STM32_RCC_PLLCFGR_PLLP_POS        16u
#define STM32_RCC_PLLCFGR_PLLP            (0x3UL << STM32_RCC_PLLCFGR_PLLP_POS)
#define STM32_RCC_PLLCFGR_PLLSRC_HSE      (1UL << 22)
#define STM32_RCC_PLLCFGR_RESET_VALUE     0x24003010UL
#define STM32_RCC_AHB1ENR_GPIOAEN         (1UL << 0)
#define STM32_RCC_APB1ENR_USART2EN        (1UL << 17)

#define STM32_USART_SR_RXNE               (1UL << 5)
#define STM32_USART_SR_TXE                (1UL << 7)
#define STM32_USART_CR1_RE                (1UL << 2)
#define STM32_USART_CR1_TE                (1UL << 3)
#define STM32_USART_CR1_UE                (1UL << 13)

#define STM32_FLASH_ACR_ICEN              (1UL << 9)
#define STM32_FLASH_ACR_DCEN              (1UL << 10)
#define STM32_FLASH_ACR_ICRST             (1UL << 11)
#define STM32_FLASH_ACR_DCRST             (1UL << 12)

#define STM32_FLASH_SR_EOP                (1UL << 0)
#define STM32_FLASH_SR_OPERR              (1UL << 1)
#define STM32_FLASH_SR_WRPERR             (1UL << 4)
#define STM32_FLASH_SR_PGAERR             (1UL << 5)
#define STM32_FLASH_SR_PGPERR             (1UL << 6)
#define STM32_FLASH_SR_PGSERR             (1UL << 7)
#define STM32_FLASH_SR_BSY                (1UL << 16)

#define STM32_FLASH_CR_PG                 (1UL << 0)
#define STM32_FLASH_CR_SER                (1UL << 1)
#define STM32_FLASH_CR_MER                (1UL << 2)
#define STM32_FLASH_CR_SNB_POS            3u
#define STM32_FLASH_CR_SNB                (0x1FUL << STM32_FLASH_CR_SNB_POS)
#define STM32_FLASH_CR_PSIZE_POS          8u
#define STM32_FLASH_CR_PSIZE              (0x3UL << STM32_FLASH_CR_PSIZE_POS)
#define STM32_FLASH_CR_STRT               (1UL << 16)
#define STM32_FLASH_CR_EOPIE              (1UL << 24)
#define STM32_FLASH_CR_ERRIE              (1UL << 25)
#define STM32_FLASH_CR_LOCK               (1UL << 31)

#define STM32_SCB_VTOR                    (*(volatile uint32*)0xE000ED08UL)
#define STM32_SCB_CPACR                   (*(volatile uint32*)0xE000ED88UL)
#define STM32_CORE_DEBUG_DHCSR            (*(volatile uint32*)0xE000EDF0UL)
#define STM32_CORE_DEBUG_DHCSR_C_DEBUGEN  (1UL << 0)
#define STM32_NVIC_ISER_BASE              ((volatile uint32*)0xE000E100UL)
#define STM32_FLASH_IRQ_NUMBER            4u

#define STM32_FLASH_MEMORY_BASE           0x08000000UL
#define STM32_HSI_CLOCK_HZ                16000000UL
#define STM32_HSE_CLOCK_HZ                8000000UL

extern uint32 Stm32_SystemCoreClock;

void Stm32_BareMetalSystemInit(void);
void Stm32_BareMetalSystemCoreClockUpdate(void);
void Stm32_BareMetalEnableIrq(uint32 irqNumber);

#endif
