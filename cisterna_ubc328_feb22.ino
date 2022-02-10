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
//   UPPER_TANK_DELAY,
};




LiquidCrystal lcd( LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7 );

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
   lcd.print( "   UB-C328" );
   lcd.setCursor( 0, 1 );
   lcd.print( " by fjrg76.com" );
   delay( 4000 );

   lcd.clear();
   lcd.print( "Waiting..." );
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
   static uint16_t lcd_backlight_timer = 0;
   static uint8_t lcd_backlight_state = 0;

   static Inputs inputs;
   static eStates state = eStates::WAITING;
   static uint16_t working_timer = 0;


   delay( SYSTEM_TICK );

   if( working_timer > 0 )
   {
      --working_timer;
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

      switch( state )
      {
         case eStates::WAITING:
            if( inputs.upper_tank_bottom == LOW and inputs.lower_tank_bottom == HIGH )
            {
               state = eStates::UPPER_TANK_FILLING;
               digitalWrite( WATER_PUMP, HIGH );

//               working_timer = ( 1000 * 60 * 15 / SYSTEM_TICK );
               working_timer = MS_TO_TICKS( 1000 * 30 );

               lcd.clear();
               lcd.print( "UP_TNK FILL" );
            }
            else
            {
               digitalWrite( WATER_PUMP, LOW );
            }
            break;

         case eStates::UPPER_TANK_FILLING:
            if( inputs.lower_tank_bottom == LOW )
            {
               state = eStates::LOWER_TANK_FILLING;
               digitalWrite( WATER_PUMP, LOW );

               lcd_backlight_timer = 0;

               lcd.clear();
               lcd.print( "LO_TNK FILL" );
            }
            else if( inputs.upper_tank_top == HIGH )
            {
               state = eStates::WAITING;
               digitalWrite( WATER_PUMP, LOW );

               lcd.clear();
               lcd.print( "WAITING..." );
            }
            else if( working_timer == 0 )
            {
               state = eStates::TIME_OVER;
               digitalWrite( WATER_PUMP, LOW );

               lcd_backlight_timer = MS_TO_TICKS( 500 );
               lcd_backlight_state = 0;

               lcd.clear();
               lcd.print( "   TIME OVER!" );
               lcd.setCursor( 0, 1 );
               lcd.print( "Press reset" );
            }
            else
            {
               ;
            }
            break;

         case eStates::LOWER_TANK_FILLING:
            if( inputs.lower_tank_top == HIGH )
            {
               state = eStates::UPPER_TANK_FILLING;
               digitalWrite( WATER_PUMP, HIGH );

               working_timer = MS_TO_TICKS( 1000 * 30 );

               lcd.clear();
               lcd.print( "UP_TNK FILL" );
            }
            break;

         case eStates::TIME_OVER: // never goes back
            break;
      }
   }
}
