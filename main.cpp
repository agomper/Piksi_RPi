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
//#include <queue>
//#include "queuelist.h"

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
struct pollfd pfd[1];
//queue <unsigned char> PackageQueue;
unsigned char Payload[PayloadLength];


void uart_setup() {
        //-------------------------
	//----- SETUP USART 0 -----
	//-------------------------
	//At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD
        
        
	//OPEN THE UART
	//The flags (defined in fcntl.h):
	//	Access modes (use 1 of these):
	//		O_RDONLY - Open for reading only.
	//		O_RDWR - Open for reading and writing.
	//		O_WRONLY - Open for writing only.
	//
	//	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	//											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	//											immediately with a failure status if the output can't be written immediately.
	//
	//	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
	pfd[0].fd = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
        pfd[0].events = POLLIN;
        
	if (pfd[0].fd == -1) {
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}
        else {
                printf("Puerto UART abierto correctamente\n");
        }
	
	//CONFIGURE THE UART
	//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)
	struct termios options;
	tcgetattr(pfd[0].fd, &options);
	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
        
	tcflush(pfd[0].fd, TCIFLUSH);
	tcsetattr(pfd[0].fd, TCSANOW, &options);
}

void show_queue() { 
    u8 n_sats, flags;
    u32 tow;
    double latitude, longitude, height;
    
    unsigned char tow_bytes[4];
    memcpy(tow_bytes, &Payload[0], 4*sizeof(unsigned char));
    memcpy(&tow, &Payload[0], 4*sizeof(unsigned char));
    
    //unsigned char latitude_bytes[8];
    //memcpy(latitude_bytes, &Payload[4], 8*sizeof(unsigned char));
    memcpy(&latitude, &Payload[4], 8*sizeof(unsigned char));
    
    //unsigned char longitude_bytes[8];
    //memcpy(longitude_bytes, &Payload[12], 8*sizeof(unsigned char));
    memcpy(&longitude, &Payload[12], 8*sizeof(unsigned char));
    
    memcpy(&height, &Payload[20], 8*sizeof(unsigned char));
    memcpy(&n_sats, &Payload[32], 1*sizeof(unsigned char));
    memcpy(&flags, &Payload[13], 1*sizeof(unsigned char));
    
    //for (int i=4; i<12; i++)
    //    printf ("%x ", tow_bytes[i]);
    //    
    //printf("\n");
    printf("Latitude: %Lf   Longitude: %Lf  Height: %Lf   Sats: %d \n", 
            latitude, longitude, height, n_sats);
    
    /*for (int i=0; i<PayloadLength; i++)
        printf ("%x ", Payload[i]);
        
    printf("\n");*/
}



void process_byte (unsigned char inByte) {
   static int PackageLength = 0/*, IteratorControl = 10*/;
   static bool PreambleFound = false, Message1Found = false, 
               Message2Found = false, ID1Found = false, ID2Found = false;
   //printf ("Procesando %x\n", inByte);
   
   while (PackageLength > 0/* && IteratorControl == 0*/) {
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
       //printf ("Nuevo paquete con %x\n", inByte);
       PreambleFound = false;
       Message1Found = false;
       Message2Found = false;
       ID1Found = false;
       ID2Found = false;
       PackageLength = inByte;
       //IteratorControl--;
       show_queue();
   }
   else {
       PreambleFound = false;
       Message1Found = false;
       Message2Found = false;
       ID1Found = false;
       ID2Found = false;
   }
   
   
    
   
//   if (inByte == 0x55) {                //SBP Preamble
//      //printf("New Package\n");
//      NewPackage = true;
//      PackageQueue.push(inByte);        //Insert the byte in the queue
//   }
//   else if (NewPackage == true) {
//      if (PackageQueue.size() == 1 && inByte == 0x01 ) {     //Code HEX Message 1 - Message type are in positions 1 and 2
//        //printf("MessageCode1 True\n");
//        MessageCode1 = true;
//        PackageQueue.push(inByte);      //Insert the byte in the queue
//      }
//      else if (PackageQueue.size() == 2 && inByte == 0x02 && MessageCode1 == true) {  //Code HEX Message 2
//        //printf("MessageCode2 True\n");
//        MessageCode2 = true;
//        PackageQueue.push(inByte);      //Insert the byte in the queue
//      }
//      else if (MessageCode2 == true) {  //Message Type Match
//        if (PackageQueue.size() == 5) {  //Length byte in position 5
//            PackageLength = inByte + 8;   //Length = Payload + 8 = Fixed bytes (Preamble, Type, Sender, Length and CRC)
//        }
//          
//        PackageQueue.push(inByte);
//      }
//      else if (MessageCode2 == false){ //No match message type. Erase.
//        clean_queue();
//        NewPackage = false;
//        MessageCode1 = false;
//      }
//   }
//
//   if (PackageQueue.size() == PackageLength) { //Got all the package. Payload + 8
//      show_queue();
//      //process_message(PackageQueue.size()); //Process Message
//      NewPackage = false;
//      MessageCode1 = MessageCode2 = false; //Reset values
//   }
}


//void process_message(int MesLength) { //Cut the message in pieces
//  byte buffer_4bytes[4];
//  
//  for (int i=0; i<10; i++) {
//    PackageQueue.pop();
//  }
//  
//  for (int i=3; i!=0; i--) {
//    buffer_4bytes[i] = PackageQueue.pop();
//  }
//  bytes4_to_int(buffer_4bytes);
//  
//
//  clean_queue();
//}


int main(int argc, char** argv) {
    uart_setup();
    
    if (pfd[0].fd != -1) { //UART was opened 
        while (1) {
            int event = poll(pfd, 2, 0);
            if (event > 0 ) {  //
                unsigned char rx_buffer[1];
                int rx_length = read(pfd[0].fd, (void*)rx_buffer, 1);		//Filestream, buffer to store in, number of bytes to read (max)
                if (rx_length > 0) {
                    //printf ("Se ha leido %d bytes y era %x\n", rx_length, rx_buffer[0]);
                    process_byte(rx_buffer[0]);
                }
                    
                
            }
        }
    }
    
}

