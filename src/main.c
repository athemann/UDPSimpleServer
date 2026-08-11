// UDP simple server for linux - no libraries / dependencies

//Copyright 2026 Alexander Themann
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

/////////////////////////////////////////////////
// compile with:
//
// cc main.c udpd.c getch.c -o udpd
//
// better to use a makefile, but not required
/////////////////////////////////////////////////
#define	Version "1.0"		// the version number


#include <stdio.h>			// for printf
#include <stdlib.h>			// for exit (required for using kbhit())
#include <unistd.h>
#include <string.h>			// for string manipulation functions
#include <time.h>			// to get system time

// includes for keyboard and network support functions
#include "getch.h"			// we want to use the keyboard in key-press mode
#include "unikcodes.h"		// definitions of control key codes, such as KEY_ESC
#include "udpd.h"			// helper functions for network

///////////////////////////////////////////////////////////////////////////////////////////
#define MTU	1500			// this is the max allowable packet length for our sent packets
#define CHUNKSIZE 1024		// size of file chunks
#define SMALLFILESIZE 65536	// max size of a "small" file (that can be sent using helper function)
// Note, the "small" file restriction prevents node buffer overflow as the the whole file is sent
// without handshaking, nor checking if the packets are well received by the node
///////////////////////////////////////////////////////////////////////////////////////////

// startup, shutdown, keyboard and packet handler prototypes:
void InitApp(void);			// code that runs at startup
void CloseApp(void);		// code that runs on exit
int ProcessKey(int key);	// handle a single key press event
int ProcessUDPCommand(int Len, char Command[], char Reply[], struct sockaddr * UDPRx);
int ProcessUDPRaw(int Len, char Packet[], struct sockaddr * UDPRx);		// handle a raw packet
//

int main(int argc,char * argv[])
{
	printf("UDPSimpleServer Version %s\n",Version);
	int port;	// to be used to hold the allocated port

	// get the port from command line parameters
	if(argc<2){printf("Usage udpd <port>\n");exit(-1);}
	if(sscanf(argv[1],"%d",&port)!=1)
	{
		port=1235;			// default port for debug
		printf("Usage udpd <port>\n");
	}
	printf("Binding to port %d\n",port);



	
	OpenUDP(port);		// setup a network receiver (bind the port)
	// SetUDPMTU(1500);				// if we want to override the MTU
	// SetUDPChunkSize(1024);		// if we want to overrider the file transfer chunk size
	// SetUDPSmallFileSize(65536);	// if we want to override the file size limit

	InitKbhit();			// set Xterm mode to pass every keystroke
		printf("Press ESC to quit\n");

	InitApp();

	for(;;sleep(0))		// main loop checking for packets or key presses to process
	{
		PollKeyboardBreak();	// callback is ProcessKey();, break if ESC is pressed
		PollUDP();		// check for waiting packets, and callback ProcessUDP(); if so
	}
	
	CloseApp();
	CloseUDP();			// clean up network interface (unbind the port)
	exit(0);			// do not change this, it's required to restore the terminal mode
}



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Application globals:
// Inset application global variables below here
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
char File[65536];


// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Inset application global variables above here
////////////////////////////////////////////////

void InitApp(void)
{
	// initialization stuff
	for(int i=0;i<65535;i++)File[i]=(i&0x1f)+'A';
}
void CloseApp(void)
{
	// shutdown stuff - is called
}


//////////////////////////////////////////////////////////////////////
// Handle key commands (if a key is pressed), the key code will be in 'int key'
int ProcessKey(int key)
{	// add keyboard commands here -> 'key' is the code key pressed (ASCII etc.)
	if(key==KEY_ESC)return 1;		// return 1 to exit main loop
	///////////////////////////////////
	// insert key processing below here
	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	
	
	
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// insert key processing above here
	///////////////////////////////////
	return 0;					// return 0 to continue with main loop
}

//////////////////////////////////////////////////////////////////////
int ProcessUDPRaw(int Len, char Packet[], struct sockaddr * UDPRx)
{	// process the raw packet.  Return 1 if it is processed here, otherwise the command processor will handle it
	/////////////////////////////////////////////////////////
	// insert raw packet processing code below here if needed
	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv


	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// insert raw packet processing code above here if needed
	/////////////////////////////////////////////////////////
	return 0;
}

//////////////////////////////////////////////////////////////////////
int ProcessUDPCommand(int Len, char Command[], char Reply[], struct sockaddr * UDPRx)
{	// process a command.  RxLen is the length of the Rxbuffer.
	// if sending binary data, must set the number of bytes added to Txxbuffer (including the ';')
	int BinaryLen=0;	// set the length if the reply is binary (non Text)

	// note, sending a file will be reordered to precede any other commands.
	// if you unsure of the compatability with the host node, then avoid using complex composite
	// commands when sending files.  If tags are required, send them as independent packets
	// in the host request
	

	// default standard commands: ?  '  #  (ID, time, and tag)
	if(Command[0]=='?')sprintf(Reply,"?UDPD1.0");
	
	else if(Command[0]=='\'')
	{
		long long stime=(long long)time(NULL)*1000;	// get system time, convert to milliseconds
		sprintf(Reply,"'%lld",stime);
	}
	
	else if(Command[0]=='#'){strcpycommand(Reply,Command);strcpycommand(GlobalTag,Command);}

	////////////////////////////////////
	// Insert custom commands below here
	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv	
	else if(Command[0]=='F')	// send a test file
	{
		if(GlobalTag[0])sprintf(Reply,"%s;",GlobalTag);	// add a tag if one is active
		strcat(Reply,"F");
		SendUDPSmallFile(1200,File,Reply);		// send a 1200 byte file
		Reply[0]=0;								// revent default reply
	}
	
	else if(Command[0]=='C')
	{
		sprintf(Reply,"C红");
	}
	
	else if(Command[0]=='@')
	{
		printf("Length %d\n",Len);
		int count=ReceiveUDPFileChunk(65536,File,Len-0,&Command[0]);
		sprintf(Reply,"Done %d",count);
	}

	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// Insert custom commands above here
	////////////////////////////////////
	if(BinaryLen)return BinaryLen;	// if there is binary content, then BinaryLen must be set
	if(Reply[0]!=0)strcat(Reply,";");
	return strlen(Reply);			// if it's a simple text reply, then compute it's length
}




