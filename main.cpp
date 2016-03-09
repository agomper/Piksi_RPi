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
#include <bitset>                       
#include <math.h>  
#include <sys/time.h>
//#include <queue>                      //No need, using arrays
//include <libsbp/sbp.h>                //Later?
//#include <libsbp/navigation.h>        //Later?
using namespace std;

typedef unsigned char 		u8;
typedef unsigned short 		u16;
typedef unsigned int 		u32;

#define PREAMBLE 0x55
#define MESG_1 0x01
#define MESG_2 0x02
#define ID_1 0x23
#define ID_2 0x6a
#define PayloadLength 34
#define pi 3.14159265358979323846

unsigned char Payload[PayloadLength];
int USBfd = serialOpen("/dev/ttyACM0", 115200);
int UARTfd = serialOpen("/dev/ttyAMA0", 115200);
FILE *fp;
char fileName[128];


char *byte_to_binary(int x) {
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1) {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts decimal degrees to radians             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double deg2rad(double deg) {
  return (deg * pi / 180);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts radians to decimal degrees             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double rad2deg(double rad) {
  return (rad * 180 / pi);
}

double calculate_distance(double lat1, double lon1, double lat2, double lon2, char unit) {
    double theta, dist;
    theta = lon1 - lon2;
    dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
    dist = acos(dist);
    dist = rad2deg(dist);
    dist = dist * 60 * 1.1515;
    switch(unit) {
      case 'M':
        break;
      case 'K':
        dist = dist * 1.609344;
        break;
      case 'N':
        dist = dist * 0.8684;
        break;
    }
    return (dist);
}


void show_queue() { 
    static char dataMode = 'N';
    u8 n_sats, flags;
    u32 tow;
    double latitude, longitude, height;
    static double latitudeA, longitudeA, heightA, distance = 0;
    char arrayToSend[200], fixMode[5], fixModeString[15], stringDistance[30];;
    char *statusFlag;
    static int sendToUSBIterator, iteratorUSBDelay = 0;
    static bool firstPoint = true;

    memcpy(&tow, &Payload[0], 4*sizeof(unsigned char));
    memcpy(&latitude, &Payload[4], 8*sizeof(unsigned char));
    memcpy(&longitude, &Payload[12], 8*sizeof(unsigned char));
    memcpy(&height, &Payload[20], 8*sizeof(unsigned char));
    memcpy(&n_sats, &Payload[32], 1*sizeof(unsigned char));
    memcpy(&flags, &Payload[33], 1*sizeof(unsigned char));

    //Fix mode values
    statusFlag = byte_to_binary(flags);
    memcpy(fixMode, &statusFlag[5], 3*sizeof(char));
    //printf("%s\n", fixMode);
    if (strcmp(fixMode, "000") == 0) {
        strcpy(fixModeString, "SPP");
    }
    else if (strcmp(fixMode, "001") == 0) {
        strcpy(fixModeString, "Fixed RTK");
    }
    else if (strcmp(fixMode, "010") == 0) {
        strcpy(fixModeString, "Float RTK");
    }
    else {
        strcpy(fixModeString, "???");
    }
    //printf("%s\n", fixModeString);
    
   char precise_latitude[30];
   snprintf(precise_latitude,30,"%4.10lf",latitude);
   
   char precise_longitude[30];
   snprintf(precise_longitude,30,"%4.10lf",longitude);
   
   char precise_height[30];
   snprintf(precise_height,30,"%4.10lf",height);
   
   
   sprintf(arrayToSend, "$%s,%s,%s,%d,%s&", 
        precise_latitude, precise_longitude, precise_height, n_sats, fixModeString);
   
   
   printf("%s\n", arrayToSend);
   if (iteratorUSBDelay == 50) {
        if (sendToUSBIterator == 10) {
            if (dataMode != 'D') {
                serialPuts (USBfd, arrayToSend);
                printf("Sending to USB...\n");
            }
        
            if (serialDataAvail (USBfd)) {
                dataMode = serialGetchar (USBfd);
                serialFlush(USBfd);
                printf("Mode received: %c\n", dataMode);
                if (dataMode == 'S') {
                    printf("Writing to file\n");
                    fp = fopen(fileName, "a");
                    fprintf(fp, arrayToSend);
                    fputc('\n', fp);
                    fclose(fp);
                }
                else if (dataMode == 'D') {
                    if (firstPoint == true) {
                        firstPoint = false;
                        latitudeA = latitude;
                        longitudeA = longitude;
                        heightA = height;
                    }
                    else {
                        distance = calculate_distance(latitudeA, longitudeA, latitude, longitude, 'K') * 1000;
                        
                        //printf("#%4.10lf&", distance);
                        sprintf(stringDistance, "#%4.10lf&", distance);
                        
                        printf("Distance: %s\n", stringDistance);
                        serialPuts (USBfd, stringDistance);
                           
                    }
                }
                else if (dataMode == 'N') {
                    firstPoint = true;
                    distance = 0;
                }
            }
            sendToUSBIterator = 0;
        }
        else {
            sendToUSBIterator++;
        }
   }
   else {
       iteratorUSBDelay++;
   }
   
       
       
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
    char TimeString[128];
    timeval curTime;
    gettimeofday(&curTime, NULL);
    
    //For the file
    strftime(TimeString, 80, "%Y-%m-%d %H:%M:%S", localtime(&curTime.tv_sec));
    sprintf(fileName, "/home/pi/Desktop/PiksiLogs/Log-%s",TimeString);
    printf("%s\n", fileName);
    
    
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
    return 0;
}

