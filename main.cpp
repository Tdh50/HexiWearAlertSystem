#include "mbed.h"
#include "Hexi_KW40Z.h"
#include "Hexi_OLED_SSD1351.h"
#include "OLED_types.h"
#include "OpenSans_Font.h"
#include "nRF24L01P.h"
#include "string.h"
#include "images.h"

#define NAME    "RB"

#define LED_ON      0
#define LED_OFF     1
#define NUM_OF_SCREENS 6
#define TRANSFER_SIZE   4
   
void StartHaptic(void);
void StopHaptic(void const *n);
void txTask(void);

void displayHome();   
void screenHandler(uint8_t stageNum,uint8_t header);

DigitalOut redLed(LED1,1);
DigitalOut greenLed(LED2,1);
DigitalOut blueLed(LED3,1);
DigitalOut haptic(PTB9);

/* Define timer for haptic feedback */
RtosTimer hapticTimer(StopHaptic, osTimerOnce);

/* Instantiate the Hexi KW40Z Driver (UART TX, UART RX) */ 
KW40Z kw40z_device(PTE24, PTE25);

/* Instantiate the SSD1351 OLED Driver */ 
SSD1351 oled(PTB22,PTB21,PTC13,PTB20,PTE6, PTD15); /* (MOSI,SCLK,POWER,CS,RST,DC) */
oled_text_properties_t textProperties = {0};

/* Instantiate the nRF24L01P Driver */ 
nRF24L01P my_nrf24l01p(PTC6,PTC7,PTC5,PTC4,PTB2,NC);    // mosi, miso, sck, csn, ce, irq

 /* Text Buffer */ 
char text[20]; 

uint8_t screenNum=0;
bool prefix=0; 
bool sentMessageDisplayedFlag=0;
char rxData[TRANSFER_SIZE];
char txData[TRANSFER_SIZE];

/* Pointer for the image to be displayed  */  
const uint8_t *homeBMP = HEXIWEAR_HOME_bmp;
const uint8_t *sendBMP  = HEXIWEAR_SEND_bmp;
const uint8_t *bannerBMP = hexicomm_bmp; 
   
/****************************Call Back Functions*******************************/
/*Send Button */
void ButtonRight(void)
{
    if (!sentMessageDisplayedFlag)
    {
        StartHaptic();
    
        // Send the transmitbuffer via the nRF24L01+
        my_nrf24l01p.write( NRF24L01P_PIPE_P0,  txData, 4 );
    }
}

/*Home Button */
void ButtonLeft(void)
{
    StartHaptic();
    screenNum = 0; 
    
    /*Turn off Green LED */
    sentMessageDisplayedFlag=0;
    greenLed = !sentMessageDisplayedFlag;
    
    /*Redraw Send Button*/
    oled.DrawImage(sendBMP,53,81);
    screenHandler(screenNum,prefix);
}

/*Toggles Between I am @ and Meet @ */
void ButtonUp(void)
{
    if (screenNum !=0)
    {
        StartHaptic();
        
        /*Turn off Green LED */
        sentMessageDisplayedFlag=0;
        greenLed = !sentMessageDisplayedFlag;
        
        /*Redraw Send Button*/
        oled.DrawImage(sendBMP,53,81);

        prefix = !prefix; 
        screenHandler(screenNum,prefix);
    }
}

/*Advances Stage Number */
void ButtonDown(void)
{
    StartHaptic();
    
    /*Turn off Green LED */
    sentMessageDisplayedFlag=0;
    greenLed = !sentMessageDisplayedFlag;
    
    /*Redraw Send Button*/
    oled.DrawImage(sendBMP,53,81);
    
    if (screenNum < NUM_OF_SCREENS -1) {
        screenNum++;
    }
    else
    {   
         screenNum = 0; 
    }
    
    screenHandler(screenNum,prefix);
}


/***********************End of Call Back Functions*****************************/

/********************************Main******************************************/

int main()
{    
    /* Wait Sequence in the beginning for board to be reset then placed in mini docking station*/ 
   
    Thread::wait(6000);
    blueLed=0;
    Thread::wait(500);
    blueLed=1;
    

    /* NRF24l0p Setup */
    my_nrf24l01p.init();
    my_nrf24l01p.powerUp();
    my_nrf24l01p.setAirDataRate(NRF24L01P_DATARATE_250_KBPS);
    my_nrf24l01p.setRfOutputPower(NRF24L01P_TX_PWR_ZERO_DB);
    my_nrf24l01p.setRxAddress(0xE7E7E7E7E8);
    my_nrf24l01p.setTxAddress(0xE7E7E7E7E8);
    my_nrf24l01p.setTransferSize( TRANSFER_SIZE );
    my_nrf24l01p.setReceiveMode();
    my_nrf24l01p.enable();
    
    /* Get OLED Class Default Text Properties */
    oled.GetTextProperties(&textProperties);    

    /* Fills the screen with solid black */         
    oled.FillScreen(COLOR_BLACK);
        
    /* Register callbacks to application functions */
    kw40z_device.attach_buttonLeft(&ButtonLeft);
    kw40z_device.attach_buttonRight(&ButtonRight);
    kw40z_device.attach_buttonUp(&ButtonUp);
    kw40z_device.attach_buttonDown(&ButtonDown);
 
    /* Change font color to white */ 
    textProperties.fontColor   = COLOR_WHITE;
    textProperties.alignParam = OLED_TEXT_ALIGN_CENTER;
    oled.SetTextProperties(&textProperties);
    
    /*Displays the Home Screen*/ 
    displayHome();   
     
    /*Draw Home Button and Send Button*/  
    oled.DrawImage(homeBMP,0,81);
    oled.DrawImage(sendBMP,53,81);
    oled.DrawImage(hexicomm_bmp,0,0);

    while (true) 
    {
        
        // If we've received anything in the nRF24L01+...
        if ( my_nrf24l01p.readable() ) {

            // ...read the data into the receive buffer
            my_nrf24l01p.read( NRF24L01P_PIPE_P0, rxData, sizeof(rxData));
            
            //Set a flag that a message has been received 
            sentMessageDisplayedFlag=1;
            
            //Turn on Green LED to indicate received message 
            greenLed = !sentMessageDisplayedFlag;
            //Turn area black to get rid of Send Button 
            oled.DrawBox (53,81,43,15,COLOR_BLACK);

     
            char name[7];
            
            name[0] = rxData[2];
            name[1] = rxData[3];
            name[2] = ' ';
            name[3] = 's';
            name[4] = 'e';
            name[5] = 'n';
            name[6] = 't';
            
            oled.TextBox((uint8_t *)name,0,20,95,18);

            switch (rxData[0])
            {
                case 'M':
                {
                    oled.TextBox("Meet",0,35,95,18);
                    break;
                }
                case 'I':
                {
                    oled.TextBox(" ",0,35,95,18);
                    break;
                }

                default: {break;}

            }

            switch (rxData[1])
            {
                case '0':
                {
                    oled.TextBox("Where Yall?",0,50,95,18);
                    break;
                }
                case '1':
                {
                    oled.TextBox("@ Stage 1",0,50,95,18);
                    break;
                }
                case '2':
                {
                    oled.TextBox("@ Stage 2",0,50,95,18);
                    break;
                }
                case '3':
                {
                    oled.TextBox("@ Stage 3",0,50,95,18);
                    break;
                }
                 case '4':
                {
                    oled.TextBox("@ Stage 4",0,50,95,18);
                    break;
                }
                 case '5':
                {
                    oled.TextBox("@ Stage 5",0,50,95,18);
                    break;
                }

                default:{break;}
            }
            StartHaptic();
        }
        
        
        Thread::wait(50);
    }
}

/******************************End of Main*************************************/

void StartHaptic(void)  {
    hapticTimer.start(50);
    haptic = 1;
}

void StopHaptic(void const *n) {
    haptic = 0;
    hapticTimer.stop();
}

void displayHome(void)  
{

    oled.TextBox(" ",0,20,95,18);           //Line 1
    oled.TextBox("Where",0,35,95,18);       //Line 2
    oled.TextBox("Yall At?",0,50,95,18);    //Line 3    
    strcpy(txData,"I");                     //Packet[0]
    strcat(txData,"0");                     //Packet[1]
    strcat(txData,NAME);                    //Packet[2:3]
}  


void screenHandler(uint8_t stageNum,uint8_t header)
{

    //Text for Line 1
    oled.TextBox(" ",0,20,95,18);

    //Text for Line 2
    switch(header)                  
    {
        case 0:
        {
            //Packet Encoding for I am @
            strcpy(txData,"I");                 
            oled.TextBox("I am",0,35,95,18);
            break;
        }    
        case 1:
        {
            //Packet Encoding for Meet @
            strcpy(txData,"M");                 
            oled.TextBox("Meet",0,35,95,18);
            break;
        }    
        default:
        {
            break;
        }
    }

    //Text for Line 3
    switch (stageNum)
    {
        case 0:
        {
            displayHome();
            break;
        }

        case 1:
        {
            //Packet Encoding for Stage 1
            strcat(txData,"1");
            oled.TextBox("@ Stage 1",0,50,95,18);
            break;
        }
        case 2:
        {
            //Packet Encoding for Stage 2
            strcat(txData,"2");
            oled.TextBox("@ Stage 2",0,50,95,18);
            break;
        }
        case 3:
        {
            //Packet Encoding for Stage 3
            strcat(txData,"3");
            oled.TextBox("@ Stage 3",0,50,95,18);
            break;
        }
        case 4:
        {
            //Packet Encoding for Stage 4
            strcat(txData,"4");
            oled.TextBox("@ Stage 4",0,50,95,18);
            break;
        }
        case 5:
        {
            //Packet Encoding for Stage 5
            strcat(txData,"5");
            oled.TextBox("@ Stage 5",0,50,95,18);
            break;
        }
        default:
        {
            break;
        }
    }
    
    //Append Initials to txData[2:3]. 
    strcat(txData,NAME);
   
}

