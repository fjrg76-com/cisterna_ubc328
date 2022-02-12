/*Copyright (C) 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * 2022 - fjrg76 dot com (c)
 */

#include <stdint.h>
#include <LiquidCrystal.h>

#define MS_TO_TICKS( x ) ( (x) / ( SYSTEM_TICK ) )
#define SYSTEM_TICK 100 // in milliseconds

enum eSensors
{
   UPPER_TANK_TOP = 15,
   UPPER_TANK_BOTTOM = 16,
   LOWER_TANK_TOP = 17,
   LOWER_TANK_BOTTOM = 18,
   SENSORS_COMMON = 19,
};

enum eActuators
{
   WATER_PUMP = 2,
   LCD_BACKLIGHT = 3,
   BUZZER = 8,
   LED_ONBOARD = 13,
};

enum eAnalogInputs
{
   KEYPAD = A0,
};

enum eLCD
{
   LCD_EN = 9,
   LCD_RS = 10,
   LCD_D4 = 4,
   LCD_D5 = 5,
   LCD_D6 = 6,
   LCD_D7 = 7,
};

struct Inputs
{
   uint8_t upper_tank_top{0};
   uint8_t upper_tank_bottom{0};
   uint8_t lower_tank_top{0};
   uint8_t lower_tank_bottom{0};
};

enum class eStates: uint8_t
{
   WAITING,
   UPPER_TANK_FILLING,
   LOWER_TANK_FILLING,
   TIME_OVER,
   SENSORS_SWAPPED,
//   UPPER_TANK_DELAY,
};

class DownTimer
{
public:
   DownTimer();

   void set( uint8_t minutes, uint8_t seconds, bool start = false );   
   void start();
   void stop();
   bool is_done();
   void get( uint8_t* minutes, uint8_t* seconds );

   void state_machine();

private:
   uint8_t minutes{0};
   uint8_t seconds{0};

   uint8_t state{0};
   bool running{false};
   bool done{false};
};

DownTimer::DownTimer()
{
   // nothing
}

void DownTimer::set( uint8_t minutes, uint8_t seconds, bool start )
{
   if( this->running ) this->running = false;

   this->minutes = minutes;
   this->seconds = seconds;
   this->done = false;

   if( start ) this->running = true;
}

void DownTimer::get( uint8_t* minutes, uint8_t* seconds )
{
   *minutes = this->minutes;
   *seconds = this->seconds;
}

void DownTimer::start()
{
   if( this->seconds > 0 )
   {
      this->running = true;
      this->done = false;
   }
}

void DownTimer::stop()
{
   this->minutes = 0;
   this->seconds = 0;
   this->running = false;
   this->done = false;
}

bool DownTimer::is_done()
{
   if( this->done )
   {
      this->done = false;
      return true;
   }
   return false;
}

void DownTimer::state_machine()
{
   if( this->running )
   {
      if( this->seconds == 0 )
      {
         this->seconds = 59;

         if( this->minutes > 0 ) --this->minutes;
      }
      else --this->seconds;


      if( this->seconds == 0 and this->minutes == 0 )
      {
         this->running = false;
         this->done = true;
      }
   }
}


//----------------------------------------------------------------------
//  
//----------------------------------------------------------------------
LiquidCrystal lcd( LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7 );

DownTimer timer;


void print_time( uint8_t col, uint8_t row, uint8_t minutes, uint8_t seconds )
{
   lcd.setCursor( col, row );

   if( minutes < 10 ) lcd.print( "0" );
   lcd.print( minutes );
   lcd.print( ":" );
   if( seconds < 10 ) lcd.print( "0" );
   lcd.print( seconds );
}

void print_sensors( uint8_t col, uint8_t row, Inputs& inputs )
{
   lcd.setCursor( col, row );

   lcd.print( inputs.upper_tank_top    ? "1" : "_" );
   lcd.print( inputs.upper_tank_bottom ? "2" : "_" );
   lcd.print( inputs.lower_tank_top    ? "3" : "_" );
   lcd.print( inputs.lower_tank_bottom ? "4" : "_" );
}

void print_text( uint8_t col, uint8_t row, const char* txt )
{
   lcd.setCursor( col, row );
   lcd.print( txt );
}


void setup()
{
   pinMode( UPPER_TANK_TOP, INPUT_PULLUP );
   pinMode( UPPER_TANK_BOTTOM, INPUT_PULLUP );
   pinMode( LOWER_TANK_TOP, INPUT_PULLUP );
   pinMode( LOWER_TANK_BOTTOM, INPUT_PULLUP );
   pinMode( SENSORS_COMMON, OUTPUT );
   digitalWrite( SENSORS_COMMON, LOW );

   pinMode( WATER_PUMP, OUTPUT );
   digitalWrite( WATER_PUMP, LOW );

   pinMode( LED_ONBOARD, OUTPUT );
   pinMode( BUZZER, OUTPUT );

   pinMode( LCD_BACKLIGHT, OUTPUT );

   Serial.begin( 115200 );

   lcd.begin( 16, 2 );

   digitalWrite( LCD_BACKLIGHT, HIGH );
   lcd.clear();
   lcd.home();
   lcd.print( "    UB-C328" );
   lcd.setCursor( 0, 1 );
   lcd.print( " by fjrg76.com" );
   delay( 4000 );

   lcd.clear();
//   lcd.print( "Waiting..." );
               print_text( 6, 0, "WAITING..." ); 
}

void read_sensors( Inputs& inputs )
{
   digitalWrite( SENSORS_COMMON, HIGH );
   delay( 1 );

   inputs.upper_tank_top = digitalRead( UPPER_TANK_TOP );
   inputs.upper_tank_bottom = digitalRead( UPPER_TANK_BOTTOM );
   inputs.lower_tank_top = digitalRead( LOWER_TANK_TOP );
   inputs.lower_tank_bottom = digitalRead( LOWER_TANK_BOTTOM );
   digitalWrite( SENSORS_COMMON, LOW );
}

void serial_report( Inputs& inputs )
{
   static uint16_t cont = 0;
   Serial.print( cont++ );

   Serial.print( "  UP_TOP: " ); Serial.print( inputs.upper_tank_top );
   Serial.print( "  UP_BOT: " ); Serial.print( inputs.upper_tank_bottom );
   Serial.print( "  LO_TOP: " ); Serial.print( inputs.lower_tank_top );
   Serial.print( "  LO_BOT: " ); Serial.println( inputs.lower_tank_bottom );
}


void loop()
{
   static uint8_t main_timer = MS_TO_TICKS( 1000 );
   static uint8_t seconds_tick_base = MS_TO_TICKS( 1000 );


   static uint16_t lcd_backlight_timer = 0;
   static uint8_t lcd_backlight_state = 0;

   static Inputs inputs;
   static eStates state = eStates::WAITING;

   static bool error = false;


   delay( SYSTEM_TICK );

   --seconds_tick_base;
   if( seconds_tick_base == 0 )
   {
      seconds_tick_base = MS_TO_TICKS( 1000 );

      timer.state_machine();
   }


   if( not error )
   {
      uint8_t minutes, seconds;
      timer.get( &minutes, &seconds );
      print_time( 0, 1, minutes, seconds );
   }


   if( lcd_backlight_timer > 0 )
   {
      --lcd_backlight_timer;

      if( lcd_backlight_timer == 0 )
      {
         lcd_backlight_timer = MS_TO_TICKS( 500 );

         switch( lcd_backlight_state )
         {
            case 0:
               lcd_backlight_state = 1;
               digitalWrite( LCD_BACKLIGHT, LOW );
               break;

            case 1:
               lcd_backlight_state = 0;
               digitalWrite( LCD_BACKLIGHT, HIGH );
               break;
         }
      }
   }

//-------------------------------------------------------------------------------- 

   --main_timer;
   if( main_timer == 0 )
   {
      main_timer = MS_TO_TICKS( 1000 );

      read_sensors( inputs );

      serial_report( inputs );

      if( not error )
      {
         print_sensors( 0, 0, inputs );
      }

      switch( state )
      {
         case eStates::WAITING:
            if( ( inputs.upper_tank_top == HIGH and inputs.upper_tank_bottom == LOW )
                  or ( inputs.lower_tank_top == HIGH and inputs.lower_tank_bottom == LOW ) )
            {
               digitalWrite( WATER_PUMP, LOW );

               state = eStates::SENSORS_SWAPPED;

               timer.stop();

               lcd_backlight_timer = MS_TO_TICKS( 500 );
               lcd_backlight_state = 0;

               error = true;

               lcd.clear();
               lcd.print( "SENSORS SWAPPED" );
               lcd.setCursor( 0, 1 );
               lcd.print( "  Verify them!" );
            }
            else if( inputs.upper_tank_bottom == LOW and inputs.lower_tank_bottom == HIGH )
            {
               state = eStates::UPPER_TANK_FILLING;
               digitalWrite( WATER_PUMP, HIGH );

               timer.set( 1, 15, true );

               print_text( 6, 0, "FIL UP TNK" );
            }
            else
            {
               digitalWrite( WATER_PUMP, LOW );
            }
            break;


         case eStates::UPPER_TANK_FILLING:
         {
            if( inputs.lower_tank_bottom == LOW )
            {
               state = eStates::LOWER_TANK_FILLING;
               digitalWrite( WATER_PUMP, LOW );

               lcd_backlight_timer = 0;

               timer.stop();

               print_text( 6, 0, "FIL LO TNK" );
            }
            else if( inputs.upper_tank_top == HIGH )
            {
               state = eStates::WAITING;
               digitalWrite( WATER_PUMP, LOW );

               timer.stop();

               print_text( 6, 0, "WAITING..." ); 
            }
            else if( timer.is_done() )
            {
               state = eStates::TIME_OVER;
               digitalWrite( WATER_PUMP, LOW );

               lcd_backlight_timer = MS_TO_TICKS( 500 );
               lcd_backlight_state = 0;

               error = true;

               lcd.clear();
               lcd.print( "   TIME OVER!" );
               lcd.setCursor( 0, 1 );
               lcd.print( "  Press reset" );
            }
            else
            {
               ;
            }
            break;
         }

         
         case eStates::LOWER_TANK_FILLING:
            if( inputs.lower_tank_top == HIGH )
            {
               state = eStates::UPPER_TANK_FILLING;
               digitalWrite( WATER_PUMP, HIGH );

               timer.set( 1, 15, true );

               print_text( 6, 0, "FIL UP TNK" );
            }
            break;

         
         case eStates::TIME_OVER: // never goes back
            break;


         case eStates::SENSORS_SWAPPED:
            if( not( ( inputs.upper_tank_top == HIGH and inputs.upper_tank_bottom == LOW )
                  or ( inputs.lower_tank_top == HIGH and inputs.lower_tank_bottom == LOW ) ) )
            {
               state = eStates::WAITING;

               error = false;
               lcd_backlight_timer = 0;
               lcd_backlight_state = 0;

               lcd.clear();
               print_text( 6, 0, "WAITING..." ); 
            }
      }
   }
}
