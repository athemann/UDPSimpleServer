// getch.c - support for getting "getch" like keyboard behaviour from a linux terminal window

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

#include <stdio.h>
#include <unistd.h>
#include "unikcodes.h"

#include <fcntl.h>
#include <termios.h>

#include <stdlib.h>
#include <string.h>
#include <sys/select.h>




static struct termios orig_termios;

static void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

static void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);	
    cfmakeraw(&new_termios);
    new_termios.c_oflag|=OPOST;
    tcsetattr(0, TCSANOW, &new_termios);
}

int InitKbhit(void)
{	
	set_conio_terminal_mode();
}
int CloseKbhit(void)
{
	reset_terminal_mode();
}
int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

int _getch()
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
         return c;
        
    }
}

int getch()
{
	// uses _getch to assemble multi-byte keycodes
	int a,b;
	a=_getch();
	if(kbhit()==0)return a;
	if(a==27)a=_getch()+256;
	if(a==347)if(kbhit())a=_getch()+256*2;
	if(a==335)if(kbhit())a=_getch()+256*3;
	if(a==566)if(kbhit())_getch();
	if(a==563)if(kbhit())_getch();
	if(a==565)if(kbhit())_getch();
	if(a==561)
	{
		if(kbhit())a=_getch()+256*4;
		if(kbhit())_getch();
	}
	if(a==562)
	{
		if(kbhit())b=_getch();
		if(b==126)a=562;
		else a=b+256*5;
		if(a==1328)if(kbhit())_getch();
		if(a==1329)if(kbhit())_getch();
		if(a==1332)if(kbhit())_getch();
	}
	if(a==1083)if(kbhit())a=_getch()+256*6;
	

	if(a==1584)if(kbhit())_getch();
	return a;
}

