// udpd.h - header for udpd.c

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


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define EUNKNOWN	-1
#define	EINVAL		-1
#define ERANGE		-2


void PollUDP(void);								// call this function in the main loop
int OpenUDP(int port);							// start-up our socket
int CloseUDP(void);								// disconnect our socket
int InitUDPTx(struct sockaddr_in * UDPTx,char * address,int port);	// configure our Tx address struct
void ReplyMessage(char buffer[],int length);	// reply to the received message
void SendMessage(char buffer[],int length);		// send a message to the address in UDPTx struct
int SendUDPFileChunk(long long offset, long long len, char * buffer, char * reply);
int SendUDPSmallFile(long long len, char * buffer,char * reply);
int ReceiveUDPFileChunk(long long len, char * buffer,int arglen, char argument[]);


int SetUDPMTU(int mtu);
long SetUDPSmallFileSize(int size);
int SetUDPChunkSize(int chunk);


extern int ProcessKey(int key);						// handle a single key press event
extern void ProcessUDPMessage(int len,char buffer[],struct sockaddr * UDPRx);	// handle a received packet
extern char GlobalTag[];


// convenience functions
char * strcpyq(char * dest, char * srce);
char *strcpycommand(char * dest, char * srce);
int strcommandlen(char * message);	// return the length of the command
int strismin(char * str, char * min);	// compare minimal string


