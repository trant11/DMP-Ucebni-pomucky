#include "stm8s.h"
#include "delay.h"
#include "milis.h"
#include "stdio.h"

//LED STATUS DEFINE
#define L_PATTERN 0b01110000    // 3x125ns (8MHZ SPI)
#define H_PATTERN 0b01111100    // 5x125ns (8MHZ SPI), first and last bit must be zero (to remain MOSI in Low between frames/bits)
// takes array of LED_number * 3 bytes (RGB per LED)

#define I_LED GPIO_WriteReverse(GPIOD,GPIO_PIN_3)
#define V1_LED GPIO_WriteReverse(GPIOD,GPIO_PIN_2)
#define VR1_LED GPIO_WriteReverse(GPIOD,GPIO_PIN_1)
#define VR2_LED GPIO_WriteReverse(GPIOC,GPIO_PIN_7)

#define I_button GPIO_ReadInputPin(GPIOB,GPIO_PIN_5)
#define V1_button GPIO_ReadInputPin(GPIOB,GPIO_PIN_4)
#define VR1_button GPIO_ReadInputPin(GPIOC,GPIO_PIN_3)
#define VR2_button GPIO_ReadInputPin(GPIOC,GPIO_PIN_4)

void setup(void){
CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1); //Taktovat MCU 16 MHz

GPIO_Init(GPIOD,GPIO_PIN_3,GPIO_MODE_OUT_PP_LOW_SLOW);
GPIO_Init(GPIOD,GPIO_PIN_2,GPIO_MODE_OUT_PP_LOW_SLOW);
GPIO_Init(GPIOD,GPIO_PIN_1,GPIO_MODE_OUT_PP_LOW_SLOW);
GPIO_Init(GPIOC,GPIO_PIN_7,GPIO_MODE_OUT_PP_LOW_SLOW);

GPIO_Init(GPIOB,GPIO_PIN_5,GPIO_MODE_IN_PU_NO_IT);
GPIO_Init(GPIOB,GPIO_PIN_4,GPIO_MODE_IN_PU_NO_IT);
GPIO_Init(GPIOC,GPIO_PIN_3,GPIO_MODE_IN_PU_NO_IT);
GPIO_Init(GPIOC,GPIO_PIN_4,GPIO_MODE_IN_PU_NO_IT);
}

void init_spi(void){
    // Software slave managment (disable CS/SS input), BiDirectional-Mode release MISO pin to general purpose
    SPI->CR2 |= SPI_CR2_SSM | SPI_CR2_SSI | SPI_CR2_BDM | SPI_CR2_BDOE;
    // Enable SPI as master at maximum speed (F_MCU/2, there 16/2=8MHz)
    SPI->CR1 |= SPI_CR1_SPE | SPI_CR1_MSTR;
}

void neopixel(uint8_t * data, uint16_t length){
    uint8_t mask;
    disableInterrupts();        // can be omitted if interrupts do not take more then about ~25us
    while(length--) {            // for all bytes from input array
        mask = 0b10000000;     // for all bits in byte
        while (mask) {
            while (!(SPI->SR & SPI_SR_TXE));    // wait for empty SPI buffer
            if (mask & data[length]) {  // send pulse with coresponding length ("L" od "H")
                SPI->DR = H_PATTERN;
            } else {
                SPI->DR = L_PATTERN;
            }
            mask = mask >> 1;
        }
    }
    enableInterrupts();
    while (SPI->SR & SPI_SR_BSY); // wait until end of transfer - there should come "reset" (>50us in Low)
}

void delay_ms(uint16_t ms) {
    uint16_t  i;
    for (i=0; i<ms; i = i+1){
        _delay_us(250);
        _delay_us(248);
        _delay_us(250);
        _delay_us(250);
    }
}

//VARIABLES
uint8_t colors[30] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t LED=0;
_Bool I_button_state=0;
_Bool I_button_past=0;
_Bool V0_button_state=0;
_Bool V0_button_past=0;
_Bool V1_button_state=0;
_Bool V1_button_past=0;
_Bool V2_button_state=0;
_Bool V2_button_past=0;
uint8_t LED_modul_on=0;
uint32_t button_timer=0;
uint32_t LED_timer=0;
uint8_t Time_mode;

void main (void){
    setup();
    init_milis();
    init_spi();

    while (1){
        //LED UPDATE
        neopixel(colors, sizeof(colors));
        delay_ms(2);
        
        //BUTTONS & LED CONTROL
        if (I_button){
            I_button_state=1;                                
		}
        else{
            I_button_state=0;
        }
        if (I_button_state==1 && I_button_past==0 && milis()-button_timer>100){
            LED_modul_on+=1;            
            if (LED_modul_on>2){
                LED_modul_on=0;
            }
            if(LED_modul_on==1 || LED_modul_on==0){
            I_LED;
            }
            button_timer=milis();
        }   
        I_button_past=I_button_state;                 

        if (V1_button){
            V0_button_state=1;
		}
        else{
            V0_button_state=0;
        }
        if (V0_button_state==1 && V0_button_past==0 && milis()-button_timer>100){
           V1_LED;
           button_timer=milis();
        }
        V0_button_past=V0_button_state;  

        if (VR1_button){
            V1_button_state=1;
		}
        else{
            V1_button_state=0;
        }
        if (V1_button_state==1 && V1_button_past==0 && milis()-button_timer>100){
           VR1_LED;
           button_timer=milis();
        }
        V1_button_past=V1_button_state;

        if (VR2_button){
            V2_button_state=1;
		}
        else{
            V2_button_state=0;
        }
        if (V2_button_state==1 && V2_button_past==0 && milis()-button_timer>100){
           VR2_LED;
           button_timer=milis();
        }
        V2_button_past=V2_button_state;   

//WS2812B PROGRAM
        if (LED_modul_on>0){
            if(LED_modul_on==1){   
                Time_mode=200;
            }
            else{
                Time_mode=30;
            }
            if (milis()-LED_timer>Time_mode){ 
                colors[LED+1]=255;
                colors[LED+2]=255;

                if (LED>8){
                    colors[LED-8]=0;
                    colors[LED-7]=0;
                }
                if (colors[1]>0){
                    colors[LED+23]=0;
                    colors[LED+22]=0;
                }

                    LED+=3;
                if (LED>=30){
                    LED=0;
                }
                LED_timer=milis(); 
            }
        }
        else{
            for (LED=0; LED<30; LED+=3){
                colors[LED+1]=0;
                colors[LED+2]=0;                         
            }
        }
    }
}