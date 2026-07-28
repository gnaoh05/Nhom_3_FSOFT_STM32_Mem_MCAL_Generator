#include "TestManager.h"
#include "system_stm32f4xx.h"

int main(void)
{
    SystemInit();

    TestManager_Run();

    while (1)
    {

    }
}
