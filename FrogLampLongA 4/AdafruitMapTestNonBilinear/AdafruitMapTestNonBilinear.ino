#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include "MatrixDraw.h"
//#include "MatrixMapping.h"

//#define DEBUG
#define LED_PIN  11

#define COLOR_ORDER GRB
#define CHIPSET     WS2811

#define BRIGHTNESS 255

int PIR1Pin = 2; // Top
int PIR2Pin = 3; // Right
int PIR3Pin = 5; // Left
int PIR4Pin = 4; // Bottom

int pausePin = 14;

long MAXtime = 0;

void setupPausePin()
{
  pinMode(pausePin, INPUT);
}

int readPausePin()
{
  Serial.println(digitalRead(pausePin));
  return digitalRead(pausePin);
}

int direction = 0;
int maxDirection = 10;
float curShapePos = 0;
float posChangeRate = 0.01;

int lastPIR1State = 0;
int PIR1State = 0;

int lastPIR2State = 0;
int PIR2State = 0;

int lastPIR3State = 0;
int PIR3State = 0;

int lastPIR4State = 0;
int PIR4State = 0;

long PIR1StartTime = -200000;
long PIR2StartTime = -200000;
long PIR3StartTime = -200000;
long PIR4StartTime = -200000;

float PIR1Val = 0;
float PIR2Val = 0;
float PIR3Val = 0;
float PIR4Val = 0;
float PIRChangeValue = 0.01;


int PIR1Triggered = 0;
int PIR2Triggered = 0;
int PIR3Triggered = 0;
int PIR4Triggered = 0;


int curDefault = 0;


boolean leftUp = false;
int lastLeft = 0;

int xSize = 13;
int ySize = 106;
int maxLEDList = 2;
int LEDMap[13][106][2];
MatrixDraw draw(xSize, ySize, 9);

// Params for width and height
const uint8_t kMatrixWidth = xSize;
const uint8_t kMatrixHeight = ySize;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;


#define RGBW
#define NUM_LEDS 400

#ifdef RGBW
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_RGBW + NEO_KHZ800);
#else
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
#endif

void initArray();
void DrawOneFrame(byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8);
void LightDefault(uint32_t ms);
void LightLeft(uint32_t ms);
void LightRight(uint32_t ms);
void LightUp(uint32_t ms);
void LightDown(uint32_t ms);
void TestColors(uint32_t ms);
void ClearAll();
void LightLeftSide();
void LightRightSide();
void LightUpSide();
void LightDownSide();


void DrawShape(float centerY, float size, float spread, float brightness)
{
    int maxX = xSize - 1;
    int maxY = ySize - 1;
    int maxLength = 10;
    int curLength = size*maxLength;
    int yPosCenter = centerY*maxY;
    int startY = yPosCenter - curLength/2;
    int endY = yPosCenter + curLength/2;
    int maxSpread = 7;
    int filterKernel = spread*maxSpread;
    int shapeBrightness = brightness*255;

    if(filterKernel%2 == 0)
      filterKernel++;
    if(filterKernel > maxSpread)
      filterKernel = maxSpread;

    if(endY > maxY)
      endY = maxY-1;
    else if(endY < 0)
      endY = 0;

    if(startY < 0)
      startY = 0;
    else if(startY > maxY)
      startY = maxY-1;

    draw.DrawRectangle(0, startY, maxX, endY, shapeBrightness);
    draw.Fill(maxX/2,yPosCenter, shapeBrightness);
    draw.MeanFilter(3);
}


void PrintPIRStates()
{
	Serial.print("PIR1: ");
	Serial.print(PIR1State);
	Serial.print(" PIR2: ");
	Serial.print(PIR2State);
	Serial.print(" PIR3: ");
	Serial.print(PIR3State);
	Serial.print(" PIR4: ");
	Serial.println(PIR4State);
}

long GetPIRStartTime(int PIRState, int lastPIRState)
{
  if(PIRState == 1 && lastPIRState == 0)
    return millis();

  return -1;
}

void ReadPIRSensors()
{
  PIR1State = digitalRead(PIR1Pin);
  PIR2State = digitalRead(PIR2Pin);
  PIR3State = digitalRead(PIR3Pin);
  PIR4State = digitalRead(PIR4Pin);

  // PIR1State = 0;
  // PIR2State = 0;
  // PIR3State = 0;
  // PIR4State = 0;

  if(PIR1State || PIR2State)
  {
    direction++;
  }
  else if(PIR3State || PIR4State)
  {
    direction--;
  }
  else
  {
    if(direction > 0)
      direction--;
    else if(direction < 0)
      direction++;
    else
      direction = 0;
  }

  if(direction > maxDirection)
  {
    direction = maxDirection;
  }
  else if(direction < -1*maxDirection)
  {
    direction = -1*maxDirection;
  }

  long curMillis = GetPIRStartTime(PIR1State, lastPIR1State);
  if(curMillis > 0 && PIR1Triggered == 0)
  {
    PIR1StartTime = curMillis;
    PIR1Triggered = 1;
  }

  curMillis = GetPIRStartTime(PIR2State, lastPIR2State);
  if(curMillis > 0 && PIR2Triggered == 0)
  {
    PIR2StartTime = curMillis;
    PIR2Triggered = 1;
  }

  curMillis = GetPIRStartTime(PIR3State, lastPIR3State);
  if(curMillis > 0  && PIR3Triggered == 0)
  {
    PIR3StartTime = curMillis;
    PIR3Triggered = 1;
  }

  curMillis = GetPIRStartTime(PIR4State, lastPIR4State);
  if(curMillis > 0 && PIR4Triggered == 0)
  {
    PIR4StartTime = curMillis;
    PIR4Triggered = 1;
  }

  lastPIR1State = PIR1State;
  lastPIR2State = PIR2State;
  lastPIR3State = PIR3State;
  lastPIR4State = PIR4State;
}

void setupPIR() 
{
	pinMode(PIR1Pin, INPUT);
	pinMode(PIR2Pin, INPUT);
	pinMode(PIR3Pin, INPUT);
	pinMode(PIR4Pin, INPUT);
}

void XY( uint8_t x, uint8_t y, uint8_t nHue, uint8_t degreeOfGreen = 255)
{ 
      int ledToWrite;
      for(int i = 0; i < maxLEDList; i++){
        
        ledToWrite = LEDMap[x][y][i];

        // int redDefault = 140;
        // int greenDefault = 112;
        // int blueDefault = 47;

        int redDefault = 30;
        int greenDefault = 60;
        int blueDefault = 30;
        int whiteDefault = 200;

        redDefault = degreeOfGreen/255.0 * redDefault;
        greenDefault = degreeOfGreen/255.0 * greenDefault;
        blueDefault = degreeOfGreen/255.0 * blueDefault;
        whiteDefault = degreeOfGreen/255.0 * whiteDefault;

        if(ledToWrite == 0)
          return;

        #ifdef RGBW
        if(ledToWrite >= 0) pixels.setPixelColor(ledToWrite, pixels.Color(greenDefault, redDefault, blueDefault, whiteDefault));
        #else
        if(ledToWrite >= 0) pixels.setPixelColor(ledToWrite, pixels.Color(redDefault, greenDefault, blueDefault));
      	//if(ledToWrite >= 0) leds[ ledToWrite ]  = CRGB( nHue, degreeOfGreen, nHue);
      	#endif
        else return;
      
      }
      
  
}

void SerialDrawMatrix()
{
  for(int y = 0; y < ySize; y++)
  {
    for(int x = 0; x < xSize; x++)
    {
      int curValue = draw.GetValueAt(x,y);
      if(curValue > 0)
      {
        Serial.print(" x");
      }
      else
      {
        Serial.print("  ");
      }
    }
    Serial.println();
  }
}

void DrawMatrix()
{
  for(int x = 0; x < xSize; x++)
  {
    for(int y = 0; y < ySize; y++)
    {
      int curValue = draw.GetValueAt(x,y);
      XY(x, y, 0, curValue);
    }
  }
}
//////////////////////////////////////////////////
///// BEGIN ANIMATION 1 /////////////////////////


// Demo that USES "XY" follows code below
#define STATE1TIME 0
#define STATE2TIME STATE1TIME + 1000   
#define STATE3TIME STATE2TIME + 1000   
#define STATE4TIME STATE3TIME + 1000   
#define STATE5TIME STATE4TIME + 1000   
#define STATE6TIME STATE5TIME + 2000  
#define STATE7TIME STATE6TIME + 1000   
#define STATE8TIME STATE7TIME + 1000   
#define STATE9TIME STATE8TIME + 1000   
#define STATE10TIME STATE9TIME + 1000  
#define MAX_TIME STATE9TIME + 1000   

#define MAXQONE 0.3
#define MAXQTWO 1.0

long lastMillis = 0;
long SpecialTriggered = -200000;

float GetPulseSpeed(int beginTime, int endTime, float maxQ, float minQ)
{
  float factor = millis() - lastMillis;
  float pulseTime = endTime - beginTime;
  return factor*((maxQ-minQ) / pulseTime);
}

// Contains Animation Information
float GetPIRValue(long PIRTime, float PIRVal, int dir, int &PIRTriggered)
{

  if(millis() - PIRTime > MAX_TIME + 1000)
    PIRTriggered = 0;

  if(millis() - PIRTime > MAX_TIME)
  {
    PIRVal -= PIRChangeValue;
    if(PIRVal < 0)
      PIRVal = 0.0;
  }
  else if(millis() - PIRTime > STATE9TIME)
  {
    //PIRVal += GetPulseSpeed(STATE9TIME, MAX_TIME, MAXQTWO, 0);
    PIRVal += PIRChangeValue;
  }
  else if(millis() - PIRTime > STATE8TIME)
  {
    //PIRVal -= GetPulseSpeed(STATE8TIME, STATE9TIME, MAXQTWO, 0);
    PIRVal -= PIRChangeValue;
  }
  else if(millis() - PIRTime > STATE7TIME)
  {
    //PIRVal += GetPulseSpeed(STATE7TIME, STATE8TIME, MAXQTWO, 0);
    PIRVal += PIRChangeValue;
  }
  else if(millis() - PIRTime > STATE6TIME)
  {
    //PIRVal -= GetPulseSpeed(STATE6TIME, STATE7TIME, MAXQTWO, 0);
    PIRVal -= PIRChangeValue;
  }
  else if(millis() - PIRTime > STATE5TIME)
  {
    ///PIRVal += GetPulseSpeed(STATE5TIME, STATE6TIME, MAXQTWO, 0);
    PIRVal += PIRChangeValue;
  }
  else if(millis() - PIRTime > STATE4TIME)
  {
    //PIRVal -= GetPulseSpeed(STATE4TIME, STATE5TIME, MAXQTWO, 0);
    PIRVal -= PIRChangeValue;
  }
  else if(millis() - PIRTime > STATE3TIME)
  {
    // Pulse Up
    //PIRVal += GetPulseSpeed(STATE3TIME, STATE4TIME, MAXQTWO, 0);
    PIRVal += PIRChangeValue;
  }
  else if(millis() - PIRTime > STATE2TIME)
  {
    // Pulse Down
    //PIRVal -= GetPulseSpeed(STATE2TIME, STATE3TIME, MAXQTWO, 0);
    PIRVal -= PIRChangeValue;
  }
  else if(millis() - PIRTime > STATE1TIME)
  {
    // Pulse Up
    //PIRVal += GetPulseSpeed(STATE1TIME, STATE2TIME, MAXQTWO, 0);
    PIRVal += PIRChangeValue;
  }
  
  if(PIRVal < 0)
    PIRVal = 0;
  else if(PIRVal > MAXQTWO)
    PIRVal = MAXQTWO;

  float curPos = 0.0;

    if(dir == 1)
      curPos = 1.0 - PIRVal;
    else
      curPos = PIRVal;

    //curPos = PIRVal;

    if(curPos > 1.0)
      curPos = 1.0;

    Serial.print(" CurPos: ");
    Serial.print(curPos); 

    DrawShape(curPos, 1.0, 1.0, PIRVal);

  return PIRVal;
}

// Can use PIR values for all lighting. They will float around based on
// The state machien described in the function above.
void DrawShapeSensor()
{

  PIR1Val = GetPIRValue(PIR1StartTime, PIR1Val, 1, PIR1Triggered);
  PIR2Val = GetPIRValue(PIR2StartTime, PIR2Val, 1, PIR2Triggered);
  PIR3Val = GetPIRValue(PIR3StartTime, PIR3Val, 0, PIR3Triggered);
  PIR4Val = GetPIRValue(PIR4StartTime, PIR4Val, 0, PIR4Triggered);

  lastMillis = millis();

  // curShapePos += direction*posChangeRate;
  // if(curShapePos > 1.0)
  //   curShapePos = 1.0;
  // else if(curShapePos < 0)
  //   curShapePos = 0.0;
  
  Serial.print(" 1Val: ");
  Serial.print(PIR1Val);
  Serial.print("   2Val: ");
  Serial.print(PIR2Val);
  Serial.print("   3Val: ");
  Serial.print(PIR3Val);
  Serial.print("   4Val: ");
  Serial.println(PIR4Val);
  // Serial.print("   Direction,ShapePos: ");
  // Serial.print(direction);
  // Serial.print(",");
  // Serial.println(curShapePos);
}

void DrawSolidColor()
{
    //draw.DrawRectangle(0, 0, xSize-1, ySize-1, 200);
    draw.ClearMatrix();
    draw.Fill(xSize/2,ySize/2, 255);
}

void loop()
{
  ReadPIRSensors();

#ifdef DEBUG
  PrintPIRStates();
#endif

    uint32_t ms = millis();
//     // int32_t yHueDelta32 = ((int32_t)cos16( ms * (27/1) ) * (350 / kMatrixWidth));
//     // int32_t xHueDelta32 = ((int32_t)cos16( ms * (39/1) ) * (310 / kMatrixHeight));
//     // DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
    //TestColors(ms);

    ClearAll();

    if(readPausePin() == 0)
    {
      draw.ClearMatrix();
      DrawShapeSensor();
    }
    
   // DrawSolidColor();

    DrawMatrix();
    pixels.show();
}

void ClearAll()
{
  
  for(byte y = 0; y < kMatrixHeight; y++) {
      for(byte x = 0; x < kMatrixWidth; x++){
        XY(x,y, 0, 0);
    }
  }
}

void TestColors(uint32_t ms)
{
	ClearAll();

  LightDefault(ms);
 // LightLeft(ms);
  LightUp(ms);
	// if(ms < 5000){
	// 	LightRight(ms);
	// }
	// else if(ms < 7000){
	// 	LightUp(ms);
	// }
	// else if(ms < 9000){
	// 	LightLeft(ms);
	// }
	// else {
	// 	LightDown(ms);
	// }
}

void LightDefault(uint32_t ms)
{
  int pulse = 0;
  pulse = (ms/50)%255;

  if(pulse < lastLeft)
  	leftUp = !leftUp;

  lastLeft = pulse;

  if(leftUp)
  	pulse = pulse;
  else 
  	pulse = 255 - pulse;

  curDefault = pulse;

  for(byte y = 0; y < kMatrixHeight; y++) {
  	for(byte x = 0; x < kMatrixWidth; x++){
  		XY(x,y, pulse);
  	}
  }
}

void LightRight(uint32_t ms)
{
  int startPoint = 0;
  int interpDistance = 10;
  startPoint = kMatrixWidth-(ms/50)%kMatrixWidth;

  for(byte y = 0; y < kMatrixHeight; y++) {
    for(byte x = 0; x < kMatrixWidth; x++){
      if(startPoint == x)// && y == kMatrixHeight/2)
        XY(x,y, 0, 0);
      else if(abs(startPoint-x) < interpDistance){
        int interpValue = 0 + curDefault/(interpDistance-abs(startPoint-x));
        int greenVal = 0 + 255/(interpDistance - abs(startPoint-x));
        XY(x,y,interpValue, greenVal);
      }
    }
  }
}

void LightUp(uint32_t ms)
{
	  int startPoint = 0;
  int interpDistance = 10;
  startPoint = kMatrixHeight-(ms/50)%kMatrixHeight;

    for(byte y = 0; y < kMatrixHeight; y++) {
      if(startPoint == y)
      {
        for(byte x = 0; x < kMatrixWidth; x++){
          XY(x,y, 0, true);
        }
      }
      else if(abs(startPoint-y) < interpDistance)
      {
        for(byte x = 0; x < kMatrixWidth; x++){
          int interpValue = 0 + curDefault/(interpDistance-abs(startPoint-y));
          int greenVal = 0 + 255/(interpDistance - abs(startPoint-y));
          XY(x,y,interpValue, greenVal);
        }
      }
      
    }
}

void LightDown(uint32_t ms)
{
	int startPoint = 0;
  int interpDistance = 10;
	startPoint = (ms/50)%kMatrixHeight;

	  for(byte y = 0; y < kMatrixHeight; y++) {
	  	if(startPoint == y)
	  	{
	  		for(byte x = 0; x < kMatrixWidth; x++){
	  			XY(x,y, 0, true);
	  		}
	  	}
      else if(abs(startPoint-y) < interpDistance)
      {
        for(byte x = 0; x < kMatrixWidth; x++){
          int interpValue = 0 + curDefault/(interpDistance-abs(startPoint-y));
          int greenVal = 0 + 255/(interpDistance - abs(startPoint-y));
          XY(x,y,interpValue, greenVal);
        }
      }
	  	
	  }
}

void LightLeft(uint32_t ms)
{
  
  int startPoint = 0;
  int interpDistance = 10;
  startPoint = (ms/50)%kMatrixWidth;


  for(byte y = 0; y < kMatrixHeight; y++) {
  	for(byte x = 0; x < kMatrixWidth; x++){
  		if(startPoint == x)// && y == kMatrixHeight/2)
  			XY(x,y, 0, 0);
      else if(abs(startPoint-x) < interpDistance){
        int interpValue = 0 + curDefault/(interpDistance-abs(startPoint-x));
        int greenVal = 0 + 255/(interpDistance - abs(startPoint-x));
        XY(x,y,interpValue, greenVal);
      }
  	}
  }
}

void LightLeftSide()
{
  for(byte y = 0; y < kMatrixHeight; y++) {
    for(byte x = kMatrixWidth/2; x < kMatrixWidth; x++){
        XY(x,y, 0, 255);
      }
    }
}

void LightRightSide()
{
  for(byte y = 0; y < kMatrixHeight; y++) {
    for(byte x = 0; x < kMatrixWidth/2; x++){
        XY(x,y, 0, 255);
      }
    }
}

void LightUpSide()
{
  for(byte y = 0; y < kMatrixHeight/2; y++) {
    for(byte x = 0; x < kMatrixWidth; x++){
        XY(x,y, 0, 255);
      }
    }
}

void LightDownSide()
{
  for(byte y = kMatrixHeight/2; y < kMatrixHeight; y++) {
    for(byte x = 0; x < kMatrixWidth; x++){
        XY(x,y, 0, 255);
      }
    }
}

void DrawOneFrame( byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8)
{
  
  // Quadrant 1
  for(byte y = 0; y < kMatrixHeight/2; y++) {
  	for(byte x = 0; x < kMatrixWidth/2; x++){
  		XY(x,y, 0);
  	}
  }

 // Quadrant 2
 for(byte y = kMatrixHeight/2; y < kMatrixHeight; y++) {
  	for(byte x = 0; x < kMatrixWidth/2; x++){
  		XY(x,y, 64);
  	}
  }

  // Quadrant 3
 for(byte y = kMatrixHeight/2; y < kMatrixHeight; y++) {
  	for(byte x = kMatrixWidth/2; x < kMatrixWidth; x++){
  		XY(x,y, 128);
  	}
  }

 // Quadrant 4
 for(byte y = 0; y < kMatrixHeight/2; y++) {
  	for(byte x = kMatrixWidth/2; x < kMatrixWidth; x++){
  		XY(x,y, 192);
  	}
  }

}

void setup() {
  Serial.begin(9600);

  setupPIR();
  initArray();
  setupPausePin();

  MAXtime = MAX_TIME;
  
  pixels.begin(); // This initializes the NeoPixel library.
  pixels.show();
}

void initArray(){
LEDMap[0][0][0] = -1;
LEDMap[0][0][1] = -1;
LEDMap[1][0][0] = -1;
LEDMap[1][0][1] = -1;
LEDMap[2][0][0] = 69;
LEDMap[2][0][1] = -1;
LEDMap[3][0][0] = -1;
LEDMap[3][0][1] = -1;
LEDMap[4][0][0] = -1;
LEDMap[4][0][1] = -1;
LEDMap[5][0][0] = -1;
LEDMap[5][0][1] = -1;
LEDMap[6][0][0] = -1;
LEDMap[6][0][1] = -1;
LEDMap[7][0][0] = -1;
LEDMap[7][0][1] = -1;
LEDMap[8][0][0] = -1;
LEDMap[8][0][1] = -1;
LEDMap[9][0][0] = -1;
LEDMap[9][0][1] = -1;
LEDMap[10][0][0] = -1;
LEDMap[10][0][1] = -1;
LEDMap[11][0][0] = -1;
LEDMap[11][0][1] = -1;
LEDMap[12][0][0] = -1;
LEDMap[12][0][1] = -1;
LEDMap[0][1][0] = -1;
LEDMap[0][1][1] = -1;
LEDMap[1][1][0] = -1;
LEDMap[1][1][1] = -1;
LEDMap[2][1][0] = -1;
LEDMap[2][1][1] = -1;
LEDMap[3][1][0] = 67;
LEDMap[3][1][1] = -1;
LEDMap[4][1][0] = -1;
LEDMap[4][1][1] = -1;
LEDMap[5][1][0] = -1;
LEDMap[5][1][1] = -1;
LEDMap[6][1][0] = -1;
LEDMap[6][1][1] = -1;
LEDMap[7][1][0] = -1;
LEDMap[7][1][1] = -1;
LEDMap[8][1][0] = -1;
LEDMap[8][1][1] = -1;
LEDMap[9][1][0] = -1;
LEDMap[9][1][1] = -1;
LEDMap[10][1][0] = -1;
LEDMap[10][1][1] = -1;
LEDMap[11][1][0] = -1;
LEDMap[11][1][1] = -1;
LEDMap[12][1][0] = -1;
LEDMap[12][1][1] = -1;
LEDMap[0][2][0] = 65;
LEDMap[0][2][1] = -1;
LEDMap[1][2][0] = -1;
LEDMap[1][2][1] = -1;
LEDMap[2][2][0] = -1;
LEDMap[2][2][1] = -1;
LEDMap[3][2][0] = 70;
LEDMap[3][2][1] = -1;
LEDMap[4][2][0] = -1;
LEDMap[4][2][1] = -1;
LEDMap[5][2][0] = -1;
LEDMap[5][2][1] = -1;
LEDMap[6][2][0] = -1;
LEDMap[6][2][1] = -1;
LEDMap[7][2][0] = -1;
LEDMap[7][2][1] = -1;
LEDMap[8][2][0] = -1;
LEDMap[8][2][1] = -1;
LEDMap[9][2][0] = -1;
LEDMap[9][2][1] = -1;
LEDMap[10][2][0] = -1;
LEDMap[10][2][1] = -1;
LEDMap[11][2][0] = -1;
LEDMap[11][2][1] = -1;
LEDMap[12][2][0] = 207;
LEDMap[12][2][1] = -1;
LEDMap[0][3][0] = 64;
LEDMap[0][3][1] = -1;
LEDMap[1][3][0] = -1;
LEDMap[1][3][1] = -1;
LEDMap[2][3][0] = -1;
LEDMap[2][3][1] = -1;
LEDMap[3][3][0] = 71;
LEDMap[3][3][1] = -1;
LEDMap[4][3][0] = -1;
LEDMap[4][3][1] = -1;
LEDMap[5][3][0] = -1;
LEDMap[5][3][1] = -1;
LEDMap[6][3][0] = 72;
LEDMap[6][3][1] = -1;
LEDMap[7][3][0] = -1;
LEDMap[7][3][1] = -1;
LEDMap[8][3][0] = -1;
LEDMap[8][3][1] = -1;
LEDMap[9][3][0] = -1;
LEDMap[9][3][1] = -1;
LEDMap[10][3][0] = -1;
LEDMap[10][3][1] = -1;
LEDMap[11][3][0] = -1;
LEDMap[11][3][1] = -1;
LEDMap[12][3][0] = -1;
LEDMap[12][3][1] = -1;
LEDMap[0][4][0] = -1;
LEDMap[0][4][1] = -1;
LEDMap[1][4][0] = -1;
LEDMap[1][4][1] = -1;
LEDMap[2][4][0] = -1;
LEDMap[2][4][1] = -1;
LEDMap[3][4][0] = -1;
LEDMap[3][4][1] = -1;
LEDMap[4][4][0] = -1;
LEDMap[4][4][1] = -1;
LEDMap[5][4][0] = -1;
LEDMap[5][4][1] = -1;
LEDMap[6][4][0] = 73;
LEDMap[6][4][1] = -1;
LEDMap[7][4][0] = -1;
LEDMap[7][4][1] = -1;
LEDMap[8][4][0] = -1;
LEDMap[8][4][1] = -1;
LEDMap[9][4][0] = -1;
LEDMap[9][4][1] = -1;
LEDMap[10][4][0] = -1;
LEDMap[10][4][1] = -1;
LEDMap[11][4][0] = -1;
LEDMap[11][4][1] = -1;
LEDMap[12][4][0] = 206;
LEDMap[12][4][1] = -1;
LEDMap[0][5][0] = 63;
LEDMap[0][5][1] = -1;
LEDMap[1][5][0] = -1;
LEDMap[1][5][1] = -1;
LEDMap[2][5][0] = -1;
LEDMap[2][5][1] = -1;
LEDMap[3][5][0] = -1;
LEDMap[3][5][1] = -1;
LEDMap[4][5][0] = -1;
LEDMap[4][5][1] = -1;
LEDMap[5][5][0] = -1;
LEDMap[5][5][1] = -1;
LEDMap[6][5][0] = -1;
LEDMap[6][5][1] = -1;
LEDMap[7][5][0] = -1;
LEDMap[7][5][1] = -1;
LEDMap[8][5][0] = 66;
LEDMap[8][5][1] = -1;
LEDMap[9][5][0] = -1;
LEDMap[9][5][1] = -1;
LEDMap[10][5][0] = -1;
LEDMap[10][5][1] = -1;
LEDMap[11][5][0] = -1;
LEDMap[11][5][1] = -1;
LEDMap[12][5][0] = -1;
LEDMap[12][5][1] = -1;
LEDMap[0][6][0] = -1;
LEDMap[0][6][1] = -1;
LEDMap[1][6][0] = -1;
LEDMap[1][6][1] = -1;
LEDMap[2][6][0] = -1;
LEDMap[2][6][1] = -1;
LEDMap[3][6][0] = -1;
LEDMap[3][6][1] = -1;
LEDMap[4][6][0] = -1;
LEDMap[4][6][1] = -1;
LEDMap[5][6][0] = -1;
LEDMap[5][6][1] = -1;
LEDMap[6][6][0] = 74;
LEDMap[6][6][1] = -1;
LEDMap[7][6][0] = -1;
LEDMap[7][6][1] = -1;
LEDMap[8][6][0] = -1;
LEDMap[8][6][1] = -1;
LEDMap[9][6][0] = -1;
LEDMap[9][6][1] = -1;
LEDMap[10][6][0] = -1;
LEDMap[10][6][1] = -1;
LEDMap[11][6][0] = -1;
LEDMap[11][6][1] = -1;
LEDMap[12][6][0] = 205;
LEDMap[12][6][1] = -1;
LEDMap[0][7][0] = 62;
LEDMap[0][7][1] = -1;
LEDMap[1][7][0] = -1;
LEDMap[1][7][1] = -1;
LEDMap[2][7][0] = -1;
LEDMap[2][7][1] = -1;
LEDMap[3][7][0] = -1;
LEDMap[3][7][1] = -1;
LEDMap[4][7][0] = -1;
LEDMap[4][7][1] = -1;
LEDMap[5][7][0] = -1;
LEDMap[5][7][1] = -1;
LEDMap[6][7][0] = -1;
LEDMap[6][7][1] = -1;
LEDMap[7][7][0] = -1;
LEDMap[7][7][1] = -1;
LEDMap[8][7][0] = -1;
LEDMap[8][7][1] = -1;
LEDMap[9][7][0] = -1;
LEDMap[9][7][1] = -1;
LEDMap[10][7][0] = -1;
LEDMap[10][7][1] = -1;
LEDMap[11][7][0] = -1;
LEDMap[11][7][1] = -1;
LEDMap[12][7][0] = 204;
LEDMap[12][7][1] = -1;
LEDMap[0][8][0] = 61;
LEDMap[0][8][1] = -1;
LEDMap[1][8][0] = -1;
LEDMap[1][8][1] = -1;
LEDMap[2][8][0] = -1;
LEDMap[2][8][1] = -1;
LEDMap[3][8][0] = -1;
LEDMap[3][8][1] = -1;
LEDMap[4][8][0] = -1;
LEDMap[4][8][1] = -1;
LEDMap[5][8][0] = -1;
LEDMap[5][8][1] = -1;
LEDMap[6][8][0] = 75;
LEDMap[6][8][1] = -1;
LEDMap[7][8][0] = -1;
LEDMap[7][8][1] = -1;
LEDMap[8][8][0] = -1;
LEDMap[8][8][1] = -1;
LEDMap[9][8][0] = -1;
LEDMap[9][8][1] = -1;
LEDMap[10][8][0] = -1;
LEDMap[10][8][1] = -1;
LEDMap[11][8][0] = -1;
LEDMap[11][8][1] = -1;
LEDMap[12][8][0] = -1;
LEDMap[12][8][1] = -1;
LEDMap[0][9][0] = -1;
LEDMap[0][9][1] = -1;
LEDMap[1][9][0] = -1;
LEDMap[1][9][1] = -1;
LEDMap[2][9][0] = -1;
LEDMap[2][9][1] = -1;
LEDMap[3][9][0] = -1;
LEDMap[3][9][1] = -1;
LEDMap[4][9][0] = -1;
LEDMap[4][9][1] = -1;
LEDMap[5][9][0] = -1;
LEDMap[5][9][1] = -1;
LEDMap[6][9][0] = 76;
LEDMap[6][9][1] = -1;
LEDMap[7][9][0] = -1;
LEDMap[7][9][1] = -1;
LEDMap[8][9][0] = -1;
LEDMap[8][9][1] = -1;
LEDMap[9][9][0] = -1;
LEDMap[9][9][1] = -1;
LEDMap[10][9][0] = -1;
LEDMap[10][9][1] = -1;
LEDMap[11][9][0] = -1;
LEDMap[11][9][1] = -1;
LEDMap[12][9][0] = 203;
LEDMap[12][9][1] = -1;
LEDMap[0][10][0] = 60;
LEDMap[0][10][1] = -1;
LEDMap[1][10][0] = -1;
LEDMap[1][10][1] = -1;
LEDMap[2][10][0] = -1;
LEDMap[2][10][1] = -1;
LEDMap[3][10][0] = -1;
LEDMap[3][10][1] = -1;
LEDMap[4][10][0] = -1;
LEDMap[4][10][1] = -1;
LEDMap[5][10][0] = -1;
LEDMap[5][10][1] = -1;
LEDMap[6][10][0] = -1;
LEDMap[6][10][1] = -1;
LEDMap[7][10][0] = -1;
LEDMap[7][10][1] = -1;
LEDMap[8][10][0] = -1;
LEDMap[8][10][1] = -1;
LEDMap[9][10][0] = -1;
LEDMap[9][10][1] = -1;
LEDMap[10][10][0] = -1;
LEDMap[10][10][1] = -1;
LEDMap[11][10][0] = -1;
LEDMap[11][10][1] = -1;
LEDMap[12][10][0] = -1;
LEDMap[12][10][1] = -1;
LEDMap[0][11][0] = -1;
LEDMap[0][11][1] = -1;
LEDMap[1][11][0] = -1;
LEDMap[1][11][1] = -1;
LEDMap[2][11][0] = -1;
LEDMap[2][11][1] = -1;
LEDMap[3][11][0] = -1;
LEDMap[3][11][1] = -1;
LEDMap[4][11][0] = -1;
LEDMap[4][11][1] = -1;
LEDMap[5][11][0] = -1;
LEDMap[5][11][1] = -1;
LEDMap[6][11][0] = 77;
LEDMap[6][11][1] = -1;
LEDMap[7][11][0] = -1;
LEDMap[7][11][1] = -1;
LEDMap[8][11][0] = -1;
LEDMap[8][11][1] = -1;
LEDMap[9][11][0] = -1;
LEDMap[9][11][1] = -1;
LEDMap[10][11][0] = -1;
LEDMap[10][11][1] = -1;
LEDMap[11][11][0] = -1;
LEDMap[11][11][1] = -1;
LEDMap[12][11][0] = 202;
LEDMap[12][11][1] = -1;
LEDMap[0][12][0] = -1;
LEDMap[0][12][1] = -1;
LEDMap[1][12][0] = -1;
LEDMap[1][12][1] = -1;
LEDMap[2][12][0] = -1;
LEDMap[2][12][1] = -1;
LEDMap[3][12][0] = -1;
LEDMap[3][12][1] = -1;
LEDMap[4][12][0] = -1;
LEDMap[4][12][1] = -1;
LEDMap[5][12][0] = -1;
LEDMap[5][12][1] = -1;
LEDMap[6][12][0] = -1;
LEDMap[6][12][1] = -1;
LEDMap[7][12][0] = -1;
LEDMap[7][12][1] = -1;
LEDMap[8][12][0] = -1;
LEDMap[8][12][1] = -1;
LEDMap[9][12][0] = -1;
LEDMap[9][12][1] = -1;
LEDMap[10][12][0] = -1;
LEDMap[10][12][1] = -1;
LEDMap[11][12][0] = -1;
LEDMap[11][12][1] = -1;
LEDMap[12][12][0] = 201;
LEDMap[12][12][1] = -1;
LEDMap[0][13][0] = 58;
LEDMap[0][13][1] = -1;
LEDMap[1][13][0] = -1;
LEDMap[1][13][1] = -1;
LEDMap[2][13][0] = -1;
LEDMap[2][13][1] = -1;
LEDMap[3][13][0] = -1;
LEDMap[3][13][1] = -1;
LEDMap[4][13][0] = -1;
LEDMap[4][13][1] = -1;
LEDMap[5][13][0] = -1;
LEDMap[5][13][1] = -1;
LEDMap[6][13][0] = 78;
LEDMap[6][13][1] = -1;
LEDMap[7][13][0] = -1;
LEDMap[7][13][1] = -1;
LEDMap[8][13][0] = -1;
LEDMap[8][13][1] = -1;
LEDMap[9][13][0] = -1;
LEDMap[9][13][1] = -1;
LEDMap[10][13][0] = -1;
LEDMap[10][13][1] = -1;
LEDMap[11][13][0] = -1;
LEDMap[11][13][1] = -1;
LEDMap[12][13][0] = -1;
LEDMap[12][13][1] = -1;
LEDMap[0][14][0] = -1;
LEDMap[0][14][1] = -1;
LEDMap[1][14][0] = -1;
LEDMap[1][14][1] = -1;
LEDMap[2][14][0] = -1;
LEDMap[2][14][1] = -1;
LEDMap[3][14][0] = -1;
LEDMap[3][14][1] = -1;
LEDMap[4][14][0] = -1;
LEDMap[4][14][1] = -1;
LEDMap[5][14][0] = -1;
LEDMap[5][14][1] = -1;
LEDMap[6][14][0] = 79;
LEDMap[6][14][1] = -1;
LEDMap[7][14][0] = -1;
LEDMap[7][14][1] = -1;
LEDMap[8][14][0] = -1;
LEDMap[8][14][1] = -1;
LEDMap[9][14][0] = -1;
LEDMap[9][14][1] = -1;
LEDMap[10][14][0] = -1;
LEDMap[10][14][1] = -1;
LEDMap[11][14][0] = -1;
LEDMap[11][14][1] = -1;
LEDMap[12][14][0] = 200;
LEDMap[12][14][1] = -1;
LEDMap[0][15][0] = 57;
LEDMap[0][15][1] = -1;
LEDMap[1][15][0] = -1;
LEDMap[1][15][1] = -1;
LEDMap[2][15][0] = -1;
LEDMap[2][15][1] = -1;
LEDMap[3][15][0] = -1;
LEDMap[3][15][1] = -1;
LEDMap[4][15][0] = -1;
LEDMap[4][15][1] = -1;
LEDMap[5][15][0] = -1;
LEDMap[5][15][1] = -1;
LEDMap[6][15][0] = -1;
LEDMap[6][15][1] = -1;
LEDMap[7][15][0] = -1;
LEDMap[7][15][1] = -1;
LEDMap[8][15][0] = -1;
LEDMap[8][15][1] = -1;
LEDMap[9][15][0] = -1;
LEDMap[9][15][1] = -1;
LEDMap[10][15][0] = -1;
LEDMap[10][15][1] = -1;
LEDMap[11][15][0] = -1;
LEDMap[11][15][1] = -1;
LEDMap[12][15][0] = -1;
LEDMap[12][15][1] = -1;
LEDMap[0][16][0] = -1;
LEDMap[0][16][1] = -1;
LEDMap[1][16][0] = -1;
LEDMap[1][16][1] = -1;
LEDMap[2][16][0] = -1;
LEDMap[2][16][1] = -1;
LEDMap[3][16][0] = -1;
LEDMap[3][16][1] = -1;
LEDMap[4][16][0] = -1;
LEDMap[4][16][1] = -1;
LEDMap[5][16][0] = -1;
LEDMap[5][16][1] = -1;
LEDMap[6][16][0] = 80;
LEDMap[6][16][1] = -1;
LEDMap[7][16][0] = -1;
LEDMap[7][16][1] = -1;
LEDMap[8][16][0] = -1;
LEDMap[8][16][1] = -1;
LEDMap[9][16][0] = -1;
LEDMap[9][16][1] = -1;
LEDMap[10][16][0] = -1;
LEDMap[10][16][1] = -1;
LEDMap[11][16][0] = -1;
LEDMap[11][16][1] = -1;
LEDMap[12][16][0] = 199;
LEDMap[12][16][1] = -1;
LEDMap[0][17][0] = 56;
LEDMap[0][17][1] = -1;
LEDMap[1][17][0] = -1;
LEDMap[1][17][1] = -1;
LEDMap[2][17][0] = -1;
LEDMap[2][17][1] = -1;
LEDMap[3][17][0] = -1;
LEDMap[3][17][1] = -1;
LEDMap[4][17][0] = -1;
LEDMap[4][17][1] = -1;
LEDMap[5][17][0] = -1;
LEDMap[5][17][1] = -1;
LEDMap[6][17][0] = 81;
LEDMap[6][17][1] = -1;
LEDMap[7][17][0] = -1;
LEDMap[7][17][1] = -1;
LEDMap[8][17][0] = -1;
LEDMap[8][17][1] = -1;
LEDMap[9][17][0] = -1;
LEDMap[9][17][1] = -1;
LEDMap[10][17][0] = -1;
LEDMap[10][17][1] = -1;
LEDMap[11][17][0] = -1;
LEDMap[11][17][1] = -1;
LEDMap[12][17][0] = 198;
LEDMap[12][17][1] = -1;
LEDMap[0][18][0] = -1;
LEDMap[0][18][1] = -1;
LEDMap[1][18][0] = -1;
LEDMap[1][18][1] = -1;
LEDMap[2][18][0] = -1;
LEDMap[2][18][1] = -1;
LEDMap[3][18][0] = -1;
LEDMap[3][18][1] = -1;
LEDMap[4][18][0] = -1;
LEDMap[4][18][1] = -1;
LEDMap[5][18][0] = -1;
LEDMap[5][18][1] = -1;
LEDMap[6][18][0] = -1;
LEDMap[6][18][1] = -1;
LEDMap[7][18][0] = -1;
LEDMap[7][18][1] = -1;
LEDMap[8][18][0] = -1;
LEDMap[8][18][1] = -1;
LEDMap[9][18][0] = -1;
LEDMap[9][18][1] = -1;
LEDMap[10][18][0] = -1;
LEDMap[10][18][1] = -1;
LEDMap[11][18][0] = -1;
LEDMap[11][18][1] = -1;
LEDMap[12][18][0] = -1;
LEDMap[12][18][1] = -1;
LEDMap[0][19][0] = 55;
LEDMap[0][19][1] = -1;
LEDMap[1][19][0] = -1;
LEDMap[1][19][1] = -1;
LEDMap[2][19][0] = -1;
LEDMap[2][19][1] = -1;
LEDMap[3][19][0] = -1;
LEDMap[3][19][1] = -1;
LEDMap[4][19][0] = -1;
LEDMap[4][19][1] = -1;
LEDMap[5][19][0] = -1;
LEDMap[5][19][1] = -1;
LEDMap[6][19][0] = 82;
LEDMap[6][19][1] = -1;
LEDMap[7][19][0] = -1;
LEDMap[7][19][1] = -1;
LEDMap[8][19][0] = -1;
LEDMap[8][19][1] = -1;
LEDMap[9][19][0] = -1;
LEDMap[9][19][1] = -1;
LEDMap[10][19][0] = -1;
LEDMap[10][19][1] = -1;
LEDMap[11][19][0] = -1;
LEDMap[11][19][1] = -1;
LEDMap[12][19][0] = 197;
LEDMap[12][19][1] = -1;
LEDMap[0][20][0] = 54;
LEDMap[0][20][1] = -1;
LEDMap[1][20][0] = -1;
LEDMap[1][20][1] = -1;
LEDMap[2][20][0] = -1;
LEDMap[2][20][1] = -1;
LEDMap[3][20][0] = -1;
LEDMap[3][20][1] = -1;
LEDMap[4][20][0] = -1;
LEDMap[4][20][1] = -1;
LEDMap[5][20][0] = -1;
LEDMap[5][20][1] = -1;
LEDMap[6][20][0] = -1;
LEDMap[6][20][1] = -1;
LEDMap[7][20][0] = -1;
LEDMap[7][20][1] = -1;
LEDMap[8][20][0] = -1;
LEDMap[8][20][1] = -1;
LEDMap[9][20][0] = -1;
LEDMap[9][20][1] = -1;
LEDMap[10][20][0] = -1;
LEDMap[10][20][1] = -1;
LEDMap[11][20][0] = -1;
LEDMap[11][20][1] = -1;
LEDMap[12][20][0] = -1;
LEDMap[12][20][1] = -1;
LEDMap[0][21][0] = -1;
LEDMap[0][21][1] = -1;
LEDMap[1][21][0] = -1;
LEDMap[1][21][1] = -1;
LEDMap[2][21][0] = -1;
LEDMap[2][21][1] = -1;
LEDMap[3][21][0] = -1;
LEDMap[3][21][1] = -1;
LEDMap[4][21][0] = -1;
LEDMap[4][21][1] = -1;
LEDMap[5][21][0] = -1;
LEDMap[5][21][1] = -1;
LEDMap[6][21][0] = 83;
LEDMap[6][21][1] = -1;
LEDMap[7][21][0] = -1;
LEDMap[7][21][1] = -1;
LEDMap[8][21][0] = -1;
LEDMap[8][21][1] = -1;
LEDMap[9][21][0] = -1;
LEDMap[9][21][1] = -1;
LEDMap[10][21][0] = -1;
LEDMap[10][21][1] = -1;
LEDMap[11][21][0] = -1;
LEDMap[11][21][1] = -1;
LEDMap[12][21][0] = 196;
LEDMap[12][21][1] = -1;
LEDMap[0][22][0] = 53;
LEDMap[0][22][1] = -1;
LEDMap[1][22][0] = -1;
LEDMap[1][22][1] = -1;
LEDMap[2][22][0] = -1;
LEDMap[2][22][1] = -1;
LEDMap[3][22][0] = -1;
LEDMap[3][22][1] = -1;
LEDMap[4][22][0] = -1;
LEDMap[4][22][1] = -1;
LEDMap[5][22][0] = -1;
LEDMap[5][22][1] = -1;
LEDMap[6][22][0] = 84;
LEDMap[6][22][1] = -1;
LEDMap[7][22][0] = -1;
LEDMap[7][22][1] = -1;
LEDMap[8][22][0] = -1;
LEDMap[8][22][1] = -1;
LEDMap[9][22][0] = -1;
LEDMap[9][22][1] = -1;
LEDMap[10][22][0] = -1;
LEDMap[10][22][1] = -1;
LEDMap[11][22][0] = -1;
LEDMap[11][22][1] = -1;
LEDMap[12][22][0] = 195;
LEDMap[12][22][1] = -1;
LEDMap[0][23][0] = -1;
LEDMap[0][23][1] = -1;
LEDMap[1][23][0] = -1;
LEDMap[1][23][1] = -1;
LEDMap[2][23][0] = -1;
LEDMap[2][23][1] = -1;
LEDMap[3][23][0] = -1;
LEDMap[3][23][1] = -1;
LEDMap[4][23][0] = -1;
LEDMap[4][23][1] = -1;
LEDMap[5][23][0] = -1;
LEDMap[5][23][1] = -1;
LEDMap[6][23][0] = -1;
LEDMap[6][23][1] = -1;
LEDMap[7][23][0] = -1;
LEDMap[7][23][1] = -1;
LEDMap[8][23][0] = -1;
LEDMap[8][23][1] = -1;
LEDMap[9][23][0] = -1;
LEDMap[9][23][1] = -1;
LEDMap[10][23][0] = -1;
LEDMap[10][23][1] = -1;
LEDMap[11][23][0] = -1;
LEDMap[11][23][1] = -1;
LEDMap[12][23][0] = -1;
LEDMap[12][23][1] = -1;
LEDMap[0][24][0] = 52;
LEDMap[0][24][1] = -1;
LEDMap[1][24][0] = -1;
LEDMap[1][24][1] = -1;
LEDMap[2][24][0] = -1;
LEDMap[2][24][1] = -1;
LEDMap[3][24][0] = -1;
LEDMap[3][24][1] = -1;
LEDMap[4][24][0] = -1;
LEDMap[4][24][1] = -1;
LEDMap[5][24][0] = -1;
LEDMap[5][24][1] = -1;
LEDMap[6][24][0] = 85;
LEDMap[6][24][1] = -1;
LEDMap[7][24][0] = -1;
LEDMap[7][24][1] = -1;
LEDMap[8][24][0] = -1;
LEDMap[8][24][1] = -1;
LEDMap[9][24][0] = -1;
LEDMap[9][24][1] = -1;
LEDMap[10][24][0] = -1;
LEDMap[10][24][1] = -1;
LEDMap[11][24][0] = -1;
LEDMap[11][24][1] = -1;
LEDMap[12][24][0] = 194;
LEDMap[12][24][1] = -1;
LEDMap[0][25][0] = 51;
LEDMap[0][25][1] = -1;
LEDMap[1][25][0] = -1;
LEDMap[1][25][1] = -1;
LEDMap[2][25][0] = -1;
LEDMap[2][25][1] = -1;
LEDMap[3][25][0] = -1;
LEDMap[3][25][1] = -1;
LEDMap[4][25][0] = -1;
LEDMap[4][25][1] = -1;
LEDMap[5][25][0] = -1;
LEDMap[5][25][1] = -1;
LEDMap[6][25][0] = 86;
LEDMap[6][25][1] = -1;
LEDMap[7][25][0] = -1;
LEDMap[7][25][1] = -1;
LEDMap[8][25][0] = -1;
LEDMap[8][25][1] = -1;
LEDMap[9][25][0] = -1;
LEDMap[9][25][1] = -1;
LEDMap[10][25][0] = -1;
LEDMap[10][25][1] = -1;
LEDMap[11][25][0] = -1;
LEDMap[11][25][1] = -1;
LEDMap[12][25][0] = -1;
LEDMap[12][25][1] = -1;
LEDMap[0][26][0] = -1;
LEDMap[0][26][1] = -1;
LEDMap[1][26][0] = -1;
LEDMap[1][26][1] = -1;
LEDMap[2][26][0] = -1;
LEDMap[2][26][1] = -1;
LEDMap[3][26][0] = -1;
LEDMap[3][26][1] = -1;
LEDMap[4][26][0] = -1;
LEDMap[4][26][1] = -1;
LEDMap[5][26][0] = -1;
LEDMap[5][26][1] = -1;
LEDMap[6][26][0] = -1;
LEDMap[6][26][1] = -1;
LEDMap[7][26][0] = -1;
LEDMap[7][26][1] = -1;
LEDMap[8][26][0] = -1;
LEDMap[8][26][1] = -1;
LEDMap[9][26][0] = -1;
LEDMap[9][26][1] = -1;
LEDMap[10][26][0] = -1;
LEDMap[10][26][1] = -1;
LEDMap[11][26][0] = -1;
LEDMap[11][26][1] = -1;
LEDMap[12][26][0] = 193;
LEDMap[12][26][1] = -1;
LEDMap[0][27][0] = -1;
LEDMap[0][27][1] = -1;
LEDMap[1][27][0] = -1;
LEDMap[1][27][1] = -1;
LEDMap[2][27][0] = -1;
LEDMap[2][27][1] = -1;
LEDMap[3][27][0] = -1;
LEDMap[3][27][1] = -1;
LEDMap[4][27][0] = -1;
LEDMap[4][27][1] = -1;
LEDMap[5][27][0] = -1;
LEDMap[5][27][1] = -1;
LEDMap[6][27][0] = 87;
LEDMap[6][27][1] = -1;
LEDMap[7][27][0] = -1;
LEDMap[7][27][1] = -1;
LEDMap[8][27][0] = -1;
LEDMap[8][27][1] = -1;
LEDMap[9][27][0] = -1;
LEDMap[9][27][1] = -1;
LEDMap[10][27][0] = -1;
LEDMap[10][27][1] = -1;
LEDMap[11][27][0] = -1;
LEDMap[11][27][1] = -1;
LEDMap[12][27][0] = 192;
LEDMap[12][27][1] = -1;
LEDMap[0][28][0] = 50;
LEDMap[0][28][1] = -1;
LEDMap[1][28][0] = -1;
LEDMap[1][28][1] = -1;
LEDMap[2][28][0] = -1;
LEDMap[2][28][1] = -1;
LEDMap[3][28][0] = -1;
LEDMap[3][28][1] = -1;
LEDMap[4][28][0] = -1;
LEDMap[4][28][1] = -1;
LEDMap[5][28][0] = -1;
LEDMap[5][28][1] = -1;
LEDMap[6][28][0] = -1;
LEDMap[6][28][1] = -1;
LEDMap[7][28][0] = -1;
LEDMap[7][28][1] = -1;
LEDMap[8][28][0] = -1;
LEDMap[8][28][1] = -1;
LEDMap[9][28][0] = -1;
LEDMap[9][28][1] = -1;
LEDMap[10][28][0] = -1;
LEDMap[10][28][1] = -1;
LEDMap[11][28][0] = -1;
LEDMap[11][28][1] = -1;
LEDMap[12][28][0] = -1;
LEDMap[12][28][1] = -1;
LEDMap[0][29][0] = 49;
LEDMap[0][29][1] = -1;
LEDMap[1][29][0] = -1;
LEDMap[1][29][1] = -1;
LEDMap[2][29][0] = -1;
LEDMap[2][29][1] = -1;
LEDMap[3][29][0] = -1;
LEDMap[3][29][1] = -1;
LEDMap[4][29][0] = -1;
LEDMap[4][29][1] = -1;
LEDMap[5][29][0] = -1;
LEDMap[5][29][1] = -1;
LEDMap[6][29][0] = 88;
LEDMap[6][29][1] = -1;
LEDMap[7][29][0] = -1;
LEDMap[7][29][1] = -1;
LEDMap[8][29][0] = -1;
LEDMap[8][29][1] = -1;
LEDMap[9][29][0] = -1;
LEDMap[9][29][1] = -1;
LEDMap[10][29][0] = -1;
LEDMap[10][29][1] = -1;
LEDMap[11][29][0] = -1;
LEDMap[11][29][1] = -1;
LEDMap[12][29][0] = 191;
LEDMap[12][29][1] = -1;
LEDMap[0][30][0] = 48;
LEDMap[0][30][1] = -1;
LEDMap[1][30][0] = -1;
LEDMap[1][30][1] = -1;
LEDMap[2][30][0] = -1;
LEDMap[2][30][1] = -1;
LEDMap[3][30][0] = -1;
LEDMap[3][30][1] = -1;
LEDMap[4][30][0] = -1;
LEDMap[4][30][1] = -1;
LEDMap[5][30][0] = -1;
LEDMap[5][30][1] = -1;
LEDMap[6][30][0] = 89;
LEDMap[6][30][1] = -1;
LEDMap[7][30][0] = -1;
LEDMap[7][30][1] = -1;
LEDMap[8][30][0] = -1;
LEDMap[8][30][1] = -1;
LEDMap[9][30][0] = -1;
LEDMap[9][30][1] = -1;
LEDMap[10][30][0] = -1;
LEDMap[10][30][1] = -1;
LEDMap[11][30][0] = -1;
LEDMap[11][30][1] = -1;
LEDMap[12][30][0] = -1;
LEDMap[12][30][1] = -1;
LEDMap[0][31][0] = -1;
LEDMap[0][31][1] = -1;
LEDMap[1][31][0] = -1;
LEDMap[1][31][1] = -1;
LEDMap[2][31][0] = -1;
LEDMap[2][31][1] = -1;
LEDMap[3][31][0] = -1;
LEDMap[3][31][1] = -1;
LEDMap[4][31][0] = -1;
LEDMap[4][31][1] = -1;
LEDMap[5][31][0] = -1;
LEDMap[5][31][1] = -1;
LEDMap[6][31][0] = -1;
LEDMap[6][31][1] = -1;
LEDMap[7][31][0] = -1;
LEDMap[7][31][1] = -1;
LEDMap[8][31][0] = -1;
LEDMap[8][31][1] = -1;
LEDMap[9][31][0] = -1;
LEDMap[9][31][1] = -1;
LEDMap[10][31][0] = -1;
LEDMap[10][31][1] = -1;
LEDMap[11][31][0] = -1;
LEDMap[11][31][1] = -1;
LEDMap[12][31][0] = 190;
LEDMap[12][31][1] = -1;
LEDMap[0][32][0] = 47;
LEDMap[0][32][1] = -1;
LEDMap[1][32][0] = -1;
LEDMap[1][32][1] = -1;
LEDMap[2][32][0] = -1;
LEDMap[2][32][1] = -1;
LEDMap[3][32][0] = -1;
LEDMap[3][32][1] = -1;
LEDMap[4][32][0] = -1;
LEDMap[4][32][1] = -1;
LEDMap[5][32][0] = -1;
LEDMap[5][32][1] = -1;
LEDMap[6][32][0] = 90;
LEDMap[6][32][1] = -1;
LEDMap[7][32][0] = -1;
LEDMap[7][32][1] = -1;
LEDMap[8][32][0] = -1;
LEDMap[8][32][1] = -1;
LEDMap[9][32][0] = -1;
LEDMap[9][32][1] = -1;
LEDMap[10][32][0] = -1;
LEDMap[10][32][1] = -1;
LEDMap[11][32][0] = -1;
LEDMap[11][32][1] = -1;
LEDMap[12][32][0] = 189;
LEDMap[12][32][1] = -1;
LEDMap[0][33][0] = -1;
LEDMap[0][33][1] = -1;
LEDMap[1][33][0] = -1;
LEDMap[1][33][1] = -1;
LEDMap[2][33][0] = -1;
LEDMap[2][33][1] = -1;
LEDMap[3][33][0] = -1;
LEDMap[3][33][1] = -1;
LEDMap[4][33][0] = -1;
LEDMap[4][33][1] = -1;
LEDMap[5][33][0] = -1;
LEDMap[5][33][1] = -1;
LEDMap[6][33][0] = -1;
LEDMap[6][33][1] = -1;
LEDMap[7][33][0] = -1;
LEDMap[7][33][1] = -1;
LEDMap[8][33][0] = -1;
LEDMap[8][33][1] = -1;
LEDMap[9][33][0] = -1;
LEDMap[9][33][1] = -1;
LEDMap[10][33][0] = -1;
LEDMap[10][33][1] = -1;
LEDMap[11][33][0] = -1;
LEDMap[11][33][1] = -1;
LEDMap[12][33][0] = -1;
LEDMap[12][33][1] = -1;
LEDMap[0][34][0] = 46;
LEDMap[0][34][1] = -1;
LEDMap[1][34][0] = -1;
LEDMap[1][34][1] = -1;
LEDMap[2][34][0] = -1;
LEDMap[2][34][1] = -1;
LEDMap[3][34][0] = -1;
LEDMap[3][34][1] = -1;
LEDMap[4][34][0] = -1;
LEDMap[4][34][1] = -1;
LEDMap[5][34][0] = -1;
LEDMap[5][34][1] = -1;
LEDMap[6][34][0] = 91;
LEDMap[6][34][1] = -1;
LEDMap[7][34][0] = -1;
LEDMap[7][34][1] = -1;
LEDMap[8][34][0] = -1;
LEDMap[8][34][1] = -1;
LEDMap[9][34][0] = -1;
LEDMap[9][34][1] = -1;
LEDMap[10][34][0] = -1;
LEDMap[10][34][1] = -1;
LEDMap[11][34][0] = -1;
LEDMap[11][34][1] = -1;
LEDMap[12][34][0] = 188;
LEDMap[12][34][1] = -1;
LEDMap[0][35][0] = 45;
LEDMap[0][35][1] = -1;
LEDMap[1][35][0] = -1;
LEDMap[1][35][1] = -1;
LEDMap[2][35][0] = -1;
LEDMap[2][35][1] = -1;
LEDMap[3][35][0] = -1;
LEDMap[3][35][1] = -1;
LEDMap[4][35][0] = -1;
LEDMap[4][35][1] = -1;
LEDMap[5][35][0] = -1;
LEDMap[5][35][1] = -1;
LEDMap[6][35][0] = 92;
LEDMap[6][35][1] = -1;
LEDMap[7][35][0] = -1;
LEDMap[7][35][1] = -1;
LEDMap[8][35][0] = -1;
LEDMap[8][35][1] = -1;
LEDMap[9][35][0] = -1;
LEDMap[9][35][1] = -1;
LEDMap[10][35][0] = -1;
LEDMap[10][35][1] = -1;
LEDMap[11][35][0] = -1;
LEDMap[11][35][1] = -1;
LEDMap[12][35][0] = -1;
LEDMap[12][35][1] = -1;
LEDMap[0][36][0] = -1;
LEDMap[0][36][1] = -1;
LEDMap[1][36][0] = -1;
LEDMap[1][36][1] = -1;
LEDMap[2][36][0] = -1;
LEDMap[2][36][1] = -1;
LEDMap[3][36][0] = -1;
LEDMap[3][36][1] = -1;
LEDMap[4][36][0] = -1;
LEDMap[4][36][1] = -1;
LEDMap[5][36][0] = -1;
LEDMap[5][36][1] = -1;
LEDMap[6][36][0] = -1;
LEDMap[6][36][1] = -1;
LEDMap[7][36][0] = -1;
LEDMap[7][36][1] = -1;
LEDMap[8][36][0] = -1;
LEDMap[8][36][1] = -1;
LEDMap[9][36][0] = -1;
LEDMap[9][36][1] = -1;
LEDMap[10][36][0] = -1;
LEDMap[10][36][1] = -1;
LEDMap[11][36][0] = -1;
LEDMap[11][36][1] = -1;
LEDMap[12][36][0] = 187;
LEDMap[12][36][1] = -1;
LEDMap[0][37][0] = 44;
LEDMap[0][37][1] = -1;
LEDMap[1][37][0] = -1;
LEDMap[1][37][1] = -1;
LEDMap[2][37][0] = -1;
LEDMap[2][37][1] = -1;
LEDMap[3][37][0] = -1;
LEDMap[3][37][1] = -1;
LEDMap[4][37][0] = -1;
LEDMap[4][37][1] = -1;
LEDMap[5][37][0] = -1;
LEDMap[5][37][1] = -1;
LEDMap[6][37][0] = 93;
LEDMap[6][37][1] = -1;
LEDMap[7][37][0] = -1;
LEDMap[7][37][1] = -1;
LEDMap[8][37][0] = -1;
LEDMap[8][37][1] = -1;
LEDMap[9][37][0] = -1;
LEDMap[9][37][1] = -1;
LEDMap[10][37][0] = -1;
LEDMap[10][37][1] = -1;
LEDMap[11][37][0] = -1;
LEDMap[11][37][1] = -1;
LEDMap[12][37][0] = 186;
LEDMap[12][37][1] = -1;
LEDMap[0][38][0] = -1;
LEDMap[0][38][1] = -1;
LEDMap[1][38][0] = -1;
LEDMap[1][38][1] = -1;
LEDMap[2][38][0] = -1;
LEDMap[2][38][1] = -1;
LEDMap[3][38][0] = -1;
LEDMap[3][38][1] = -1;
LEDMap[4][38][0] = -1;
LEDMap[4][38][1] = -1;
LEDMap[5][38][0] = -1;
LEDMap[5][38][1] = -1;
LEDMap[6][38][0] = 94;
LEDMap[6][38][1] = -1;
LEDMap[7][38][0] = -1;
LEDMap[7][38][1] = -1;
LEDMap[8][38][0] = -1;
LEDMap[8][38][1] = -1;
LEDMap[9][38][0] = -1;
LEDMap[9][38][1] = -1;
LEDMap[10][38][0] = -1;
LEDMap[10][38][1] = -1;
LEDMap[11][38][0] = -1;
LEDMap[11][38][1] = -1;
LEDMap[12][38][0] = -1;
LEDMap[12][38][1] = -1;
LEDMap[0][39][0] = 43;
LEDMap[0][39][1] = -1;
LEDMap[1][39][0] = -1;
LEDMap[1][39][1] = -1;
LEDMap[2][39][0] = -1;
LEDMap[2][39][1] = -1;
LEDMap[3][39][0] = -1;
LEDMap[3][39][1] = -1;
LEDMap[4][39][0] = -1;
LEDMap[4][39][1] = -1;
LEDMap[5][39][0] = -1;
LEDMap[5][39][1] = -1;
LEDMap[6][39][0] = -1;
LEDMap[6][39][1] = -1;
LEDMap[7][39][0] = -1;
LEDMap[7][39][1] = -1;
LEDMap[8][39][0] = -1;
LEDMap[8][39][1] = -1;
LEDMap[9][39][0] = -1;
LEDMap[9][39][1] = -1;
LEDMap[10][39][0] = -1;
LEDMap[10][39][1] = -1;
LEDMap[11][39][0] = -1;
LEDMap[11][39][1] = -1;
LEDMap[12][39][0] = 185;
LEDMap[12][39][1] = -1;
LEDMap[0][40][0] = 42;
LEDMap[0][40][1] = -1;
LEDMap[1][40][0] = -1;
LEDMap[1][40][1] = -1;
LEDMap[2][40][0] = -1;
LEDMap[2][40][1] = -1;
LEDMap[3][40][0] = -1;
LEDMap[3][40][1] = -1;
LEDMap[4][40][0] = -1;
LEDMap[4][40][1] = -1;
LEDMap[5][40][0] = -1;
LEDMap[5][40][1] = -1;
LEDMap[6][40][0] = 95;
LEDMap[6][40][1] = -1;
LEDMap[7][40][0] = -1;
LEDMap[7][40][1] = -1;
LEDMap[8][40][0] = -1;
LEDMap[8][40][1] = -1;
LEDMap[9][40][0] = -1;
LEDMap[9][40][1] = -1;
LEDMap[10][40][0] = -1;
LEDMap[10][40][1] = -1;
LEDMap[11][40][0] = -1;
LEDMap[11][40][1] = -1;
LEDMap[12][40][0] = -1;
LEDMap[12][40][1] = -1;
LEDMap[0][41][0] = -1;
LEDMap[0][41][1] = -1;
LEDMap[1][41][0] = -1;
LEDMap[1][41][1] = -1;
LEDMap[2][41][0] = -1;
LEDMap[2][41][1] = -1;
LEDMap[3][41][0] = -1;
LEDMap[3][41][1] = -1;
LEDMap[4][41][0] = -1;
LEDMap[4][41][1] = -1;
LEDMap[5][41][0] = -1;
LEDMap[5][41][1] = -1;
LEDMap[6][41][0] = 96;
LEDMap[6][41][1] = -1;
LEDMap[7][41][0] = -1;
LEDMap[7][41][1] = -1;
LEDMap[8][41][0] = -1;
LEDMap[8][41][1] = -1;
LEDMap[9][41][0] = -1;
LEDMap[9][41][1] = -1;
LEDMap[10][41][0] = -1;
LEDMap[10][41][1] = -1;
LEDMap[11][41][0] = -1;
LEDMap[11][41][1] = -1;
LEDMap[12][41][0] = 184;
LEDMap[12][41][1] = -1;
LEDMap[0][42][0] = 41;
LEDMap[0][42][1] = -1;
LEDMap[1][42][0] = -1;
LEDMap[1][42][1] = -1;
LEDMap[2][42][0] = -1;
LEDMap[2][42][1] = -1;
LEDMap[3][42][0] = -1;
LEDMap[3][42][1] = -1;
LEDMap[4][42][0] = -1;
LEDMap[4][42][1] = -1;
LEDMap[5][42][0] = -1;
LEDMap[5][42][1] = -1;
LEDMap[6][42][0] = -1;
LEDMap[6][42][1] = -1;
LEDMap[7][42][0] = -1;
LEDMap[7][42][1] = -1;
LEDMap[8][42][0] = -1;
LEDMap[8][42][1] = -1;
LEDMap[9][42][0] = -1;
LEDMap[9][42][1] = -1;
LEDMap[10][42][0] = -1;
LEDMap[10][42][1] = -1;
LEDMap[11][42][0] = -1;
LEDMap[11][42][1] = -1;
LEDMap[12][42][0] = 183;
LEDMap[12][42][1] = -1;
LEDMap[0][43][0] = 40;
LEDMap[0][43][1] = -1;
LEDMap[1][43][0] = -1;
LEDMap[1][43][1] = -1;
LEDMap[2][43][0] = -1;
LEDMap[2][43][1] = -1;
LEDMap[3][43][0] = -1;
LEDMap[3][43][1] = -1;
LEDMap[4][43][0] = -1;
LEDMap[4][43][1] = -1;
LEDMap[5][43][0] = -1;
LEDMap[5][43][1] = -1;
LEDMap[6][43][0] = 97;
LEDMap[6][43][1] = -1;
LEDMap[7][43][0] = -1;
LEDMap[7][43][1] = -1;
LEDMap[8][43][0] = -1;
LEDMap[8][43][1] = -1;
LEDMap[9][43][0] = -1;
LEDMap[9][43][1] = -1;
LEDMap[10][43][0] = -1;
LEDMap[10][43][1] = -1;
LEDMap[11][43][0] = -1;
LEDMap[11][43][1] = -1;
LEDMap[12][43][0] = -1;
LEDMap[12][43][1] = -1;
LEDMap[0][44][0] = -1;
LEDMap[0][44][1] = -1;
LEDMap[1][44][0] = -1;
LEDMap[1][44][1] = -1;
LEDMap[2][44][0] = -1;
LEDMap[2][44][1] = -1;
LEDMap[3][44][0] = -1;
LEDMap[3][44][1] = -1;
LEDMap[4][44][0] = -1;
LEDMap[4][44][1] = -1;
LEDMap[5][44][0] = -1;
LEDMap[5][44][1] = -1;
LEDMap[6][44][0] = -1;
LEDMap[6][44][1] = -1;
LEDMap[7][44][0] = -1;
LEDMap[7][44][1] = -1;
LEDMap[8][44][0] = -1;
LEDMap[8][44][1] = -1;
LEDMap[9][44][0] = -1;
LEDMap[9][44][1] = -1;
LEDMap[10][44][0] = -1;
LEDMap[10][44][1] = -1;
LEDMap[11][44][0] = -1;
LEDMap[11][44][1] = -1;
LEDMap[12][44][0] = 182;
LEDMap[12][44][1] = -1;
LEDMap[0][45][0] = 39;
LEDMap[0][45][1] = -1;
LEDMap[1][45][0] = -1;
LEDMap[1][45][1] = -1;
LEDMap[2][45][0] = -1;
LEDMap[2][45][1] = -1;
LEDMap[3][45][0] = -1;
LEDMap[3][45][1] = -1;
LEDMap[4][45][0] = -1;
LEDMap[4][45][1] = -1;
LEDMap[5][45][0] = -1;
LEDMap[5][45][1] = -1;
LEDMap[6][45][0] = 98;
LEDMap[6][45][1] = -1;
LEDMap[7][45][0] = -1;
LEDMap[7][45][1] = -1;
LEDMap[8][45][0] = -1;
LEDMap[8][45][1] = -1;
LEDMap[9][45][0] = -1;
LEDMap[9][45][1] = -1;
LEDMap[10][45][0] = -1;
LEDMap[10][45][1] = -1;
LEDMap[11][45][0] = -1;
LEDMap[11][45][1] = -1;
LEDMap[12][45][0] = 181;
LEDMap[12][45][1] = -1;
LEDMap[0][46][0] = -1;
LEDMap[0][46][1] = -1;
LEDMap[1][46][0] = -1;
LEDMap[1][46][1] = -1;
LEDMap[2][46][0] = -1;
LEDMap[2][46][1] = -1;
LEDMap[3][46][0] = -1;
LEDMap[3][46][1] = -1;
LEDMap[4][46][0] = -1;
LEDMap[4][46][1] = -1;
LEDMap[5][46][0] = -1;
LEDMap[5][46][1] = -1;
LEDMap[6][46][0] = 99;
LEDMap[6][46][1] = -1;
LEDMap[7][46][0] = -1;
LEDMap[7][46][1] = -1;
LEDMap[8][46][0] = -1;
LEDMap[8][46][1] = -1;
LEDMap[9][46][0] = -1;
LEDMap[9][46][1] = -1;
LEDMap[10][46][0] = -1;
LEDMap[10][46][1] = -1;
LEDMap[11][46][0] = -1;
LEDMap[11][46][1] = -1;
LEDMap[12][46][0] = -1;
LEDMap[12][46][1] = -1;
LEDMap[0][47][0] = 38;
LEDMap[0][47][1] = -1;
LEDMap[1][47][0] = -1;
LEDMap[1][47][1] = -1;
LEDMap[2][47][0] = -1;
LEDMap[2][47][1] = -1;
LEDMap[3][47][0] = -1;
LEDMap[3][47][1] = -1;
LEDMap[4][47][0] = -1;
LEDMap[4][47][1] = -1;
LEDMap[5][47][0] = -1;
LEDMap[5][47][1] = -1;
LEDMap[6][47][0] = 100;
LEDMap[6][47][1] = -1;
LEDMap[7][47][0] = -1;
LEDMap[7][47][1] = -1;
LEDMap[8][47][0] = -1;
LEDMap[8][47][1] = -1;
LEDMap[9][47][0] = -1;
LEDMap[9][47][1] = -1;
LEDMap[10][47][0] = -1;
LEDMap[10][47][1] = -1;
LEDMap[11][47][0] = -1;
LEDMap[11][47][1] = -1;
LEDMap[12][47][0] = 180;
LEDMap[12][47][1] = -1;
LEDMap[0][48][0] = 37;
LEDMap[0][48][1] = -1;
LEDMap[1][48][0] = -1;
LEDMap[1][48][1] = -1;
LEDMap[2][48][0] = -1;
LEDMap[2][48][1] = -1;
LEDMap[3][48][0] = -1;
LEDMap[3][48][1] = -1;
LEDMap[4][48][0] = -1;
LEDMap[4][48][1] = -1;
LEDMap[5][48][0] = -1;
LEDMap[5][48][1] = -1;
LEDMap[6][48][0] = -1;
LEDMap[6][48][1] = -1;
LEDMap[7][48][0] = -1;
LEDMap[7][48][1] = -1;
LEDMap[8][48][0] = -1;
LEDMap[8][48][1] = -1;
LEDMap[9][48][0] = -1;
LEDMap[9][48][1] = -1;
LEDMap[10][48][0] = -1;
LEDMap[10][48][1] = -1;
LEDMap[11][48][0] = -1;
LEDMap[11][48][1] = -1;
LEDMap[12][48][0] = -1;
LEDMap[12][48][1] = -1;
LEDMap[0][49][0] = -1;
LEDMap[0][49][1] = -1;
LEDMap[1][49][0] = -1;
LEDMap[1][49][1] = -1;
LEDMap[2][49][0] = -1;
LEDMap[2][49][1] = -1;
LEDMap[3][49][0] = -1;
LEDMap[3][49][1] = -1;
LEDMap[4][49][0] = -1;
LEDMap[4][49][1] = -1;
LEDMap[5][49][0] = -1;
LEDMap[5][49][1] = -1;
LEDMap[6][49][0] = -1;
LEDMap[6][49][1] = -1;
LEDMap[7][49][0] = -1;
LEDMap[7][49][1] = -1;
LEDMap[8][49][0] = -1;
LEDMap[8][49][1] = -1;
LEDMap[9][49][0] = -1;
LEDMap[9][49][1] = -1;
LEDMap[10][49][0] = -1;
LEDMap[10][49][1] = -1;
LEDMap[11][49][0] = -1;
LEDMap[11][49][1] = -1;
LEDMap[12][49][0] = 179;
LEDMap[12][49][1] = -1;
LEDMap[0][50][0] = 36;
LEDMap[0][50][1] = -1;
LEDMap[1][50][0] = -1;
LEDMap[1][50][1] = -1;
LEDMap[2][50][0] = -1;
LEDMap[2][50][1] = -1;
LEDMap[3][50][0] = -1;
LEDMap[3][50][1] = -1;
LEDMap[4][50][0] = -1;
LEDMap[4][50][1] = -1;
LEDMap[5][50][0] = -1;
LEDMap[5][50][1] = -1;
LEDMap[6][50][0] = 101;
LEDMap[6][50][1] = -1;
LEDMap[7][50][0] = -1;
LEDMap[7][50][1] = -1;
LEDMap[8][50][0] = -1;
LEDMap[8][50][1] = -1;
LEDMap[9][50][0] = -1;
LEDMap[9][50][1] = -1;
LEDMap[10][50][0] = -1;
LEDMap[10][50][1] = -1;
LEDMap[11][50][0] = -1;
LEDMap[11][50][1] = -1;
LEDMap[12][50][0] = 178;
LEDMap[12][50][1] = -1;
LEDMap[0][51][0] = 35;
LEDMap[0][51][1] = -1;
LEDMap[1][51][0] = -1;
LEDMap[1][51][1] = -1;
LEDMap[2][51][0] = -1;
LEDMap[2][51][1] = -1;
LEDMap[3][51][0] = -1;
LEDMap[3][51][1] = -1;
LEDMap[4][51][0] = -1;
LEDMap[4][51][1] = -1;
LEDMap[5][51][0] = -1;
LEDMap[5][51][1] = -1;
LEDMap[6][51][0] = 102;
LEDMap[6][51][1] = -1;
LEDMap[7][51][0] = -1;
LEDMap[7][51][1] = -1;
LEDMap[8][51][0] = -1;
LEDMap[8][51][1] = -1;
LEDMap[9][51][0] = -1;
LEDMap[9][51][1] = -1;
LEDMap[10][51][0] = -1;
LEDMap[10][51][1] = -1;
LEDMap[11][51][0] = -1;
LEDMap[11][51][1] = -1;
LEDMap[12][51][0] = -1;
LEDMap[12][51][1] = -1;
LEDMap[0][52][0] = -1;
LEDMap[0][52][1] = -1;
LEDMap[1][52][0] = -1;
LEDMap[1][52][1] = -1;
LEDMap[2][52][0] = -1;
LEDMap[2][52][1] = -1;
LEDMap[3][52][0] = -1;
LEDMap[3][52][1] = -1;
LEDMap[4][52][0] = -1;
LEDMap[4][52][1] = -1;
LEDMap[5][52][0] = -1;
LEDMap[5][52][1] = -1;
LEDMap[6][52][0] = 103;
LEDMap[6][52][1] = -1;
LEDMap[7][52][0] = -1;
LEDMap[7][52][1] = -1;
LEDMap[8][52][0] = -1;
LEDMap[8][52][1] = -1;
LEDMap[9][52][0] = -1;
LEDMap[9][52][1] = -1;
LEDMap[10][52][0] = -1;
LEDMap[10][52][1] = -1;
LEDMap[11][52][0] = -1;
LEDMap[11][52][1] = -1;
LEDMap[12][52][0] = 177;
LEDMap[12][52][1] = -1;
LEDMap[0][53][0] = 34;
LEDMap[0][53][1] = -1;
LEDMap[1][53][0] = -1;
LEDMap[1][53][1] = -1;
LEDMap[2][53][0] = -1;
LEDMap[2][53][1] = -1;
LEDMap[3][53][0] = -1;
LEDMap[3][53][1] = -1;
LEDMap[4][53][0] = -1;
LEDMap[4][53][1] = -1;
LEDMap[5][53][0] = -1;
LEDMap[5][53][1] = -1;
LEDMap[6][53][0] = -1;
LEDMap[6][53][1] = -1;
LEDMap[7][53][0] = -1;
LEDMap[7][53][1] = -1;
LEDMap[8][53][0] = -1;
LEDMap[8][53][1] = -1;
LEDMap[9][53][0] = -1;
LEDMap[9][53][1] = -1;
LEDMap[10][53][0] = -1;
LEDMap[10][53][1] = -1;
LEDMap[11][53][0] = -1;
LEDMap[11][53][1] = -1;
LEDMap[12][53][0] = 176;
LEDMap[12][53][1] = -1;
LEDMap[0][54][0] = -1;
LEDMap[0][54][1] = -1;
LEDMap[1][54][0] = -1;
LEDMap[1][54][1] = -1;
LEDMap[2][54][0] = -1;
LEDMap[2][54][1] = -1;
LEDMap[3][54][0] = -1;
LEDMap[3][54][1] = -1;
LEDMap[4][54][0] = -1;
LEDMap[4][54][1] = -1;
LEDMap[5][54][0] = -1;
LEDMap[5][54][1] = -1;
LEDMap[6][54][0] = 104;
LEDMap[6][54][1] = -1;
LEDMap[7][54][0] = -1;
LEDMap[7][54][1] = -1;
LEDMap[8][54][0] = -1;
LEDMap[8][54][1] = -1;
LEDMap[9][54][0] = -1;
LEDMap[9][54][1] = -1;
LEDMap[10][54][0] = -1;
LEDMap[10][54][1] = -1;
LEDMap[11][54][0] = -1;
LEDMap[11][54][1] = -1;
LEDMap[12][54][0] = -1;
LEDMap[12][54][1] = -1;
LEDMap[0][55][0] = 33;
LEDMap[0][55][1] = -1;
LEDMap[1][55][0] = -1;
LEDMap[1][55][1] = -1;
LEDMap[2][55][0] = -1;
LEDMap[2][55][1] = -1;
LEDMap[3][55][0] = -1;
LEDMap[3][55][1] = -1;
LEDMap[4][55][0] = -1;
LEDMap[4][55][1] = -1;
LEDMap[5][55][0] = -1;
LEDMap[5][55][1] = -1;
LEDMap[6][55][0] = 105;
LEDMap[6][55][1] = -1;
LEDMap[7][55][0] = -1;
LEDMap[7][55][1] = -1;
LEDMap[8][55][0] = -1;
LEDMap[8][55][1] = -1;
LEDMap[9][55][0] = -1;
LEDMap[9][55][1] = -1;
LEDMap[10][55][0] = -1;
LEDMap[10][55][1] = -1;
LEDMap[11][55][0] = -1;
LEDMap[11][55][1] = -1;
LEDMap[12][55][0] = 175;
LEDMap[12][55][1] = -1;
LEDMap[0][56][0] = 32;
LEDMap[0][56][1] = -1;
LEDMap[1][56][0] = -1;
LEDMap[1][56][1] = -1;
LEDMap[2][56][0] = -1;
LEDMap[2][56][1] = -1;
LEDMap[3][56][0] = -1;
LEDMap[3][56][1] = -1;
LEDMap[4][56][0] = -1;
LEDMap[4][56][1] = -1;
LEDMap[5][56][0] = -1;
LEDMap[5][56][1] = -1;
LEDMap[6][56][0] = -1;
LEDMap[6][56][1] = -1;
LEDMap[7][56][0] = -1;
LEDMap[7][56][1] = -1;
LEDMap[8][56][0] = -1;
LEDMap[8][56][1] = -1;
LEDMap[9][56][0] = -1;
LEDMap[9][56][1] = -1;
LEDMap[10][56][0] = -1;
LEDMap[10][56][1] = -1;
LEDMap[11][56][0] = -1;
LEDMap[11][56][1] = -1;
LEDMap[12][56][0] = -1;
LEDMap[12][56][1] = -1;
LEDMap[0][57][0] = -1;
LEDMap[0][57][1] = -1;
LEDMap[1][57][0] = -1;
LEDMap[1][57][1] = -1;
LEDMap[2][57][0] = -1;
LEDMap[2][57][1] = -1;
LEDMap[3][57][0] = -1;
LEDMap[3][57][1] = -1;
LEDMap[4][57][0] = -1;
LEDMap[4][57][1] = -1;
LEDMap[5][57][0] = -1;
LEDMap[5][57][1] = -1;
LEDMap[6][57][0] = 106;
LEDMap[6][57][1] = -1;
LEDMap[7][57][0] = -1;
LEDMap[7][57][1] = -1;
LEDMap[8][57][0] = -1;
LEDMap[8][57][1] = -1;
LEDMap[9][57][0] = -1;
LEDMap[9][57][1] = -1;
LEDMap[10][57][0] = -1;
LEDMap[10][57][1] = -1;
LEDMap[11][57][0] = -1;
LEDMap[11][57][1] = -1;
LEDMap[12][57][0] = 174;
LEDMap[12][57][1] = -1;
LEDMap[0][58][0] = 31;
LEDMap[0][58][1] = -1;
LEDMap[1][58][0] = -1;
LEDMap[1][58][1] = -1;
LEDMap[2][58][0] = -1;
LEDMap[2][58][1] = -1;
LEDMap[3][58][0] = -1;
LEDMap[3][58][1] = -1;
LEDMap[4][58][0] = -1;
LEDMap[4][58][1] = -1;
LEDMap[5][58][0] = -1;
LEDMap[5][58][1] = -1;
LEDMap[6][58][0] = -1;
LEDMap[6][58][1] = -1;
LEDMap[7][58][0] = -1;
LEDMap[7][58][1] = -1;
LEDMap[8][58][0] = -1;
LEDMap[8][58][1] = -1;
LEDMap[9][58][0] = -1;
LEDMap[9][58][1] = -1;
LEDMap[10][58][0] = -1;
LEDMap[10][58][1] = -1;
LEDMap[11][58][0] = -1;
LEDMap[11][58][1] = -1;
LEDMap[12][58][0] = 173;
LEDMap[12][58][1] = -1;
LEDMap[0][59][0] = 30;
LEDMap[0][59][1] = -1;
LEDMap[1][59][0] = -1;
LEDMap[1][59][1] = -1;
LEDMap[2][59][0] = -1;
LEDMap[2][59][1] = -1;
LEDMap[3][59][0] = -1;
LEDMap[3][59][1] = -1;
LEDMap[4][59][0] = -1;
LEDMap[4][59][1] = -1;
LEDMap[5][59][0] = -1;
LEDMap[5][59][1] = -1;
LEDMap[6][59][0] = 107;
LEDMap[6][59][1] = -1;
LEDMap[7][59][0] = -1;
LEDMap[7][59][1] = -1;
LEDMap[8][59][0] = -1;
LEDMap[8][59][1] = -1;
LEDMap[9][59][0] = -1;
LEDMap[9][59][1] = -1;
LEDMap[10][59][0] = -1;
LEDMap[10][59][1] = -1;
LEDMap[11][59][0] = -1;
LEDMap[11][59][1] = -1;
LEDMap[12][59][0] = -1;
LEDMap[12][59][1] = -1;
LEDMap[0][60][0] = -1;
LEDMap[0][60][1] = -1;
LEDMap[1][60][0] = -1;
LEDMap[1][60][1] = -1;
LEDMap[2][60][0] = -1;
LEDMap[2][60][1] = -1;
LEDMap[3][60][0] = -1;
LEDMap[3][60][1] = -1;
LEDMap[4][60][0] = -1;
LEDMap[4][60][1] = -1;
LEDMap[5][60][0] = -1;
LEDMap[5][60][1] = -1;
LEDMap[6][60][0] = 108;
LEDMap[6][60][1] = -1;
LEDMap[7][60][0] = -1;
LEDMap[7][60][1] = -1;
LEDMap[8][60][0] = -1;
LEDMap[8][60][1] = -1;
LEDMap[9][60][0] = -1;
LEDMap[9][60][1] = -1;
LEDMap[10][60][0] = -1;
LEDMap[10][60][1] = -1;
LEDMap[11][60][0] = -1;
LEDMap[11][60][1] = -1;
LEDMap[12][60][0] = 172;
LEDMap[12][60][1] = -1;
LEDMap[0][61][0] = 29;
LEDMap[0][61][1] = -1;
LEDMap[1][61][0] = -1;
LEDMap[1][61][1] = -1;
LEDMap[2][61][0] = -1;
LEDMap[2][61][1] = -1;
LEDMap[3][61][0] = -1;
LEDMap[3][61][1] = -1;
LEDMap[4][61][0] = -1;
LEDMap[4][61][1] = -1;
LEDMap[5][61][0] = -1;
LEDMap[5][61][1] = -1;
LEDMap[6][61][0] = -1;
LEDMap[6][61][1] = -1;
LEDMap[7][61][0] = -1;
LEDMap[7][61][1] = -1;
LEDMap[8][61][0] = -1;
LEDMap[8][61][1] = -1;
LEDMap[9][61][0] = -1;
LEDMap[9][61][1] = -1;
LEDMap[10][61][0] = -1;
LEDMap[10][61][1] = -1;
LEDMap[11][61][0] = -1;
LEDMap[11][61][1] = -1;
LEDMap[12][61][0] = 171;
LEDMap[12][61][1] = -1;
LEDMap[0][62][0] = -1;
LEDMap[0][62][1] = -1;
LEDMap[1][62][0] = -1;
LEDMap[1][62][1] = -1;
LEDMap[2][62][0] = -1;
LEDMap[2][62][1] = -1;
LEDMap[3][62][0] = -1;
LEDMap[3][62][1] = -1;
LEDMap[4][62][0] = -1;
LEDMap[4][62][1] = -1;
LEDMap[5][62][0] = -1;
LEDMap[5][62][1] = -1;
LEDMap[6][62][0] = 109;
LEDMap[6][62][1] = -1;
LEDMap[7][62][0] = -1;
LEDMap[7][62][1] = -1;
LEDMap[8][62][0] = -1;
LEDMap[8][62][1] = -1;
LEDMap[9][62][0] = -1;
LEDMap[9][62][1] = -1;
LEDMap[10][62][0] = -1;
LEDMap[10][62][1] = -1;
LEDMap[11][62][0] = -1;
LEDMap[11][62][1] = -1;
LEDMap[12][62][0] = -1;
LEDMap[12][62][1] = -1;
LEDMap[0][63][0] = 28;
LEDMap[0][63][1] = -1;
LEDMap[1][63][0] = -1;
LEDMap[1][63][1] = -1;
LEDMap[2][63][0] = -1;
LEDMap[2][63][1] = -1;
LEDMap[3][63][0] = -1;
LEDMap[3][63][1] = -1;
LEDMap[4][63][0] = -1;
LEDMap[4][63][1] = -1;
LEDMap[5][63][0] = -1;
LEDMap[5][63][1] = -1;
LEDMap[6][63][0] = 110;
LEDMap[6][63][1] = -1;
LEDMap[7][63][0] = -1;
LEDMap[7][63][1] = -1;
LEDMap[8][63][0] = -1;
LEDMap[8][63][1] = -1;
LEDMap[9][63][0] = -1;
LEDMap[9][63][1] = -1;
LEDMap[10][63][0] = -1;
LEDMap[10][63][1] = -1;
LEDMap[11][63][0] = -1;
LEDMap[11][63][1] = -1;
LEDMap[12][63][0] = 170;
LEDMap[12][63][1] = -1;
LEDMap[0][64][0] = -1;
LEDMap[0][64][1] = -1;
LEDMap[1][64][0] = -1;
LEDMap[1][64][1] = -1;
LEDMap[2][64][0] = -1;
LEDMap[2][64][1] = -1;
LEDMap[3][64][0] = -1;
LEDMap[3][64][1] = -1;
LEDMap[4][64][0] = -1;
LEDMap[4][64][1] = -1;
LEDMap[5][64][0] = -1;
LEDMap[5][64][1] = -1;
LEDMap[6][64][0] = -1;
LEDMap[6][64][1] = -1;
LEDMap[7][64][0] = -1;
LEDMap[7][64][1] = -1;
LEDMap[8][64][0] = -1;
LEDMap[8][64][1] = -1;
LEDMap[9][64][0] = -1;
LEDMap[9][64][1] = -1;
LEDMap[10][64][0] = -1;
LEDMap[10][64][1] = -1;
LEDMap[11][64][0] = -1;
LEDMap[11][64][1] = -1;
LEDMap[12][64][0] = 169;
LEDMap[12][64][1] = -1;
LEDMap[0][65][0] = 27;
LEDMap[0][65][1] = -1;
LEDMap[1][65][0] = -1;
LEDMap[1][65][1] = -1;
LEDMap[2][65][0] = -1;
LEDMap[2][65][1] = -1;
LEDMap[3][65][0] = -1;
LEDMap[3][65][1] = -1;
LEDMap[4][65][0] = -1;
LEDMap[4][65][1] = -1;
LEDMap[5][65][0] = -1;
LEDMap[5][65][1] = -1;
LEDMap[6][65][0] = 111;
LEDMap[6][65][1] = -1;
LEDMap[7][65][0] = -1;
LEDMap[7][65][1] = -1;
LEDMap[8][65][0] = -1;
LEDMap[8][65][1] = -1;
LEDMap[9][65][0] = -1;
LEDMap[9][65][1] = -1;
LEDMap[10][65][0] = -1;
LEDMap[10][65][1] = -1;
LEDMap[11][65][0] = -1;
LEDMap[11][65][1] = -1;
LEDMap[12][65][0] = -1;
LEDMap[12][65][1] = -1;
LEDMap[0][66][0] = 26;
LEDMap[0][66][1] = -1;
LEDMap[1][66][0] = -1;
LEDMap[1][66][1] = -1;
LEDMap[2][66][0] = -1;
LEDMap[2][66][1] = -1;
LEDMap[3][66][0] = -1;
LEDMap[3][66][1] = -1;
LEDMap[4][66][0] = -1;
LEDMap[4][66][1] = -1;
LEDMap[5][66][0] = -1;
LEDMap[5][66][1] = -1;
LEDMap[6][66][0] = -1;
LEDMap[6][66][1] = -1;
LEDMap[7][66][0] = -1;
LEDMap[7][66][1] = -1;
LEDMap[8][66][0] = -1;
LEDMap[8][66][1] = -1;
LEDMap[9][66][0] = -1;
LEDMap[9][66][1] = -1;
LEDMap[10][66][0] = -1;
LEDMap[10][66][1] = -1;
LEDMap[11][66][0] = -1;
LEDMap[11][66][1] = -1;
LEDMap[12][66][0] = 168;
LEDMap[12][66][1] = -1;
LEDMap[0][67][0] = 25;
LEDMap[0][67][1] = -1;
LEDMap[1][67][0] = -1;
LEDMap[1][67][1] = -1;
LEDMap[2][67][0] = -1;
LEDMap[2][67][1] = -1;
LEDMap[3][67][0] = -1;
LEDMap[3][67][1] = -1;
LEDMap[4][67][0] = -1;
LEDMap[4][67][1] = -1;
LEDMap[5][67][0] = -1;
LEDMap[5][67][1] = -1;
LEDMap[6][67][0] = 112;
LEDMap[6][67][1] = -1;
LEDMap[7][67][0] = -1;
LEDMap[7][67][1] = -1;
LEDMap[8][67][0] = -1;
LEDMap[8][67][1] = -1;
LEDMap[9][67][0] = -1;
LEDMap[9][67][1] = -1;
LEDMap[10][67][0] = -1;
LEDMap[10][67][1] = -1;
LEDMap[11][67][0] = -1;
LEDMap[11][67][1] = -1;
LEDMap[12][67][0] = -1;
LEDMap[12][67][1] = -1;
LEDMap[0][68][0] = -1;
LEDMap[0][68][1] = -1;
LEDMap[1][68][0] = -1;
LEDMap[1][68][1] = -1;
LEDMap[2][68][0] = -1;
LEDMap[2][68][1] = -1;
LEDMap[3][68][0] = -1;
LEDMap[3][68][1] = -1;
LEDMap[4][68][0] = -1;
LEDMap[4][68][1] = -1;
LEDMap[5][68][0] = -1;
LEDMap[5][68][1] = -1;
LEDMap[6][68][0] = 113;
LEDMap[6][68][1] = -1;
LEDMap[7][68][0] = -1;
LEDMap[7][68][1] = -1;
LEDMap[8][68][0] = -1;
LEDMap[8][68][1] = -1;
LEDMap[9][68][0] = -1;
LEDMap[9][68][1] = -1;
LEDMap[10][68][0] = -1;
LEDMap[10][68][1] = -1;
LEDMap[11][68][0] = -1;
LEDMap[11][68][1] = -1;
LEDMap[12][68][0] = 167;
LEDMap[12][68][1] = -1;
LEDMap[0][69][0] = 24;
LEDMap[0][69][1] = -1;
LEDMap[1][69][0] = -1;
LEDMap[1][69][1] = -1;
LEDMap[2][69][0] = -1;
LEDMap[2][69][1] = -1;
LEDMap[3][69][0] = -1;
LEDMap[3][69][1] = -1;
LEDMap[4][69][0] = -1;
LEDMap[4][69][1] = -1;
LEDMap[5][69][0] = -1;
LEDMap[5][69][1] = -1;
LEDMap[6][69][0] = -1;
LEDMap[6][69][1] = -1;
LEDMap[7][69][0] = -1;
LEDMap[7][69][1] = -1;
LEDMap[8][69][0] = -1;
LEDMap[8][69][1] = -1;
LEDMap[9][69][0] = -1;
LEDMap[9][69][1] = -1;
LEDMap[10][69][0] = -1;
LEDMap[10][69][1] = -1;
LEDMap[11][69][0] = -1;
LEDMap[11][69][1] = -1;
LEDMap[12][69][0] = 166;
LEDMap[12][69][1] = -1;
LEDMap[0][70][0] = -1;
LEDMap[0][70][1] = -1;
LEDMap[1][70][0] = -1;
LEDMap[1][70][1] = -1;
LEDMap[2][70][0] = -1;
LEDMap[2][70][1] = -1;
LEDMap[3][70][0] = -1;
LEDMap[3][70][1] = -1;
LEDMap[4][70][0] = -1;
LEDMap[4][70][1] = -1;
LEDMap[5][70][0] = -1;
LEDMap[5][70][1] = -1;
LEDMap[6][70][0] = 114;
LEDMap[6][70][1] = -1;
LEDMap[7][70][0] = -1;
LEDMap[7][70][1] = -1;
LEDMap[8][70][0] = -1;
LEDMap[8][70][1] = -1;
LEDMap[9][70][0] = -1;
LEDMap[9][70][1] = -1;
LEDMap[10][70][0] = -1;
LEDMap[10][70][1] = -1;
LEDMap[11][70][0] = -1;
LEDMap[11][70][1] = -1;
LEDMap[12][70][0] = -1;
LEDMap[12][70][1] = -1;
LEDMap[0][71][0] = 23;
LEDMap[0][71][1] = -1;
LEDMap[1][71][0] = -1;
LEDMap[1][71][1] = -1;
LEDMap[2][71][0] = -1;
LEDMap[2][71][1] = -1;
LEDMap[3][71][0] = -1;
LEDMap[3][71][1] = -1;
LEDMap[4][71][0] = -1;
LEDMap[4][71][1] = -1;
LEDMap[5][71][0] = -1;
LEDMap[5][71][1] = -1;
LEDMap[6][71][0] = 115;
LEDMap[6][71][1] = -1;
LEDMap[7][71][0] = -1;
LEDMap[7][71][1] = -1;
LEDMap[8][71][0] = -1;
LEDMap[8][71][1] = -1;
LEDMap[9][71][0] = -1;
LEDMap[9][71][1] = -1;
LEDMap[10][71][0] = -1;
LEDMap[10][71][1] = -1;
LEDMap[11][71][0] = -1;
LEDMap[11][71][1] = -1;
LEDMap[12][71][0] = 165;
LEDMap[12][71][1] = -1;
LEDMap[0][72][0] = 22;
LEDMap[0][72][1] = -1;
LEDMap[1][72][0] = -1;
LEDMap[1][72][1] = -1;
LEDMap[2][72][0] = -1;
LEDMap[2][72][1] = -1;
LEDMap[3][72][0] = -1;
LEDMap[3][72][1] = -1;
LEDMap[4][72][0] = -1;
LEDMap[4][72][1] = -1;
LEDMap[5][72][0] = -1;
LEDMap[5][72][1] = -1;
LEDMap[6][72][0] = -1;
LEDMap[6][72][1] = -1;
LEDMap[7][72][0] = -1;
LEDMap[7][72][1] = -1;
LEDMap[8][72][0] = -1;
LEDMap[8][72][1] = -1;
LEDMap[9][72][0] = -1;
LEDMap[9][72][1] = -1;
LEDMap[10][72][0] = -1;
LEDMap[10][72][1] = -1;
LEDMap[11][72][0] = -1;
LEDMap[11][72][1] = -1;
LEDMap[12][72][0] = 164;
LEDMap[12][72][1] = -1;
LEDMap[0][73][0] = -1;
LEDMap[0][73][1] = -1;
LEDMap[1][73][0] = -1;
LEDMap[1][73][1] = -1;
LEDMap[2][73][0] = -1;
LEDMap[2][73][1] = -1;
LEDMap[3][73][0] = -1;
LEDMap[3][73][1] = -1;
LEDMap[4][73][0] = -1;
LEDMap[4][73][1] = -1;
LEDMap[5][73][0] = -1;
LEDMap[5][73][1] = -1;
LEDMap[6][73][0] = 116;
LEDMap[6][73][1] = -1;
LEDMap[7][73][0] = -1;
LEDMap[7][73][1] = -1;
LEDMap[8][73][0] = -1;
LEDMap[8][73][1] = -1;
LEDMap[9][73][0] = -1;
LEDMap[9][73][1] = -1;
LEDMap[10][73][0] = -1;
LEDMap[10][73][1] = -1;
LEDMap[11][73][0] = -1;
LEDMap[11][73][1] = -1;
LEDMap[12][73][0] = -1;
LEDMap[12][73][1] = -1;
LEDMap[0][74][0] = 21;
LEDMap[0][74][1] = -1;
LEDMap[1][74][0] = -1;
LEDMap[1][74][1] = -1;
LEDMap[2][74][0] = -1;
LEDMap[2][74][1] = -1;
LEDMap[3][74][0] = -1;
LEDMap[3][74][1] = -1;
LEDMap[4][74][0] = -1;
LEDMap[4][74][1] = -1;
LEDMap[5][74][0] = -1;
LEDMap[5][74][1] = -1;
LEDMap[6][74][0] = 117;
LEDMap[6][74][1] = -1;
LEDMap[7][74][0] = -1;
LEDMap[7][74][1] = -1;
LEDMap[8][74][0] = -1;
LEDMap[8][74][1] = -1;
LEDMap[9][74][0] = -1;
LEDMap[9][74][1] = -1;
LEDMap[10][74][0] = -1;
LEDMap[10][74][1] = -1;
LEDMap[11][74][0] = -1;
LEDMap[11][74][1] = -1;
LEDMap[12][74][0] = 163;
LEDMap[12][74][1] = -1;
LEDMap[0][75][0] = -1;
LEDMap[0][75][1] = -1;
LEDMap[1][75][0] = -1;
LEDMap[1][75][1] = -1;
LEDMap[2][75][0] = -1;
LEDMap[2][75][1] = -1;
LEDMap[3][75][0] = -1;
LEDMap[3][75][1] = -1;
LEDMap[4][75][0] = -1;
LEDMap[4][75][1] = -1;
LEDMap[5][75][0] = -1;
LEDMap[5][75][1] = -1;
LEDMap[6][75][0] = -1;
LEDMap[6][75][1] = -1;
LEDMap[7][75][0] = -1;
LEDMap[7][75][1] = -1;
LEDMap[8][75][0] = -1;
LEDMap[8][75][1] = -1;
LEDMap[9][75][0] = -1;
LEDMap[9][75][1] = -1;
LEDMap[10][75][0] = -1;
LEDMap[10][75][1] = -1;
LEDMap[11][75][0] = -1;
LEDMap[11][75][1] = -1;
LEDMap[12][75][0] = 162;
LEDMap[12][75][1] = -1;
LEDMap[0][76][0] = 20;
LEDMap[0][76][1] = -1;
LEDMap[1][76][0] = -1;
LEDMap[1][76][1] = -1;
LEDMap[2][76][0] = -1;
LEDMap[2][76][1] = -1;
LEDMap[3][76][0] = -1;
LEDMap[3][76][1] = -1;
LEDMap[4][76][0] = -1;
LEDMap[4][76][1] = -1;
LEDMap[5][76][0] = -1;
LEDMap[5][76][1] = -1;
LEDMap[6][76][0] = 118;
LEDMap[6][76][1] = -1;
LEDMap[7][76][0] = -1;
LEDMap[7][76][1] = -1;
LEDMap[8][76][0] = -1;
LEDMap[8][76][1] = -1;
LEDMap[9][76][0] = -1;
LEDMap[9][76][1] = -1;
LEDMap[10][76][0] = -1;
LEDMap[10][76][1] = -1;
LEDMap[11][76][0] = -1;
LEDMap[11][76][1] = -1;
LEDMap[12][76][0] = -1;
LEDMap[12][76][1] = -1;
LEDMap[0][77][0] = 19;
LEDMap[0][77][1] = -1;
LEDMap[1][77][0] = -1;
LEDMap[1][77][1] = -1;
LEDMap[2][77][0] = -1;
LEDMap[2][77][1] = -1;
LEDMap[3][77][0] = -1;
LEDMap[3][77][1] = -1;
LEDMap[4][77][0] = -1;
LEDMap[4][77][1] = -1;
LEDMap[5][77][0] = -1;
LEDMap[5][77][1] = -1;
LEDMap[6][77][0] = 119;
LEDMap[6][77][1] = -1;
LEDMap[7][77][0] = -1;
LEDMap[7][77][1] = -1;
LEDMap[8][77][0] = -1;
LEDMap[8][77][1] = -1;
LEDMap[9][77][0] = -1;
LEDMap[9][77][1] = -1;
LEDMap[10][77][0] = -1;
LEDMap[10][77][1] = -1;
LEDMap[11][77][0] = -1;
LEDMap[11][77][1] = -1;
LEDMap[12][77][0] = 161;
LEDMap[12][77][1] = -1;
LEDMap[0][78][0] = 18;
LEDMap[0][78][1] = -1;
LEDMap[1][78][0] = -1;
LEDMap[1][78][1] = -1;
LEDMap[2][78][0] = -1;
LEDMap[2][78][1] = -1;
LEDMap[3][78][0] = -1;
LEDMap[3][78][1] = -1;
LEDMap[4][78][0] = -1;
LEDMap[4][78][1] = -1;
LEDMap[5][78][0] = -1;
LEDMap[5][78][1] = -1;
LEDMap[6][78][0] = -1;
LEDMap[6][78][1] = -1;
LEDMap[7][78][0] = -1;
LEDMap[7][78][1] = -1;
LEDMap[8][78][0] = -1;
LEDMap[8][78][1] = -1;
LEDMap[9][78][0] = -1;
LEDMap[9][78][1] = -1;
LEDMap[10][78][0] = -1;
LEDMap[10][78][1] = -1;
LEDMap[11][78][0] = -1;
LEDMap[11][78][1] = -1;
LEDMap[12][78][0] = -1;
LEDMap[12][78][1] = -1;
LEDMap[0][79][0] = -1;
LEDMap[0][79][1] = -1;
LEDMap[1][79][0] = -1;
LEDMap[1][79][1] = -1;
LEDMap[2][79][0] = -1;
LEDMap[2][79][1] = -1;
LEDMap[3][79][0] = -1;
LEDMap[3][79][1] = -1;
LEDMap[4][79][0] = -1;
LEDMap[4][79][1] = -1;
LEDMap[5][79][0] = -1;
LEDMap[5][79][1] = -1;
LEDMap[6][79][0] = 120;
LEDMap[6][79][1] = -1;
LEDMap[7][79][0] = -1;
LEDMap[7][79][1] = -1;
LEDMap[8][79][0] = -1;
LEDMap[8][79][1] = -1;
LEDMap[9][79][0] = -1;
LEDMap[9][79][1] = -1;
LEDMap[10][79][0] = -1;
LEDMap[10][79][1] = -1;
LEDMap[11][79][0] = -1;
LEDMap[11][79][1] = -1;
LEDMap[12][79][0] = 160;
LEDMap[12][79][1] = -1;
LEDMap[0][80][0] = 17;
LEDMap[0][80][1] = -1;
LEDMap[1][80][0] = -1;
LEDMap[1][80][1] = -1;
LEDMap[2][80][0] = -1;
LEDMap[2][80][1] = -1;
LEDMap[3][80][0] = -1;
LEDMap[3][80][1] = -1;
LEDMap[4][80][0] = -1;
LEDMap[4][80][1] = -1;
LEDMap[5][80][0] = -1;
LEDMap[5][80][1] = -1;
LEDMap[6][80][0] = 121;
LEDMap[6][80][1] = -1;
LEDMap[7][80][0] = -1;
LEDMap[7][80][1] = -1;
LEDMap[8][80][0] = -1;
LEDMap[8][80][1] = -1;
LEDMap[9][80][0] = -1;
LEDMap[9][80][1] = -1;
LEDMap[10][80][0] = -1;
LEDMap[10][80][1] = -1;
LEDMap[11][80][0] = -1;
LEDMap[11][80][1] = -1;
LEDMap[12][80][0] = 159;
LEDMap[12][80][1] = -1;
LEDMap[0][81][0] = -1;
LEDMap[0][81][1] = -1;
LEDMap[1][81][0] = -1;
LEDMap[1][81][1] = -1;
LEDMap[2][81][0] = -1;
LEDMap[2][81][1] = -1;
LEDMap[3][81][0] = -1;
LEDMap[3][81][1] = -1;
LEDMap[4][81][0] = -1;
LEDMap[4][81][1] = -1;
LEDMap[5][81][0] = -1;
LEDMap[5][81][1] = -1;
LEDMap[6][81][0] = -1;
LEDMap[6][81][1] = -1;
LEDMap[7][81][0] = -1;
LEDMap[7][81][1] = -1;
LEDMap[8][81][0] = -1;
LEDMap[8][81][1] = -1;
LEDMap[9][81][0] = -1;
LEDMap[9][81][1] = -1;
LEDMap[10][81][0] = -1;
LEDMap[10][81][1] = -1;
LEDMap[11][81][0] = -1;
LEDMap[11][81][1] = -1;
LEDMap[12][81][0] = 158;
LEDMap[12][81][1] = -1;
LEDMap[0][82][0] = 16;
LEDMap[0][82][1] = -1;
LEDMap[1][82][0] = -1;
LEDMap[1][82][1] = -1;
LEDMap[2][82][0] = -1;
LEDMap[2][82][1] = -1;
LEDMap[3][82][0] = -1;
LEDMap[3][82][1] = -1;
LEDMap[4][82][0] = -1;
LEDMap[4][82][1] = -1;
LEDMap[5][82][0] = -1;
LEDMap[5][82][1] = -1;
LEDMap[6][82][0] = 122;
LEDMap[6][82][1] = -1;
LEDMap[7][82][0] = -1;
LEDMap[7][82][1] = -1;
LEDMap[8][82][0] = -1;
LEDMap[8][82][1] = -1;
LEDMap[9][82][0] = -1;
LEDMap[9][82][1] = -1;
LEDMap[10][82][0] = -1;
LEDMap[10][82][1] = -1;
LEDMap[11][82][0] = -1;
LEDMap[11][82][1] = -1;
LEDMap[12][82][0] = -1;
LEDMap[12][82][1] = -1;
LEDMap[0][83][0] = 15;
LEDMap[0][83][1] = -1;
LEDMap[1][83][0] = -1;
LEDMap[1][83][1] = -1;
LEDMap[2][83][0] = -1;
LEDMap[2][83][1] = -1;
LEDMap[3][83][0] = -1;
LEDMap[3][83][1] = -1;
LEDMap[4][83][0] = -1;
LEDMap[4][83][1] = -1;
LEDMap[5][83][0] = -1;
LEDMap[5][83][1] = -1;
LEDMap[6][83][0] = 123;
LEDMap[6][83][1] = -1;
LEDMap[7][83][0] = -1;
LEDMap[7][83][1] = -1;
LEDMap[8][83][0] = -1;
LEDMap[8][83][1] = -1;
LEDMap[9][83][0] = -1;
LEDMap[9][83][1] = -1;
LEDMap[10][83][0] = -1;
LEDMap[10][83][1] = -1;
LEDMap[11][83][0] = -1;
LEDMap[11][83][1] = -1;
LEDMap[12][83][0] = 157;
LEDMap[12][83][1] = -1;
LEDMap[0][84][0] = -1;
LEDMap[0][84][1] = -1;
LEDMap[1][84][0] = -1;
LEDMap[1][84][1] = -1;
LEDMap[2][84][0] = -1;
LEDMap[2][84][1] = -1;
LEDMap[3][84][0] = -1;
LEDMap[3][84][1] = -1;
LEDMap[4][84][0] = -1;
LEDMap[4][84][1] = -1;
LEDMap[5][84][0] = -1;
LEDMap[5][84][1] = -1;
LEDMap[6][84][0] = -1;
LEDMap[6][84][1] = -1;
LEDMap[7][84][0] = -1;
LEDMap[7][84][1] = -1;
LEDMap[8][84][0] = -1;
LEDMap[8][84][1] = -1;
LEDMap[9][84][0] = -1;
LEDMap[9][84][1] = -1;
LEDMap[10][84][0] = -1;
LEDMap[10][84][1] = -1;
LEDMap[11][84][0] = -1;
LEDMap[11][84][1] = -1;
LEDMap[12][84][0] = 156;
LEDMap[12][84][1] = -1;
LEDMap[0][85][0] = 14;
LEDMap[0][85][1] = -1;
LEDMap[1][85][0] = -1;
LEDMap[1][85][1] = -1;
LEDMap[2][85][0] = -1;
LEDMap[2][85][1] = -1;
LEDMap[3][85][0] = -1;
LEDMap[3][85][1] = -1;
LEDMap[4][85][0] = -1;
LEDMap[4][85][1] = -1;
LEDMap[5][85][0] = -1;
LEDMap[5][85][1] = -1;
LEDMap[6][85][0] = 124;
LEDMap[6][85][1] = -1;
LEDMap[7][85][0] = -1;
LEDMap[7][85][1] = -1;
LEDMap[8][85][0] = -1;
LEDMap[8][85][1] = -1;
LEDMap[9][85][0] = -1;
LEDMap[9][85][1] = -1;
LEDMap[10][85][0] = -1;
LEDMap[10][85][1] = -1;
LEDMap[11][85][0] = -1;
LEDMap[11][85][1] = -1;
LEDMap[12][85][0] = -1;
LEDMap[12][85][1] = -1;
LEDMap[0][86][0] = 13;
LEDMap[0][86][1] = -1;
LEDMap[1][86][0] = -1;
LEDMap[1][86][1] = -1;
LEDMap[2][86][0] = -1;
LEDMap[2][86][1] = -1;
LEDMap[3][86][0] = -1;
LEDMap[3][86][1] = -1;
LEDMap[4][86][0] = -1;
LEDMap[4][86][1] = -1;
LEDMap[5][86][0] = -1;
LEDMap[5][86][1] = -1;
LEDMap[6][86][0] = 125;
LEDMap[6][86][1] = -1;
LEDMap[7][86][0] = -1;
LEDMap[7][86][1] = -1;
LEDMap[8][86][0] = -1;
LEDMap[8][86][1] = -1;
LEDMap[9][86][0] = -1;
LEDMap[9][86][1] = -1;
LEDMap[10][86][0] = -1;
LEDMap[10][86][1] = -1;
LEDMap[11][86][0] = -1;
LEDMap[11][86][1] = -1;
LEDMap[12][86][0] = 155;
LEDMap[12][86][1] = -1;
LEDMap[0][87][0] = -1;
LEDMap[0][87][1] = -1;
LEDMap[1][87][0] = -1;
LEDMap[1][87][1] = -1;
LEDMap[2][87][0] = -1;
LEDMap[2][87][1] = -1;
LEDMap[3][87][0] = -1;
LEDMap[3][87][1] = -1;
LEDMap[4][87][0] = -1;
LEDMap[4][87][1] = -1;
LEDMap[5][87][0] = -1;
LEDMap[5][87][1] = -1;
LEDMap[6][87][0] = -1;
LEDMap[6][87][1] = -1;
LEDMap[7][87][0] = -1;
LEDMap[7][87][1] = -1;
LEDMap[8][87][0] = -1;
LEDMap[8][87][1] = -1;
LEDMap[9][87][0] = -1;
LEDMap[9][87][1] = -1;
LEDMap[10][87][0] = -1;
LEDMap[10][87][1] = -1;
LEDMap[11][87][0] = -1;
LEDMap[11][87][1] = -1;
LEDMap[12][87][0] = -1;
LEDMap[12][87][1] = -1;
LEDMap[0][88][0] = 12;
LEDMap[0][88][1] = -1;
LEDMap[1][88][0] = -1;
LEDMap[1][88][1] = -1;
LEDMap[2][88][0] = -1;
LEDMap[2][88][1] = -1;
LEDMap[3][88][0] = -1;
LEDMap[3][88][1] = -1;
LEDMap[4][88][0] = -1;
LEDMap[4][88][1] = -1;
LEDMap[5][88][0] = 0;
LEDMap[5][88][1] = -1;
LEDMap[6][88][0] = 126;
LEDMap[6][88][1] = -1;
LEDMap[7][88][0] = -1;
LEDMap[7][88][1] = -1;
LEDMap[8][88][0] = -1;
LEDMap[8][88][1] = -1;
LEDMap[9][88][0] = -1;
LEDMap[9][88][1] = -1;
LEDMap[10][88][0] = -1;
LEDMap[10][88][1] = -1;
LEDMap[11][88][0] = -1;
LEDMap[11][88][1] = -1;
LEDMap[12][88][0] = 154;
LEDMap[12][88][1] = -1;
LEDMap[0][89][0] = 11;
LEDMap[0][89][1] = -1;
LEDMap[1][89][0] = -1;
LEDMap[1][89][1] = -1;
LEDMap[2][89][0] = -1;
LEDMap[2][89][1] = -1;
LEDMap[3][89][0] = -1;
LEDMap[3][89][1] = -1;
LEDMap[4][89][0] = -1;
LEDMap[4][89][1] = -1;
LEDMap[5][89][0] = -1;
LEDMap[5][89][1] = -1;
LEDMap[6][89][0] = 127;
LEDMap[6][89][1] = -1;
LEDMap[7][89][0] = -1;
LEDMap[7][89][1] = -1;
LEDMap[8][89][0] = -1;
LEDMap[8][89][1] = -1;
LEDMap[9][89][0] = -1;
LEDMap[9][89][1] = -1;
LEDMap[10][89][0] = -1;
LEDMap[10][89][1] = -1;
LEDMap[11][89][0] = -1;
LEDMap[11][89][1] = -1;
LEDMap[12][89][0] = 153;
LEDMap[12][89][1] = -1;
LEDMap[0][90][0] = -1;
LEDMap[0][90][1] = -1;
LEDMap[1][90][0] = -1;
LEDMap[1][90][1] = -1;
LEDMap[2][90][0] = -1;
LEDMap[2][90][1] = -1;
LEDMap[3][90][0] = -1;
LEDMap[3][90][1] = -1;
LEDMap[4][90][0] = -1;
LEDMap[4][90][1] = -1;
LEDMap[5][90][0] = -1;
LEDMap[5][90][1] = -1;
LEDMap[6][90][0] = -1;
LEDMap[6][90][1] = -1;
LEDMap[7][90][0] = -1;
LEDMap[7][90][1] = -1;
LEDMap[8][90][0] = -1;
LEDMap[8][90][1] = -1;
LEDMap[9][90][0] = -1;
LEDMap[9][90][1] = -1;
LEDMap[10][90][0] = -1;
LEDMap[10][90][1] = -1;
LEDMap[11][90][0] = -1;
LEDMap[11][90][1] = -1;
LEDMap[12][90][0] = -1;
LEDMap[12][90][1] = -1;
LEDMap[0][91][0] = 10;
LEDMap[0][91][1] = -1;
LEDMap[1][91][0] = -1;
LEDMap[1][91][1] = -1;
LEDMap[2][91][0] = -1;
LEDMap[2][91][1] = -1;
LEDMap[3][91][0] = -1;
LEDMap[3][91][1] = -1;
LEDMap[4][91][0] = -1;
LEDMap[4][91][1] = -1;
LEDMap[5][91][0] = -1;
LEDMap[5][91][1] = -1;
LEDMap[6][91][0] = 128;
LEDMap[6][91][1] = -1;
LEDMap[7][91][0] = -1;
LEDMap[7][91][1] = -1;
LEDMap[8][91][0] = -1;
LEDMap[8][91][1] = -1;
LEDMap[9][91][0] = -1;
LEDMap[9][91][1] = -1;
LEDMap[10][91][0] = -1;
LEDMap[10][91][1] = -1;
LEDMap[11][91][0] = -1;
LEDMap[11][91][1] = -1;
LEDMap[12][91][0] = 152;
LEDMap[12][91][1] = -1;
LEDMap[0][92][0] = 9;
LEDMap[0][92][1] = -1;
LEDMap[1][92][0] = -1;
LEDMap[1][92][1] = -1;
LEDMap[2][92][0] = -1;
LEDMap[2][92][1] = -1;
LEDMap[3][92][0] = -1;
LEDMap[3][92][1] = -1;
LEDMap[4][92][0] = -1;
LEDMap[4][92][1] = -1;
LEDMap[5][92][0] = -1;
LEDMap[5][92][1] = -1;
LEDMap[6][92][0] = 129;
LEDMap[6][92][1] = -1;
LEDMap[7][92][0] = -1;
LEDMap[7][92][1] = -1;
LEDMap[8][92][0] = -1;
LEDMap[8][92][1] = -1;
LEDMap[9][92][0] = -1;
LEDMap[9][92][1] = -1;
LEDMap[10][92][0] = -1;
LEDMap[10][92][1] = -1;
LEDMap[11][92][0] = -1;
LEDMap[11][92][1] = -1;
LEDMap[12][92][0] = 151;
LEDMap[12][92][1] = -1;
LEDMap[0][93][0] = -1;
LEDMap[0][93][1] = -1;
LEDMap[1][93][0] = -1;
LEDMap[1][93][1] = -1;
LEDMap[2][93][0] = -1;
LEDMap[2][93][1] = -1;
LEDMap[3][93][0] = -1;
LEDMap[3][93][1] = -1;
LEDMap[4][93][0] = -1;
LEDMap[4][93][1] = -1;
LEDMap[5][93][0] = -1;
LEDMap[5][93][1] = -1;
LEDMap[6][93][0] = 130;
LEDMap[6][93][1] = -1;
LEDMap[7][93][0] = -1;
LEDMap[7][93][1] = -1;
LEDMap[8][93][0] = -1;
LEDMap[8][93][1] = -1;
LEDMap[9][93][0] = -1;
LEDMap[9][93][1] = -1;
LEDMap[10][93][0] = -1;
LEDMap[10][93][1] = -1;
LEDMap[11][93][0] = -1;
LEDMap[11][93][1] = -1;
LEDMap[12][93][0] = -1;
LEDMap[12][93][1] = -1;
LEDMap[0][94][0] = 8;
LEDMap[0][94][1] = -1;
LEDMap[1][94][0] = -1;
LEDMap[1][94][1] = -1;
LEDMap[2][94][0] = -1;
LEDMap[2][94][1] = -1;
LEDMap[3][94][0] = -1;
LEDMap[3][94][1] = -1;
LEDMap[4][94][0] = -1;
LEDMap[4][94][1] = -1;
LEDMap[5][94][0] = -1;
LEDMap[5][94][1] = -1;
LEDMap[6][94][0] = -1;
LEDMap[6][94][1] = -1;
LEDMap[7][94][0] = -1;
LEDMap[7][94][1] = -1;
LEDMap[8][94][0] = -1;
LEDMap[8][94][1] = -1;
LEDMap[9][94][0] = -1;
LEDMap[9][94][1] = -1;
LEDMap[10][94][0] = -1;
LEDMap[10][94][1] = -1;
LEDMap[11][94][0] = -1;
LEDMap[11][94][1] = -1;
LEDMap[12][94][0] = 150;
LEDMap[12][94][1] = -1;
LEDMap[0][95][0] = 7;
LEDMap[0][95][1] = -1;
LEDMap[1][95][0] = -1;
LEDMap[1][95][1] = -1;
LEDMap[2][95][0] = -1;
LEDMap[2][95][1] = -1;
LEDMap[3][95][0] = -1;
LEDMap[3][95][1] = -1;
LEDMap[4][95][0] = -1;
LEDMap[4][95][1] = -1;
LEDMap[5][95][0] = -1;
LEDMap[5][95][1] = -1;
LEDMap[6][95][0] = 131;
LEDMap[6][95][1] = -1;
LEDMap[7][95][0] = -1;
LEDMap[7][95][1] = -1;
LEDMap[8][95][0] = -1;
LEDMap[8][95][1] = -1;
LEDMap[9][95][0] = -1;
LEDMap[9][95][1] = -1;
LEDMap[10][95][0] = -1;
LEDMap[10][95][1] = -1;
LEDMap[11][95][0] = -1;
LEDMap[11][95][1] = -1;
LEDMap[12][95][0] = 149;
LEDMap[12][95][1] = -1;
LEDMap[0][96][0] = -1;
LEDMap[0][96][1] = -1;
LEDMap[1][96][0] = -1;
LEDMap[1][96][1] = -1;
LEDMap[2][96][0] = -1;
LEDMap[2][96][1] = -1;
LEDMap[3][96][0] = -1;
LEDMap[3][96][1] = -1;
LEDMap[4][96][0] = -1;
LEDMap[4][96][1] = -1;
LEDMap[5][96][0] = -1;
LEDMap[5][96][1] = -1;
LEDMap[6][96][0] = 132;
LEDMap[6][96][1] = -1;
LEDMap[7][96][0] = -1;
LEDMap[7][96][1] = -1;
LEDMap[8][96][0] = -1;
LEDMap[8][96][1] = -1;
LEDMap[9][96][0] = -1;
LEDMap[9][96][1] = -1;
LEDMap[10][96][0] = -1;
LEDMap[10][96][1] = -1;
LEDMap[11][96][0] = -1;
LEDMap[11][96][1] = -1;
LEDMap[12][96][0] = -1;
LEDMap[12][96][1] = -1;
LEDMap[0][97][0] = 6;
LEDMap[0][97][1] = -1;
LEDMap[1][97][0] = -1;
LEDMap[1][97][1] = -1;
LEDMap[2][97][0] = -1;
LEDMap[2][97][1] = -1;
LEDMap[3][97][0] = -1;
LEDMap[3][97][1] = -1;
LEDMap[4][97][0] = -1;
LEDMap[4][97][1] = -1;
LEDMap[5][97][0] = -1;
LEDMap[5][97][1] = -1;
LEDMap[6][97][0] = -1;
LEDMap[6][97][1] = -1;
LEDMap[7][97][0] = -1;
LEDMap[7][97][1] = -1;
LEDMap[8][97][0] = -1;
LEDMap[8][97][1] = -1;
LEDMap[9][97][0] = -1;
LEDMap[9][97][1] = -1;
LEDMap[10][97][0] = -1;
LEDMap[10][97][1] = -1;
LEDMap[11][97][0] = -1;
LEDMap[11][97][1] = -1;
LEDMap[12][97][0] = 148;
LEDMap[12][97][1] = -1;
LEDMap[0][98][0] = 5;
LEDMap[0][98][1] = -1;
LEDMap[1][98][0] = -1;
LEDMap[1][98][1] = -1;
LEDMap[2][98][0] = -1;
LEDMap[2][98][1] = -1;
LEDMap[3][98][0] = -1;
LEDMap[3][98][1] = -1;
LEDMap[4][98][0] = -1;
LEDMap[4][98][1] = -1;
LEDMap[5][98][0] = -1;
LEDMap[5][98][1] = -1;
LEDMap[6][98][0] = 133;
LEDMap[6][98][1] = -1;
LEDMap[7][98][0] = -1;
LEDMap[7][98][1] = -1;
LEDMap[8][98][0] = -1;
LEDMap[8][98][1] = -1;
LEDMap[9][98][0] = -1;
LEDMap[9][98][1] = -1;
LEDMap[10][98][0] = -1;
LEDMap[10][98][1] = -1;
LEDMap[11][98][0] = -1;
LEDMap[11][98][1] = -1;
LEDMap[12][98][0] = 147;
LEDMap[12][98][1] = -1;
LEDMap[0][99][0] = -1;
LEDMap[0][99][1] = -1;
LEDMap[1][99][0] = -1;
LEDMap[1][99][1] = -1;
LEDMap[2][99][0] = -1;
LEDMap[2][99][1] = -1;
LEDMap[3][99][0] = -1;
LEDMap[3][99][1] = -1;
LEDMap[4][99][0] = -1;
LEDMap[4][99][1] = -1;
LEDMap[5][99][0] = -1;
LEDMap[5][99][1] = -1;
LEDMap[6][99][0] = 134;
LEDMap[6][99][1] = -1;
LEDMap[7][99][0] = -1;
LEDMap[7][99][1] = -1;
LEDMap[8][99][0] = -1;
LEDMap[8][99][1] = -1;
LEDMap[9][99][0] = -1;
LEDMap[9][99][1] = -1;
LEDMap[10][99][0] = -1;
LEDMap[10][99][1] = -1;
LEDMap[11][99][0] = -1;
LEDMap[11][99][1] = -1;
LEDMap[12][99][0] = -1;
LEDMap[12][99][1] = -1;
LEDMap[0][100][0] = 4;
LEDMap[0][100][1] = -1;
LEDMap[1][100][0] = -1;
LEDMap[1][100][1] = -1;
LEDMap[2][100][0] = -1;
LEDMap[2][100][1] = -1;
LEDMap[3][100][0] = -1;
LEDMap[3][100][1] = -1;
LEDMap[4][100][0] = -1;
LEDMap[4][100][1] = -1;
LEDMap[5][100][0] = -1;
LEDMap[5][100][1] = -1;
LEDMap[6][100][0] = -1;
LEDMap[6][100][1] = -1;
LEDMap[7][100][0] = -1;
LEDMap[7][100][1] = -1;
LEDMap[8][100][0] = -1;
LEDMap[8][100][1] = -1;
LEDMap[9][100][0] = -1;
LEDMap[9][100][1] = -1;
LEDMap[10][100][0] = -1;
LEDMap[10][100][1] = -1;
LEDMap[11][100][0] = -1;
LEDMap[11][100][1] = -1;
LEDMap[12][100][0] = 146;
LEDMap[12][100][1] = -1;
LEDMap[0][101][0] = -1;
LEDMap[0][101][1] = -1;
LEDMap[1][101][0] = -1;
LEDMap[1][101][1] = -1;
LEDMap[2][101][0] = -1;
LEDMap[2][101][1] = -1;
LEDMap[3][101][0] = -1;
LEDMap[3][101][1] = -1;
LEDMap[4][101][0] = -1;
LEDMap[4][101][1] = -1;
LEDMap[5][101][0] = -1;
LEDMap[5][101][1] = -1;
LEDMap[6][101][0] = 135;
LEDMap[6][101][1] = -1;
LEDMap[7][101][0] = -1;
LEDMap[7][101][1] = -1;
LEDMap[8][101][0] = -1;
LEDMap[8][101][1] = -1;
LEDMap[9][101][0] = -1;
LEDMap[9][101][1] = -1;
LEDMap[10][101][0] = -1;
LEDMap[10][101][1] = -1;
LEDMap[11][101][0] = -1;
LEDMap[11][101][1] = -1;
LEDMap[12][101][0] = 145;
LEDMap[12][101][1] = -1;
LEDMap[0][102][0] = 3;
LEDMap[0][102][1] = -1;
LEDMap[1][102][0] = -1;
LEDMap[1][102][1] = -1;
LEDMap[2][102][0] = -1;
LEDMap[2][102][1] = -1;
LEDMap[3][102][0] = -1;
LEDMap[3][102][1] = -1;
LEDMap[4][102][0] = -1;
LEDMap[4][102][1] = -1;
LEDMap[5][102][0] = -1;
LEDMap[5][102][1] = -1;
LEDMap[6][102][0] = 136;
LEDMap[6][102][1] = -1;
LEDMap[7][102][0] = -1;
LEDMap[7][102][1] = -1;
LEDMap[8][102][0] = -1;
LEDMap[8][102][1] = -1;
LEDMap[9][102][0] = -1;
LEDMap[9][102][1] = -1;
LEDMap[10][102][0] = -1;
LEDMap[10][102][1] = -1;
LEDMap[11][102][0] = -1;
LEDMap[11][102][1] = -1;
LEDMap[12][102][0] = 144;
LEDMap[12][102][1] = -1;
LEDMap[0][103][0] = 2;
LEDMap[0][103][1] = -1;
LEDMap[1][103][0] = -1;
LEDMap[1][103][1] = -1;
LEDMap[2][103][0] = -1;
LEDMap[2][103][1] = -1;
LEDMap[3][103][0] = -1;
LEDMap[3][103][1] = -1;
LEDMap[4][103][0] = -1;
LEDMap[4][103][1] = -1;
LEDMap[5][103][0] = -1;
LEDMap[5][103][1] = -1;
LEDMap[6][103][0] = 137;
LEDMap[6][103][1] = -1;
LEDMap[7][103][0] = -1;
LEDMap[7][103][1] = -1;
LEDMap[8][103][0] = -1;
LEDMap[8][103][1] = -1;
LEDMap[9][103][0] = -1;
LEDMap[9][103][1] = -1;
LEDMap[10][103][0] = -1;
LEDMap[10][103][1] = -1;
LEDMap[11][103][0] = -1;
LEDMap[11][103][1] = -1;
LEDMap[12][103][0] = 139;
LEDMap[12][103][1] = -1;
LEDMap[0][104][0] = 1;
LEDMap[0][104][1] = -1;
LEDMap[1][104][0] = -1;
LEDMap[1][104][1] = -1;
LEDMap[2][104][0] = -1;
LEDMap[2][104][1] = -1;
LEDMap[3][104][0] = -1;
LEDMap[3][104][1] = -1;
LEDMap[4][104][0] = -1;
LEDMap[4][104][1] = -1;
LEDMap[5][104][0] = -1;
LEDMap[5][104][1] = -1;
LEDMap[6][104][0] = -1;
LEDMap[6][104][1] = -1;
LEDMap[7][104][0] = 138;
LEDMap[7][104][1] = -1;
LEDMap[8][104][0] = -1;
LEDMap[8][104][1] = -1;
LEDMap[9][104][0] = -1;
LEDMap[9][104][1] = -1;
LEDMap[10][104][0] = -1;
LEDMap[10][104][1] = -1;
LEDMap[11][104][0] = -1;
LEDMap[11][104][1] = -1;
LEDMap[12][104][0] = 140;
LEDMap[12][104][1] = 143;
LEDMap[0][105][0] = -1;
LEDMap[0][105][1] = -1;
LEDMap[1][105][0] = -1;
LEDMap[1][105][1] = -1;
LEDMap[2][105][0] = -1;
LEDMap[2][105][1] = -1;
LEDMap[3][105][0] = -1;
LEDMap[3][105][1] = -1;
LEDMap[4][105][0] = -1;
LEDMap[4][105][1] = -1;
LEDMap[5][105][0] = -1;
LEDMap[5][105][1] = -1;
LEDMap[6][105][0] = -1;
LEDMap[6][105][1] = -1;
LEDMap[7][105][0] = -1;
LEDMap[7][105][1] = -1;
LEDMap[8][105][0] = -1;
LEDMap[8][105][1] = -1;
LEDMap[9][105][0] = -1;
LEDMap[9][105][1] = -1;
LEDMap[10][105][0] = -1;
LEDMap[10][105][1] = -1;
LEDMap[11][105][0] = 142;
LEDMap[11][105][1] = -1;
LEDMap[12][105][0] = -1;
LEDMap[12][105][1] = -1;

}
