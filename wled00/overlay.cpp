#include "wled.h"

/*
 * Used to draw clock overlays over the strip
 */
 
void initCronixie()
{
  if (overlayCurrent == 3 && !cronixieInit)
  {
    setCronixie();
    strip.getSegment(0).grouping = 10; //10 LEDs per digit
    cronixieInit = true;
  } else if (cronixieInit && overlayCurrent != 3)
  {
    strip.getSegment(0).grouping = 1;
    cronixieInit = false; 
  }
}


void handleOverlays()
{
  if (millis() - overlayRefreshedTime > overlayRefreshMs)
  {
    initCronixie();
    updateLocalTime();
    checkTimers();
    checkCountdown();
    if (overlayCurrent == 3) _overlayCronixie();//Diamex cronixie clock kit
    overlayRefreshedTime = millis();
  }
}


void _overlayAnalogClock()
{
  int overlaySize = overlayMax - overlayMin +1;
  if (countdownMode)
  {
    _overlayAnalogCountdown(); return;
  }
  double hourP = ((double)(hour(localTime)%12))/12;
  double minuteP = ((double)minute(localTime))/60;
  hourP = hourP + minuteP/12;
  double secondP = ((double)second(localTime))/60;
  int hourPixel = floor(analogClock12pixel + overlaySize*hourP);
  if (hourPixel > overlayMax) hourPixel = overlayMin -1 + hourPixel - overlayMax;
  int minutePixel = floor(analogClock12pixel + overlaySize*minuteP);
  if (minutePixel > overlayMax) minutePixel = overlayMin -1 + minutePixel - overlayMax; 
  int secondPixel = floor(analogClock12pixel + overlaySize*secondP);
  if (secondPixel > overlayMax) secondPixel = overlayMin -1 + secondPixel - overlayMax;
  if (analogClockSecondsTrail)
  {
    if (secondPixel < analogClock12pixel)
    {
      strip.setRange(analogClock12pixel, overlayMax, 0xFF0000);
      strip.setRange(overlayMin, secondPixel, 0xFF0000);
    } else
    {
      strip.setRange(analogClock12pixel, secondPixel, 0xFF0000);
    }
  }
  if (analogClock5MinuteMarks)
  {
    int pix;
    for (int i = 0; i <= 12; i++)
    {
      pix = analogClock12pixel + round((overlaySize / 12.0) *i);
      if (pix > overlayMax) pix -= overlaySize;
      strip.setPixelColor(pix, 0x00FFAA);
    }
  }
  if (!analogClockSecondsTrail) strip.setPixelColor(secondPixel, 0xFF0000);
  strip.setPixelColor(minutePixel, 0x00FF00);
  strip.setPixelColor(hourPixel, 0x0000FF);
  overlayRefreshMs = 998;
}


void _overlayAnalogCountdown()
{
  if (now() < countdownTime)
  {
    long diff = countdownTime - now();
    double pval = 60;
    if (diff > 31557600L) //display in years if more than 365 days
    {
      pval = 315576000L; //10 years
    } else if (diff > 2592000L) //display in months if more than a month
    {
      pval = 31557600L; //1 year
    } else if (diff > 604800) //display in weeks if more than a week
    {
      pval = 2592000L; //1 month
    } else if (diff > 86400) //display in days if more than 24 hours
    {
      pval = 604800; //1 week
    } else if (diff > 3600) //display in hours if more than 60 minutes
    {
      pval = 86400; //1 day
    } else if (diff > 60) //display in minutes if more than 60 seconds
    {
      pval = 3600; //1 hour
    }
    int overlaySize = overlayMax - overlayMin +1;
    double perc = (pval-(double)diff)/pval;
    if (perc > 1.0) perc = 1.0;
    byte pixelCnt = perc*overlaySize;
    if (analogClock12pixel + pixelCnt > overlayMax)
    {
      strip.setRange(analogClock12pixel, overlayMax, ((uint32_t)colSec[3] << 24)| ((uint32_t)colSec[0] << 16) | ((uint32_t)colSec[1] << 8) | colSec[2]);
      strip.setRange(overlayMin, overlayMin +pixelCnt -(1+ overlayMax -analogClock12pixel), ((uint32_t)colSec[3] << 24)| ((uint32_t)colSec[0] << 16) | ((uint32_t)colSec[1] << 8) | colSec[2]);
    } else
    {
      strip.setRange(analogClock12pixel, analogClock12pixel + pixelCnt, ((uint32_t)colSec[3] << 24)| ((uint32_t)colSec[0] << 16) | ((uint32_t)colSec[1] << 8) | colSec[2]);
    }
  }
  overlayRefreshMs = 998;
}


void handleOverlayDraw() {
  if (!overlayCurrent) return;
  switch (overlayCurrent)
  {
    case 1: _overlayAnalogClock(); break;
    case 2: _overlayMatrixClock(); break;
    case 3: _drawOverlayCronixie(); break;
  }
}


/*
 * Support for the Cronixie clock
 */
 
#ifndef WLED_DISABLE_CRONIXIE
byte _digitOut[6] = {10,10,10,10,10,10};
 
byte getSameCodeLength(char code, int index, char const cronixieDisplay[])
{
  byte counter = 0;
  
  for (int i = index+1; i < 6; i++)
  {
    if (cronixieDisplay[i] == code)
    {
      counter++;
    } else {
      return counter;
    }
  }
  return counter;
}

void setCronixie()
{
  /*
   * digit purpose index
   * 0-9 | 0-9 (incl. random)
   * 10 | blank
   * 11 | blank, bg off
   * 12 | test upw.
   * 13 | test dnw.
   * 14 | binary AM/PM
   * 15 | BB upper +50 for no trailing 0
   * 16 | BBB
   * 17 | BBBB
   * 18 | BBBBB
   * 19 | BBBBBB
   * 20 | H
   * 21 | HH
   * 22 | HHH
   * 23 | HHHH
   * 24 | M
   * 25 | MM
   * 26 | MMM
   * 27 | MMMM
   * 28 | MMMMM
   * 29 | MMMMMM
   * 30 | S
   * 31 | SS
   * 32 | SSS
   * 33 | SSSS
   * 34 | SSSSS
   * 35 | SSSSSS
   * 36 | Y
   * 37 | YY
   * 38 | YYYY
   * 39 | I
   * 40 | II
   * 41 | W
   * 42 | WW
   * 43 | D
   * 44 | DD
   * 45 | DDD
   * 46 | V
   * 47 | VV
   * 48 | VVV
   * 49 | VVVV
   * 50 | VVVVV
   * 51 | VVVVVV
   * 52 | v
   * 53 | vv
   * 54 | vvv
   * 55 | vvvv
   * 56 | vvvvv
   * 57 | vvvvvv
   */

  //H HourLower | HH - Hour 24. | AH - Hour 12. | HHH Hour of Month | HHHH Hour of Year
  //M MinuteUpper | MM Minute of Hour | MMM Minute of 12h | MMMM Minute of Day | MMMMM Minute of Month | MMMMMM Minute of Year
  //S SecondUpper | SS Second of Minute | SSS Second of 10 Minute | SSSS Second of Hour | SSSSS Second of Day | SSSSSS Second of Week
  //B AM/PM | BB 0-6/6-12/12-18/18-24 | BBB 0-3... | BBBB 0-1.5... | BBBBB 0-1 | BBBBBB 0-0.5
  
  //Y YearLower | YY - Year LU | YYYY - Std.
  //I MonthLower | II - Month of Year 
  //W Week of Month | WW Week of Year
  //D Day of Week | DD Day Of Month | DDD Day Of Year

  DEBUG_PRINT("cset ");
  DEBUG_PRINTLN(cronixieDisplay);

  overlayRefreshMs = 1997; //Only refresh every 2secs if no seconds are displayed
  
  for (int i = 0; i < 6; i++)
  {
    dP[i] = 10;
    switch (cronixieDisplay[i])
    {
      case '_': dP[i] = 10; break; 
      case '-': dP[i] = 11; break; 
      case 'r': dP[i] = random(1,7); break; //random btw. 1-6
      case 'R': dP[i] = random(0,10); break; //random btw. 0-9
      //case 't': break; //Test upw.
      //case 'T': break; //Test dnw.
      case 'b': dP[i] = 14 + getSameCodeLength('b',i,cronixieDisplay); i = i+dP[i]-14; break; 
      case 'B': dP[i] = 14 + getSameCodeLength('B',i,cronixieDisplay); i = i+dP[i]-14; break;
      case 'h': dP[i] = 70 + getSameCodeLength('h',i,cronixieDisplay); i = i+dP[i]-70; break;
      case 'H': dP[i] = 20 + getSameCodeLength('H',i,cronixieDisplay); i = i+dP[i]-20; break;
      case 'A': dP[i] = 108; i++; break;
      case 'a': dP[i] = 58; i++; break;
      case 'm': dP[i] = 74 + getSameCodeLength('m',i,cronixieDisplay); i = i+dP[i]-74; break;
      case 'M': dP[i] = 24 + getSameCodeLength('M',i,cronixieDisplay); i = i+dP[i]-24; break;
      case 's': dP[i] = 80 + getSameCodeLength('s',i,cronixieDisplay); i = i+dP[i]-80; overlayRefreshMs = 497; break; //refresh more often bc. of secs
      case 'S': dP[i] = 30 + getSameCodeLength('S',i,cronixieDisplay); i = i+dP[i]-30; overlayRefreshMs = 497; break;
      case 'Y': dP[i] = 36 + getSameCodeLength('Y',i,cronixieDisplay); i = i+dP[i]-36; break; 
      case 'y': dP[i] = 86 + getSameCodeLength('y',i,cronixieDisplay); i = i+dP[i]-86; break; 
      case 'I': dP[i] = 39 + getSameCodeLength('I',i,cronixieDisplay); i = i+dP[i]-39; break;  //Month. Don't ask me why month and minute both start with M.
      case 'i': dP[i] = 89 + getSameCodeLength('i',i,cronixieDisplay); i = i+dP[i]-89; break; 
      //case 'W': break;
      //case 'w': break;
      case 'D': dP[i] = 43 + getSameCodeLength('D',i,cronixieDisplay); i = i+dP[i]-43; break;
      case 'd': dP[i] = 93 + getSameCodeLength('d',i,cronixieDisplay); i = i+dP[i]-93; break;
      case '0': dP[i] = 0; break;
      case '1': dP[i] = 1; break;
      case '2': dP[i] = 2; break;
      case '3': dP[i] = 3; break;
      case '4': dP[i] = 4; break;
      case '5': dP[i] = 5; break;
      case '6': dP[i] = 6; break;
      case '7': dP[i] = 7; break;
      case '8': dP[i] = 8; break;
      case '9': dP[i] = 9; break;
      //case 'V': break; //user var0
      //case 'v': break; //user var1
    }
  }
  DEBUG_PRINT("result ");
  for (int i = 0; i < 5; i++)
  {
    DEBUG_PRINT((int)dP[i]);
    DEBUG_PRINT(" ");
  }
  DEBUG_PRINTLN((int)dP[5]);

  _overlayCronixie(); //refresh
}

void _overlayCronixie()
{
  byte h = hour(localTime);
  byte h0 = h;
  byte m = minute(localTime);
  byte s = second(localTime);
  byte d = day(localTime);
  byte mi = month(localTime);
  int y = year(localTime);
  //this has to be changed in time for 22nd century
  y -= 2000; if (y<0) y += 30; //makes countdown work

  if (useAMPM && !countdownMode)
  {
    if (h>12) h-=12;
    else if (h==0) h+=12;
  }
  for (int i = 0; i < 6; i++)
  {
    if (dP[i] < 12) _digitOut[i] = dP[i];
    else {
      if (dP[i] < 65)
      {
        switch(dP[i])
        {
          case 21: _digitOut[i] = h/10; _digitOut[i+1] = h- _digitOut[i]*10; i++; break; //HH
          case 25: _digitOut[i] = m/10; _digitOut[i+1] = m- _digitOut[i]*10; i++; break; //MM
          case 31: _digitOut[i] = s/10; _digitOut[i+1] = s- _digitOut[i]*10; i++; break; //SS

          case 20: _digitOut[i] = h- (h/10)*10; break; //H
          case 24: _digitOut[i] = m/10; break; //M
          case 30: _digitOut[i] = s/10; break; //S
          
          case 43: _digitOut[i] = weekday(localTime); _digitOut[i]--; if (_digitOut[i]<1) _digitOut[i]= 7; break; //D
          case 44: _digitOut[i] = d/10; _digitOut[i+1] = d- _digitOut[i]*10; i++; break; //DD
          case 40: _digitOut[i] = mi/10; _digitOut[i+1] = mi- _digitOut[i]*10; i++; break; //II
          case 37: _digitOut[i] = y/10; _digitOut[i+1] = y- _digitOut[i]*10; i++; break; //YY
          case 39: _digitOut[i] = 2; _digitOut[i+1] = 0; _digitOut[i+2] = y/10; _digitOut[i+3] = y- _digitOut[i+2]*10; i+=3; break; //YYYY
          
          //case 16: _digitOut[i+2] = ((h0/3)&1)?1:0; i++; //BBB (BBBB NI)
          //case 15: _digitOut[i+1] = (h0>17 || (h0>5 && h0<12))?1:0; i++; //BB
          case 14: _digitOut[i] = (h0>11)?1:0; break; //B
        }
      } else
      {
        switch(dP[i])
        {
          case 71: _digitOut[i] = h/10; _digitOut[i+1] = h- _digitOut[i]*10; if(_digitOut[i] == 0) _digitOut[i]=10; i++; break; //hh
          case 75: _digitOut[i] = m/10; _digitOut[i+1] = m- _digitOut[i]*10; if(_digitOut[i] == 0) _digitOut[i]=10; i++; break; //mm
          case 81: _digitOut[i] = s/10; _digitOut[i+1] = s- _digitOut[i]*10; if(_digitOut[i] == 0) _digitOut[i]=10; i++; break; //ss
          //case 66: _digitOut[i+2] = ((h0/3)&1)?1:10; i++; //bbb (bbbb NI)
          //case 65: _digitOut[i+1] = (h0>17 || (h0>5 && h0<12))?1:10; i++; //bb
          case 64: _digitOut[i] = (h0>11)?1:10; break; //b

          case 93: _digitOut[i] = weekday(localTime); _digitOut[i]--; if (_digitOut[i]<1) _digitOut[i]= 7; break; //d
          case 94: _digitOut[i] = d/10; _digitOut[i+1] = d- _digitOut[i]*10; if(_digitOut[i] == 0) _digitOut[i]=10; i++; break; //dd
          case 90: _digitOut[i] = mi/10; _digitOut[i+1] = mi- _digitOut[i]*10; if(_digitOut[i] == 0) _digitOut[i]=10; i++; break; //ii
          case 87: _digitOut[i] = y/10; _digitOut[i+1] = y- _digitOut[i]*10; i++; break; //yy
          case 89: _digitOut[i] = 2; _digitOut[i+1] = 0; _digitOut[i+2] = y/10; _digitOut[i+3] = y- _digitOut[i+2]*10; i+=3; break; //yyyy
        }
      }
    }
  }
}

void _drawOverlayCronixie()
{
  byte offsets[] = {5, 0, 6, 1, 7, 2, 8, 3, 9, 4};
  
  for (uint16_t i = 0; i < 6; i++)
  {
    byte o = 10*i;
    byte excl = 10;
    if(_digitOut[i] < 10) excl = offsets[_digitOut[i]];
    excl += o;
    
    if (cronixieBacklight && _digitOut[i] <11)
    {
      uint32_t col = strip.gamma32(strip.getSegment(0).colors[1]);
      for (uint16_t j=o; j< o+10; j++) {
        if (j != excl) strip.setPixelColor(j, col);
      }
    } else
    {
      for (uint16_t j=o; j< o+10; j++) {
        if (j != excl) strip.setPixelColor(j, 0);
      }
    }
  }
}

#else // WLED_DISABLE_CRONIXIE
byte getSameCodeLength(char code, int index, char const cronixieDisplay[]) {}
void setCronixie() {}
void _overlayCronixie() {}
void _drawOverlayCronixie() {}
#endif

/*
 * Support for matrix clock
 */

void drawMatrixNumber(uint8_t x_offset, uint8_t num){
  switch(num){
    case(0):
      drawMatrixNumber(x_offset, 8);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset+1, 2), 0);
    break;
    case(1):
      for(uint8_t y=1; y<4; y++){
        strip.setPixelColor(strip.matrixXYToIndex(x_offset, y), 0);
        strip.setPixelColor(strip.matrixXYToIndex(x_offset+2, y), 0);
      }
      strip.setPixelColor(strip.matrixXYToIndex(x_offset+2, 4), 0);
    break;     
    case(2):
      drawMatrixNumber(x_offset, 8);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset, 3), 0);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset+2, 1), 0);
    break;
    case(3):
      drawMatrixNumber(x_offset, 8);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset, 1), 0);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset, 3), 0);
    break;
    case(4):
      drawMatrixNumber(x_offset, 8);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset+1, 4), 0);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset+1, 0), 0);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset, 0), 0);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset, 1), 0);
    break;
    case(5):
      drawMatrixNumber(x_offset, 6);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset, 1), 0);
    break;
    case(6):
      drawMatrixNumber(x_offset, 8);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset+2, 3), 0);
    break;
    case(7):
      drawMatrixNumber(x_offset, 3);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset, 0), 0);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset+1, 0), 0);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset, 2), 0);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset+1, 2), 0);
    break;
    case(8):
      strip.setPixelColor(strip.matrixXYToIndex(x_offset+1, 1), 0);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset+1, 3), 0);
    break;
    case(9):
      drawMatrixNumber(x_offset, 8);
      strip.setPixelColor(strip.matrixXYToIndex(x_offset, 1), 0);
    break;
  }
}

void overlayMatrixNums(uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4){
  bool matrixColonShow = false;
  
  if((millis() & 512) > 500) matrixColonShow = true;

  drawMatrixNumber(0, val1);
  drawMatrixNumber(4, val2);
  drawMatrixNumber(10, val3);
  drawMatrixNumber(14, val4);
  
  if(matrixColonShow){
    strip.setPixelColor(strip.matrixXYToIndex(8, 0), 0);
    strip.setPixelColor(strip.matrixXYToIndex(8, 2), 0);
    strip.setPixelColor(strip.matrixXYToIndex(8, 4), 0);
  }
  for(uint8_t y = 0; y < strip.matrixHeight; y++){
    strip.setPixelColor(strip.matrixXYToIndex(3, y), 0);
    strip.setPixelColor(strip.matrixXYToIndex(7, y), 0);
    if(!matrixColonShow) strip.setPixelColor(strip.matrixXYToIndex(8, y), 0);
    strip.setPixelColor(strip.matrixXYToIndex(9, y), 0);
    strip.setPixelColor(strip.matrixXYToIndex(13, y), 0);
    }
}

void matrixClockCountdown(){
  // DEBUG_PRINT("Countdown time: ");
 // DEBUG_PRINTLN(countdownTime);

  if (now() < countdownTime)
  {
    uint32_t diff = countdownTime - now();
    if (diff >= 31536000L) //display in years if more than 365 days
    {
      uint32_t years = diff/31536000L;
      
      drawMatrixNumber(1, years/100);
      drawMatrixNumber(5, (years%100)/10);
      drawMatrixNumber(9, (years%100)%10);
      strip.setPixelColumnColor(0, 0);
      strip.setPixelColumnColor(4, 0);
      strip.setPixelColumnColor(8, 0);
      strip.setPixelColumnColor(12, 0);
      
      // Draw y
      strip.setPixelColor(strip.matrixXYToIndex(13, 0), 0);
      strip.setPixelColor(strip.matrixXYToIndex(13, 1), 0);
      strip.setPixelColor(strip.matrixXYToIndex(13, 4), 0);
      strip.setPixelColor(strip.matrixXYToIndex(14, 2), 0);
      strip.setPixelColor(strip.matrixXYToIndex(14, 3), 0);
      strip.setPixelColor(strip.matrixXYToIndex(14, 4), 0);
      strip.setPixelColor(strip.matrixXYToIndex(15, 0), 0);
      strip.setPixelColor(strip.matrixXYToIndex(15, 1), 0);
      strip.setPixelColor(strip.matrixXYToIndex(15, 4), 0);
      strip.setPixelColumnColor(16, 0);
      
    } else if (diff >= 86400) //display in days if more than 24 hours
    {
      uint32_t days = diff/86400;
      drawMatrixNumber(1, days/100);
      drawMatrixNumber(5, (days%100)/10);
      drawMatrixNumber(9, (days%100)%10);
      strip.setPixelColumnColor(0, 0);
      strip.setPixelColumnColor(4, 0);
      strip.setPixelColumnColor(8, 0);
      strip.setPixelColumnColor(12, 0);
    
      // Draw D
      strip.setPixelColor(strip.matrixXYToIndex(13, 4), 0);
      strip.setPixelColor(strip.matrixXYToIndex(14, 1), 0);
      strip.setPixelColor(strip.matrixXYToIndex(14, 2), 0);
      strip.setPixelColor(strip.matrixXYToIndex(14, 4), 0);
      strip.setPixelColor(strip.matrixXYToIndex(15, 0), 0);
      strip.setPixelColor(strip.matrixXYToIndex(15, 3), 0);
      strip.setPixelColor(strip.matrixXYToIndex(15, 4), 0);   

      strip.setPixelColumnColor(16, 0);   
    } 
    else if(diff >= 3600){  // Display in hours and minutes
      uint16_t hours = diff/3600;
      uint16_t minutes = (diff - hours*3600)/60;
      overlayMatrixNums(hours/10, hours%10, minutes/10, minutes%10);
    }
    else{  // Minutes and seconds
      uint16_t minutes = diff/60;
      uint16_t seconds = diff - minutes*60;

      overlayMatrixNums(minutes/10, minutes%10, seconds/10, seconds%10);
    }
  }
  else overlayMatrixNums(0, 0, 0, 0);
}

void _overlayMatrixClock(){
  if(!countdownMode){
    byte h = hour(localTime);
    byte m = minute(localTime);
    if (useAMPM){
      if (h>12) h-=12;
      else if (h==0) h+=12;
    }

    overlayMatrixNums(h/10, h%10, m/10, m%10);
  } 
  else matrixClockCountdown();
}
