
/*
* The MIT License (MIT)
*
* Copyright (c) 2015 Marco Russi
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/


/* ---------- Inclusions ------------ */
#include <Wire.h>


/* ---------- Defines ------------ */
/* BH1750 sensor I2C address */
#define BH1750_I2C_ADD  0x23
/* Push button input channel */
#define BUTTON_CH 5
/* PWM channels */
#define PWM_CH_1  9
#define PWM_CH_2  10
#define PWM_CH_3  11
/* PWM channels */
#define PWM_CH_RED    PWM_CH_2
#define PWM_CH_GREEN  PWM_CH_3
#define PWM_CH_BLUE   PWM_CH_1
/* LED states */
#define STATE_OFF           0
#define FIRST_STATE_ON      1
#define LAST_STATE_ON       3
#define NUM_OF_STATES_ON    6 /* states 4,5,6 are the same sequence 2 but for respectively red, green and blue channels only */
/* Num of LEDs light steps for each sequence */
#define NUM_OF_LIGHT_STEPS      40
/* Light thresholds */
#define LOW_TH  2
/* Number of values to store before validating the environment light change */
#define NUM_OF_LIGHT_VALUES 6


/* ---------- Local variables ------------ */
/* light values count */
char light_count = 0;
/* LEDs PWM DC values - red sequences 1,2,3 */
int array_red_pwm_dc[LAST_STATE_ON][NUM_OF_LIGHT_STEPS] = {
  {0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,200,0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,200},
  {0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,190,200,190,180,170,160,150,140,130,120,110,100,90,80,70,60,50,40,30,20,10},
  {0,0,0,0,180,180,180,180,0,0,0,0,180,180,180,180,0,0,0,0,180,180,180,180,0,0,0,0,180,180,180,180,0,0,0,0,180,180,180,180}};
/* LEDs PWM DC values - green sequences 1,2,3 */
int array_green_pwm_dc[LAST_STATE_ON][NUM_OF_LIGHT_STEPS] = {
  {0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,200,0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,200},
  {0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,190,200,190,180,170,160,150,140,130,120,110,100,90,80,70,60,50,40,30,20,10},
  {0,0,0,0,180,180,180,180,0,0,0,0,180,180,180,180,0,0,0,0,180,180,180,180,0,0,0,0,180,180,180,180,0,0,0,0,180,180,180,180}};
/* LEDs PWM DC values - blue sequences 1,2,3 */
int array_blue_pwm_dc[LAST_STATE_ON][NUM_OF_LIGHT_STEPS] = {
  {0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,200,0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,200},
  {0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,190,200,190,180,170,160,150,140,130,120,110,100,90,80,70,60,50,40,30,20,10},
  {0,0,0,0,180,180,180,180,0,0,0,0,180,180,180,180,0,0,0,0,180,180,180,180,0,0,0,0,180,180,180,180,0,0,0,0,180,180,180,180}};
/* PWM DC values array index */
int array_pwm_dc_index = 0;
/* current LED state */
char curr_state = STATE_OFF;
/* state forced to ON */
char forced_on = false;


/* ---------- Local functions prototypes ------------ */
void check_light(int new_value);
void manageLED(bool);


/* --------- Setup function ----------- */
void setup() {
  /* init RGB LED pins as output */
  pinMode(PWM_CH_RED, OUTPUT);
  pinMode(PWM_CH_GREEN, OUTPUT);
  pinMode(PWM_CH_BLUE, OUTPUT);
  /* turn LED OFF */
  digitalWrite(PWM_CH_RED, LOW);
  digitalWrite(PWM_CH_GREEN, LOW);
  digitalWrite(PWM_CH_BLUE, LOW);
      
  /* init I2C interface */
  Wire.begin();        
  /* transmit to BH1750 */
  Wire.beginTransmission(BH1750_I2C_ADD);
  /* write opcode=0x10. Continuous update */
  Wire.write(0x10);  
  /* End transmission */         
  Wire.endTransmission();
  /* wait 200ms */
  delay(100);
}


/* ---------- Loop function ----------- */
void loop() {
  int value = 0;
  
  /* read 2 bytes from the BH1750 sensor */
  Wire.requestFrom(BH1750_I2C_ADD, 2, true);
  /* if 2 bytes are ready */
  if (Wire.available() == 2)   // slave may send less than requested
  {
    /* obtain light value */
    value = (((int)Wire.read() << 8) & 0xFF00); // receive a byte as character
    value |= ((int)Wire.read() & 0x00FF); // receive a byte as character  
  }
  else
  {
    /* error: invalid read value length */
  }

  /* if forced ON mode is disabled */
  if(forced_on == false)
  {
    /* check an eventual light change */
    check_light(value);
  }
  else
  {
    /* do not check light */
  }


  /* manage LED illumination */
  manageLED();


  /* check if push button is pressed */
  if( digitalRead(BUTTON_CH) == LOW )
  {
    /* if state is not OFF */
    if( curr_state != STATE_OFF )
    {
      /* change LEDs state */
      if( curr_state < NUM_OF_STATES_ON)
      {
        /* next light state */
        curr_state++;
        /* start from first PWM step */
        array_pwm_dc_index = 0;
      }
      else
      {
        /* turn OFF and reset sequence */
        curr_state = STATE_OFF;
        /* forced ON mode disabled */
        forced_on = false;
      }
    }
    /* if state is OFF */
    else
    {
      /* turn ON starting from the first state */
      curr_state = FIRST_STATE_ON;
      /* forced ON mode enabled */
      forced_on = true;
      /* start from first PWM step */
      array_pwm_dc_index = 0;
    }
  }
  else
  {
    /* do nothing */
  }
  

  /* wait 300ms */
  delay(300);
}


/* ---------- Local functions implementation ------------ */

/* Function to manage LED illumination */
void manageLED(void)
{
  /* if LED is ON */
  if(curr_state != STATE_OFF)
  {
    if(array_pwm_dc_index < NUM_OF_LIGHT_STEPS)
    {
      /* states 4,5,6 are the same sequence #2 but for respectively red, green and blue channels only */
      if(curr_state == 4)
      {
        analogWrite(PWM_CH_RED, array_red_pwm_dc[1][array_pwm_dc_index]);
        analogWrite(PWM_CH_GREEN, 0);
        analogWrite(PWM_CH_BLUE, 0);
      }
      else if(curr_state == 5)
      {
        analogWrite(PWM_CH_RED, 0);
        analogWrite(PWM_CH_GREEN, array_green_pwm_dc[1][array_pwm_dc_index]);
        analogWrite(PWM_CH_BLUE, 0);
      }
      else if(curr_state == 6)
      {
        analogWrite(PWM_CH_RED, 0);
        analogWrite(PWM_CH_GREEN, 0);
        analogWrite(PWM_CH_BLUE, array_blue_pwm_dc[1][array_pwm_dc_index]);
      }
      else
      {
        /* update PWM DC on the channels */
        analogWrite(PWM_CH_RED, array_red_pwm_dc[curr_state-1][array_pwm_dc_index]);
        analogWrite(PWM_CH_GREEN, array_green_pwm_dc[curr_state-1][array_pwm_dc_index]);
        analogWrite(PWM_CH_BLUE, array_blue_pwm_dc[curr_state-1][array_pwm_dc_index]);
      }

      array_pwm_dc_index++;
    }
    else
    {
      array_pwm_dc_index = 0;
    }
  }
  else
  {
    /* update PWM DC on the channels */
    analogWrite(PWM_CH_RED, 0);
    analogWrite(PWM_CH_GREEN, 0);
    analogWrite(PWM_CH_BLUE, 0);
  }
}


/* check light, return true in case of a light change event */
void check_light(int new_value)
{
  /* if current state is OFF */
  if(curr_state == STATE_OFF)
  {
    /* if value is under LOW threshold */
    if(new_value <= LOW_TH)
    {
      /* count another valid value */
      light_count++;
      /* if valid light values are enough */
      if(light_count == NUM_OF_LIGHT_VALUES)
      {
        /* reset light array index */
        light_count = 0;
        /* change state to the first one ON */
        curr_state = FIRST_STATE_ON;
        /* start from first PWM step */
        array_pwm_dc_index = 0;
      }
      else
      {
        /* do nothing */
      }
    }
    else
    {
      /* unexpected value, reset count */
      light_count = 0;
    }
  }
  else
  {
    /* do nothing at the moment */
  }
}


/* End of file */


