/**
**********************************************************************
********
*	@file TIM/TimeBase/main.c
*	@author Grupo C - FEI Engenharia Elétrica
*	@version V4.0
*	@date 23/10/2012
*	@brief Main program body
**********************************************************************
********
/* Includes	*/
#include "stm32f10x.h" #include <stdio.h>

#include "stm32f10x_adc.h" #include "stm32f10x_dma.h" #include "stm32f10x_dac.h" #include "stm32f10x_gpio.h" #include "stm32f10x_usart.h" #include "antilib_gpio.h"

/** @addtogroup STM32F10x_StdPeriph_Examples
 *	@{
 */
/** @addtogroup TIM_TimeBase
 *	@{
 */
/* Private typedef	*/
/* Private define	*/
#define BufferSize 32
#define ADC1_DR_Address((uint32_t) 0x4001244C)
/* Private macro	*/
/* Private variables	*/
TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
TIM_OCInitTypeDef TIM_OCInitStructure;
USART_InitTypeDef USART_InitStructure;
GPIO_InitTypeDef GPIO_InitStructure;
ADC_InitTypeDef ADC_InitStructure;
u16 ADC_ConvertedValue; //[16]; DMA_InitTypeDef DMA_InitStructure; DAC_InitTypeDef DAC_InitStruct; RCC_ClocksTypeDef RCC_Clock; TIM_OCInitTypeDef TIM_OCInitStructure; uint16_t PrescalerValue = 0; RCC_ClocksTypeDef RCC_Clock;
char string[20];
int time = 0, A = 1, inv = 1, Pulso;

int VADC = 0, VMAX = 0, VMIN = 4095;
int Polos = 6, Vsoma = 0, Vsomaf = 0;
float Frequencia = 0, Vmed = 0, Rot = 0;
int Ti = 0, Tf = 0, CTf = 0, CTi = 0;
_Bool press1 = 1;
_Bool press2 = 1;
_Bool escrita = 1;
_Bool Polaridade = 1;
_Bool FLAG_CTi = 1;
_Bool Pisca = 1;
_Bool inicia_s = 0;
static uint16_t CounterTim2 = 0;
/* Private function prototypes	*/
void Delay(IO uint32_t nCount);
void inicia_lcd(void); // Configura o LCD
void config_port(void); // Configura os Ports utilizados
void atualiza_lcd(void); // Gera uma borda de descida para executar as intruções
void escreve_lcd(char letra[]); // Escreve strings no LCD
main * -Printed on 24 / 11 / 2012 00: 54: 24 Page 2
void config_ADC(void); // Configura e inicializa o ADC
void config_DAC(void); // Configura e inicializa o DAC
void TIM2_Configuration(void); // Configura e inicializa o Timer
void USART1_Config(void); // Configura e inicializa a USART1
void USART_PutString(uint8_t * str); // Escreve na Usart1 e envia dado pelo TX do RS232
/* Private functions	*/
/**
 *	@brief Main program
 *	@param None
 *	@retval None
 */
int main(void) {
  // - Configuração inicial dos Ports ---
  config_port();
  // - Configuração inicial do display ---
  inicia_lcd();
  //--- Configura o ADC
  config_ADC();
  //--- Configura o DAC
  config_DAC();
  //--- TIM2 Configuration
  TIM2_Configuration();
  //--- USART1 configuration USART1_Config(); RCC_GetClocksFreq(&RCC_Clock); Delay(0xF0FFF);
  Delay(0xF0FFF);
  GPIO_Write(GPIOC, 0x0001); // Clear All

  atualiza_lcd();
  escreve_lcd(" ");
  GPIO_Write(GPIOC, 0x0080); //Endereça a primeira Linha
  atualiza_lcd();
  Delay(0x00FFF);
  escreve_lcd(" FEI");
  GPIO_Write(GPIOC, 0x00C0); // Endereça a segunda Linha
  atualiza_lcd();
  escreve_lcd("Entre com Numero");
  GPIO_Write(GPIOC, 0x0090); // Endereça a terceira Linha
  atualiza_lcd();
  escreve_lcd("de Polos: ");
  while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)) //Verifica se o enter foi pressionado
  {
    GPIO_Write(GPIOC, 0x009A); // Endereça a terceira Linha
    atualiza_lcd();
    Delay(0x4FFFF);
    sprintf(string, "%d ", Polos);
    escreve_lcd(string);
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)) //verifica se PA1 foi pressionado
    {
      if (press1) {
        press1 = 0;
        if (Polos < 20) Polos++;
      }
    } // Este botão incrementa Polos
    else
      press1 = 1;
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2)) //Verifica se PA2 foi pressionado
    {
      if (press2) {
        press2 = 0;
        if (Polos > 2) Polos--;
      }
    } // Este botão decrementa Polos
    else
      press2 = 1;
    while (escrita) // Faz o display piscar a palavra "Press enter"
    {
      if (Pisca) {
        GPIO_Write(GPIOC, 0x00D2); // Endereça a terceira Linha
        main * -Printed on 24 / 11 / 2012 00: 54: 24 Page 3
        atualiza_lcd();
        escreve_lcd("Press enter");
        Pisca = 0;
      } else {
        GPIO_Write(GPIOC, 0x00D2); // Endereça a terceira Linha
        atualiza_lcd();
        escreve_lcd(" ");
        Pisca = 1;
      }
      escrita = 0;
    }
  }
  GPIO_Write(GPIOC, 0x00C0); //Endereça a primeira Linha

  atualiza_lcd();
  sprintf(string, "Motor %d polos ", Polos);
  escreve_lcd(string);
  GPIO_Write(GPIOC, 0x0090); //Endereça a primeira Linha
  atualiza_lcd();
  escreve_lcd(" ");
  GPIO_Write(GPIOC, 0x000C); // liga display e desliga cursor
  atualiza_lcd();
  Pulso = 0;
  while (1) // Looping infinito
  {
    if (escrita) {
      if (Pulso == 0 || Pulso == 1) // Se o pulso for menor que 2, não é possivel calcular
        Frequencia = 0;
      else
        Frequencia = (Pulso - 1) / (1 - ((Tf + Ti) * 0.0001)); //Calcula a Frequencia
      // Vmed = (Vmed/(10000-Tf-Ti))*(3.3/4095); // Calcula a tensão média que entra no ADC
      // atravez do calculo da integral da função
      // dividida pelo periodo
      GPIO_Write(GPIOC, 0x0090); //
      atualiza_lcd();
      //GPIO_ResetBits(GPIOC,GPIO_Pin_8);
      //sprintf(string, "Freq. %.2f Hz", Frequencia); sprintf(string, "Freq. %.2f Hz", Frequencia); escreve_lcd(string);
      escreve_lcd(" ");
      GPIO_Write(GPIOC, 0x00D0); //
      atualiza_lcd();
      //GPIO_ResetBits(GPIOC,GPIO_Pin_8); Rot = (Frequencia*60)/(Polos*2)) sprintf(string, "Rot: %.1f RPM", Rot;
      //sprintf(string, "Vmed.: %.2f V", Vmed);
      escreve_lcd(string);
      escreve_lcd(" ");
      escrita = 0;
      sprintf(string, "Rot.: %.4f V |", Rot);
      USART_PutString(string);
    }
    //Delay(0x00040); // para onda de 60Hz
    //Delay(0x0018F); // para onda de 10Hz
    /*
    // Gera uma onda Triangular ---------------- if(A>0x0FEA) //fef
    inv = 0; if(A<0x001F)
    inv=1; if(inv==1) A+=16;
     
    else
    A-=16; */
    //
    DAC_SetChannel1Data(DAC_Align_12b_R, Rot / 2); // Escreve no DAC a rotação do motor
    main * -Printed on 24 / 11 / 2012 00: 54: 24 Page 4
    // com uma precisão de 2 em 2 RPM
    // ou seja a velocidade varia de 0 a 8190RPM
  }
}
/*	*/
void Delay(IO uint32_t nCount) {
  for (; nCount != 0; nCount--);
}
/*	*/
// Writes in LCD
void escreve_lcd(char letra[]) {
  int x;
  GPIO_SetBits(GPIOB, GPIO_Pin_0); // RS = '1' habilita a escrita de dados
  for (x = 0; letra[x] != 0; x++) {
    GPIO_Write(GPIOC, letra[x]); // escreve letra no display
    GPIO_SetBits(GPIOB, GPIO_Pin_2);
    Delay(0x000BF); // Geração de um pequeno Duty,
    // valor 2 vezes maior q o minimo permitido.
    GPIO_ResetBits(GPIOB, GPIO_Pin_2);
  }
  GPIO_ResetBits(GPIOB, GPIO_Pin_0); // RS = '0' hab. escrita de instruções
}
/*	*/
// Função q gera a borda de descida p/ executar uma instrução ou enviar um dado
void atualiza_lcd(void) // Se no display não aparecer todos os caracteres
{ // o valor do Delay tem que ser aumentado GPIO_SetBits(GPIOB,GPIO_Pin_2); Delay(0x000BF); // Geração de um pequeno Delay,
  // valor 2 vezes maior q o minimo permitido.
  GPIO_ResetBits(GPIOB, GPIO_Pin_2);
}
/*	*/
// ---- Configures the LCD ------------------
void inicia_lcd(void) {
  GPIO_ResetBits(GPIOB, GPIO_Pin_0); // RS GPIO_ResetBits(GPIOB,GPIO_Pin_1); // R/W escrita ou Leitura GPIO_SetBits(GPIOB,GPIO_Pin_2); // E = Enable GPIO_Write(GPIOC, 0x0038); // 8 bits mas 2 linhas e matriz 5x7 atualiza_lcd();

  Delay(0x04000); // aguarda GPIO_Write(GPIOC, 0x0002); // Cursor Home atualiza_lcd();
  Delay(0x04000); // aguarda
  GPIO_Write(GPIOC, 0x0006); // Escrita a esquerda
  atualiza_lcd();
  Delay(0x04000); // aguarda
  GPIO_Write(GPIOC, 0x000F); // liga display e o cursor modo piscante
  atualiza_lcd();
  Delay(0x04000); // aguarda
  GPIO_Write(GPIOC, 0x0001); // Clear display
  atualiza_lcd();
}
/*	*/
// Configures all ports ---------------------------
void config_port(void) {
  /* Initialize the Peripheral GPIO by enabling the clock*/
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  /* Initialize Led4 */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, & GPIO_InitStructure);
  /* Initialize Led3 */
  main * -Printed on 24 / 11 / 2012 00: 54: 24 Page 5
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, & GPIO_InitStructure);
  /* Initialize PB0 on STM32VLDISCOVERY board */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOB, & GPIO_InitStructure);
  /* Initialize PB1 on STM32VLDISCOVERY board */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOB, & GPIO_InitStructure);
  /* Initialize PB2 on STM32VLDISCOVERY board */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOB, & GPIO_InitStructure);
  /* Initialize the key mounted on PA0 STM32VLDISCOVERY board */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, & GPIO_InitStructure);
  /* Initialize the key mounted on PA1 STM32VLDISCOVERY board */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, & GPIO_InitStructure);
  /* Initialize the key mounted on PA2 STM32VLDISCOVERY board */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, & GPIO_InitStructure);
  /* Initialize PC7 PC6 PC5 PC4 PC3 PC2 PC1 PC0 on STM32VLDISCOVERY board */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_6 | GPIO_Pin_5 | GPIO_Pin_4 | GPIO_Pin_3 | GPIO_Pin_2 | GPIO_Pin_1 | GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, & GPIO_InitStructure);
}
/*	*/
// -- Configurações iniciais do ADC
void config_ADC(void) {
  /* Enable DMA clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  /* Enable ADC1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  /* Configure PA.07 (ADC Channel7) as analog input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, & GPIO_InitStructure);
  /* DMA1 channel1 configuration	*/
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) & ADC_ConvertedValue;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = 1; //16; DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; DMA_InitStructure.DMA_MemoryInc =
  /*DMA_MemoryInc_Enable;*/
  DMA_MemoryInc_Disable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, & DMA_InitStructure);
  /* Enable DMA1 channel1 */
  main * -Printed on 24 / 11 / 2012 00: 54: 24 Page 6
  DMA_Cmd(DMA1_Channel1, ENABLE);
  /* ADC1 configuration	*/

  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, & ADC_InitStructure);
  /* ADC1 regular channel14 configuration */
  ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 1, ADC_SampleTime_55Cycles5);
  /* Enable ADC1 DMA */
  ADC_DMACmd(ADC1, ENABLE);
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);
  /* Enable ADC1 reset calibration register */
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while (ADC_GetResetCalibrationStatus(ADC1));
  /* Start ADC1 calibration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while (ADC_GetCalibrationStatus(ADC1));
  /* Start ADC1 Software Conversion */
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}
/*	*/
//--- configures DAC ---------------
void config_DAC(void) {
  /* Enable DAC clock */
  /* Configure PA.04 (DAC Channe1) as analog input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, & GPIO_InitStructure);
  /* Configure PA.05 (DAC Channe2) as analog input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, & GPIO_InitStructure);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
  /* DAC channel Configuration ********************************/
  DAC_InitStruct.DAC_Trigger = DAC_Trigger_None;
  // Digital to Analog conversion can be non-triggered
  DAC_InitStruct.DAC_WaveGeneration = DAC_WaveGeneration_None;
  // Wave generation disable
  DAC_InitStruct.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  /* Enable DAC Channel1 */
  DAC_Cmd(DAC_Channel_1, ENABLE);
  /* Enable DAC Channel2 */
  DAC_Cmd(DAC_Channel_2, ENABLE);
  /* Set DAC channel1 DHR12RD register */

  DAC_SetChannel1Data(DAC_Align_12b_R, 868);
  /* Set DAC channel2 DHR12RD register */
  DAC_SetChannel2Data(DAC_Align_12b_R, 4095);
}
/*	*/
// Configura um Timer
void TIM2_Configuration(void) {
  /**
   *	@brief Configures the different system clocks.
   *	@param None
   *	@retval None
   */
  //RCC_DeInit();
  main * -Printed on 24 / 11 / 2012 00: 54: 24 Page 7
  RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
  //RCC_PLLConfig(RCC_PLLSource_PREDIV1,RCC_PLLMul_6);
  RCC_PLLCmd(ENABLE);
  /* PCLK1 = HCLK/1 */
  RCC_PCLK1Config(RCC_HCLK_Div1);
  /* TIM2 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  /* GPIOC clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  /*defines the AHB clock divider. This clock is derived from the system clock (SYSCLK)*/
  RCC_HCLKConfig(RCC_SYSCLK_Div1);
  /**
   *	@brief Configure the GPIOD Pins.
   *	@param None
   *	@retval None
   */
  GPIO_InitTypeDef GPIO_InitStructure;
  /* GPIOC Configuration:Pin8 as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  /* GPIOC Configuration:Pin9 as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, & GPIO_InitStructure);
  /**
   *	@brief Configure the nested vectored interrupt controller.
   *	@param None
   *	@retval None
   */
  NVIC_InitTypeDef NVIC_InitStructure;
  /* Enable the TIM2 gloabal Interrupt */

  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init( & NVIC_InitStructure);
  /**
   *	@brief Configures the different system clocks.
   *	@param None
   *	@retval None
   */
  TIM_DeInit(TIM2);
  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 1199;
  TIM_TimeBaseStructure.TIM_Prescaler = 1;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM2, & TIM_TimeBaseStructure);
  /* TIM IT enable */
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
  /* TIM2 enable counter */
  TIM_Cmd(TIM2, ENABLE);
}
/*	*/
// - Configura a USART 1
void USART1_Config(void) {
    RCC -> APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
    main * -Printed on 24 / 11 / 2012 00: 54: 24 Page 8
    GPIOA -> CRH = (GPIOA -> CRH & CONFMASKH(10)) | GPIOPINCONFH(10, GPIOCONF(GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULLUPDOWN));
    GPIOA -> CRH = (GPIOA -> CRH & CONFMASKH(9)) | GPIOPINCONFH(9, GPIOCONF(GPIO_MODE_OUTPUT2MHz, GPIO_CNF_AFIO_PUSHPULL));
    USART1 -> CR1 = USART_CR1_UE | USART_CR1_TE;
    USART1 -> BRR = (SystemCoreClock / 57600);
  }
  /*	*/
  //===================================================
  ===
  === === === === === === === ==
  // - Envia strings pela porta serial RS232
  void USART_PutString(uint8_t * str) {
    while ( * str != 0) {
      while (!(USART1 -> SR & USART_SR_TXE));
      USART1 -> DR = * str;
      str++;
    }
  }
  //===================================================

  ===
  === === === === === === === ==
  void TIM2_IRQHandler(void) {
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    if (CounterTim2 == 9999) {
      Pulso = time;
      time = 0;
      Vmed = Vsoma;
      Vsoma = 0;
      Vsomaf = 0;
      inicia_s = 0;
      //GPIO_WriteBit(GPIOC, GPIO_Pin_9, (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_9))); escrita = 1;
      CounterTim2 = 0;
      Tf = CTf;
      Ti = CTi;
      CTi = 0;
      CTf = 0;
      FLAG_CTi = 1;
    } else {
      CounterTim2++;
    }
    VADC = ADC_GetConversionValue(ADC1);
    if (Polaridade == 1) {
      if (VADC > VMAX) VMAX = VADC;
      if (VADC < (VMAX * 0.9)) {
        Polaridade = 0;
        VMAX = 0;
        time++;
        CTf = 0;
        FLAG_CTi = 0;
        inicia_s = 1;
        Vsoma += Vsomaf;
        Vsomaf = 0;
      }
    } else {
      if (VADC < VMIN) VMIN = VADC;
      if (VADC > (VMIN * 1.1)) {
        main * -Printed on 24 / 11 / 2012 00: 54: 24 Page 9
        Polaridade = 1;
        VMIN = 4095;

      }
    }
    CTf++;
    if (FLAG_CTi) CTi++;
    if (inicia_s) Vsomaf += VADC;
  }
#ifdef USE_FULL_ASSERT
/**
 *	@brief Reports the name of the source file and the source line number
 *	where the assert_param error has occurred.
 *	@param file: pointer to the source file name
 *	@param line: assert_param error line source number
 *	@retval None
 */
void assert_failed(uint8_t * file, uint32_t line) {
  /* User can add his own implementation to report the file name and line number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  while (1) {}
}
#endif
// Any questions contact the creators of the program, Group C - FEI
/*********************** END OF FILE **********************/