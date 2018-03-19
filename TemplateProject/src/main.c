#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_utils.h"
#include "stm32f0xx_ll_exti.h"
//короче по тикам мерять время и по ним выходить
#define ANODICS   LL_GPIO_PIN_1  | LL_GPIO_PIN_2 | \
                  LL_GPIO_PIN_3  | LL_GPIO_PIN_4 | \
                  LL_GPIO_PIN_5  | LL_GPIO_PIN_7 | \
                  LL_GPIO_PIN_10 | LL_GPIO_PIN_11
#define KATHODES  LL_GPIO_PIN_13 | LL_GPIO_PIN_14 | LL_GPIO_PIN_12 | LL_GPIO_PIN_6

void UserButtonInit(void);
void SystemClock_Config(void);
inline void SwapDiode(uint8_t cnt);
void ButtonHandler(void);
void GenSound(uint32_t len);
void DynamicInd(void);
void GetTime16(uint8_t* time1);
void TimeIncr(uint8_t* time1);
void SetTime(void);
void ChangeTime(uint8_t* time1);
void DigIncr(uint32_t num, uint8_t* time1);
void SetAlarm(uint8_t* time1);
void ShowAlarm(uint8_t* time1); 
                                          /*  0     1     2     3     4     5     6     7     8     9 */
static volatile const uint8_t digits[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}; 
uint8_t time[4] = {0, 0, 0, 0};
volatile uint8_t time16[4] = {0, 0, 0, 0};
volatile int32_t mask[4]   = {1, 1, 1, 1};
uint8_t alarm[4]  = {0, 0, 0, 0};
volatile uint32_t tick = 0;
volatile uint32_t tick1 = 0;
uint8_t num = 0;
uint8_t flag = 0;
uint8_t flagALARM = 0;
uint8_t access = 0;
uint8_t is_button = 0;
uint8_t first = 0;
int32_t curr = 0;
/*************************************************************/
/*Connection diagram:                                        */
/*           K1        K2        K3        K4                */
/*           a                                               */
/*        ////////  ////////  ////////  ////////             */
/*        /      /b /      /  /      /  /      /             */       
/*       f/   g  /  /      /  /      /  /      /             */        
/*        ////////  ////////  ////////  ////////             */       
/*        /      /c /      /  /      /  /      /             */       
/*      e /      /  /      /  /      /  /      /             */       
/*        //////// .//////// .//////// .//////// .           */
/*            d                                              */         
/*   PC1 PC2 PC3 PC4 PC5 PC6 PC7 PC13 PC14 PC10 PC11 PC12    */     
/*    e   d  DP   c   g  K4   b   K3   K2    f    a    K1    */     
/*    PC15 - sound   PA2 - button2                           */ 
/*************************************************************/

int main(void) 
{
        SystemClock_Config();
        
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
       
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_1 , LL_GPIO_MODE_OUTPUT);                   
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_2 , LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_3 , LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_4 , LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_5 , LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_6 , LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_7 , LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_8 , LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_10, LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_11, LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_12, LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_14, LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_15, LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_0, LL_GPIO_MODE_INPUT);
        
        UserButtonInit();
        GetTime16(time);
        SetTime();
        DynamicInd();
}

void SystemClock_Config(void) 
{
        /* Set FLASH latency */
        LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);

        /* Enable HSI and wait for activation*/
        LL_RCC_HSI_Enable();
        while (LL_RCC_HSI_IsReady() != 1);

        /* Main PLL configuration and activation */
        LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2,
                                    LL_RCC_PLL_MUL_12);

        LL_RCC_PLL_Enable();
        while (LL_RCC_PLL_IsReady() != 1);

        /* Sysclk activation on the main PLL */
        LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
        LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
        while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);

        /* Set APB1 prescaler */
        LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

        /* Set systick to 1ms */
        SysTick_Config(48000000/1000);

        /* Update CMSIS variable (which can be updated also
         * through SystemCoreClockUpdate function) */
        SystemCoreClock = 168000000;
}

void UserButtonInit(void)
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_0, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_0, LL_GPIO_PULL_NO);
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE0);
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_0);
    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_0);
    NVIC_EnableIRQ(EXTI0_1_IRQn);
    NVIC_SetPriority(EXTI0_1_IRQn, 0);
}

inline void SwapDiode(uint8_t cnt)
{
    LL_GPIO_SetOutputPin(GPIOC, KATHODES);
    LL_GPIO_ResetOutputPin(GPIOC, ANODICS);
    switch (cnt)
    {
        case 0:
            LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_12);
            LL_GPIO_SetOutputPin(GPIOC, mask[0]);
            break;
        case 1:
            LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_14);
            LL_GPIO_SetOutputPin(GPIOC, mask[1] | LL_GPIO_PIN_3);
            if (flag)
                LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_3);
            else
                LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_3);
            break;
        case 2:
            LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_13);
            LL_GPIO_SetOutputPin(GPIOC, mask[2]); 
            break;
        case 3:
            LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_6);
            LL_GPIO_SetOutputPin(GPIOC, mask[3]); 
            break;
        default:
            break;
    }        
}

void ButtonHandler(void)
{
    if (flagALARM)
    {
        flagALARM = 0;
        GenSound(70);
        return;    
    }   
    if (!access)
    { 
        access++;                                                                  
        LL_GPIO_SetOutputPin(GPIOC, KATHODES);
        GenSound(35);
        switch (is_button)
        {
            case 1:
                ChangeTime(time);
                break;
            case 2: 
                SetAlarm(alarm);
                break;                   
            case 3:        
                ShowAlarm(alarm);
                break;                    
            default:
                break;
        }
        access = 0;
    }       
}

void DynamicInd(void)
{
    uint8_t count = 0;
    while (1)
    {   
        if (tick == 0) curr -= 60000;
        if (is_button && ((tick - curr) >= 2000))
        {
            NVIC_DisableIRQ(EXTI0_1_IRQn);
            ButtonHandler();
            is_button = 0;
            curr = 0;
            NVIC_EnableIRQ(EXTI0_1_IRQn);
        }        
        SwapDiode(count);
        count++;
        if (count == 4) count = 0;
        if (flagALARM) GenSound(5);
    }   
}

void TimeIncr(uint8_t* time1)
{
    time1[3]++;
    
    if (time1[3] == 10)
    {
        time1[2]++;
        time1[3] = 0;
    } 
    if (time1[2] == 6)
    {
        time1[1]++;
        time1[2] = 0;
    }
    if (time1[1] == 4 && time1[0] == 2)
    {
        time1[0] = 0;
        time1[1] = 0;   
    }   
    if (time1[1] == 10)
    {
       time1[0]++;
       time1[1] = 0; 
    }
    if (time1[0] == 3)
    {
        time1[0] = 0;
    }
}

void SetTime(void)
{
    
    uint8_t i = 0;
    uint8_t j = 0;
    
    for (j = 0; j < 4; j++)
    {
        mask[j] = 0;
        for (i = 0; i < 8; i++)
        {
            if((time16[j] & (1 << i)) != 0)
            { 
                mask[j] |= LL_GPIO_PIN_11 * (i == 0) | \
                           LL_GPIO_PIN_7  * (i == 1) | \
                           LL_GPIO_PIN_4  * (i == 2) | \
                           LL_GPIO_PIN_2  * (i == 3) | \
                           LL_GPIO_PIN_1  * (i == 4) | \
                           LL_GPIO_PIN_10 * (i == 5) | \
                           LL_GPIO_PIN_5  * (i == 6) | \
                           LL_GPIO_PIN_3  * (i == 7)   ;     
            }
        }
    }               
}

void GetTime16(uint8_t* time1) 
{
    time16[0] = digits[ time1[0] ];
    time16[1] = digits[ time1[1] ];
    time16[2] = digits[ time1[2] ];
    time16[3] = digits[ time1[3] ];
}

void GenSound(uint32_t len)
{ 
    uint32_t i = 0;
    volatile uint32_t j = 1000;
    for (i = 0; i < len; ++i)
    {
        LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_15); 
        if (!flagALARM) LL_mDelay(1);
        else
        {
            while (j)
            {
                j--;
            }
        }           
    }
}

void ChangeTime(uint8_t* time1)
{
    uint8_t op = 0;
    int32_t curr1 = 0;
    while (1)
    {
        SwapDiode(num);
        while (1)
        {
            if (op == 0) curr1 = tick;
            else if (tick == 0) curr1 -= 60000;
            if (LL_GPIO_IsInputPinSet(GPIOA, 0b01) && ((tick - curr1) < 1000) && op == 0)
            {
                GenSound(35);
                op++;
            }
            if ((LL_GPIO_IsInputPinSet(GPIOA, 0b01)) && ((tick - curr1) >= 1000) && op == 1)
            {
                GenSound(140);
                op++; 
            }
            if (op == 1 && ((tick - curr1) >= 2000))
            {
                DigIncr(num, time1);
                GetTime16(time1);
                SetTime();
                op = 0;
                break;
            }
            if (op == 2 && ((tick - curr1) >= 2000))
            {
                num++;
                op = 0;
                break;   
            }
        }
        if (num == 4) 
        {
            num = 0;
            break;
        }       
    }   
}

void DigIncr(uint32_t num, uint8_t* time1)
{
    time1[num]++;
    switch (num)
    {
        case 0:
            if (time1[num] >= 3)
                time1[num] = 0;
            break;
        case 1:
            if (time1[num] >= 5 && time1[num - 1] >= 2)
                time1[num] = 0;
            if (time1[num] >= 10)
                time1[num] = 0;
            break;
        case 2:
            if (time1[num] >= 6)
                time1[num] = 0;
            break;
        case 3:
            if (time1[num] >= 10)
                time1[num] = 0;            
            break;
        default:
            break;    
    }
}

void SetAlarm(uint8_t* time1)
{   
    ChangeTime(time1);
    GetTime16(time);
    SetTime();
    first++;
}

void ShowAlarm(uint8_t* time1)
{
    GetTime16(time1);
    SetTime();
    uint32_t i = 100000;
  
    while (i)
    {
        SwapDiode(num);
        num++;
        if (num == 4) num = 0;    
        i--;
    }        
    GetTime16(time);
    SetTime();
}

void NMI_Handler(void) 
{
}

void HardFault_Handler(void)
{
    while (1);
}

void SVC_Handler(void) 
{
}

void PendSV_Handler(void) 
{
}

void SysTick_Handler(void) 
{
    tick++;
    tick1++;
    if (tick == 60000 & !access)  
    {
        TimeIncr(time);
        GetTime16(time);
        SetTime();
        tick = 0;
    } 
    if ((tick % 500) == 0) 
        flag ^= 1;
    if ((time[0] == alarm[0]) & (time[1] == alarm[1]) & \
        (time[2] == alarm[2]) & (time[3] == alarm[3]) & first)
    {    
        flagALARM++;
        alarm[0] = 0;
        alarm[1] = 0;
        alarm[2] = 0;
        alarm[3] = 0;
    }        
}

void EXTI0_1_IRQHandler(void)
{                                         
    is_button++;
    if (is_button == 1) curr = tick;
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_0);    
}        
