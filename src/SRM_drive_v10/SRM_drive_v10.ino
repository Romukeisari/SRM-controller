//
// This file is part of the SRM-controller project.
//
// Copyright (C) 2025 OH2NLT <juha.niinikoski@kolumbus.fi>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// SRM driver tests OH2NLT / 27.08.2025
// new phase drive strategy 30.08.2025
// encoder pulse counter & Lock Unlock drive tests 15.09.2025
// EEPROM P-lock state memory 16.09.2025
// I/O definitions for factory PCB 22.09.2025
// Buzzer & Error Code blink added 24.09.2025
// Buzzer tone tuned 20.10.2025  v1.0
//
// Some observations
// Motor resistance 1,35ohm / phase
// Phase inductance wary between about 1,6mH to 4,4mH depending rotor position
// phase coil saturation. On pole ~4ms, between poles ~1ms

// actuator stroke about 155 encoder A or B pulses
// tests with actual transmission. 104 pulses switches state ok. = 35 TCOUNT rounds.
// actuator movement about 34-35 deg


#include <EEPROM.h>

#define U_SW 10    // Motor Phase switch drive signals
#define V_SW 11
#define W_SW 12
#define A_SW 9     // Motor Power switch

#define PH_LEN 1  // phase length(ms)
#define PWR_LEN 4 // Motoring time

#define FWD 17      // Forward drive switch, test
#define REW 15      // Reverse drive switch, test
#define PBUT 15     // P-lock push Button, 0 (GND) = pressed
#define BRKSW 16    // Brake light switch, 1 (12V) = pressed

#define ENCA 3      // IQ encoder input pins
#define ENCB 2

#define BUZZ 13      //Miniature speaker / Buzzer
#define TEST_IN 7       // test input

#define START_PULSES 10 // rotor start position (U-V) repeats 24.09.2025

//Buzzer
//#define CYC 400     // 1/2 cycle (us) -> 990Hz
//#define LEN 250     // number of cycles

#define CYC 200       // 2.22kHz Mag Transducer PUI AT-1224-TWT-5V-2-R
#define LEN 500

//#define CYC 102       // 4.17kHz Piezzo TDK PS1240P02BT
//#define LEN 1000

int tctr;               // SRM travel counter
#define TCOUNT 45       // Lock - Unlock travel (Rotor U->V->W->U shifts)

volatile int encctr = 0;    // encoder +/- counter, + = forward (open)

#define BTNLED 5            // Button Park Lock LED
#define P_FLAG 10           // EEPROM address for the status byte
#define P_ON  0xAA          // flag for P On condition
#define P_OFF 0x55          // flag for P Off condition
unsigned char pstatus = 0;  // Park status

#define F_ERR 0x01          // forward drive / open error
#define R_ERR 0x02          // reverse drive / close error
#define EE_ERR 0x04         // EEPROM status error
unsigned char op_err = 0;   // operation error

#define BOARDLED 8         // controller board LED

//__________________________
// Setup & system init
void setup()
 {
   Serial.begin(115200);    // open the serial port
    
   pinMode(LED_BUILTIN, OUTPUT);  // Nano board LED (13)
   pinMode(BTNLED, OUTPUT);       // Push button park lock LED
   pinMode(BOARDLED, OUTPUT);     // controller board LED
   pinMode(BUZZ, OUTPUT);         // miniature speaker
   
   pinMode(FWD, INPUT_PULLUP);      // control inputs
   pinMode(REW, INPUT_PULLUP);
   pinMode(PBUT, INPUT_PULLUP);
   pinMode(BRKSW, INPUT);
   pinMode(TEST_IN, INPUT_PULLUP);  // test input
   
   pinMode(U_SW, OUTPUT);           // Phase drivers
   pinMode(V_SW, OUTPUT);
   pinMode(W_SW, OUTPUT);
   pinMode(A_SW, OUTPUT);
         
  digitalWrite(LED_BUILTIN, HIGH);   // started

 // digitalWrite(A_SW, HIGH);   // power on, Forse start position
 // digitalWrite(U_SW, HIGH);   // SRM motor phase drive switches
 // delay(500);

 attachInterrupt(1, enca, RISING); // IQ encoder code INT 1 / encoder A

 pstatus = EEPROM.read(P_FLAG);   // get last P-Lock status

 Serial.println();
 Serial.println();
 Serial.println("SRM motor driver OH2NLT / v1.0 20.10.2025");
 Serial.println("For Nissan Leaf P-lock actuator");
 }
 
//____________________________________
// very simple beep generator. 
 void buzz(int cyc, int len, int num) // sound buzzer 1/2 cycle(us), length n of cycles, number of beeps
   {
    int x, y;
    for(x=0; x<num; x++)              // number of beeps
      {
      for(y=0; y<len; y++)            // sound
       { 
       digitalWrite(BUZZ, HIGH);
       delayMicroseconds(cyc);
       digitalWrite(BUZZ, LOW);
       delayMicroseconds(cyc);
       }
      for(y=0; y<len; y++)            // silence
       {
       delayMicroseconds(2*cyc);
       }
       
      }
   }
   
//___________________________________
// Error code blinker
    
void blink_code(int code)
   {
   int x;

     for(x=0; x<code; x++)
        {
         digitalWrite(BTNLED, HIGH);
         delay(40);
         digitalWrite(BTNLED, LOW);
         delay(300);                    
        }
     delay(1000);
   }
   
//____________________________________
// IQ encoder counter code

void enca()
{
if(digitalRead(ENCB) == 0)
 encctr ++;
else
 encctr --; 
}

void check_encctr(int lim)           // check encoder. Set error if not moved
 {
// positive = open lock, negative = close lock

  Serial.print("Error flags ");
  Serial.print(op_err);
  Serial.print(" Encoder ");
  Serial.print(encctr);
  Serial.println();

  if(lim < 0)                    // close
  {
   if(encctr < lim)
   {
    Serial.println("Sucesfull Close ");
    op_err = op_err & ~R_ERR;  // clear Error flag   
   }
   else
   {
    Serial.println("Error Close ");
    op_err = op_err | R_ERR;   // flag Error
   }
  }

  else                          // open
   if(encctr > lim)
   {
    Serial.println("Sucesfull Open ");
    op_err = op_err & ~F_ERR;  // clear Error flag    
   }
   else
   {
    Serial.println("Error Open ");
    op_err = op_err | F_ERR;   // flag Error
   }
 }
 
//____________________________________

void start_position()           // Force motor to start position
 {
 int x;

 digitalWrite(A_SW, HIGH);      // power on
 for(x=0; x<START_PULSES; x++)  // repeats
  {
  digitalWrite(U_SW, HIGH);     // Start from UV-phase
  digitalWrite(V_SW, HIGH);
  delay(2); 
  digitalWrite(U_SW, LOW);
  digitalWrite(V_SW, LOW);
  delay(4);
  }
 digitalWrite(A_SW, LOW);     // power off

 encctr = 0;                  // reset encoder counter
 tctr = TCOUNT;               // set travel
 }

void eval_pstatus(void)       // evaluate P-lock status
 {
 int x;
  
  switch (pstatus)
    {
    case P_ON:                    // P-lock on
      digitalWrite(BTNLED, HIGH); // LED on
      op_err = op_err & ~EE_ERR;  // clear Error flag
      break;
      
    case P_OFF:                   // P-lock off
      digitalWrite(BTNLED, LOW);  // LED off
      op_err = op_err & ~EE_ERR;  // clear Error flag
      break;

    default:                      // all others = Error
      op_err = op_err | EE_ERR;   // flag Error
  }
 }

void write_pstatus()   // write P-lock status into the EEPROM
  {
   EEPROM.write(P_FLAG, pstatus);
                        // put confirmation code here
  }

// SRM motor drive
//__________________________________________________

void drive_forward()                // drive Forward (Open Lock)
{
start_position();                   // seek start position
 digitalWrite(LED_BUILTIN, HIGH);   // Operate
 digitalWrite(A_SW, HIGH);          // power on
  
 while(tctr != 0)
  { 
  digitalWrite(U_SW, HIGH);   // SRM motor phase drive switches
  digitalWrite(V_SW, HIGH);   // UV on
  delay(PWR_LEN);
  digitalWrite(U_SW, LOW);    // V on
  delay(PWR_LEN);
  digitalWrite(W_SW, HIGH);   // VW on
  delay(PWR_LEN);
  digitalWrite(V_SW, LOW);    // W on
  delay(PWR_LEN);
  digitalWrite(U_SW, HIGH);   // WU on
  delay(PWR_LEN);
  digitalWrite(W_SW, LOW);    // U on
  delay(PWR_LEN);

  tctr --;                    // loop counter
  }
  
  digitalWrite(A_SW, LOW);          // power off
  digitalWrite(LED_BUILTIN, LOW);   // Operate off

  pstatus = P_OFF;                  // lock open
  write_pstatus();                  // update EEPROM

  check_encctr(100);                // check encoder
  buzz(CYC, LEN, 1);                // 1 beeps = open
}

//_________________________________________________

void drive_reverse()                // drive Reverse (P-lock on)
{
 start_position();                  // seek start position
 digitalWrite(LED_BUILTIN, HIGH);   // Operate
 digitalWrite(A_SW, HIGH);          // power on 

  while(tctr != 0)
  {
  digitalWrite(U_SW, HIGH);   // SRM motor phase drive switches
  digitalWrite(V_SW, HIGH);   // UV on
  delay(PWR_LEN);
  digitalWrite(V_SW, LOW);    // U on
  delay(PWR_LEN);
  digitalWrite(W_SW, HIGH);   // UW on
  delay(PWR_LEN);
  digitalWrite(U_SW, LOW);    // W on
  delay(PWR_LEN);
  digitalWrite(V_SW, HIGH);   // WV on
  delay(PWR_LEN);
  digitalWrite(W_SW, LOW);    // V on
  delay(PWR_LEN);

  tctr --;                    // loop counter
  }
  
  digitalWrite(A_SW, LOW);          // power off
  digitalWrite(LED_BUILTIN, LOW);   // Operate off

  pstatus = P_ON;                   // lock open
  write_pstatus();                  // update EEPROM

  check_encctr(-100);               // check encoder
  buzz(CYC, LEN, 2);                // 2 beeps = lock
 }
//__________________________________________________

void loop() 
{
//  tests
 if(digitalRead(TEST_IN) == 0)    // trigger some tests
   {
    pstatus = 0;        // make intentional EEPROM error
    write_pstatus();    
   }

//_______________________
  
 digitalWrite(LED_BUILTIN, LOW);    // idle
 eval_pstatus();                    // check EEPROM status flag & show status
 if(op_err != 0)                    // error state
  {
  blink_code(op_err);               // show error code, over ride status
  }
  
// One button interface
//________________________
if(digitalRead(BRKSW) == 1)        // Brake Switch
  {
   digitalWrite(BOARDLED, HIGH);   // Brake pressed
     
   if((digitalRead(PBUT) == 0))    // P-button
     {
      if(pstatus == P_OFF)         // if lock open
        drive_reverse();           // close P-lock
      else
        drive_forward();           // all other conditions open P-lock
     }
  }
else
 {
  digitalWrite(BOARDLED, LOW);   // Brake released 
 }
}
