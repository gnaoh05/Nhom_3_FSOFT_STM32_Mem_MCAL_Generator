#include <stddef.h>
#include <Stm32F401_BareMetal.h>

typedef char Stm32_CheckGpioAfrOffset[
        (offsetof(Stm32_GpioRegistersType, AFR) == 0x20u) ? 1 : -1];
typedef char Stm32_CheckUsartBrrOffset[
        (offsetof(Stm32_UsartRegistersType, BRR) == 0x08u) ? 1 : -1];
typedef char Stm32_CheckRccAhb1EnrOffset[
        (offsetof(Stm32_RccRegistersType, AHB1ENR) == 0x30u) ? 1 : -1];
typedef char Stm32_CheckRccApb1EnrOffset[
        (offsetof(Stm32_RccRegistersType, APB1ENR) == 0x40u) ? 1 : -1];
typedef char Stm32_CheckFlashCrOffset[
        (offsetof(Stm32_FlashRegistersType, CR) == 0x10u) ? 1 : -1];

uint32 Stm32_SystemCoreClock = STM32_HSI_CLOCK_HZ;

void Stm32_BareMetalSystemInit(void)
{
    STM32_SCB_CPACR |= (0xFUL << 20);
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb" ::: "memory");

    STM32_RCC->CR |= STM32_RCC_CR_HSION;
    STM32_RCC->CFGR = 0u;

    while ((STM32_RCC->CFGR & STM32_RCC_CFGR_SWS) != STM32_RCC_CFGR_SWS_HSI)
    {
    }

    STM32_RCC->CR &= ~(STM32_RCC_CR_HSEON |
                       STM32_RCC_CR_CSSON |
                       STM32_RCC_CR_PLLON |
                       STM32_RCC_CR_HSEBYP);
    STM32_RCC->PLLCFGR = STM32_RCC_PLLCFGR_RESET_VALUE;
    STM32_RCC->CIR = 0u;
    STM32_SCB_VTOR = STM32_FLASH_MEMORY_BASE;
    Stm32_SystemCoreClock = STM32_HSI_CLOCK_HZ;
}

void Stm32_BareMetalSystemCoreClockUpdate(void)
{
    static const uint16 ahbPrescaler[16] =
    {
        1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
        2u, 4u, 8u, 16u, 64u, 128u, 256u, 512u
    };
    uint32 systemClock;
    uint32 clockSource = STM32_RCC->CFGR & STM32_RCC_CFGR_SWS;

    if (clockSource == STM32_RCC_CFGR_SWS_HSE)
    {
        systemClock = STM32_HSE_CLOCK_HZ;
    }
    else if (clockSource == STM32_RCC_CFGR_SWS_PLL)
    {
        uint32 pllConfig = STM32_RCC->PLLCFGR;
        uint32 pllM = pllConfig & STM32_RCC_PLLCFGR_PLLM;
        uint32 pllN = (pllConfig & STM32_RCC_PLLCFGR_PLLN) >> STM32_RCC_PLLCFGR_PLLN_POS;
        uint32 pllP = ((((pllConfig & STM32_RCC_PLLCFGR_PLLP) >>
                         STM32_RCC_PLLCFGR_PLLP_POS) + 1u) * 2u);
        uint32 pllInput = ((pllConfig & STM32_RCC_PLLCFGR_PLLSRC_HSE) != 0u) ?
                          STM32_HSE_CLOCK_HZ : STM32_HSI_CLOCK_HZ;

        if ((pllM == 0u) || (pllP == 0u))
        {
            systemClock = STM32_HSI_CLOCK_HZ;
        }
        else
        {
            systemClock = ((pllInput / pllM) * pllN) / pllP;
        }
    }
    else
    {
        systemClock = STM32_HSI_CLOCK_HZ;
    }

    Stm32_SystemCoreClock =
            systemClock / ahbPrescaler[(STM32_RCC->CFGR >> STM32_RCC_CFGR_HPRE_POS) & 0x0Fu];
}

void Stm32_BareMetalEnableIrq(uint32 irqNumber)
{
    STM32_NVIC_ISER_BASE[irqNumber >> 5u] = (1UL << (irqNumber & 0x1Fu));
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb" ::: "memory");
}
