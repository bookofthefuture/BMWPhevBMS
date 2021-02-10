/*
 * Google Turbo Colour Map used to indicate delta V and delta T from:
 * https://gist.githubusercontent.com/mikhailov-work/6a308c20e494d9e0ccc29036b28faa7a/raw/a438da918f140eac8775be5e302f63795d9e28c3/turbo_colormap.c
 * 
 * 
 * 
 * 
***************************************************
  This is a library for several Adafruit displays based on ST77* drivers.

  Works with the Adafruit 1.8" TFT Breakout w/SD card
    ----> http://www.adafruit.com/products/358
  The 1.8" TFT shield
    ----> https://www.adafruit.com/product/802
  The 1.44" TFT breakout
    ----> https://www.adafruit.com/product/2088
  The 1.54" TFT breakout
    ----> https://www.adafruit.com/product/3787
  The 2.0" TFT breakout
    ----> https://www.adafruit.com/product/4311
  as well as Adafruit raw 1.8" TFT display
    ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// This Teensy3 and 4 native optimized and extended version
// requires specific pins. 
// If you use the short version of the constructor and the DC
// pin is hardware CS pin, then it will be slower.

#include <ST7735_t3.h> // Hardware-specific library
#include <SPI.h>
#include "SimpBMS-BMW.h"
#include "TurboColourMap.h"
#include "SinLookUp.h"
#include <st7735_t3_font_Arial.h>

#define TFT_MISO  12
#define TFT_MOSI  11  //a12
#define TFT_SCK   13  //a13
#define TFT_DC   9 
#define TFT_CS   7  
#define TFT_RST  8

ST7735_t3 tft = ST7735_t3(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);

int8_t   testInput; //just for testing
int8_t   bandCount;
int8_t   b=1;
int16_t  colour=0;
uint16_t ti;
uint16_t to;
int16_t  amps=0;
int8_t   power=0;
int16_t  volts=292;
int16_t  temp=26; //pack temperature
long     i=0;
bool     preContactor,mainContactorPlus,mainContactorGnd;


uint16_t ConvertRGBto565( uint8_t R, uint8_t G, uint8_t B)
//Function to provide 565 colour codes from RGB codes
{
  return ( ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3) );
}


uint16_t tMapColour(uint8_t m)
{
  return ConvertRGBto565(pgm_read_byte(&(tcmap[m][0])),pgm_read_byte(&(tcmap[m][1])),pgm_read_byte(&(tcmap[m][2])));
  
}


void moduleDV()
//display array of bars showing cell delta V from average pack cell voltage
{
const uint8_t tLineW=2;
const uint8_t tLineH=9;
const uint8_t hSpacing = 25;
const uint8_t vSpacing = tLineH+1;
uint8_t n=0;

for (uint8_t k = 0; k < 2; k++)
 {
  for (uint8_t j = 0; j < 5; j++)
   {
    for (uint8_t i = 0; i < 8; i++)
     {
       tft.fillRect((hSpacing*j)+2+(i*(tLineW+1)), 160-(tLineH+1+k*vSpacing), tLineW, tLineH, tMapColour(n*3+10));
       n++;
     }
   }
 }
}


void bitmap1()
{
  tft.writeRect(0, 0, 128, 160, SimpBMSBMW);
  tft.updateScreen();
}

void backgroundGrad() //grey graduation
{
  const int8_t bands = 80;
  const int8_t bandHeight = 160/bands;
  for (int8_t  bandCount = 0; bandCount < bands; bandCount++)
     {
       int shade565=ConvertRGBto565(160-(bandCount*160/bands),160-(bandCount*160/bands),160-(bandCount*160/bands));
       tft.fillRect(0, bandHeight*bandCount, 128, bandHeight, shade565); 
     }
}


void SOCtext(byte soc)
{
  tft.setFont(Arial_13);tft.setTextColor(ST7735_YELLOW);
  tft.drawString("SOC",1,2);
  if (soc<10) tft.drawNumber(soc,101,2);
  if ((soc>=10)&&(soc<100)) tft.drawNumber(soc,91,2);
  if (soc>99) tft.drawNumber(soc,81,2);
  tft.drawString("%",112,2);
}


void infoText()
{
  tft.setFont(Arial_12);tft.setTextColor(ST7735_RED);
  tft.drawNumber(45,3,124);
  tft.drawString("mV",23,124);

//Mode Display
  tft.setFont(Arial_12);tft.setTextColor(ST7735_GREEN);
  tft.drawString("M",55,124);
  tft.setTextColor(ST7735_GREEN);
  tft.drawNumber(2,70,124);  

  tft.setFont(Arial_12);tft.setTextColor(ST7735_RED);
  //tft.drawString("292",86,124);
  tft.drawNumber(volts,86,124);
  tft.drawString("V",115,124);

//Write Pack Current:
  tft.setFont(Arial_12);
  
  //positives: (providing power)
  if (amps>=0) tft.setTextColor(0xFD00); //orange
  if ((amps<10) &&(amps>=0))  tft.drawNumber(amps,24,107);
  if ((amps>=10)&&(amps<100)) tft.drawNumber(amps,15, 107);
  if (amps>99)                tft.drawNumber(amps,6, 107);

  //negatives: (being charged)
  if (amps<0) tft.setTextColor(ST7735_GREEN);
  if ((amps>-10) &&(amps<0))    tft.drawNumber(amps,20,107);
  if ((amps<=-10)&&(amps>-100)) tft.drawNumber(amps,11, 107);
  if (amps<-99)                 tft.drawNumber(amps,2, 107);  
  tft.drawString("A",35,107);

//Power:
  power=volts*amps/1000;
  tft.setFont(Arial_11);
  if (power>=0)
  {
    tft.setTextColor(0xFD00); //orange
    if(power>=10) 
    {
      tft.drawNumber(power,66,107);
    }
    else tft.drawNumber(power,73,107);
  }
  if (power<0)
  {
    tft.setTextColor(ST7735_GREEN);
    if (power>-10)
    {
      tft.drawNumber(power,67,107);
    }
    else tft.drawNumber(power,60,107);
  }
  tft.drawString("P",51,107);

//Temperature
  tft.setFont(Arial_12);tft.setTextColor(ST7735_RED);  
  tft.drawNumber(temp,89,107);
  tft.drawCircle(111,109,2,ST7735_RED);
  tft.drawString("C",115,107);

//Frame around text  
  tft.drawFastHLine(0,  121, 128, ST7735_BLUE);
  tft.drawFastHLine(0,  138, 128, ST7735_BLUE);
  tft.drawFastHLine(0,  159, 128, ST7735_BLUE);
  
  tft.drawFastVLine(0,  104,  55, ST7735_BLUE);
  tft.drawFastVLine(127,104,  55, ST7735_BLUE);
  tft.drawFastVLine(48,  104, 34, ST7735_BLUE);
  tft.drawFastVLine(83,  104, 34, ST7735_BLUE);  

//Display the delta Voltage of the cells in all the modules:
  moduleDV();

//Generate a changing input for SOC and Current  
  testInput+=2*b;
  if (testInput>99) b=-1;
  if (testInput<1) b=1;
}





uint16_t rndColour()
{
  uint8_t m=(uint8_t)random(0,255);
  return ConvertRGBto565(pgm_read_byte(&(tcmap[m][0])),pgm_read_byte(&(tcmap[m][1])),pgm_read_byte(&(tcmap[m][2])));
  
}


//***********************************
int ringMeter(int value, int vmin, int vmax, int x, int y, int r)
// Heavily modified from: https://www.instructables.com/id/Arduino-analogue-ring-meter-on-colour-TFT-display/
// Thank you Bodmer
// Uses float SINE and COSINE lookup table
{
  x += r; y += r;         // Calculate coords of centre of ring

  int w = r / 5;          // Width of outer ring is 1/4 of radius
  
  const int angle = 110;  // DO NOT INCREASE OVER 120! Half the sweep angle of gauge

  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v

  // DO NOT CHANGE!
  byte seg = 1; // Segment width in deg
  byte inc = 1; // Draw segments every x degrees

  for (int i = -angle; i < angle; i += inc)
  //for (int i = 0; i < 360; i++) 
   {
    // Choose colour for ring based on SOC value:
    if (value<20) colour  = ST7735_RED;
    if (value>=20) colour = 0x27EA;  //bright green
    if (value>=80) colour = 0x27DF;  //light blue green

    // Calculate pair of coordinates for segment start
    
    //float sx = cos((i - 90) * 0.0174532925);
    float sx = cosineL[abs(i - 90)];  
   
    //float sy = sin((i - 90) * 0.0174532925);
    float sy;
    if ((i-90)>=0) sy = sineL[i - 90];
    if ( (i-90)<0) sy = sineL[360+(i - 90)];
              
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    
    uint16_t x1 = sx * r + x;
    
    uint16_t y1 = sy * r + y;
    
    // Calculate pair of coordinates for segment end
    //float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sx2 = cosineL[abs(i + seg - 90)];
        
    //float sy2 = sin((i + seg - 90) * 0.0174532925);
    float sy2; 
    if ((i + seg - 90)>=0) sy2 = sineL[i + seg - 90];
    if ( (i + seg - 90)<0) sy2 = sineL[360+(i + seg - 90)];
    
    //Serial.print(i);
    //Serial.print(",");
    //Serial.print((double)sy2,4);
    //Serial.println(",");
    
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v) 
    { // Fill in coloured segments with 2 triangles
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
    }
    else // Fill in blank segments
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, 0x6B4D); //grey=0x6B4D
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, 0x6B4D);
    }    
  }
// put on tick marks:

 inc=angle*2/10; //(10 tick marks in 100% gauge)
 for (int i = -angle; i < angle+inc; i += inc)
  {
    
    // Calculate pair of coordinates for segment start
    float sx = cosineL[abs(i - 90)];

    //float sy = sin((i - 90) * 0.0174532925);
    float sy;
    if ((i - 90)>=0) sy = sineL[i - 90];
    if ( (i - 90)<0) sy = sineL[360+(i - 90)];
    
    uint16_t x0 = sx * (r - w/2) + x;
    uint16_t y0 = sy * (r - w/2) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    tft.drawLine(x0, y0, x1, y1,ST7735_BLUE);
  }
 inc=angle*2/200; //(100 point around the gauge)
 
 for (int i = -angle; i < angle+inc; i += inc)
  {
    // Calculate pair of coordinates for segment start
    //float sx = cos((i - 90) * 0.0174532925);
    float sx = cosineL[abs(i - 90)];
    
    //float sy = sin((i - 90) * 0.0174532925);
    float sy;
    if ((i - 90)>=0) sy = sineL[i - 90];
    if ( (i - 90)<0) sy = sineL[360+(i - 90)];
    
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    tft.drawPixel(x0, y0,ST7735_BLUE);
    tft.drawPixel(x1, y1,ST7735_BLUE);
  }

//Axis text
  tft.setFont(Arial_8);tft.setTextColor(0xFD81); //orange
  tft.drawString("0%",14,83);
  tft.drawString("50%",57,1);
  tft.drawString("100%",92,83);

//Put on pointer triangle

    // Calculate pair of coordinates for segment start
    //float sx = cos((v - 90) * 0.0174532925);
    float sx = cosineL[abs(v - 90)];
    
    //float sy = sin((v - 90) * 0.0174532925);   
    float sy;
    if ((v - 90)>=0) sy = sineL[v - 90];
    if ( (v - 90)<0) sy = sineL[360+(v - 90)];
    
    
    uint16_t x0 = sx * (r - w) + x; //on inside line
    uint16_t y0 = sy * (r - w) + y;

    //sx = cos((v - 90-5) * 0.0174532925);
    sx = cosineL[abs(v - 90-5)];
     
    //sy = sin((v - 90-5) * 0.0174532925);
    if ((v - 90-5)>=0) sy = sineL[v - 90-5];
    if ( (v - 90-5)<0) sy = sineL[360+(v - 90-5)];
    
    uint16_t x1 = sx * (r-w-6)+ x; //pointer is 4 long
    uint16_t y1 = sy * (r-w-6)+ y;

    //sx = cos((v - 90+5) * 0.0174532925);
    sx = cosineL[abs(v - 90+5)];
    //sy = sin((v - 90+5) * 0.0174532925);
    if ((v - 90+5)>=0) sy = sineL[v - 90+5];
    if ( (v - 90+5)<0) sy = sineL[360+(v - 90+5)];
     
    uint16_t x2 = sx * (r-w-6)+ x; //pointer is 4 long
    uint16_t y2 = sy * (r-w-6)+ y;

    tft.fillTriangle(x0, y0, x1, y1, x2, y2, ST7735_YELLOW);
}
//**************************************
void SOCmeter(int soc)
{
  ringMeter(soc, 0, 100, 14, 12, 50);  
}


void currentMeter(int c)
{
  const byte v=104; //vertical base line position
  const byte h=9; //indicies height
  
// draw bar: 
  int value=map(c,-200, 200, -63, 63);
  if (value>0) tft.fillRect(64, v-h+3, value, h-3, 0xFD81); //orange
  if (value<0) tft.fillRect(64+value, v-h+3, -value, h-3, ST7735_GREEN);

//Incicies, pointer and center point last:
  tft.drawFastHLine(0, v, 128, ST7735_BLUE);
  tft.drawFastHLine(0, v-h+2, 128, ST7735_BLUE); 
  
  for (int n = 0; n < 5; n++) //incicies large
   {
    if (n<2) tft.drawFastVLine(32*n, v-h+2, h-2,ST7735_BLUE); //large 100s
    if (n>2) tft.drawFastVLine((32*n)-1, v-h+2, h-2,ST7735_BLUE); //large 100s
   }
  
  for (int m = 0; m < 4; m++) //incicies small
   {
    tft.drawFastVLine(16+32*m, v-h+5, h-5,ST7735_RED); //small 50s
   }
  
  tft.drawFastVLine(64, v-h, h+1, ST7735_WHITE); //center mark 0A
  tft.fillTriangle(64+value, v-h+3, 64+value-2, v-h, 64+value+2, v-h, ST7735_YELLOW);
}

void deltaTempGraph()
{
  //Input module delta T in degC*10
  int8_t moduleTempsIn[10]={0,5,15,0,-11,-21,26,13,-2,-11,};
  
  //Positioning Variables:
  int8_t tMin=-30; //temp *10
  int8_t tMax=30;  //temp *10
  int8_t vMin=44;
  int8_t vLength=6*(tMax-tMin)/10; //must be divisible for indicies to line up
  int8_t vMax=vMin+vLength;
  int8_t hor[5]={39,51,63,75,87,}; //Horizontal Position of lines
  int8_t pointerW=3; // pointer width
  int8_t frameSpacing=2;

  for(int8_t h=0; h<5; h++)
  {
     tft.drawFastVLine(hor[h], vMin, vLength,ST7735_BLUE); //   
  }

//Frames:
  for(int8_t v=0; v<5; v++)
   {
     tft.drawFastVLine(hor[v]-frameSpacing, vMin, vLength,ST7735_BLUE);
     tft.drawFastVLine(hor[v]+frameSpacing, vMin, vLength,ST7735_BLUE);
   }

//Horizontal Incicies:  
  for(int8_t h=0; h<5; h++)
  {
    for (int8_t i=0;i<((tMax-tMin)/10)+1;i++)
     {
      tft.drawFastHLine(hor[h]-2, vMin+(vLength/((tMax-tMin)/10))*i, 6, ST7735_RED); // 
     }
  }
  tft.drawFastHLine(36, vMin+vLength/2, 56, ST7735_WHITE); //  

//Text on Indicies:
  tft.setFont(Arial_8);
  tft.setTextColor(ST7735_RED); //orange
  tft.drawNumber(tMax/10,hor[2]-4,vMin-10);
  tft.drawCircle(hor[2]+4,vMin-9,1,ST7735_RED); //orange
  tft.drawString("C",hor[2]+7,vMin-10);
  
  tft.setTextColor(ST7735_BLUE); //orange
  tft.drawNumber(tMin/10,hor[2]-8,vMax+4);
  tft.drawCircle(hor[2]+4,vMax+3,1,ST7735_BLUE); //orange
  tft.drawString("C",hor[2]+7,vMax+4);  

//Pointer: 
 int8_t scaledTemps[10];

 for(int8_t v=0; v<10; v++)
   {
      scaledTemps[v]=map(moduleTempsIn[v],tMax,tMin,vMin,vMax);
   }
 for(int8_t p=0; p<5; p++)
   {
      tft.fillTriangle(hor[p], scaledTemps[p*2], hor[p]-pointerW, scaledTemps[p*2]-3, hor[p]-pointerW, scaledTemps[p*2]+3, ST7735_YELLOW);
      tft.fillTriangle(hor[p]+1, scaledTemps[p*2+1], hor[p]+pointerW+1, scaledTemps[p*2+1]-3, hor[p]+pointerW+1, scaledTemps[p*2+1]+3, ST7735_YELLOW);
   }
}

void contactorStatus()
//Draw box on screen with contactor status grey/green with 
{
  tft.fillRect(0, 20, 10, 28, 0x21D4);      //BLUE contactor off
  tft.setFont(Arial_8);
  tft.setTextColor(0xFD81); //orange
  tft.drawString("P",1,21);
  tft.drawString("r",2,29);
  tft.drawString("E",1,39);

  tft.fillRect(0, 50, 10, 36, ST7735_GREEN);      //BLUE contactor off
  tft.setFont(Arial_8);
  tft.setTextColor(0xFD81); //orange
  tft.drawString("C",1,21+30);
  tft.drawString("o",2,29+30);
  tft.drawString("o",2,38+30);
  tft.drawString("L",1,47+30);
  
  tft.fillRect(118, 22, 10, 30, ST7735_GREEN); //GREEN contactor on
  tft.setTextColor(0xFD81); //orange
  tft.drawString("M",120,25);
  tft.drawString("n",121,33);
  tft.drawString("+",121,42);

  tft.fillRect(118, 53, 10, 30, ST7735_GREEN); //GREEN contactor on
  tft.setTextColor(0xFD81); //orange
  tft.drawString("M",120,23+34);
  tft.drawString("n",121,31+34);
  tft.drawString("-",122,39+34);
}

void ScreenSetup() 
{
  tft.initR(INITR_BLACKTAB);
  tft.useFrameBuffer(true);
  //bool  updateScreenAsync(bool update_cont = false);
  tft.setRotation(2);
  tft.fillScreen(ST7735_BLACK);
  bitmap1(); // splash Logo
  delay(1000);
}


void ScreenLoop() 
{

if(millis()>=(i+1000))
 {
  ti=millis();

      backgroundGrad();

      SOCtext(testInput);
      SOCmeter(testInput);
      infoText();
      
      amps=testInput*4-200;
      
      currentMeter(amps);
      deltaTempGraph();
      contactorStatus();

      tft.updateScreen();
      to=millis()-ti;
      i=millis();
 }
}
 
