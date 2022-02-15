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

#define SYSTEM_TICK 100 // in milliseconds
#define MS_TO_TICKS( x ) ( (x) / ( SYSTEM_TICK ) )

enum eSensors
{
   UPPER_TANK_TOP    = 15,
   UPPER_TANK_BOTTOM = 16,
   LOWER_TANK_TOP    = 17,
   LOWER_TANK_BOTTOM = 18,
   SENSORS_COMMON    = 19,
};

enum eActuators
{
   WATER_PUMP    = 2,
   LCD_BACKLIGHT = 3,
   BUZZER        = 8,
   LED_ONBOARD   = 13,
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

struct Time
{
   uint8_t minutes;
   uint8_t seconds;

   void operator+=( uint8_t inc );
   void operator-=( uint8_t dec );
};


void Time::operator+=( uint8_t inc )
{
   this->seconds += inc;
   if( this->seconds > 59 )
   {
      this->seconds = 0;
      ++this->minutes;
      if( this->minutes > 99 ) this->minutes = 0;
   }
}

void Time::operator-=( uint8_t dec )
{
   if( (this->seconds - dec) >= 0 )
   {
      this->seconds -= dec;
   }
   else
   {
      this->seconds = 60 - dec;

      if( this->minutes > 0 )
      {
         --this->minutes;
      }
      else
      {
         this->minutes = 99;
      }
   }
}

//----------------------------------------------------------------------
//  
//----------------------------------------------------------------------
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


class Keypad
{
public:
   enum class eKey: uint8_t
   {
      Enter,
      Down,
      Up,
      Back,
      Menu,
      None,
   };

   Keypad();
   Keypad& operator=(Keypad&) = delete;
   Keypad(Keypad&) = delete;

   void begin( uint8_t analog_pin );
   void state_machine();
   eKey get();

private:
   uint8_t pin{0};
   bool ready{false};
   eKey key{eKey::None};
   uint8_t state{0};
   uint16_t ticks{0};
   bool decoding{false};

   eKey read();
};

Keypad::Keypad()
{
   // nothing
}

void Keypad::begin( uint8_t analog_pin )
{
   this->pin = analog_pin;
}


Keypad::eKey Keypad::get()
{
   static uint8_t state = 0;
   static uint16_t ticks = 0;
   static auto last_key = eKey::None;

   switch( state )
   {
      case 0:
         last_key = read();
         if( last_key != eKey::None )
         {
            state = 1;
            ticks = MS_TO_TICKS( 100 );
            this->key = eKey::None;
         }
         break;

      case 1:
         --ticks;
         if( ticks == 0 )
         {
            if( last_key == read() )
            {
               state = 2;
               ticks = MS_TO_TICKS( 2000 );
               this->key = last_key;
            }
            else
            {
               state = 0;
               last_key = eKey::None;
            }
         }
         break;

      case 2:
         --ticks;
         if( ticks == 0 )
         {
            if( last_key == read() )
            {
               state = 2;
               ticks = MS_TO_TICKS( 200 );
               this->key = last_key;
            }
            else
            {
               state = 3;
               ticks = MS_TO_TICKS( 500 );
               last_key = eKey::None;
               this->key = eKey::None;
            }
         }
         else
         {
            this->key = eKey::None;
         }
         break;

      case 3:
         --ticks;
         if( ticks == 0 )
         {
            state = 0;
            last_key = eKey::None;
         }
         this->key = eKey::None;
         break;
   }

   return this->key;
}

Keypad::eKey Keypad::read()
{
   uint16_t val = 0;
   for( uint8_t i = 8; i > 0; --i ) val += analogRead( this->pin );
   val >>= 3;

   if( val < 100 ) return eKey::Enter;
   if( 160 < val and val <= 300 ) return eKey::Down;
   if( 390 < val and val <= 500 ) return eKey::Up;
   if( 560 < val and val <= 700 ) return eKey::Back;
   if( 780 < val and val <= 850 ) return eKey::Menu;
   else return eKey::None;
}

void Keypad::state_machine()
{

}




//----------------------------------------------------------------------
//  
//----------------------------------------------------------------------


class Blink
{
public:
   enum class eMode: uint8_t { ONCE, REPETITIVE, FOREVER };

   Blink();
   Blink& operator=(Blink&) = delete;
   Blink(Blink&) = delete;

   void begin( uint8_t pin );
   void set( eMode mode, uint16_t ticks_on, uint16_t ticks_off = 0, uint16_t times = 1);
   void start();
   void stop();
   void state_machine();

private:
   uint8_t pin{0};

   uint16_t ticks_onMOD{0};
   uint16_t ticks_offMOD{0};
   uint16_t ticks{0};

   eMode mode{eMode::ONCE};

   uint16_t timesMOD{0};
   uint16_t times{0};
   bool running{false};
   uint8_t state{0};
};

Blink::Blink()
{
   // nothing
}

void Blink::begin( uint8_t pin )
{
   this->pin = pin;
   pinMode( this->pin, OUTPUT );
}

void Blink::set( eMode mode, uint16_t ticks_on, uint16_t ticks_off, uint16_t times )
{
   this->mode         = mode;
   this->ticks_onMOD  = ticks_on;
   this->ticks_offMOD = ticks_off;
   this->timesMOD     = times;
}

void Blink::start()
{
   this->running = false;

   this->ticks = this->ticks_onMOD;

   if( this->mode == eMode::REPETITIVE )
   {
      this->times = this->timesMOD;
   }

   this->state = 0;
   this->running = true;

   digitalWrite( this->pin, HIGH );
}

void Blink::stop()
{
   this->running = false;
}

void Blink::state_machine()
{
   if( this->running )
   {
      switch( this->state )
      {
         case 0:
            --this->ticks;
            if( this->ticks == 0 )
            {
               digitalWrite( this->pin, LOW );

               if( this->mode == eMode::REPETITIVE or this->mode == eMode::FOREVER )
               {
                  this->ticks = this->ticks_offMOD;
                  this->state = 1;
               }
               else
               {
                  this->running = false;
               }
            }
            break;

         case 1:
            --this->ticks;
            if( this->ticks == 0 )
            {
               if( this->mode == eMode::REPETITIVE )
               {
                  --this->times;
                  if( this->times == 0 )
                  {
                     this->running = false;
                  }
               }
               else // eMode::FOREVER:
               {
                  this->state = 0;
                  this->ticks = this->ticks_onMOD;

                  digitalWrite( this->pin, HIGH );
               }
            }
            break;

      } // switch state
   } // if this->running
}


//----------------------------------------------------------------------
//  
//----------------------------------------------------------------------

LiquidCrystal lcd( LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7 );

DownTimer timer;

Keypad keypad;

Blink lcd_backlight;

Blink led13;

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

//   pinMode( LED_ONBOARD, OUTPUT );
   pinMode( BUZZER, OUTPUT );

//   pinMode( LCD_BACKLIGHT, OUTPUT );

   Serial.begin( 115200 );
   lcd.begin( 16, 2 );
   keypad.begin( A0 );

   lcd_backlight.begin( LCD_BACKLIGHT );
               lcd_backlight.set( Blink::eMode::ONCE, MS_TO_TICKS( 30000 ) );
               lcd_backlight.start();

   led13.begin( LED_ONBOARD );
   led13.set( Blink::eMode::FOREVER, MS_TO_TICKS( 100 ), MS_TO_TICKS( 900 ) );
   led13.start();

//   digitalWrite( LCD_BACKLIGHT, HIGH );
   lcd.clear();
   lcd.home();
   lcd.print( "    UB-C328" );
   lcd.setCursor( 0, 1 );
   lcd.print( " by fjrg76.com" );
   delay( 4000 );

   lcd.clear();
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
   static uint16_t main_timer = MS_TO_TICKS( 1000 );
   static uint16_t seconds_tick_base = MS_TO_TICKS( 1000 );


//   static uint16_t lcd_backlight_timer = 0;
//   static uint8_t lcd_backlight_state = 0;

   static Inputs inputs;
   static eStates state = eStates::WAITING;

   static bool error = false;


   static Time downTimer_set = { 1, 15 };
   

   static unsigned long last_time = 0;
   auto now = millis();
   if( not ( now - last_time >= ( SYSTEM_TICK ) ) ) return;
   last_time = now;

   keypad.state_machine();
   lcd_backlight.state_machine();
   led13.state_machine();


   --seconds_tick_base;
   if( seconds_tick_base == 0 )
   {
      seconds_tick_base = MS_TO_TICKS( 1000 );

      timer.state_machine();
   }


   static bool settings = false;
   if( not error and settings == false )
   {
      uint8_t minutes, seconds;
      timer.get( &minutes, &seconds );
      print_time( 0, 1, minutes, seconds );
   }


#if 0 
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
#endif  

//-------------------------------------------------------------------------------- 

   --main_timer;
   if( main_timer == 0 )
   {
      main_timer = MS_TO_TICKS( 1000 );

      read_sensors( inputs );

//      serial_report( inputs );

      if( not error )
      {
         print_sensors( 0, 0, inputs );
      }

      switch( state )
      {
         case eStates::WAITING:
            if(   ( inputs.upper_tank_top == HIGH and inputs.upper_tank_bottom == LOW )
               or ( inputs.lower_tank_top == HIGH and inputs.lower_tank_bottom == LOW ) )
            {
               digitalWrite( WATER_PUMP, LOW );

               state = eStates::SENSORS_SWAPPED;

               timer.stop();

//               lcd_backlight_timer = MS_TO_TICKS( 500 );
//               lcd_backlight_state = 0;
               lcd_backlight.stop();
               lcd_backlight.set( Blink::eMode::FOREVER, MS_TO_TICKS( 500 ), MS_TO_TICKS( 500 ) );
               lcd_backlight.start();

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

//               timer.set( 1, 15, true );
               timer.set( downTimer_set.minutes, downTimer_set.seconds, true );

               lcd_backlight.stop();
               lcd_backlight.set( Blink::eMode::ONCE, MS_TO_TICKS( 10000 ) );
               lcd_backlight.start();

               print_text( 6, 0, "FIL UP TNK" );
            }
            else // safety condition:
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

//               lcd_backlight_timer = 0;

               timer.stop();

               lcd_backlight.stop();
               lcd_backlight.set( Blink::eMode::ONCE, MS_TO_TICKS( 10000 ) );
               lcd_backlight.start();

               print_text( 6, 0, "FIL LO TNK" );
            }
            else if( inputs.upper_tank_top == HIGH )
            {
               state = eStates::WAITING;
               digitalWrite( WATER_PUMP, LOW );

               timer.stop();

               lcd_backlight.stop();
               lcd_backlight.set( Blink::eMode::ONCE, MS_TO_TICKS( 10000 ) );
               lcd_backlight.start();

               print_text( 6, 0, "WAITING..." ); 
            }
            else if( timer.is_done() )
            {
               state = eStates::TIME_OVER;
               digitalWrite( WATER_PUMP, LOW );

//               lcd_backlight_timer = MS_TO_TICKS( 500 );
//               lcd_backlight_state = 0;
               lcd_backlight.set( Blink::eMode::FOREVER, MS_TO_TICKS( 500 ), MS_TO_TICKS( 500 ) );
               lcd_backlight.start();

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

               timer.set( downTimer_set.minutes, downTimer_set.seconds, true );

               lcd_backlight.stop();
               lcd_backlight.set( Blink::eMode::ONCE, MS_TO_TICKS( 10000 ) );
               lcd_backlight.start();

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
//               lcd_backlight_timer = 0;
//               lcd_backlight_state = 0;
               lcd_backlight.stop();
               lcd_backlight.set( Blink::eMode::ONCE, MS_TO_TICKS( 30000 ) );
               lcd_backlight.start();

               lcd.clear();
               print_text( 6, 0, "WAITING..." ); 
            }
      }
   }


   if( state == eStates::WAITING )
   {
      Keypad::eKey key = keypad.get();
      // .get() must be called in every system tick

      switch( key )
      {
         case Keypad::eKey::Up:
            settings = true;
            downTimer_set += 15;
            Serial.println( "UP" );
            break;

         case Keypad::eKey::Down:
            settings = true;
            downTimer_set -= 15;
            Serial.println( "DN" );
            break;

         case Keypad::eKey::Enter:
            settings = false;
            timer.set( downTimer_set.minutes, downTimer_set.seconds, false );
            Serial.println( "OK" );
            break;

         case Keypad::eKey::Back:
            settings = false;
            timer.get( &downTimer_set.minutes, &downTimer_set.seconds );
            Serial.println( "BK" );
            break;

         case Keypad::eKey::Menu:
            break;

         case Keypad::eKey::None:
            break;
      }

      if( key != Keypad::eKey::None )
      {
         print_time( 0, 1, downTimer_set.minutes, downTimer_set.seconds );

               lcd_backlight.stop();
               lcd_backlight.set( Blink::eMode::ONCE, MS_TO_TICKS( 30000 ) );
               lcd_backlight.start();
      }
   }
}
