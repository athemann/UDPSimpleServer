//getch.h - header for getch.c

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

int	kbhit(void);	// standard kbhit() like function
int getch(void);	// standard getch() like function - manages multibyte keycodes
int InitKbhit(void);	// setup the terminal mode to accept keypresses; setup callback on exit
int CloseKbhit(void);
void set_conio_terminal_mode();	// "turn on" the keyboard listener, but store the original mode first
	// note that calling the restore happens automatically via callback - see code for more info
	
#define PollKeyboardBreak()	if(kbhit())if(ProcessKey( getch() ) )break
	
/// *** Note: this likely doesn't support Unicode input, rather just the keys used
/// If you have an international keyboard, use with caution.
