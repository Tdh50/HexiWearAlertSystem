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
// Instantiate the nRF24L01P Driver  
nRF24L01P my_nrf24l01p(PTC6,PTC7,PTC5,PTC4,PTB2,NC);    // mosi, miso, sck, csn, ce, irq

// Text Buffer  
char text1[20]; // Text Buffer for dynamic value displayed
char text2[20]; // Text Buffer for dynamic value displayed
char text3[20]; // Text Buffer for dynamic value displayed

float accel_data[3]; // Storage for the data from the sensor
float accel_rms=0.0; // RMS value from the sensor
float ax, ay, az; // Integer value from the sensor to be displayed
uint8_t screenNum=0;
bool prefix=0;
bool accelerometer = false;
bool sentMessageDisplayedFlag=0;
char rxData[TRANSFER_SIZE];
char txData[TRANSFER_SIZE];

// Pointer for the image to be displayed   
const uint8_t *SafeBMP = HexiSafe96_bmp;
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
                accelerometer = false;
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
    if(screenNum > 0) {
        StartHaptic();
        //Allow user to go back to correct screen based on srceen number
        //Refer to screenHandler for screen numbers
        if(screenNum == 3 || screenNum == 4 || screenNum == 5) {
            screenNum = screenNum - 2;
            accelerometer = false;
        } else {
            screenNum--;
        } screenHandler(screenNum);
    }
    else{
        
        screenHandler(6);
        
        }
}

//Advances to Heartrate only when user
//is on Hexisafe screen  
void ButtonUp(void)
{
    if (screenNum == 1) {
        StartHaptic();
        screenNum++;
        screenHandler(screenNum);
    }


}

//Advances to Fall Detection only when user
//is on Hexisafe screen  
void ButtonDown(void)
{
    if (screenNum == 1) {
        StartHaptic();
        screenNum= screenNum + 2;
        screenHandler(screenNum);
    }

}


//**********************End of Call Back Functions**************************** 

//*******************************Main***************************************** 

int main()
{
    // Wait Sequence in the beginning for board to be reset then placed in mini docking station 
    blueLed=1;



    // Get OLED Class Default Text Properties  
    oled.GetTextProperties(&textProperties);

    // Register callbacks to application functions  
    kw40z_device.attach_buttonLeft(&ButtonLeft);
    kw40z_device.attach_buttonRight(&ButtonRight);
    kw40z_device.attach_buttonUp(&ButtonUp);
    kw40z_device.attach_buttonDown(&ButtonDown);

    // Change font color to white  
    textProperties.fontColor   = COLOR_WHITE;
    textProperties.alignParam = OLED_TEXT_ALIGN_CENTER;
    oled.SetTextProperties(&textProperties);

    //Displays the Home Screen 
    displayHome();

    while (true) {
        accelero();
        redLed = 1 ;
        Thread::wait(50);
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
            drawAccel();
            break;
        }
        case 6: {
            //AlertNotification
            oled.DrawImage(AlertBMP,10,25);
            }
        default: {
            break;
        }
    }


    //Append Initials to txData[2:3].
    //strcat(txData,NAME);

}

void accelero(void) {
    
    // Configure Accelerometer FXOS8700, Magnetometer FXOS8700
    accel.accel_config();
    //oled.DrawImage(FallPageBMP,0,0);
    int count = 0;// count data reading for accelerometer
    // Fill 96px by 96px Screen with 96px by 96px Image starting at x=0,y=0
    while (1) {

        accel.acquire_accel_data_g(accel_data);
        accel_rms = sqrt(((accel_data[0]*accel_data[0])+(accel_data[1]*accel_data[1])+(accel_data[2]*accel_data[2]))/3);
        printf("%i %4.2f\n\r",count,accel_rms);
        count++;
        wait(0.01);
        greenLed = !greenLed ;
        Thread::wait(200);
    }
}

void drawAccel(void){
            
            
             oled.DrawImage(FallPageBMP,0,0); 
             while(true){   
             ax = accel_data[0];
             ay = accel_data[1];
             az = accel_data[2];
                   // Get OLED Class Default Text Properties
             oled_text_properties_t textProperties = {0};
             oled.GetTextProperties(&textProperties);

             // Set text properties to white and right aligned for the dynamic text
             textProperties.fontColor = COLOR_BLUE;
             textProperties.alignParam = OLED_TEXT_ALIGN_LEFT;
             oled.SetTextProperties(&textProperties);

             // Display Legends
             strcpy((char *) text1,"X-Axis (g):");
             oled.Label((uint8_t *)text1,5,26);

             // Format the value
             sprintf(text1,"%4.2f",ax);
             // Display time reading in 35px by 15px textbox at(x=55, y=40)
             oled.TextBox((uint8_t *)text1,70,26,20,15); //Increase textbox for more digits

             // Set text properties to white and right aligned for the dynamic text
             textProperties.fontColor = COLOR_GREEN;
             textProperties.alignParam = OLED_TEXT_ALIGN_LEFT;
             oled.SetTextProperties(&textProperties);

             // Display Legends
             strcpy((char *) text2,"Y-Axis (g):");
             oled.Label((uint8_t *)text2,5,43);

             // Format the value
             sprintf(text2,"%4.2f",ay);
             // Display time reading in 35px by 15px textbox at(x=55, y=40)
             oled.TextBox((uint8_t *)text2,70,43,20,15); //Increase textbox for more digits

             // Set text properties to white and right aligned for the dynamic text
             textProperties.fontColor = COLOR_RED;
             textProperties.alignParam = OLED_TEXT_ALIGN_LEFT;
             oled.SetTextProperties(&textProperties);

             // Display Legends
             strcpy((char *) text3,"Z-Axis (g):");
             oled.Label((uint8_t *)text3,5,60);

             // Format the value
             sprintf(text3,"%4.2f",az);
             // Display time reading in 35px by 15px textbox at(x=55, y=40)
             oled.TextBox((uint8_t *)text3,70,60,20,15); //Increase textbox for more digits

     Thread::wait(200);  
       
    }
    
    
}

