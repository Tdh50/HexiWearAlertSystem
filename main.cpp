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
#define NUM_OF_SCREENS 4
#define TRANSFER_SIZE   4

void StartHaptic(void);
void StopHaptic(void const *n);
void txTask(void);

void displayHome();
void screenHandler(uint8_t screen);

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
const uint8_t *SafeBMP = HexiSafe96_bmp;
const uint8_t *HeartBMP = HeartRate_bmp;
const uint8_t *FallBMP = FallDet_bmp;
const uint8_t *FallPageBMP = FallDetPage_bmp;
const uint8_t *HomeBMP = Home_bmp;
const uint8_t *HeartPageBMP = HeartRatePage_bmp;


/****************************Call Back Functions*******************************/
/*Enter Button */
void ButtonRight(void)
{
    // All screens other than 1 have either and enter button 
    // or a home buttom. 
    if(screenNum != 1) {
        StartHaptic();
        switch(screenNum) {
            case 0: {
                screenNum++;
                screenHandler(screenNum);
                break;
            }
            case 2: {
                screenNum = screenNum + 2;
                screenHandler(screenNum);
                break;
            }
            case 3: {
                screenNum = screenNum + 2;
                screenHandler(screenNum);
                break;
            }
            case 4:
            case 5: {
                displayHome();
                screenNum = 0;
                break;
            }
            default: {
                break;
            }
        }
    }
}

/*Back Button */
void ButtonLeft(void)
{
    if(screenNum > 0) {
        StartHaptic();
        //Allow user to go back to correct screen based on srceen number
        //Refer to screenHandler for screen numbers
        if(screenNum == 3 || screenNum == 4 || screenNum == 5) {
            screenNum = screenNum - 2;
        } else {
            screenNum--;
        }
        screenHandler(screenNum);
    }

}

/*Advances to Heartrate only when user
is on Hexisafe screen */
void ButtonUp(void)
{
    if (screenNum == 1) {
        StartHaptic();
        screenNum++;
        screenHandler(screenNum);
    }


}

/*Advances to Fall Detection only when user
is on Hexisafe screen */
void ButtonDown(void)
{
    if (screenNum == 1) {
        StartHaptic();
        screenNum= screenNum + 2;
        screenHandler(screenNum);
    }

}


/***********************End of Call Back Functions*****************************/

/********************************Main******************************************/

int main()
{
    /* Wait Sequence in the beginning for board to be reset then placed in mini docking station*/
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

    while (true) {

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

            switch (rxData[0]) {
                case 'M': {
                    oled.TextBox("Meet",0,35,95,18);
                    break;
                }
                case 'I': {
                    oled.TextBox(" ",0,35,95,18);
                    break;
                }

                default: {
                    break;
                }

            }

            switch (rxData[1]) {
                case '0': {
                    oled.TextBox("Where Yall?",0,50,95,18);
                    break;
                }
                case '1': {
                    oled.TextBox("@ Stage 1",0,50,95,18);
                    break;
                }
                case '2': {
                    oled.TextBox("@ Stage 2",0,50,95,18);
                    break;
                }
                case '3': {
                    oled.TextBox("@ Stage 3",0,50,95,18);
                    break;
                }
                case '4': {
                    oled.TextBox("@ Stage 4",0,50,95,18);
                    break;
                }
                case '5': {
                    oled.TextBox("@ Stage 5",0,50,95,18);
                    break;
                }

                default: {
                    break;
                }
            }
            StartHaptic();
        }


        Thread::wait(50);
    }
}

/******************************End of Main*************************************/
//Intiates Vibration
void StartHaptic(void)
{
    hapticTimer.start(50);
    haptic = 1;
}

void StopHaptic(void const *n)
{
    haptic = 0;
    hapticTimer.stop();
}

void displayHome(void)
{
    oled.DrawImage(HomeBMP,0,0);
}


void screenHandler(uint8_t screen)
{
    //Switching screens
    switch(screen) {
        case 0: {
            displayHome();
            break;
        }
        case 1: {
            //Switching to SafeBMP
            oled.DrawImage(SafeBMP,0,0);
            break;
        }
        case 2: {
            //Switching to HeartBMP
            oled.DrawImage(HeartBMP,0,0);
            break;
        }
        case 3: {
            //Switching to FallBMP
            oled.DrawImage(FallBMP,0,0);
            break;
        }
        case 4: {
            //Switching to HeartPageBMP
            oled.DrawImage(HeartPageBMP,0,0);
            break;
        }
        case 5: {
            //Switching to FallPageBMP
            oled.DrawImage(FallPageBMP,0,0);
            break;
        }
        default: {
            break;
        }
    }


    //Append Initials to txData[2:3].
    //strcat(txData,NAME);

}

