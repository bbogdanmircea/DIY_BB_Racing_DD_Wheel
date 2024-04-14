#include <ESP32Encoder.h>     // https://github.com/madhephaestus/ESP32Encoder/
#include <Keypad.h>           // https://github.com/Chris--A/Keypad
#include <BleGamepad.h>       // https://github.com/MagnusThome/ESP32-BLE-Gamepad


BleGamepad bleGamepad("RaceKeys", "Arduino", 100);


////////////////////// BUTTON MATRIX //////////////////////
#define ROWS 4
#define COLS 4
uint8_t rowPins[ROWS] = {25, 26, 32, 33};
uint8_t colPins[COLS] = {18, 19, 23, 22};
uint8_t keymap[ROWS][COLS] = {
  {0,1,2,3},
  {4,5,6,7},
  {8,9,10,11},
  {12,13,14,15}
};
Keypad customKeypad = Keypad( makeKeymap(keymap), rowPins, colPins, ROWS, COLS); 


//////////// ROTARY ENCODERS (WITH PUSH SWITCHES) ////////////
#define MAXENC 2
uint8_t uppPin[MAXENC] = {13, 5};
uint8_t dwnPin[MAXENC] = {12, 4};
uint8_t prsPin[MAXENC] = {17, 16};
uint8_t encoderUpp[MAXENC] = {16,19};
uint8_t encoderDwn[MAXENC] = {17,20};
uint8_t encoderPrs[MAXENC] = {18,21};
ESP32Encoder encoder[MAXENC];
unsigned long holdoff[MAXENC] = {0,0};
int32_t prevenccntr[MAXENC] = {0,0};
bool prevprs[MAXENC] = {0,0};
#define HOLDOFFTIME 150   // TO PREVENT MULTIPLE ROTATE "CLICKS" WITH CHEAP ENCODERS WHEN ONLY ONE CLICK IS INTENDED

//////////// BT STATUS LED ////////////
uint8_t BT_LED_Pin = 21;
boolean BT_LED_State = LOW;
unsigned long last_LED_update = 0;
#define LED_UPDATE_MS 1000



////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);

  for (uint8_t i=0; i<MAXENC; i++) {
    encoder[i].clearCount();
    encoder[i].attachSingleEdge(dwnPin[i], uppPin[i]);
    pinMode(prsPin[i], INPUT_PULLUP);
  }
  customKeypad.addEventListener(keypadEvent);
  //customKeypad.setHoldTime(1);
  pinMode(BT_LED_Pin, OUTPUT);
  bleGamepad.begin();
  Serial.println("Booted!");
}



////////////////////////////////////////////////////////////////////////////////////////

void loop() {

  unsigned long now = millis();


  // -- ROTARY ENCODERS : ROTATION -- //

  for (uint8_t i=0; i<MAXENC; i++) {
    int32_t cntr = encoder[i].getCount();
    if (cntr!=prevenccntr[i]) {
      if (!holdoff[i]) {
        if (cntr>prevenccntr[i]) { sendKey(encoderUpp[i]); }
        if (cntr<prevenccntr[i]) { sendKey(encoderDwn[i]); }
        holdoff[i] = now;
        if (holdoff[i]==0) holdoff[i] = 1;  // SAFEGUARD WRAP AROUND OF millis() (WHICH IS TO 0) SINCE holdoff[i]==0 HAS A SPECIAL MEANING ABOVE
      }
      else if (now-holdoff[i] > HOLDOFFTIME) {
        prevenccntr[i] = encoder[i].getCount();
        holdoff[i] = 0;
      }
    }
    
  // -- ROTARY ENCODERS : PUSH SWITCH -- //

    bool pressed = !digitalRead(prsPin[i]);
    if (pressed != prevprs[i]) {
      if (pressed) {  // PRESSED
        pressKey(encoderPrs[i]);
      }
      else {          // RELEASED
        releaseKey(encoderPrs[i]);
      }
      prevprs[i] = !prevprs[i];
    }
  }

  customKeypad.getKey();    // READ BUTTON MATRIX (EVENT CALLBACK SETUP)

  if (now-last_LED_update > LED_UPDATE_MS)
  {
    if(bleGamepad.isConnected())
    {
      BT_LED_State = HIGH;
    } else {
      BT_LED_State = !BT_LED_State;
    }
    digitalWrite(BT_LED_Pin, BT_LED_State);
    Serial.print("BT_LED_State \t");
    Serial.println(BT_LED_State);
    Serial.print("bleGamepad.isConnected() \t");
    Serial.println(bleGamepad.isConnected());
    last_LED_update = now;
  }

  delay(10);
 
}




////////////////////////////////////////////////////////////////////////////////////////

void keypadEvent(KeypadEvent key){
  uint8_t keystate = customKeypad.getState();
  if (keystate==PRESSED)  { pressKey(key); }
  if (keystate==RELEASED) { releaseKey(key); }
}


////////////////////////////////////////////////////////////////////////////////////////

void sendKey(uint8_t key) {
    uint32_t gamepadbutton = pow(2,key);      // CONVERT TO THE BINARY MAPPING GAMEPAD KEYS USE
    Serial.print("pulse\t");
    Serial.println(key);
    if(bleGamepad.isConnected()) {
      bleGamepad.press(gamepadbutton);
      delay(150);
      bleGamepad.release(gamepadbutton);
    }
}

void pressKey(uint8_t key) {
    uint32_t gamepadbutton = pow(2,key);      // CONVERT TO THE BINARY MAPPING GAMEPAD KEYS USE
    Serial.print("hold\t");
    Serial.println(key);
    if(bleGamepad.isConnected()) {
      bleGamepad.press(gamepadbutton);
    }
}

void releaseKey(uint8_t key) {
    uint32_t gamepadbutton = pow(2,key);      // CONVERT TO THE BINARY MAPPING GAMEPAD KEYS USE
    Serial.print("release\t");
    Serial.println(key);
    if(bleGamepad.isConnected()) {
      bleGamepad.release(gamepadbutton);
    }
}



////////////////////////////////////////////////////////////////////////////////////////
