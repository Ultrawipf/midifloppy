#include <DueTimer.h>
#include <LiquidCrystal.h>
#include <MIDIUSB.h>

#define RESOLUTION 30 //Microsecond resolution for notes

//lcd settings. If you have no lcd remove all lcd functions
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
byte lcdNumCols = 16;

//First pin being used for floppies, and the last pin.  Used for looping over all pins.
const byte PIN_MIN = 22;
const byte PIN_MAX = 53;

//Precalculated note times in microseconds.
int noteToPeriod[127]=
{0 ,115447 ,108967 ,102851 ,97079 ,91630 ,86487 ,81633 ,77051 ,72727 ,68645 ,64792 ,
61156 ,57723 ,54483 ,51425 ,48539 ,45815 ,43243 ,40816 ,38525 ,36363 ,34322 ,32396 ,
30578 ,28861 ,27241 ,25712 ,24269 ,22907 ,21621 ,20408 ,19262 ,18181 ,17161 ,16198 ,
15289 ,14430 ,13620 ,12856 ,12134 ,11453 ,10810 ,10204 ,9631 ,9090 ,8580 ,8099 ,
7644 ,7215 ,6810 ,6428 ,6067 ,5726 ,5405 ,5102 ,4815 ,4545 ,4290 ,4049 ,
3822 ,3607 ,3405 ,3214 ,3033 ,2863 ,2702 ,2551 ,2407 ,2272 ,2145 ,2024 ,
1911 ,1803 ,1702 ,1607 ,1516 ,1431 ,1351 ,1275 ,1203 ,1136 ,1072 ,1012 ,
955 ,901 ,851 ,803 ,758 ,715 ,675 ,637 ,601 ,568 ,536 ,506 ,
477 ,450 ,425 ,401 ,379 ,357 ,337 ,318 ,300 ,284 ,268 ,253 ,
238 ,225 ,212 ,200 ,189 ,178 ,168 ,159 ,150 ,142 ,134 ,126 ,
0 ,0 ,0 ,0 ,0 ,0 ,0};


/*An array of maximum track positions for each step-control pin.  Even pins
are used for control, so only even numbers need a value here.  3.5" Floppies have
80 tracks, 5.25" have 50.  These should be doubled, because each tick is now
half a position (use 158 and 98).
*/
const byte MAX_POSITION[] = {
  158,0,158,0,158,0,158,0,158,0,158,0,158,0,158,0,158,0,158,0,158,0,158,0,158,0,158,0,0,0,0,0};
  
/*Array to track the current position of each floppy head. 
The odd values between step pin are the current note number used for effects.*/
byte currentState[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/*Array to keep track of state of each pin.  Even indexes track the control-pins for toggle purposes.  Odd indexes
track direction-pins.  LOW = forward, HIGH=reverse
*/
int pinState[] = {
  LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW};
  
/*Current period assigned to each pin.  0 = off.  Each period is of the length specified by the RESOLUTION
variable above.  i.e. A period of 10 is (RESOLUTION x 10) microseconds long.
Odd value on pin+1 is the current tick.*/
unsigned int currentPeriod[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


//Setup pins (Even-odd pairs for step control and direction)
void setup() {
  //for performance precalculate the periods. could be done by preprocessor...
  for(int i=0;i<127;i++)
    noteToPeriod[i]=noteToPeriod[i] / (2*RESOLUTION); 

  //lcd init
  lcd.begin(2, lcdNumCols);
  lcd.clear();

  //set all used pins as output
  for(int i=PIN_MIN;i<=PIN_MAX;i++)
    pinMode(i, OUTPUT);

  //Serial.begin(19200);
  
  Timer1.attachInterrupt(tick); // Attach the tick function
  Timer1.start(RESOLUTION); // Set up a timer at the defined resolution

  //Display welcome Message
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Starting up");
  lcd.setCursor(4, 0);
  lcd.print("Gigawipf");

  //Run all heads forward once.
  for (byte s=0;s<80;s++){ //Assuming we have 80 tracks on every drive.
    for (byte p=PIN_MIN;p<=PIN_MAX;p+=2){
      digitalWrite(p+1,LOW); // Go
      digitalWrite(p,HIGH);
      digitalWrite(p,LOW);
    }
    delay(5);
  }
  //Prepare for reset test.
  for (byte p=PIN_MIN;p<=PIN_MAX;p+=2){
    currentState[p-PIN_MIN] = 158;
    digitalWrite(p+1,HIGH);
    pinState[p+1-PIN_MIN] = HIGH;
  }
  delay(500);
  resetAll(); 
}

//Working loop for midi messages
void loop() {
  midiEventPacket_t rx = MidiUSB.read(); //read midi packet
  
  if (rx.header == 0) return; // if no midi packet do nothing

  byte chan = (rx.byte1 & 0x0f)*2;
  
  //Note off if event matches our current note
  if((rx.header == 0x08 || (rx.header == 0x09 && rx.byte3 == 0x00) || (rx.byte2 == 0xB0 && rx.byte3 == 0x00)) && currentState[chan+1] == rx.byte2) {
    currentPeriod[chan] = 0;
    currentState[chan+1] = 0;
    digitalWrite(chan+PIN_MIN,LOW);
    pinState[chan]=LOW;
      
  } else if(rx.header == 0x09) { //note on
  currentPeriod[chan] = noteToPeriod[rx.byte2]; //convert note number to period
  currentState[chan+1] = rx.byte2; //save note
  
  } else if(rx.header==0x0B) { //special
    if(rx.byte1 == 0xB0 && (rx.byte2==0x78 || rx.byte2==0x7B)){
      resetAll();
    }      
  } else if(rx.header==0x0E) { //pitch Bend
    int pb = ((rx.byte3 & 0x7f) << 7) + (rx.byte2 & 0x7f) - 8192;
    if(pb!=0){
      float pbMult = pow(2.0, pb / 8192.0);
      currentPeriod[chan] = noteToPeriod[currentState[chan+1]] / pbMult;
    }
  }
}


/*
Called by the timer inturrupt at the specified resolution.
*/
void tick()
{
  /* 
  If there is a period set, count the number of
  ticks that pass, and toggle the pin if the current period is reached.
  */
  for(int i=0;i<=30;i+=2) {
    if (currentPeriod[i]) {
      currentPeriod[i+1]++;
      if (currentPeriod[i+1] >= currentPeriod[i]){
        togglePin(i,i+1);
        currentPeriod[i+1]=0;
      }
    }
  }
}

void togglePin(byte pin, byte direction_pin) {
 
  //Switch directions if end has been reached
  if(MAX_POSITION[pin]){
    if (currentState[pin] >= MAX_POSITION[pin]) {
      pinState[direction_pin] = HIGH;
      digitalWrite(direction_pin+PIN_MIN,HIGH);
    } 
    else if (currentState[pin] == 0) {
      pinState[direction_pin] = LOW;
      digitalWrite(direction_pin+PIN_MIN,LOW);
    }
    
    //Update currentState
    if (pinState[direction_pin] == HIGH) {
      currentState[pin]--;
    } 
    else {
      currentState[pin]++;
    }
  }
  
  //Pulse the control pin
  digitalWrite(pin+PIN_MIN,pinState[pin]);
  pinState[pin] = ~pinState[pin];
}


//Resets all the pins
void resetAll(){
  bool res=true;
  for (byte p=PIN_MIN;p<=PIN_MAX;p+=2){
      currentPeriod[p-PIN_MIN] = 0;
      if(currentState[p-PIN_MIN]) //if any drive is not at zero to continue resetting
        res=false;
  }
  if(res==true) //all drives are already reset
    return;
  
  //Display reset message
  lcd.clear();
  lcd.setCursor(2, 1);
  lcd.print("Resetting...");
  lcd.setCursor(4, 0);
  lcd.print("Gigawipf");
  
  for (byte s=0;s<80;s++){
    for (byte p=PIN_MIN;p<=PIN_MAX;p+=2){
      if(currentState[p-PIN_MIN] == 0) //Don't run the head into end stops
	      continue;
      digitalWrite(p+1,HIGH); // Go in reverse
      digitalWrite(p,HIGH);
      digitalWrite(p,LOW);
    }
    delay(4);
  }

  //Prepare to go forward.
  for (byte p=PIN_MIN;p<=PIN_MAX;p+=2){
    currentState[p-PIN_MIN] = 0;
    digitalWrite(p+1,LOW);
    digitalWrite(p,LOW);
    pinState[p+1 - PIN_MIN] = LOW;
  }

  //Display idle message
  lcd.clear();
  lcd.setCursor(2, 1);
  lcd.print("Floppy Music");
  lcd.setCursor(4, 0);
  lcd.print("Gigawipf");
}



