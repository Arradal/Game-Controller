//Libraries used for this project:
#include "timer.h"
#include "PinChangeInterrupt.h"
#include <LiquidCrystal_I2C.h>
#include <XInput.h>
#include <FastLED.h>

// Define
#define NUM_AROUND_DIAl_LEDS 10 
#define NUM_SHOOT_LEDS 5 
#define NUM_THREE_STRIP_LEDS 12 
#define DATA_PIN_DIAL 6
#define DATA_PIN_SHOOT 5
#define DATA_PIN_THREE_STRIP 13 
#define COLOR_ORDER GRB
#define CHIPSET WS2812B
#define BRIGHTNESS 200
#define VOLTS 5
#define MAX_AMPS 500 // alt efter usb type (2.0 = 500 mA, 3.0 = 900mA, 3.0+ = 1500mA)
#define LUTSIZE 40
#define STEPSIZE 360/LUTSIZE

// Button pins
const uint8_t Pin_ButtonJump = 2;   // Jump
const uint8_t Pin_ButtonShoot = 7; // Shoot
// D-pad pins 
const int Pin_DpadUp    = 9;    // Up 
const int Pin_DpadDown  = 10;  //Down
const int Pin_DpadLeft  = 1;  //Left
const int Pin_DpadRight = 0; //Right
// Dial
const int CLK  = 11;
const int DT  = 12;

//Variabler til at styre led
CRGB ledDial[NUM_AROUND_DIAl_LEDS];
CRGB ledShoot[NUM_SHOOT_LEDS];
CRGB ledThreeStrips[NUM_THREE_STRIP_LEDS];

CRGB playerColorNotAssigned =  CRGB(220,220,220);
CRGB colorPlayer1 = CRGB(0,255,0); // grøn
CRGB colorPlayer2 = CRGB(246, 255, 0); // gul
CRGB colorPlayer3 = CRGB(0, 0, 255); // blå
CRGB colorPlayer4 = CRGB(251, 80, 9); // orange
CRGB playerColor = CRGB(0,255,0);
CRGB shootingColor = CRGB(200,200,200);
uint8_t BeatToShoot; 
uint8_t BeatToJump;
uint8_t BeatToJoy; 
uint8_t BeatToDial; 
boolean shooting = false;
int shootValue = 20;
int ledSpeed = 20;
Timer timer;

//This is to define the size of the LCD Screen. It is 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variabler til Dial
int counter = 0;
int aState;
int aLastState = 0;
int angle = 0;
int px = 511; 
int py = 511;
int16_t xLUT[LUTSIZE];
int16_t yLUT[LUTSIZE];
const int AnalogRead_Max = 1023;  // Analog Input Range, 10-bit ADC
unsigned long x = 0;
int ledCounter;

// funktioner til interrupts
void jump(){ 
    boolean pressJump = !digitalRead(Pin_ButtonJump);
    XInput.setButton(BUTTON_A, pressJump);
    px = 511, py = 511;
}

void walkLeft(){
    boolean dpadLeft  = !digitalRead(Pin_DpadLeft);
    XInput.setDpad(DPAD_LEFT, dpadLeft);
    px = 511, py = 511;
}

void walkRight(){   
    boolean dpadRight = !digitalRead(Pin_DpadRight);
    XInput.setDpad(DPAD_RIGHT, dpadRight);
    px = 511, py = 511;
}

void walkUp(){ 
    boolean dpadUp = !digitalRead(Pin_DpadUp);
    XInput.setDpad(DPAD_UP, dpadUp);
    px = 511, py = 511;
}

void walkDown(){ 
    boolean dpadDown = !digitalRead(Pin_DpadDown);
    XInput.setDpad(DPAD_DOWN, dpadDown);
    px = 511, py = 511;
}

void shoot(){ 
    boolean pressShoot = !digitalRead(Pin_ButtonShoot);
    XInput.setButton(BUTTON_X,  pressShoot);
    timer.reset();
    timer.update();
    shooting = true;
}
    
void dial(){
    aState = digitalRead(CLK); // Reads the "current" state of the outputA
}
  
void setShootValue(){
  shooting = false;
}

void setup(){
  // set timer
  timer.setInterval(600); // sætter antal millisekunder
  timer.setCallback(setShootValue); // kalder funktion der sætter farve til player color
  timer.start();
  
  // Set led
  FastLED.addLeds<CHIPSET,DATA_PIN_DIAL,COLOR_ORDER>(ledDial,NUM_AROUND_DIAl_LEDS); // find ud af de andre data pins
  FastLED.addLeds<CHIPSET,DATA_PIN_SHOOT,COLOR_ORDER>(ledShoot,NUM_SHOOT_LEDS);
  FastLED.addLeds<CHIPSET,DATA_PIN_THREE_STRIP,COLOR_ORDER>(ledThreeStrips,NUM_THREE_STRIP_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS,MAX_AMPS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  
  //set buttons
  pinMode(Pin_ButtonJump, INPUT_PULLUP);
  pinMode(Pin_ButtonShoot, INPUT_PULLUP);
  
  //set D-pad
  pinMode(Pin_DpadUp, INPUT_PULLUP);
  pinMode(Pin_DpadDown, INPUT_PULLUP);
  pinMode(Pin_DpadLeft, INPUT_PULLUP);
  pinMode(Pin_DpadRight, INPUT_PULLUP);
  
  //set dial 
  pinMode (CLK, INPUT);
  pinMode (DT, INPUT);
  XInput.setRange(JOY_LEFT, 0, AnalogRead_Max);
  
  // start lcd og XInput
  XInput.begin();
  lcd.begin();

  int player = XInput.getPlayer();
  // controller bliver tillagt farve ud fra spiller nummer 
  if (player == 0){ playerColor = playerColorNotAssigned;} //0 = player not connected
  else if (player == 1){ playerColor = colorPlayer1;}
  else if (player == 2){ playerColor = colorPlayer2;}
  else if (player == 3){ playerColor = colorPlayer3;}
  else if (player == 4){ playerColor = colorPlayer4;}

  //initualisation of lookup tables:
  for(int i = 0 ; i < LUTSIZE ; i++){
    
    float rad = (i * STEPSIZE * 3.14) / 180; //fra vinkel til radianer. 3.14 er pi...
    xLUT[i] = 511 + int(cos(rad)*511.0);
    yLUT[i] = 511 - int(sin(rad)*511.0);
  }

  // Interrupts
  attachInterrupt(digitalPinToInterrupt(Pin_ButtonJump),jump, CHANGE);  //interrupt
  attachInterrupt(digitalPinToInterrupt(Pin_DpadLeft),walkLeft, CHANGE);  //interrupt
  attachInterrupt(digitalPinToInterrupt(Pin_DpadRight),walkRight, CHANGE);  //interrupt
  attachInterrupt(digitalPinToInterrupt(Pin_ButtonShoot),shoot, CHANGE);  //interrupt
  attachPCINT(digitalPinToPCINT(Pin_DpadUp),walkUp, CHANGE); //interrupt with lib
  attachPCINT(digitalPinToPCINT(Pin_DpadDown),walkDown, CHANGE); //interrupt with lib
  attachPCINT(digitalPinToPCINT(CLK),dial, CHANGE); //interrupt with lib
}

unsigned long rotorTimer = 0;
unsigned int offset = 180;

void loop() {
    timer.update();
    if (py == 511 && px == 511){
      ledDial[ledCounter] =  CRGB::Black;
    }
    
    if (aState != aLastState) {
      if (digitalRead(DT) != aState) {
        counter ++;
      } 
      else {
        counter --;
      }
    
      angle = (counter * STEPSIZE);
    
      if (angle < 0) {
        angle = 360 + (angle % 360);
        counter += 40;
      } 
      else if (angle >= 360) {
        angle = angle % 360;
        counter -= 40;
      }
  
      px = xLUT[counter];
      py = yLUT[counter];
      ledCounter = (counter/4)+4;
      if (ledCounter > 9){
            ledCounter =(counter/4)-6;
      }
      
      FastLED.clear();
      ledDial[ledCounter] = playerColor;
      FastLED.show(); 
      rotorTimer = millis();
  }
    else if (rotorTimer + offset < millis()) {
    px = 511;
    py = 511;
  }  
  
  aLastState = aState; // Updates the previous state of the outputA with the current state
  int leftJoyX = px; // analogRead(Pin_LeftJoyX) - 511 home position on joystick 0-1023
  int leftJoyY = py; //analogRead(Pin_LeftJoyY)
  
  XInput.setJoystickX(JOY_LEFT, leftJoyX);
  XInput.setJoystickY(JOY_LEFT, leftJoyY);
  
  // Get rumble value
  uint16_t rumble = XInput.getRumble();
  
  if (rumble > 0) {
    shootValue = 80;
    ledSpeed = 80;
    
    BeatToShoot = map(beat8(shootValue,0),0,255, 5, 0);
    BeatToJump = map(beat8(ledSpeed,0),0,255, 0, 3);
    BeatToJoy = map(beat8(ledSpeed,0),0,255, 4, 8);
    BeatToDial = map(beat8(ledSpeed,0),0,255, 9, 13);
    
    ledShoot[BeatToShoot] = CRGB::Red;
    ledThreeStrips[BeatToJump] = CRGB::Red;
    ledThreeStrips[BeatToDial] = CRGB::Red;
    ledThreeStrips[BeatToJoy] = CRGB::Red;
    
    FastLED.setBrightness(BRIGHTNESS);
    //fadeToBlackBy(leds,NUM_LEDS,10);
    fadeToBlackBy(ledShoot,NUM_SHOOT_LEDS,10);
    fadeToBlackBy(ledThreeStrips,NUM_THREE_STRIP_LEDS,10);
    FastLED.show();
  }

  else { 
    shootValue = 20;
    ledSpeed = 20;
    
    BeatToShoot = map(beat8(shootValue,0),0,255, 4, -1);
    BeatToJump = map(beat8(ledSpeed,0),0,255, 0, 4);
    BeatToJoy = map(beat8(ledSpeed,0),0,255, 4, 8);
    BeatToDial = map(beat8(ledSpeed,0),0,255, 8, 12);
 
    ledShoot[BeatToShoot] = playerColor;
    ledThreeStrips[BeatToJump] = playerColor;
    ledThreeStrips[BeatToJoy]  = playerColor;
    ledThreeStrips[BeatToDial] = playerColor;

    ledThreeStrips[8] = playerColor;
    ledShoot[4] = playerColor;
    
    if (shooting == true){
      ledShoot[0]  = CRGB ::Black;  
      ledShoot[1]  = CRGB ::Black;
      ledShoot[2]  = CRGB ::Black;
      ledShoot[3]  = CRGB ::Black;
      
      BeatToShoot = map(beat8(90,0),0,255, 0, 4);
      ledShoot[BeatToShoot] = CRGB ::Red;
      }

    fadeToBlackBy(ledShoot,NUM_SHOOT_LEDS,8);
    fadeToBlackBy(ledThreeStrips,NUM_THREE_STRIP_LEDS,1);
    
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.show();
  
  }

}
