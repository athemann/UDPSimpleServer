// udpd.c simple UDP server for linux 

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


#define	testVersion "1.0"		// the version number


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>
#include <ctype.h>			// for toupper();

#include "getch.h"			// we want to use the keyboard in key-press mode


// Simple UDP interface functions
int MTU=1500;				// global MTU setting
int CHUNKSIZE=1024;			// file transfers use 1k chunks
int SMALLFILESIZE=65536;	// restrict "small" files to 64k

int UDPSock;									// our socket connection to the network
char GlobalTag[4096];							// our tag if any
struct sockaddr_in UDPRx;						// address holder for receive and reply
struct sockaddr_in UDPTx;						// address holder for sending

void PollUDP(void);								// call this function in the main loop
int OpenUDP(int port);							// start-up our socket
int CloseUDP(void);								// disconnect our socket
int InitUDPTx(struct sockaddr_in * UDPTx,char * address,int port);	// configure our Tx address struct
void ReplyMessage(char buffer[],int length);	// reply to the received message
void SendMessage(char buffer[],int length);		// send a message to the address in UDPTx struct
static int length=sizeof(struct sockaddr_in);	// used for recvfrom()

// user supplied functions
extern int ProcessUDPRaw(int Len, char Packet[], struct sockaddr * UDPRx);
extern int ProcessUDPCommand(int Len, char Command[], char Reply[], struct sockaddr * UDPRx);


int ProcessKey(int key);						// handle a single key press event
void ProcessUDPMessage(int len,char buffer[],struct sockaddr * UDPRx);	// handle a received packet

// helper functions
char * strcpyq(char * dest, char * srce);
char *strcpycommand(char * dest, char * srce);
int strcommandlen(char * message);	// return the length of the command



int _main(int argc,char * argv[])	// a test main for validation
{
	printf("UDPSimpleServer Version %s\n",testVersion);
	int port;	// to be used to hold the allocated port


	// get the port from command line parameters
	if(argc<2){printf("Usage udpd <port>\n");exit(-1);}
	if(sscanf(argv[1],"%d",&port)!=1)
	{
		port=1235;			// default port for debug
		printf("Usage udpd <port>\n");
	}
	printf("Binding to port %d\n",port);

	
	// initialize the network
	OpenUDP(port);			// setup a receiver

	// init the keytboard interface (set the term mode to pass every keystroke)
	InitKbhit();	printf("Press ESC to quit\n");


	// main loop
	while(1)	// loop checking for packets or key presses to process
	{
		PollKeyboardBreak();	// callback is ProcessKey();

		PollUDP();		// check for waiting packets, and callback ProcessUDP(); if so

		sleep(0);		// yield CPU
	}
	
	// clean up
	CloseUDP();
	exit(0);
}

int SetUDPMTU(int mtu){MTU=mtu;if(MTU>16394)MTU=16384;return MTU;}
long SetUDPSmallFileSize(int size){SMALLFILESIZE=size;return SMALLFILESIZE;}
int SetUDPChunkSize(int chunk){CHUNKSIZE=chunk;if(CHUNKSIZE>(MTU-7))CHUNKSIZE=MTU-7;return CHUNKSIZE;}





void PollUDP(void)
{
	static char payload[4096];				// our packet payload buffer - note: MTU need be respected
	int a=recvfrom(UDPSock,payload,sizeof(payload)-1,0,(struct sockaddr*)&UDPRx,&length);
			if(a>0) ProcessUDPMessage(a,payload,(struct sockaddr*)&UDPRx);
}

// initialize a network socket / receiver
int OpenUDP(int port)
{
	// initialize a UDP socket
	int flags;
	UDPSock=socket(AF_INET, SOCK_DGRAM,0);
	flags=fcntl(UDPSock,F_GETFL,0);
	flags|=O_NONBLOCK;
	fcntl(UDPSock,F_SETFL,flags);

	// setup the receiver structure
	memset(&UDPRx,0,sizeof(struct sockaddr));
	UDPRx.sin_family=AF_INET;
	UDPRx.sin_addr.s_addr=htonl(INADDR_ANY);
	UDPRx.sin_port=htons(port);	// we are on a fixed port for this

	int e=bind(UDPSock,(struct sockaddr*)&UDPRx,sizeof(struct sockaddr));
		if(e){printf("ERROR: Bind() errorcode: %d\n",e);return -1;}
	return UDPSock;
}
// initialize a network destinationa address to send packets to
int InitUDPTx(struct sockaddr_in * UDPTx,char * address,int port)
{
	struct hostent * UDPhp;
	memset(UDPTx,0,sizeof(struct sockaddr_in));	
	UDPTx->sin_family=AF_INET;

	UDPhp=gethostbyname(address);		// default to what is typically the gateway
	bcopy((char*)UDPhp->h_addr,(char*)&UDPTx->sin_addr,UDPhp->h_length);
	UDPTx->sin_port=htons(port);		// when we "send" (not reply though), it will be to this port
}
int CloseUDP(void){close(UDPSock);}


//////////////////////////////////////////////////////////////////////
// Handle an incomming packet
void ProcessUDPMessage(int len,char buffer[],struct sockaddr * UDPRx)
{
	static char txbuffer[4096];
	buffer[len]=0;	// and the incomming packet because it's likely a string anyway
	
	if(ProcessUDPRaw(len,buffer,UDPRx))return;

	txbuffer[0]=0;	// terminate the outgoing packet just in case
	
	int commandpos=0;
	int replypos=0;
	
	while(buffer[commandpos] && (commandpos < MTU) )	// while there is still command text in the buffer
	{
		char * reply=&txbuffer[replypos];			// the reply slot
		char * command=&buffer[commandpos];			// the command slot

		*reply=0;	// terminate the reply in case we forget to later
		
		int replylen=ProcessUDPCommand(strcommandlen(command),command,reply,UDPRx);
//		printf("Reply len %d,%s\n",replylen,reply);
		if( (replypos + replylen +1) > MTU)	// packet over flow
		{
			if(replypos)	// if there is a reply to send
			{
				ReplyMessage(txbuffer,replypos);
				memmove(txbuffer,reply,replylen);	// restoart the packet
				replypos=0;	// clear the pointer to the "previous" occupied buffer location
			}
		}

		replypos+=replylen;			// add the new command reply to the aggregate
		commandpos+=strcommandlen(command);
		if(buffer[commandpos]==';')commandpos++;	// get rid of terminator
	}
	if(replypos)ReplyMessage(txbuffer,replypos);
	GlobalTag[0]=0;	// clear the tag to avoid confusion
}



// reply to the sender of the message
void ReplyMessage(char buffer[],int length)
{
	sendto(UDPSock,buffer,length,0,(struct sockaddr*)&UDPRx,sizeof(UDPRx));	
}

// send a message to the address in the UDPTx struct
void SendMessage(char buffer[],int length)
{
	sendto(UDPSock,buffer,length,0,(struct sockaddr*)&UDPTx,sizeof(UDPTx));	
}

// send a chunk of size CHUNKSIZE
int SendUDPFileChunk(long long offset, long long len, char * buffer, char * reply)
{
	// send a file using a header of reply, and appending the "@xxxx:", and of course the data (upto 1024 bytes)
	if( offset > (len-1) )return 0;		// we have attempted to send beyond end of buffer
	char TxPacket[4096];
	strcpy(TxPacket,reply);
	sprintf(&TxPacket[strlen(TxPacket)],"@%llX:",offset);
	int pos=strlen(TxPacket);	// get the position to append the data

	int i; for(i=0;offset+i<len && i<CHUNKSIZE ;i++,pos++)
	{
		TxPacket[pos]=buffer[offset+i];
	}
	ReplyMessage(TxPacket,pos+1);
	return i;
}
int SendUDPSmallFile(long long len, char * buffer,char * reply)	// send a whole file (upto 64k)
{
	if(len>SMALLFILESIZE)return 0;	// we only allow a small file size to be sent this way
	int pos=0;
	while(1)
	{
		int chunk=SendUDPFileChunk(pos,len,buffer,reply);
		if(chunk==0)return 1;
		pos+=chunk;
	}
}

int ReceiveUDPFileChunk(long long len, char * buffer,int arglen, char argument[])
{	// receive the argument of a command into a buffer that starts with the '@' mnemonic
	if(argument[0]!='@')return -1;	// not a valid argument
	long long address=0;
	
	int i; for(i=1;i<arglen;i++)
	{
		if(argument[i]==':')break;
		if(argument[i]>='0' && argument[i]<='9'){address<<=4;address+=argument[i]-'0';}
		else if(argument[i]>='a' && argument[i]<='f'){address<<=4;address+=argument[i]-'a'+10;}
		else if(argument[i]>='A' && argument[i]<='F'){address<<=4;address+=argument[i]-'A'+10;}
	}
	i++;	// remove the ':'
	
	printf("Address %lld\n",address);
	int count; for(count=0;i<arglen;i++,address++,count++)	
	{
		if(address>=len)break;
		buffer[address]=argument[i];
	}
	return count;
}



/////////////////////////////////////////////////////
// String manipulation helper functions
char * strcpyq(char * dest, char * srce)	// copy a quoted string
{	// copy quoted string, removing quote from it
	char * r=dest;
	if(*srce=='"')srce++;	// remove the quote
	while(*srce)
	{
		if(*srce=='"' && *(srce+1)==';')break;	// end of quote
		//if(*srce=='"' && *(srce+1)=='\t')break;	// end of quote
		*dest++=*srce++;
	}
	*dest=0;
	return r;
}

int strcommandlen(char * message)	// return the length of the command
{	
	int inquote=0;
	for(int r=0;;message++,r++)		// r is the return value
	{
		if(*message==0)return r;	// end of string
		if(inquote==0)
		{
			if(*message=='"')inquote=1;
			else if(*message==';')return r;		// command termination
		}
		else // not inquote==0 //
		{
			if(*message=='"' && *(message+1)==';')return r+1;	// final quote
			//if(*message=='"' && *(message+1)=='\t')inquote=0;	// quote tab indicates end of record
		}
	}
}

char *strcpycommand(char * dest, char * srce)
{
	int len=strcommandlen(srce);
	if(len>MTU)len=MTU;
	memcpy(dest,srce,len);
	dest[len]=0;	// terminate
	return dest;
}

int strismin(char * str, char * min)
{
	while(*min)	if(toupper(*str++)!=toupper(*min++))return 0;
	if(*str==0)return 2;	// is complete
	return 1;				// is a partial match
}



