/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: Master
 *
 * Created on 1 de marzo de 2016, 13:22
 */

#include <cstdlib>
#include<stdio.h>                       //Printf
#include <bcm2835.h>                    //IO Raspberry
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>                    //Used for UART
#include <poll.h>                       //Used for UART
#include <cstring>
#include <wiringSerial.h>               //Simpler way to use the serial
//#include <queue>                      //No need, using arrays
//include <libsbp/sbp.h>                //Later?
//#include <libsbp/navigation.h>        //Later?

typedef unsigned char 		u8;
typedef unsigned short 		u16;
typedef unsigned int 		u32;

#define PREAMBLE 0x55
#define MESG_1 0x01
#define MESG_2 0x02
#define ID_1 0x23
#define ID_2 0x6a
#define PayloadLength 34

using namespace std;
unsigned char Payload[PayloadLength];
int USBfd = serialOpen("/dev/ttyACM0", 115200);
int UARTfd = serialOpen("/dev/ttyAMA0", 115200);



void show_queue() { 
    u8 n_sats, flags;
    u32 tow;
    double latitude, longitude, height;
    char arrayToSend [200];
    
    memcpy(&tow, &Payload[0], 4*sizeof(unsigned char));
    memcpy(&latitude, &Payload[4], 8*sizeof(unsigned char));
    memcpy(&longitude, &Payload[12], 8*sizeof(unsigned char));
    memcpy(&height, &Payload[20], 8*sizeof(unsigned char));
    memcpy(&n_sats, &Payload[32], 1*sizeof(unsigned char));
    memcpy(&flags, &Payload[13], 1*sizeof(unsigned char));
    

   char precise_latitude[30];
   snprintf(precise_latitude,30,"%4.10lf",latitude);
   
   char precise_longitude[30];
   snprintf(precise_longitude,30,"%4.10lf",longitude);
   
   char precise_height[30];
   snprintf(precise_height,30,"%4.10lf",height);
   
   sprintf(arrayToSend, "%s,%s,%s,%d\n", 
        precise_latitude, precise_longitude, precise_height, n_sats);
   
   printf(arrayToSend);
   serialPuts (USBfd, arrayToSend); 
}


void process_byte (unsigned char inByte) {
   static int PackageLength = 0;
   static bool PreambleFound = false, Message1Found = false, 
               Message2Found = false, ID1Found = false, ID2Found = false;
   
   while (PackageLength > 0) {
       Payload[abs(PackageLength-PayloadLength)] = inByte;
       PackageLength--;
       break;
   }
   
   if (inByte == PREAMBLE) {
       PreambleFound = true;
   }
   else if (PreambleFound == true && inByte == MESG_1) {
       Message1Found = true;
   }
   else if (Message1Found == true && inByte == MESG_2) {
       Message2Found = true;
   }
   else if (Message2Found == true && inByte == ID_1) {
       ID1Found = true;
   }
   else if (ID1Found == true && inByte == ID_2) {
       ID2Found = true;
   }
   else if (ID2Found == true){
       PackageLength = inByte;
       PreambleFound = false;
       Message1Found = false;
       Message2Found = false;
       ID1Found = false;
       ID2Found = false;
       PackageLength = inByte;
       show_queue();
   }
   else {
       PreambleFound = false;
       Message1Found = false;
       Message2Found = false;
       ID1Found = false;
       ID2Found = false;
   }
}


int main(int argc, char** argv) { 
    //Check UART
    if (UARTfd == -1) {
	printf("Error - Unable to open UART.  Ensure it is not in use by another application.\n");
    }
    else {
        printf("UART opened OK.\n");
    }
    
    //Check USB
    if (USBfd == -1) {
	printf("Error - Unable to open USB.  Ensure it is connected.\n");
    }
    else {
        printf("USB opened OK.\n");
    }
    
    while (1) {
        while (serialDataAvail (UARTfd)) {
          process_byte(serialGetchar (UARTfd));   
        }
    }
    
}

