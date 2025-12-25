// Version: modified by HE Zhenliang, 20230921
/* Analog and Digital Input and Output Server for MATLAB
   Giampiero Campa, Copyright 2013 The MathWorks, Inc        
                                                             
   This version is modified for timed TTLs (see command 50)  
   Version V1.01                                             


   This file is meant to be used with the MATLAB arduino IO
	 package, however, it can be used from the IDE environment
	 (or any other serial terminal) by typing commands like:

	 0e0 : assigns digital pin #4 (e) as input
	 0f1 : assigns digital pin #5 (f) as output
	 0n1 : assigns digital pin #13 (n) as output

	 1c  : reads digital pin #2 (c)
	 1e  : reads digital pin #4 (e)
	 2n0 : sets digital pin #13 (n) low
	 2n1 : sets digital pin #13 (n) high
	 2f1 : sets digital pin #5 (f) high
	 2f0 : sets digital pin #5 (f) low
	 4j2 : sets digital pin #9 (j) to 50=ascii(2) over 255
	 4jz : sets digital pin #9 (j) to 122=ascii(z) over 255
	 3a  : reads analog pin #0 (a)
	 3f  : reads analog pin #5 (f)

	 5c!! : sends a timed TTL to pin #12 (n) for ('!' = 33 in bytes, [33 33]) = 8481ms
        : Two bytes can encode a 16bit unsigned value 0 to 65536 ms

	 R0    : sets analog reference to DEFAULT
	 R1    : sets analog reference to INTERNAL
	 R2    : sets analog reference to EXTERNAL

	 X3    : roundtrip example case returning the input (ascii(3))
	 99    : returns script type (0 adio.pde ... 3 motor.pde ) */

/* define internal for the MEGA as 1.1V (as as for the 328)  */
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define INTERNAL INTERNAL1V1
#endif

// added by HZL
#define toBinary(arr) (arr[0] | arr[1]<<1 | arr[2]<<2 | arr[3]<<3 | arr[4]<<4 | arr[5]<<5 | arr[6]<<6 | arr[7]<<7)

void setup() {
  Serial.begin(115200);
  while (!Serial) { } // wait for serial to come up
  for (byte i = 2; i < 11; i++) { //safe range of 2-10, uno can't access pins 0 1
    pinMode(i, OUTPUT); // assume we want TTL output by default
    digitalWrite(i, LOW);
  }
  DDRD = DDRD & B00000011;  // PIN 2~7 as strobed word outputs; by HZL
}

void loop() {
  // variables declaration and initialization                
  static int  s   = -1;    // state                         
  static int  pin = 13;    // generic pin number             
  static byte rbyte[2] = {0, 0}; //two bytes used to encode the timedTTL time
  static word pausetime = 0;   // calculated timedTTL time
  static byte durStrbwordByte[4] = {0, 0, 0, 0}; // four bytes used to encode the timed strobed word time; by HZL
  static byte durSettleByte[4] = {0, 0, 0, 0}; // four bytes used to encode the timed strobed word time; by HZL
  static int strbword[8];  // by HZL
  unsigned long strbwordTS = micros(); // by HZL
  unsigned long durStrbwordStatic = 200; // by HZL
  unsigned long durStrbword = 0; // by HZL
  unsigned long settleTS = micros(); // by HZL
  unsigned long durSettleStatic = 200; // by HZL
  unsigned long durSettle = 0; // by HZL
  int  val =  0;           // generic value read from serial 
  int  agv =  0;           // generic analog value           
  int  dgv =  0;           // generic digital value

  /* The following instruction constantly checks if anything
    is available on the serial port. Nothing gets executed in
    the loop if nothing is available to be read, but as soon
    as anything becomes available, then the part coded after
    the if statement (that is the real stuff) gets executed */
  if (Serial.available() > 0) {

    /* whatever is available from the serial is read here    */
    val = Serial.read();

    /* This part basically implements a state machine that
    	 reads the serial port and makes just one transition
    	 to a new state, depending on both the previous state
    	 and the command that is read from the serial port.
    	 Some commands need additional inputs from the serial
    	 port, so they need 2 or 3 state transitions (each one
    	 happening as soon as anything new is available from
    	 the serial port) to be fully executed. After a command
    	 is fully executed the state returns to its initial
    	 value s=-1                                            */

    switch (s) {

      /*=====================================================*/
      /* s=-1 means NOTHING RECEIVED YET ******************* */
      case -1:
        /* calculate next state                                */
        if (val > 47 && val < 90) {
          /* the first received value indicates the mode
            48 is ascii for 0, ... 90 is ascii for Z
            s=0 is change-pin mode;
            s=10 is DI;  s=20 is DO;  s=30 is AI;  s=40 is AO;
            s=50 sends a timed TTL
            s=90 is query script type (1 basic, 2 motor);
            s=340 is change analog reference;
            s=400 example echo returning the input argument;
          */
          s = 10 * (val - 48);
        }
        /* the following statements are needed to handle
          unexpected first values coming from the serial (if
          the value is unrecognized then it defaults to s=-1) */
        if ((s > 80 && s < 90) || (s > 90 && s != 340 && s != 400)) {
          s = -1;
        }
        /* the break statements gets out of the switch-case, so
          /* we go back and wait for new serial data             */
        break; /* s=-1 (initial state) taken care of           */

      /*=====================================================*/
      /* s = 0 to 2 means CHANGE PIN MODE                      */
      case 0:
        /* the second received value indicates the pin
          from abs('a')=97, pin 0, to abs('¦')=166, pin 69    */
        if (val > 96 && val < 167) {
          pin = val - 97;            /* calculate pin          */
          s = 1; /* next we will need to get 0, 1, or 2 from serial  */
        }
        else {
          s = -1; /* if value is not a pin then return to -1     */
        }
        break; /* s=0 taken care of                            */
      case 1:
        /* the third received value indicates the value 0 or 1 */
        if (val > 47 && val < 51) {
          /* set pin mode */
          if (val == 48) {
            pinMode(pin, INPUT);
          }          
          else if (val == 49) {
            pinMode(pin, OUTPUT);
          }
          else if (val == 50) {
            pinMode(pin, INPUT_PULLUP);
//            Serial.print("PIN mode INPUT_PULLUP set according to value got: ");
//            Serial.println(val);
          }
        }
        s = -1; /* we are done with CHANGE PIN so go to -1      */
        break; /* s=1 taken care of                            */
      /*=====================================================*/


      /*=====================================================*/
      /* s=10 means DIGITAL INPUT ************************** */
      case 10:
        /* the second received value indicates the pin
          from abs('a')=97, pin 0, to abs('¦')=166, pin 69    */
        if (val > 96 && val < 167) {
          pin = val - 97;            /* calculate pin          */
          dgv = digitalRead(pin);    /* perform Digital Input  */
          Serial.write(dgv);       /* send value via serial  */
        }
        s = -1; /* we are done with DI so next state is -1      */
        break; /* s=10 taken care of                           */
      /*=====================================================*/


      /*=====================================================*/
      /* s=20 or 21 means DIGITAL OUTPUT ******************* */
      case 20:
        /* the second received value indicates the pin
          from abs('a')=97, pin 0, to abs('¦')=166, pin 69    */
        if (val > 96 && val < 167) {
          pin = val - 97;            /* calculate pin          */
          s = 21; /* next we will need to get 0 or 1 from serial */
        }
        else {
          s = -1; /* if value is not a pin then return to -1     */
        }
        break; /* s=20 taken care of                           */
      case 21:
        /* the third received value indicates the value 0 or 1 */
        if (val > 47 && val < 50) {
          dgv = val - 48;            /* calculate value        */
          digitalWrite(pin, dgv);    /* perform Digital Output */
        }
        s = -1; /* we are done with DO so next state is -1      */
        break; /* s=21 taken care of                           */
      /*=====================================================*/


      /*=====================================================*/
      /* s=30 means ANALOG INPUT *************************** */
      case 30:
        /* the second received value indicates the pin
          from abs('a')=97, pin 0, to abs('p')=112, pin 15    */
        if (val > 96 && val < 113) {
          pin = val - 97;            /* calculate pin          */
          agv = analogRead(pin);     /* perform Analog Input   */
          Serial.write(agv);       /* send value via serial  */
        }
        s = -1; /* we are done with AI so next state is -1      */
        break; /* s=30 taken care of                           */
      /*=====================================================*/


      /*=====================================================*/
      /* s=40 or 41 means ANALOG OUTPUT ******************** */
      case 40:
        /* the second received value indicates the pin
          from abs('a')=97, pin 0, to abs('¦')=166, pin 69    */
        if (val > 96 && val < 167) {
          pin = val - 97;            /* calculate pin          */
          s = 41; /* next we will need to get value from serial  */
        }
        else {
          s = -1; /* if value is not a pin then return to -1     */
        }
        break; /* s=40 taken care of                           */
      case 41:
        /* the third received value indicates the analog value */
        analogWrite(pin, val);       /* perform Analog Output  */
        s = -1; /* we are done with AO so next state is -1      */
        break; /* s=41 taken care of                           */


      /*=====================================================*/
      /* s=50 to 52 means TIMED TTL  *********************** */
      case 50:
        /* the second received value indicates the pin
          from abs('a')=97, pin 0, to abs('¦')=166, pin 69    */
        if (val > 96 && val < 167) {
          pin = val - 97;            /* calculate pin          */
          val = 0;
          rbyte[0] = 0; rbyte[1] = 0; pausetime = 0;
          s = 51; /* next we will need to get value from serial  */
        }
        else {
          s = -1; /* if value is not a pin then return to -1     */
        }
        break; 
      case 51:
        rbyte[0] = val;
        val = 0;
        s = 52;
        break; /* s=51 taken care of                        */
      case 52:
        /* combine two bytes into a 16bit (unsigned) word   */
        rbyte[1] = val;
        pausetime = word(rbyte[1], rbyte[0]);  //(rbyte[1]<<8) | rbyte[0]
        if (pausetime > 0) {
          digitalWrite(pin, HIGH);/* perform Digital Output */
          delay(pausetime);
          digitalWrite(pin, LOW); /* perform Digital Output */
        }
        rbyte[0] = 0; rbyte[1] = 0; pausetime = 0;
        s = -1; /* we are done with AO so next state is -1   */
        break; /* s=52 taken care of                         */
      /*=====================================================*/

      // /*=====================================================*/
      // /* s=60 means 2-9 pin strobe word 0-255  ************* */
      // case 60:
      //   for (int i = 0; i <= 7; i++) {
      //     int test = bitRead(val, i);
      //     if (test == 1) {
      //       digitalWrite(i+2, HIGH);
      //     }
      //     else {
      //       digitalWrite(i+2, LOW);
      //     }
      //   }
      //   delay(1);
      //   for (int i = 0; i <= 7; i++) {
      //       digitalWrite(i + 2, LOW);
      //   }
      //   s = -1; /* we are done with AO so next state is -1      */
      //   break; /* s=60 taken care of                           */
      // /*=====================================================*/

      /*=====================================================*/
      /* s=60 means 2-7 pin strobed word 0-63  ************** */
      /* added by HZL  ***************************************/
      case 60:
//        Serial.print("Called strobedWord. Got value: ");
//        Serial.println(val);
        strbword[0] = 0;
        strbword[1] = 0;
        for (int i = 0; i <= 5; i++) {
          strbword[i+2] = bitRead(val, i);          
        }
        PORTD |= toBinary(strbword);        
        // delayMicroseconds(durStrbwordStatic);
        strbwordTS = micros();
        while (micros() < strbwordTS + durStrbwordStatic) {
        }
        strbword[0] = 1;
        strbword[1] = 1;
        for (int i = 0; i <= 5; i++) {
          strbword[i+2] = 0;          
        }
        PORTD &= toBinary(strbword);
        // delayMicroseconds(durSettleStatic);
        settleTS = micros();
        while (micros() < settleTS + durSettleStatic) {
        }
        s = -1; /* we are done with AO so next state is -1      */
        break; /* s=60 taken care of                           */
      /*=====================================================*/

      /*=====================================================*/
      /* s=70 means 2-7 pin timed strobed word 0-63  ************** */
      /* added by HZL  ***************************************/
      case 70:
//        Serial.print("Called timedStrobedWord. Got value: ");
//        Serial.println(val);
        strbword[0] = 0;
        strbword[1] = 0;
        for (int i = 0; i <= 5; i++) {
          strbword[i+2] = bitRead(val, i);          
        }
        val = 0;
        durStrbwordByte[0] = 0; durStrbwordByte[1] = 0; durStrbwordByte[2] = 0; durStrbwordByte[3] = 0;
        durSettleByte[0] = 0; durSettleByte[1] = 0; durSettleByte[2] = 0; durSettleByte[3] = 0;
        durStrbword = 0;
        durSettle = 0;
        s = 71; /* next we will need to get value from serial  */
        break; /* s=70 taken care of                           */
      case 71:
        durStrbwordByte[0] = val;
        val = 0;
        s = 72;
        break; /* s=71 taken care of                        */
      case 72:
        durStrbwordByte[1] = val;
        val = 0;
        s = 73;
        break; /* s=72 taken care of                        */
      case 73:
        durStrbwordByte[2] = val;
        val = 0;
        s = 74;
        break; /* s=73 taken care of                        */
      case 74:
        durStrbwordByte[3] = val;
        val = 0;
        s = 75;
        break; /* s=74 taken care of                        */
      case 75:
        durSettleByte[0] = val;
        val = 0;
        s = 76;
        break; /* s=75 taken care of                        */
      case 76:
        durSettleByte[1] = val;
        val = 0;
        s = 77;
        break; /* s=76 taken care of                        */
      case 77:
        durSettleByte[2] = val;
        val = 0;
        s = 78;
        break; /* s=77 taken care of                        */
      case 78:
        durSettleByte[3] = val;
        /* combine four bytes of durStrbwordByte and durSettleByte into a 32-bit unsigned long   */
        durStrbword = ((unsigned long)durStrbwordByte[3]<<24) | ((unsigned long)durStrbwordByte[2]<<16) | 
                      ((unsigned long)durStrbwordByte[1]<<8) | (unsigned long)durStrbwordByte[0];
        durSettle = ((unsigned long)durSettleByte[3]<<24) | ((unsigned long)durSettleByte[2]<<16) | 
                    ((unsigned long)durSettleByte[1]<<8) | (unsigned long)durSettleByte[0];
        PORTD |= toBinary(strbword);
        // delayMicroseconds(durStrbword);
        strbwordTS = micros();
        while (micros() < strbwordTS + durStrbword) {
        }
        strbword[0] = 1;
        strbword[1] = 1;
        for (int i = 0; i <= 5; i++) {
          strbword[i+2] = 0;
        }
        PORTD &= toBinary(strbword);
        // delayMicroseconds(durSettle);
        settleTS = micros();
        while (micros() < settleTS + durSettle) {
        }
        durStrbwordByte[0] = 0; durStrbwordByte[1] = 0; durStrbwordByte[2] = 0; durStrbwordByte[3] = 0;
        durSettleByte[0] = 0; durSettleByte[1] = 0; durSettleByte[2] = 0; durSettleByte[3] = 0;
        durStrbword = 0;
        durSettle = 0;
        s = -1; /* we are done with AO so next state is -1      */
        break; /* s=60 taken care of                           */
      /*=====================================================*/


      /*=====================================================*/
      /* s=90 means Query Script Type:
        (0 adio, 1 adioenc, 2 adiosrv, 3 motor)             */

      case 90:
        if (val == 57) {
          /* if string sent is 99  send script type via serial */
          Serial.println(0);
        }
        s = -1; /* we are done with this so next state is -1   */
        break; /* s=90 taken care of                           */


      /*=====================================================*/
      /* s=340 or 341 means ANALOG REFERENCE *************** */
      case 340:
        /* the second received value indicates the reference,
          which is encoded as is 0,1,2 for DEFAULT, INTERNAL
          and EXTERNAL, respectively. Note that this function
          is ignored for boards not featuring AVR or PIC32    */
#if defined(__AVR__) || defined(__PIC32MX__)

        switch (val) {

          case 48:
            analogReference(DEFAULT);
            break;

          case 49:
            analogReference(INTERNAL);
            break;

          case 50:
            analogReference(EXTERNAL);
            break;

          default:                 /* unrecognized, no action  */
            break;
        }

#endif
        s = -1; /* we are done with this so next state is -1    */
        break; /* s=341 taken care of                          */
      /*=====================================================*/


      /*=====================================================*/
      /* s=400 roundtrip example function (returns the input)*/
      case 400:
        /* The second value (val) can really be anything here. 
           This is an auxiliary function that returns the BYTE
           value of its first argument. It is provided as an
           example for people that want to add their own code  
           your own code goes here instead of the serial print */
          Serial.write(val);
          s = -1; /* we are done with the aux function so -1   */
          break; /* s=400 taken care of                        */
      /*=====================================================*/


      /*=====================================================*/
      /* ******* UNRECOGNIZED STATE, go back to s=-1 ******* */
      default:
        /* we should never get here but if we do it means we
          are in an unexpected state so whatever is the second
          received value we get out of here and back to s=-1  */
        s = -1; /* go back to the initial state, break unneeded */

    } /* end switch on state s                               */

  } /* end if serial available                               */

} /* end loop statement                                      */
