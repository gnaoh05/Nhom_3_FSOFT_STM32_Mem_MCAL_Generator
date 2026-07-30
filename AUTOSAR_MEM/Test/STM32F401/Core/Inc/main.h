#ifndef MAIN_H
#define MAIN_H

#include "Stm32F401_BareMetal.h"

#define B1_PIN                 (1UL << 13)
#define B1_GPIO_PORT           STM32_GPIOC
#define USART_TX_PIN           (1UL << 2)
#define USART_TX_GPIO_PORT     STM32_GPIOA
#define USART_RX_PIN           (1UL << 3)
#define USART_RX_GPIO_PORT     STM32_GPIOA
#define LD2_PIN                (1UL << 5)
#define LD2_GPIO_PORT          STM32_GPIOA

#endif
