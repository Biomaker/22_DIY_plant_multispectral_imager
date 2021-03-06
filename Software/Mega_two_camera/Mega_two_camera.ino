//*******************************************************
// 
//  Adapted from manufacturer's code to use pcDuino camera
//
// This is designed to take two images from 2 pcDuino cameras using an Arduino Mega
//
// The user can alter baudrates and image sizes by changing the settings as outlined in the functions defined below.
//
// This cketch will only take two images then hold the camera. Adding a for loop shouldn't be too hard to do repeat imaging
//
// Note:
// 1. SD must be formated to FAT16
// 2. Bridge the 53 pin to the Chip sleect pin on the SD shield
/
//
// * SD card attached to SPI bus as follows:
// * MOSI - pin 51
// * MISO - pin 50
// * CLK - pin 49
// * CS - pin 53
//*******************************************************
#include <SPI.h>
#include <SD.h>
 
byte ZERO = 0x00;
byte incomingbyte;
long int j1=0,k1=0,count1=0,ad1=0x0000;
long int j2=0,k2=2,count2=0,ad2=0x0000;
uint8_t MH,ML;
boolean EndFlag1=0, EndFlag2=0;
File  File1, File2;

// Define Functions for pcDuino cameras
 
void SendResetCmd(HardwareSerial &port)
{
    port.write(0x56);
    port.write(ZERO);
    port.write(0x26);
    port.write(ZERO);
}
 
/*************************************/
/* Set ImageSize :
/* <1> 0x22 : 160*120
/* <2> 0x11 : 320*240
/* <3> 0x00 : 640*480
/* <4> 0x1D : 800*600
/* <5> 0x1C : 1024*768
/* <6> 0x1B : 1280*960
/* <7> 0x21 : 1600*1200
/************************************/
void SetImageSizeCmd(byte Size, HardwareSerial &port)
{
    port.write(0x56);
    port.write(ZERO);  
    port.write(0x54);
    port.write(0x01);
    port.write(Size);
}
 
/*************************************/
/* Set BaudRate :
/* <1>¡¡0xAE  :   9600
/* <2>¡¡0x2A  :   38400
/* <3>¡¡0x1C  :   57600
/* <4>¡¡0x0D  :   115200
/* <5>¡¡0xAE  :   128000
/* <6>¡¡0x56  :   256000
/*************************************/
void SetBaudRateCmd(byte baudrate, HardwareSerial &port)
{
    port.write(0x56);
    port.write(ZERO);
    port.write(0x24);
    port.write(0x03);
    port.write(0x01);
    port.write(baudrate);
}
 
void SendTakePhotoCmd(HardwareSerial &port)
{
    port.write(0x56);
    port.write(ZERO);
    port.write(0x36);
    port.write(0x01);
    port.write(ZERO); 
}
 
void SendReadDataCmd(HardwareSerial &port, int opt)
{
  if(opt == 1){
    MH=ad1/0x100;
    ML=ad1%0x100;
    ad1+=0x20;
  } else if(opt==2){
    MH=ad2/0x100;
    ML=ad2%0x100;
    ad2+=0x20;
  };
    port.write(0x56);
    port.write(ZERO);
    port.write(0x32);
    port.write(0x0c);
    port.write(ZERO);
    port.write(0x0a);
    port.write(ZERO);
    port.write(ZERO);
    port.write(MH);
    port.write(ML);
    port.write(ZERO);
    port.write(ZERO);
    port.write(ZERO);
    port.write(0x20);
    port.write(ZERO);
    port.write(0x0a);
    
}
 
void StopTakePhotoCmd(HardwareSerial &port)
{
    port.write(0x56);
    port.write(ZERO);
    port.write(0x36);
    port.write(0x01);
    port.write(0x03);
}

// Setup by initialising cameras
 
void setup()
{
    Serial.begin(38400);
    while (!Serial)
    {
        ; // wait for serial port to connect.
    }

    // Set up SD Card
 
    Serial.print("Initializing SD card...");
    pinMode(53, OUTPUT);
    if (!SD.begin(53))
    {
      // If this happens check you have bridged the SD chip select pin to pin 53 of the Mega
        Serial.println("initialization failed!");
        return;
    }
    Serial.println("initialization done.");
    
    // Set up Cameras
    Serial.print("Initialising Cameras...");

    // COnnect to cameras
    Serial1.begin(115200);
    Serial2.begin(115200);
    delay(100);
    // Send reset commands and wait for this to happen
    SendResetCmd(Serial1);
    SendResetCmd(Serial2);
    delay(4000);
    
    // Set Baudrate - EDIT HERE IF YOU WANT A CHANGE
    SetBaudRateCmd(0x2A,Serial1);
    SetBaudRateCmd(0x2A,Serial2);
    delay(2000);

    
    // MAKE SURE THIS MATCHES YOUR NEW BAUDRATE
    Serial1.begin(38400);
    Serial2.begin(38400);
    delay(100);
    
    // Set image size - EDIT HERE IF YOU WANT A CHANGE
    SetImageSizeCmd(0x1C,Serial1);
    SetImageSizeCmd(0x1C,Serial2);
    delay(3000);
    Serial.println("Cameras Running!");
}
 
void loop()
{
    byte buff1[512], buff2[512];
    int bf1i = 0,bf1j=0, bf2i=0, bf2j=0;
    byte a1[32], a2[32];
    int ii1,ii2;
    int testi1,testi2;

    // take pictures
    Serial.println("Taking Pictures");
    SendTakePhotoCmd(Serial1);
    SendTakePhotoCmd(Serial2);
    delay(6000);

    // Clear returns from taking picture
    while(Serial1.available()>0)
    {
        incomingbyte=Serial1.read();
    }
     while(Serial2.available()>0)
    {
        incomingbyte=Serial2.read();
    }

    // Save Pictue 1

    // Here the data is read in the 32 byte chunks sent by the pcDuino camera.
    // These are then stored in a 512 byte buffer which is written to SD card when full
    // This reduces the handshakes with the SD card and speeds up data writing
    Serial.println("Saving Picture 1...");
    
    File1 = SD.open("serA.jpg", FILE_WRITE); //The file name should not be too long
    
    delay(200);
 
    while(!EndFlag1)
    {
        j1=0;
        k1=0;
        count1=0;
        //mySerial.flush();
        SendReadDataCmd(Serial1,1);
        delay(200); // Frustrating necessary or else you don't always get data
        Serial.print(Serial1.available());
        while(Serial1.available()>0)
        {
            incomingbyte=Serial1.read();
            k1++;
            delay(1); //250 for regular
            if((k1>5)&&(j1<32)&&(!EndFlag1))
            {
                a1[j1]=incomingbyte;
                if((j1==1)&&(incomingbyte==0xD9)&&buff1[32*(bf1i-1)+31]==0xFF){
                    EndFlag1=1;
                }
                else if((a1[j1-1]==0xFF)&&(a1[j1]==0xD9))     //tell if the picture is finished
                {
                    EndFlag1=1;
                }
                j1++;
                count1++;
            }
        }
        for(ii1=0; ii1<count1; ii1++){
          buff1[32*bf1i + ii1] = a1[ii1];
        }

        // Comment out the loop below if you don't want the data bytes sent to the serial monitor
        for(testi1=0;testi1<count1;testi1++)
        {
            if(a1[testi1]<0x10)  Serial.print("0");
            Serial.print(a1[testi1],HEX);           // observe the image through serial port
            Serial.print(" ");
        }
        Serial.println();
        
        
        bf1i++;
        if((bf1i == 16)){
          File1.write(buff1,bf1i*32);
          bf1i = 0;
        }
        else if(EndFlag1){
          for(bf1j=0;bf1j<32*(bf1i-1)+count1;bf1j++){
            File1.write(buff1[bf1j]);
          }
          Serial.println();
        }
    }
 
    File1.close();
    Serial.println("Finished writing data for Camera 1");

    // Save Pictue 2

    Serial.println("Saving Picture 2...");
    File2 = SD.open("serB.jpg", FILE_WRITE);
    while(!EndFlag2)
    {
        j2=0;
        k2=0;
        count2=0;
        //mySerial.flush();
        SendReadDataCmd(Serial2,2);
        delay(200);
        Serial.print(Serial2.available());
        while(Serial2.available()>0)
        {
            incomingbyte=Serial2.read();
            k2++;
            delay(1); //250 for regular
            if((k2>5)&&(j2<32)&&(!EndFlag2))
            {
                a2[j2]=incomingbyte;
                if((j2==1)&&(incomingbyte==0xD9)&&buff2[32*(bf2i-1)+31]==0xFF){
                    EndFlag1=1;
                }
                if((a2[j2-1]==0xFF)&&(a2[j2]==0xD9))     //tell if the picture is finished
                {
                    EndFlag2=1;
                }
                j2++;
                count2++;
            }
        }
        for(testi2=0;testi2<count2;testi2++)
        {
            if(a2[testi2]<0x10)  Serial.print("0");
            Serial.print(a2[testi2],HEX);           // observe the image through serial port
            Serial.print(" ");
        }
        Serial.println();
        
        for(ii2=0; ii2<count2; ii2++){
          buff2[32*bf2i + ii2] = a2[ii2];
        }
        bf2i++;
        if((bf2i == 16)){
          File2.write(buff2,bf2i*32);
          bf2i = 0;
        }
        else if(EndFlag2){
          for(bf2j=0;bf2j<32*(bf2i-1)+count2;bf2j++){
            File2.write(buff2[bf2j]);
          }
          Serial.println();
        }
    }
 
    File2.close();
    Serial.println("Finished writing data for Camera 2");

  // Now hold until reboot
    
    while(1);
}

