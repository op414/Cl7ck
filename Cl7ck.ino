/*
 * Cl7ck
 * Author : op414
 * http://op414.net
 * License : CC BY-NC-SA http://creativecommons.org/licenses/by-nc-sa/3.0/
 * ------------
 * Cl7ck.ino : Main file
 * Created 11 may 2013
 */

#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>
#include <avr/pgmspace.h>

#include "settings.h"
#include "Display.h"
#include "font.h"
#include "TimeHandler.h"
#include "InputsHandler.h"
#include "TempSensor.h"

prog_char fontSet[FONT_SET_SIZE] PROGMEM = {
  f_0,f_1,f_2,f_3,f_4,f_5,f_6,f_7,f_8,f_9,
  f_A,f_B,f_C,f_D,f_E,f_F,f_G,f_H,f_I,f_J,f_K,f_L,f_M,f_N,f_O,f_P,f_Q,f_R,f_S,f_T,f_U,f_V,f_W,f_X,f_Y,f_Z,
  f_a,f_b,f_c,f_d,f_e,f_f,f_g,f_h,f_i,f_j,f_k,f_l,f_m,f_n,f_o,f_p,f_q,f_r,f_s,f_t,f_u,f_v,f_w,f_x,f_y,f_z};

// Lambda enumeration for modes
enum{TIME = 0, DATE = 1, TEMP = 2, MENU = 3};
enum{ALARM = 0, SET_BRIGHTNESS = 1, SET_TIME = 2, SET_DATE = 3};
enum{SET_ALARM_ON_OFF = 0, SET_ALARM_TIME = 1};

#define STATE_LEVELS 10

Display disp(fontSet);
TimeHandler rtc;
InputsHandler inputs;
TempSensor ts;

int mode[STATE_LEVELS];
int level = 0;
boolean firstCall = true;
boolean quickGoBack = false; // Modes set it to false when a complicated action is needed when back is pressed
boolean quickGoToNextLevel = false; // Modes set it to false when a complicated action is needed when ok is pressed

void setup()
{
  #ifdef DEBUG
    Serial.begin(9600);
  #endif

  // Initialize RTC   
  rtc.initializeRTC();

  // Initialize the state array
  for(int i = 0 ; i < STATE_LEVELS ; i++)
    mode[i] = 0;

  disp.clearBuffer();
  disp.setString(0, "Hello");
  disp.display();
  delay(1000);
  disp.clearBuffer();
}

void loop()
{
  inputs.updateButtonStates();
  if(rtc.checkAlarm())
  {
    rtc.ring();
    
    if(level != 0 || mode[0] != TIME)
    {
      firstCall = true;
      disp.clearBuffer();
      level = 0;
      mode[0] = TIME;
    }
  }

  // ----------- Change Mode -------------
  if(level == 0 && inputs.pullButtonPress(OK, 1000))
  {
    disp.clearBuffer();

    mode[0] = MENU;
    mode[1] = ALARM;
    firstCall = true;
  }

  if(level == 0 && mode[0] != MENU && inputs.pullButtonPress(OK))
  {
    firstCall = true;
    disp.clearBuffer();
    mode[0] = (mode[0] + 1) % 3;
  }

  // ---------- Modes handling ------------
  if(level == 0 &&  mode[0] == TIME) // ----------------- Time --------------
  {
    quickGoBack = false;
    quickGoToNextLevel = false;
    
    if(inputs.pullButtonPress(BACK))
      rtc.stopAlarm();

    if(rtc.updateTime())
    {
      DateTime t = rtc.getTime();

      disp.setString(0, t.hours, 2);
      disp.setString(3, t.mins, 2);
      disp.setString(6, t.secs, 2);
      
      disp.setDP(7, rtc.getAlarm()); // Dot to indicate when alarm is on/off
      disp.display();
    }
  }
  else if(level == 0 && mode[0] == DATE) // -------------- Date -------------
  {
    quickGoBack = false;
    quickGoToNextLevel = false;

    if(rtc.updateTime())
    {
      DateTime t = rtc.getTime();

      disp.setString(0, t.DOM, 2);
      disp.setString(3, t.month, 2);
      disp.setString(6, t.year, 2);

      disp.setString(2, '-');
      disp.setString(5, '-');
      disp.setDP(7, rtc.getAlarm()); // Dot to indicate when alarm is on/off
      disp.display();
    }
  }
  else if(level == 0 && mode[0] == TEMP) // ------------ Temperature ---------
  {
    quickGoBack = false;
    quickGoToNextLevel = false;
    
    if(firstCall)
    {
      disp.setString(0,"----  @C");
      disp.setDP(1, true);
    }
    
    if(ts.updateTemp() || firstCall)
    {
      firstCall = false;
      
      disp.setString(0, ts.getWhole(), 2);
      disp.setString(2, ts.getFract(), 2);
      
      disp.display();
    }
  }
  else if(level == 0 && mode[0] == MENU) // -------------- Menu --------------
  {
    boolean refresh = false;
    quickGoToNextLevel = true;
    quickGoBack = false;

    if(firstCall)
    {
      refresh = true;
      firstCall = false;
      disp.setDP(7, false); // Remove the alarm indicator dot
    }

    if(inputs.pullButtonPress(MINUS))
    {
      refresh = true;
      mode[level + 1] = (mode[level + 1] == 0) ? SET_DATE : (mode[level + 1] - 1);
    }

    if(inputs.pullButtonPress(PLUS))
    {
      refresh = true;
      mode[level + 1] = (mode[level + 1] + 1) % 4;
    }

    if(inputs.pullButtonPress(BACK))
    {
      level = 0;
      mode[0] = 0;
      disp.clearBuffer();
    }

    if(refresh)
    {
      disp.clearBuffer();

      switch(mode[level + 1])
      {
      case ALARM:
        {
          disp.setString(0, "Alarm");
          break;
        }
      case SET_BRIGHTNESS:
        {
          disp.setString(0, "Brghtnss");
          break;
        }
      case SET_TIME:
        {
          disp.setString(0, "Set Time");
          break;
        }
      case SET_DATE:
        {
          disp.setString(0, "Set Date");
          break;
        }
      }

      disp.display();
    }
  }
  else if(level == 1 && mode[level] == ALARM) // ----------- Alarm menu ---------
  {
    boolean refresh = false;
    quickGoToNextLevel = true;
    quickGoBack = true;
    
    if(firstCall)
    {
      firstCall = false;
      refresh = true;
    }
    
    
    if(inputs.pullButtonPress(MINUS) || inputs.pullButtonPress(PLUS))
    {
      refresh = true;
      mode[level + 1] = !(mode[level + 1]);
    }
    
    if(refresh)
    {
      disp.clearBuffer();
      
      switch(mode[level + 1])
      {
        case SET_ALARM_ON_OFF:
        {
          disp.setString(0, "On-Off");
          break;
        }
        case SET_BRIGHTNESS:
        {
          disp.setString(0, "Time");
          break;
        }
      }
      
      disp.display();
    }
  }
  else if(level >= 1 && mode[1] == ALARM)
  {
    if(mode[2] == SET_ALARM_ON_OFF) // --------- Setting alarm on/off --------
    {
      boolean refresh = false;
      quickGoToNextLevel = false;
      quickGoBack = true;
      
      if(firstCall)
      {
        firstCall = false;
        refresh = true;
      }
      
      if(inputs.pullButtonPress(PLUS) || inputs.pullButtonPress(MINUS))
      {
        rtc.setAlarm(!(rtc.getAlarm()));
        refresh = true;
      }
      
      if(inputs.pullButtonPress(OK))
        goBack();
        
      if(refresh)
      {
        disp.clearBuffer();
        
        if(rtc.getAlarm()) // Alarm on
          disp.setString(0, "On");
        else
          disp.setString(0, "Off");
          
        disp.display();
      }
    }
    else if(mode[2] == SET_ALARM_TIME) // --------- Setting alarm time -------
    {
      boolean refresh = false;
      quickGoBack = true;
  
      if(firstCall)
      {
        firstCall = false;
        refresh = true;
  
        if(level == 2) // Hours
        {
          rtc.getRTCAlarmTime();
          disp.setDP(0, true);
          disp.setDP(1, true);
        }
        else if(level == 3) // Mins
        {
          disp.setDP(0, false); 
          disp.setDP(1, false);
          disp.setDP(3, true);
          disp.setDP(4, true);
        }
        else if(level == 4) // Seconds
        {
          disp.setDP(3, false); 
          disp.setDP(4, false);
          disp.setDP(6, true);
          disp.setDP(7, true);
        }
      }
  
      DateTime atime = rtc.getAlarmTime();
  
      if(level ==  2) // Hours
      {
        quickGoToNextLevel = true;
  
        if(inputs.pullButtonPress(PLUS))
        {
          refresh = true;
          atime.hours = atime.hours + 1;
        }
  
        if(inputs.pullButtonPress(MINUS))
        {
          refresh = true;
          atime.hours = atime.hours - 1;
        }
      }
      else if(level == 3) // Mins
      {
        quickGoToNextLevel = true;
  
        if(inputs.pullButtonPress(PLUS))
        {
          refresh = true;
          atime.mins = atime.mins + 1;
        }
  
        if(inputs.pullButtonPress(MINUS))
        {
          refresh = true;
          atime.mins = atime.mins - 1;
        }
      }
      else if(level == 4) // Secs
      {
        quickGoToNextLevel = false;
  
        if(inputs.pullButtonPress(PLUS))
        {
          refresh = true;
          atime.secs = atime.secs + 1;
        }
  
        if(inputs.pullButtonPress(MINUS))
        {
          refresh = true;
          atime.secs = atime.secs - 1;
        }
  
        if(inputs.pullButtonPress(OK)) // Done !
        {
          rtc.setAlarmTime(atime);
          rtc.setRTCAlarmTime(); // Write alarm time to DS3231
  
          level = 1;
          mode[2] = SET_ALARM_ON_OFF;
          firstCall = true;
          disp.clearBuffer();
          disp.setDP(6, false);
          disp.setDP(7, false);
        }
      }
  
      if(refresh)
      {
        // The TimeHandler class validates the data, so use it before display.
        rtc.setAlarmTime(atime);
        atime = rtc.getAlarmTime(); // Get the validated data
  
        // Display it !
        disp.setString(0, atime.hours, 2);
        disp.setString(3, atime.mins, 2);
        disp.setString(6, atime.secs, 2);
        disp.display();
      }
    }
  }
  else if(level == 1 && mode[level] == SET_BRIGHTNESS) // ---------- Set Brightness -----------
  {
    boolean refresh = false;
    quickGoToNextLevel = false;
    quickGoBack = true;

    if(firstCall)
    {
      firstCall = false;
      refresh = true;
    }

    if(inputs.pullButtonPress(PLUS))
    {
      refresh = true;
      disp.setBrightness(disp.getBrightness() + 1); // Display::setBrightness(int) takes care of overflow
    }

    if(inputs.pullButtonPress(MINUS))
    {
      refresh = true;
      disp.setBrightness(disp.getBrightness() - 1); // Display::setBrightness(int) takes care of overflow
    }
    
    if(inputs.pullButtonPress(OK))
      goBack();

    if(refresh)
    {
      disp.setString(3, disp.getBrightness(), 2);
      disp.display();
    }
  }
  else if(level >= 1 && mode[1] == SET_TIME) // ------------- Setting Time -------------
  {
    boolean refresh = false;
    quickGoBack = true;

    if(firstCall)
    {
      firstCall = false;
      refresh = true;

      if(level == 1)
      {
        disp.setDP(0, true);
        disp.setDP(1, true);
      }
      else if(level == 2)
      {
        disp.setDP(0, false); 
        disp.setDP(1, false);
        disp.setDP(3, true);
        disp.setDP(4, true);
      }
      else if(level == 3)
      {
        disp.setDP(3, false); 
        disp.setDP(4, false);
        disp.setDP(6, true);
        disp.setDP(7, true);
      }
    }

    DateTime time = rtc.getTime();

    if(level ==  1) // Hours
    {
      quickGoToNextLevel = true;

      if(inputs.pullButtonPress(PLUS))
      {
        refresh = true;
        time.hours = time.hours + 1;
      }

      if(inputs.pullButtonPress(MINUS))
      {
        refresh = true;
        time.hours = time.hours - 1;
      }
    }
    else if(level == 2) // Mins
    {
      quickGoToNextLevel = true;

      if(inputs.pullButtonPress(PLUS))
      {
        refresh = true;
        time.mins = time.mins + 1;
      }

      if(inputs.pullButtonPress(MINUS))
      {
        refresh = true;
        time.mins = time.mins - 1;
      }
    }
    else if(level == 3) // Secs
    {
      quickGoToNextLevel = false;

      if(inputs.pullButtonPress(PLUS))
      {
        refresh = true;
        time.secs = time.secs + 1;
      }

      if(inputs.pullButtonPress(MINUS))
      {
        refresh = true;
        time.secs = time.secs - 1;
      }

      if(inputs.pullButtonPress(OK)) // Done !
      {
        rtc.setTime(time);
        rtc.setRTCTime(); // Write time to DS3231

        level = 0;
        mode[0] = TIME;
        disp.setDP(6, false);
        disp.setDP(7, false);
      }
    }

    if(refresh)
    {
      // The TimeHandler class validates the data, so use it before display.
      rtc.setTime(time);
      time = rtc.getTime(); // Get the validated data

      // Display it !
      disp.setString(0, time.hours, 2);
      disp.setString(3, time.mins, 2);
      disp.setString(6, time.secs, 2);
      disp.display();
    }
  }
  else if(level >= 1 && mode[1] == SET_DATE) // ------------- Setting date ---------
  {
    boolean refresh = false;
    quickGoBack = true;

    if(firstCall)
    {
      firstCall = false;
      refresh = true;

      disp.setString(2, '-');
      disp.setString(5, '-');

      if(level == 1)
      {
        disp.setDP(0, true);
        disp.setDP(1, true);
      }
      else if(level == 2)
      {
        disp.setDP(0, false); 
        disp.setDP(1, false);
        disp.setDP(3, true);
        disp.setDP(4, true);
      }
      else if(level == 3)
      {
        disp.setDP(3, false); 
        disp.setDP(4, false);
        disp.setDP(6, true);
        disp.setDP(7, true);
      }
    }

    DateTime time = rtc.getTime();

    if(level ==  1) // Hours
    {
      quickGoToNextLevel = true;

      if(inputs.pullButtonPress(PLUS))
      {
        refresh = true;
        time.DOM = time.DOM + 1;
      }

      if(inputs.pullButtonPress(MINUS))
      {
        refresh = true;
        time.DOM = time.DOM - 1;
      }
    }
    else if(level == 2) // Mins
    {
      quickGoToNextLevel = true;

      if(inputs.pullButtonPress(PLUS))
      {
        refresh = true;
        time.month = time.month + 1;
      }

      if(inputs.pullButtonPress(MINUS))
      {
        refresh = true;
        time.month = time.month - 1;
      }
    }
    else if(level == 3) // Secs
    {
      quickGoToNextLevel = false;

      if(inputs.pullButtonPress(PLUS))
      {
        refresh = true;
        time.year = time.year + 1;
      }

      if(inputs.pullButtonPress(MINUS))
      {
        refresh = true;
        time.year = time.year - 1;
      }

      if(inputs.pullButtonPress(OK)) // Done !
      {
        // It is quite complicated because the time (hours, mins, seconds) have not been updated during the entire process...
        
        // First, we have to get the current time from the RTC
        rtc.updateTime();
        DateTime currentTime = rtc.getTime();
        
        // Mix the time with the date
        time.hours = currentTime.hours;
        time.mins = currentTime.mins;
        time.secs = currentTime.secs;
        
        // Then set object data
        rtc.setTime(time);

        // Finaly, write date and time to DS3231
        if(rtc.setRTCTime() == 0) // Everything's fine
        {
          level = 0;
          mode[0] = DATE;
          disp.setDP(6, false);
          disp.setDP(7, false);
          firstCall = true;
        }
        else // Unvalid date : go back to begining
        {
          level = 1;
          firstCall = true;
        }
      }
    }

    if(refresh)
    {
      // The TimeHandler class validates the data, so use it before display.
      rtc.setTime(time);
      time = rtc.getTime(); // Get the validated data

      // Display it !
      disp.setString(0, time.DOM, 2);
      disp.setString(3, time.month, 2);
      disp.setString(6, time.year, 2);

      disp.display();
    }
  }

  if(inputs.pullButtonPress(OK) && quickGoToNextLevel)
    goToNextLevel();
  if(inputs.pullButtonPress(BACK) && quickGoBack)
    goBack();
}

// ------------------------------------------------------------------------------


void goToNextLevel()
{
  level++;
  firstCall = true;
  disp.clearBuffer();
}

void goBack()
{
  level--;
  firstCall = true;
  disp.clearBuffer();
}




