/**
 ******************************************************************************
 * File Name          : main.c
 * Description        : Main program body
 ******************************************************************************
 *
 * COPYRIGHT(c) 2015 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "cmsis_os.h"
#include "lwip.h"

#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_sdram.h"

#include <stdbool.h>

static osThreadId defaultTaskHandle;

static void SystemClock_Config( void );
static void MX_GPIO_Init( void );
static void StartDefaultTask( void const * args );
static void CpuCacheEnable( void );
static void Demo( void );

int main( void )
{
	CpuCacheEnable();

	HAL_Init();

	SystemClock_Config();

	MX_GPIO_Init();

	BSP_LCD_Init();

	BSP_LCD_LayerDefaultInit( LTDC_ACTIVE_LAYER, SDRAM_DEVICE_ADDR );

	osThreadDef( defaultTask, StartDefaultTask, osPriorityNormal, 0, 128 );
	defaultTaskHandle = osThreadCreate( osThread( defaultTask ), NULL );

	osKernelStart();

	while( 1 )
	{
	}
}

void SystemClock_Config( void )
{
	__HAL_RCC_PWR_CLK_ENABLE();

	__HAL_PWR_VOLTAGESCALING_CONFIG( PWR_REGULATOR_VOLTAGE_SCALE1 );

	RCC_OscInitTypeDef oscCfg;
	oscCfg.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	oscCfg.HSEState = RCC_HSE_ON;
	oscCfg.PLL.PLLState = RCC_PLL_ON;
	oscCfg.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	oscCfg.PLL.PLLM = 25;
	oscCfg.PLL.PLLN = 400;
	oscCfg.PLL.PLLP = RCC_PLLP_DIV2;
	oscCfg.PLL.PLLQ = 8;
	HAL_RCC_OscConfig( &oscCfg );

	HAL_PWREx_ActivateOverDrive();

	RCC_ClkInitTypeDef clkCfg;
	clkCfg.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	clkCfg.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	clkCfg.AHBCLKDivider = RCC_SYSCLK_DIV1;
	clkCfg.APB1CLKDivider = RCC_HCLK_DIV4;
	clkCfg.APB2CLKDivider = RCC_HCLK_DIV2;
	HAL_RCC_ClockConfig( &clkCfg, FLASH_LATENCY_5 );

	HAL_SYSTICK_Config( HAL_RCC_GetHCLKFreq() / 1000 );

	HAL_SYSTICK_CLKSourceConfig( SYSTICK_CLKSOURCE_HCLK );

	HAL_NVIC_SetPriority( SysTick_IRQn, 0, 0 );
}

/**
 * @brief  CPU L1-Cache enable.
 *         Invalidate Data cache before enabling
 *         Enable Data & Instruction Cache
 */
void CpuCacheEnable( void )
{
	(*(uint32_t *) 0xE000ED94) &= ~0x5;
	(*(uint32_t *) 0xE000ED98) = 0x0; //MPU->RNR
	(*(uint32_t *) 0xE000ED9C) = 0x20010000 | 1 << 4; //MPU->RBAR
	(*(uint32_t *) 0xE000EDA0) = 0 << 28 | 3 << 24 | 0 << 19 | 0 << 18 | 1 << 17 | 0 << 16 | 0 << 8 | 30 << 1 | 1 << 0; //MPU->RASE  WT
	(*(uint32_t *) 0xE000ED94) = 0x5;

	SCB_InvalidateICache();

	/* Enable branch prediction */
	SCB->CCR |= (1 << 18);
	__DSB();

	SCB_EnableICache();

	SCB_InvalidateDCache();
	SCB_EnableDCache();
}

void MX_GPIO_Init( void )
{
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
}

void StartDefaultTask( void const * args )
{
	Demo();

	MX_LWIP_Reset();
	MX_LWIP_Init();

	for( ;; )
	{
		char* ip = ipaddr_ntoa( &gnetif.ip_addr );
		BSP_LCD_DisplayStringAt( 0, 50, (uint8_t *) ip, CENTER_MODE );
		ip = ipaddr_ntoa( &gnetif.netmask );
		BSP_LCD_DisplayStringAt( 0, 100, (uint8_t *) ip, CENTER_MODE );
		ip = ipaddr_ntoa( &gnetif.gw );
		BSP_LCD_DisplayStringAt( 0, 150, (uint8_t *) ip, CENTER_MODE );

		osDelay( 1 );
	}
}

void Demo( void )
{
	BSP_LCD_SelectLayer( LTDC_ACTIVE_LAYER );
	BSP_LCD_SetFont( &LCD_DEFAULT_FONT );
	BSP_LCD_SetBackColor( LCD_COLOR_WHITE );
	BSP_LCD_SetTextColor( LCD_COLOR_DARKBLUE );
	BSP_LCD_Clear( LCD_COLOR_WHITE );
}

#ifdef USE_FULL_ASSERT

/**
 * @brief Reports the name of the source file and the source line number
 * where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */

}

#endif

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
