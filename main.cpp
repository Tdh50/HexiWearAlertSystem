#include "mbed.h"
#include "Hexi_KW40Z.h"
#include "FXOS8700.h"
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
void accelero(void);
void drawAccel(void);
void displayHome();
void screenHandler(uint8_t screen);


DigitalOut redLed(LED1,1);
DigitalOut greenLed(LED2,1);
DigitalOut blueLed(LED3,1);
DigitalOut haptic(PTB9);

// Define timer for haptic feedback
RtosTimer hapticTimer(StopHaptic, osTimerOnce);
//
FXOS8700 accel(PTC11, PTC10);
// Instantiate the Hexi KW40Z Driver (UART TX, UART RX)
KW40Z kw40z_device(PTE24, PTE25);
Serial pc(USBTX, USBRX); // Serial interface
// Instantiate the SSD1351 OLED Driver
SSD1351 oled(PTB22,PTB21,PTC13,PTB20,PTE6, PTD15); // (MOSI,SCLK,POWER,CS,RST,DC)
oled_text_properties_t textProperties = {0};


/*Create a Thread to handle sending BLE Sensor Data */
Thread txThread;


// Text Buffer
char text1[20]; // Text Buffer for dynamic value displayed
char text2[20]; // Text Buffer for dynamic value displayed
char text3[20]; // Text Buffer for dynamic value displayed
char text [20];

float accel_data[3]; // Storage for the data from the sensor
float accel_rms=0.0; // RMS value from the sensor
float ax, ay, az; // Integer value from the sensor to be displayed
uint8_t timer = 30;
uint8_t screenNum=0;
bool prefix=0;
bool accelerometer = false;
bool sentMessageDisplayedFlag=0;
char rxData[TRANSFER_SIZE];
char txData[TRANSFER_SIZE];
int16_t x=0,y=0,z=0;

// Pointer for the image to be displayed
//const uint8_t *SafeBMP = HexiSafe96_bmp;
const uint8_t *HeartBMP = HeartRate_bmp;
const uint8_t *FallBMP = FallDet_bmp;
const uint8_t *FallPageBMP = FallDetPage_bmp;
const uint8_t *HomeBMP = Home_bmp;
const uint8_t *HeartPageBMP = HeartRatePage_bmp;
const uint8_t  *AlertBMP = Alert_bmp;

//***************************Call Back Functions******************************
//Enter Button
void ButtonRight(void)
{
    // All screens other than 1 have either and enter button
    // or a home buttom.
    if(screenNum != 0) {
        StartHaptic();
        switch(screenNum) {
            case 1: {
                screenNum=screenNum + 2;
                screenHandler(screenNum);
                break;
            }
            case 2: {
                screenNum = screenNum + 2;
                break;
            }
            case 3:
            case 4: {
                screenNum = 0;
                break;
            }
            case 5: {
                screenNum = 0;
                break;
            }
            default: {
                break;
            }
        }
        screenHandler(screenNum);
    }
}

//Back Button
void ButtonLeft(void)
{
    StartHaptic();
    if(screenNum > 0) {
        //Allow user to go back to correct screen based on srceen number
        //Refer to screenHandler for screen numbers
        if(screenNum == 3 || screenNum == 4 || screenNum == 2) {
            screenNum = screenNum - 2;
        } else {
            screenNum--;
        }
    } 
    if(screenNum == 0) {
        screenNum = 5;
    }
    screenHandler(screenNum);
}

//Advances to Heartrate only when user
//is on Hexisafe screen
void ButtonUp(void)
{
    if (screenNum == 0) {
        StartHaptic();
        screenNum++;
        screenHandler(screenNum);
    }


}

//Advances to Fall Detection only when user
//is on Hexisafe screen
void ButtonDown(void)
{
    if (screenNum == 0) {
        StartHaptic();
        screenNum= screenNum + 2;
        screenHandler(screenNum);
    }

}

void PassKey(void)
{
    StartHaptic();
    strcpy((char *) text,"PAIR CODE");
    oled.TextBox((uint8_t *)text,0,25,95,18);

    /* Display Bond Pass Key in a 95px by 18px textbox at x=0,y=40 */
    sprintf(text,"%d", kw40z_device.GetPassKey());
    oled.TextBox((uint8_t *)text,0,40,95,18);
}


//**********************End of Call Back Functions****************************

//*******************************Main*****************************************

int main()
{
    // Wait Sequence in the beginning for board to be reset then placed in mini docking station
    accel.accel_config();

    // Get OLED Class Default Text Properties
    oled.GetTextProperties(&textProperties);

    // Register callbacks to application functions
    kw40z_device.attach_buttonLeft(&ButtonLeft);
    kw40z_device.attach_buttonRight(&ButtonRight);
    kw40z_device.attach_buttonUp(&ButtonUp);
    kw40z_device.attach_buttonDown(&ButtonDown);
    oled_text_properties_t textProperties = {0};
    oled.SetTextProperties(&textProperties);

    //Passcode
    kw40z_device.attach_passkey(&PassKey);

    // Change font color to white
    textProperties.fontColor   = COLOR_WHITE;
    textProperties.alignParam = OLED_TEXT_ALIGN_CENTER;

    //txThread.start(txTask); /*Start transmitting Sensor Tag Data */

    //Displays the Home Screen
    displayHome();
    //bool trigger = 0;

    while (true) {
        accel.acquire_accel_data_g(accel_data);
        accel_rms = sqrt(((accel_data[0]*accel_data[0])+(accel_data[1]*accel_data[1])+(accel_data[2]*accel_data[2]))/3);
        x = accel_data[0] *10000;
        y = accel_data[1] *10000;
        z = accel_data[2] *10000;
        printf("x = %4.4f y = %4.4f z = %4.4f\n\rx = %i y = %i z = %i\n\r",accel_data[0],accel_data[1],accel_data[2],x,y,z);
        if(screenNum == 4) {
            drawAccel();
        }
        Thread::wait(300);
    }


}

//*****************************End of Main************************************
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
            //Switching to HeartBMP
            oled.DrawImage(HeartBMP,0,0);
            break;
        }
        case 2: {
            //Switching to FallBMP
            oled.DrawImage(FallBMP,0,0);
            break;
        }
        case 3: {
            //Switching to HeartPageBMP
            oled.DrawImage(HeartPageBMP,0,0);
            break;
        }
        case 4: {
            //Switching to FallPageBMP
            oled.DrawBox (23,18,50 ,50 , COLOR_BLACK);
            break;
        }
        case 5: {
            //Switching to alarm
            oled.DrawImage(AlertBMP,0,0);
            break;
        }
        default: {
            break;
        }
    }


    //Append Initials to txData[2:3].
    //strcat(txData,NAME);

}

void drawAccel(void)
{

    textProperties.fontColor = COLOR_GREEN;
    textProperties.alignParam = OLED_TEXT_ALIGN_LEFT;
    oled.SetTextProperties(&textProperties);

    // Display Legends
    strcpy((char *) text1,"X-Axis (g):");
    oled.Label((uint8_t *)text1,5,26);
    strcpy((char *) text2,"Y-Axis (g):");
    oled.Label((uint8_t *)text2,5,43);
    strcpy((char *) text3,"Z-Axis (g):");
    oled.Label((uint8_t *)text3,5,60);

    // Format the value
    sprintf(text1,"%4.2f", accel_data[0]);
    // Display time reading in 35px by 15px textbox at(x=55, y=40)
    oled.TextBox((uint8_t *)text1,70,26,20,15); //Increase textbox for more digits

    // Format the value
    sprintf(text2,"%4.2f",accel_data[1]);
    // Display time reading in 35px by 15px textbox at(x=55, y=40)
    oled.TextBox((uint8_t *)text2,70,43,20,15); //Increase textbox for more digits


    // Format the value
    sprintf(text3,"%4.2f", accel_data[2]);
    // Display time reading in 35px by 15px textbox at(x=55, y=40)
    oled.TextBox((uint8_t *)text3,70,60,20,15); //Increase textbox for more digits

}

// txTask() transmits the sensor data
void txTask(void)
{

    while (true) {

        //UpdateSensorData();
        kw40z_device.SendSetApplicationMode(GUI_CURRENT_APP_SENSOR_TAG);

        //Send Accel Data.
        kw40z_device.SendAccel(x,y,z);
    }
}


