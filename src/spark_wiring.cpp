/**
 ******************************************************************************
 * @file    spark_wiring.cpp
 * @authors Satish Nair, Zachary Crockett, Zach Supalla, Mohit Bhoite and
 *          Brett Walach
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "spark_wiring.h"
#include "spark_wiring_interrupts.h"
#include "spark_wiring_usartserial.h"
#include "spark_wiring_spi.h"
#include "spark_wiring_i2c.h"

/*
 * @brief Set the mode of the pin to OUTPUT, INPUT, INPUT_PULLUP, 
 * or INPUT_PULLDOWN
 */
void pinMode(uint16_t pin, PinMode setMode)
{

  if(pin >= TOTAL_PINS || setMode == NONE )
  {
    return;
  }

  // Safety check
  if( !pinAvailable(pin) ) {
    return;
  }

  HAL_Pin_Mode(pin, setMode);
}

/*
 * @brief Perform safety check on desired pin to see if it's already
 * being used.  Return 0 if used, otherwise return 1 if available.
 */
bool pinAvailable(uint16_t pin) {

  // SPI safety check
  if(SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
  {
    return 0; // 'pin' is used
  }

  // I2C safety check
  if(Wire.isEnabled() == true && (pin == SCL || pin == SDA))
  {
    return 0; // 'pin' is used
  }

  // Serial1 safety check
  if(Serial1.isEnabled() == true && (pin == RX || pin == TX))
  {
    return 0; // 'pin' is used
  }

  return 1; // 'pin' is available
}

/*
 * @brief Sets a GPIO pin to HIGH or LOW.
 */
void digitalWrite(uint16_t pin, uint8_t value)
{
  if(pin >= TOTAL_PINS || PIN_MAP[pin].pin_mode == INPUT || 
     PIN_MAP[pin].pin_mode == INPUT_PULLUP || 
     PIN_MAP[pin].pin_mode == INPUT_PULLDOWN || 
     PIN_MAP[pin].pin_mode == AN_INPUT || 
     PIN_MAP[pin].pin_mode == NONE)
  {
    return;
  }

  // Safety check
  if( !pinAvailable(pin) ) {
    return;
  }

  HAL_GPIO_Write(pin, value);
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t digitalRead(uint16_t pin)
{
  if( pin >= TOTAL_PINS || 
      PIN_MAP[pin].pin_mode == NONE || 
      PIN_MAP[pin].pin_mode == AF_OUTPUT_PUSHPULL || 
      PIN_MAP[pin].pin_mode == AF_OUTPUT_DRAIN)
  {
    return LOW;
  }

  // Safety check
  if( !pinAvailable(pin) ) {
    return LOW;
  }

  int32_t rv = HAL_GPIO_Read(pin);
  return rv;
}

/*
 * @brief  Override the default ADC Sample time depending on requirement
 * @param  ADC_SampleTime: The sample time value to be set.
 * This parameter can be one of the following values:
 * @arg ADC_SampleTime_1Cycles5: Sample time equal to 1.5 cycles
 * @arg ADC_SampleTime_7Cycles5: Sample time equal to 7.5 cycles
 * @arg ADC_SampleTime_13Cycles5: Sample time equal to 13.5 cycles
 * @arg ADC_SampleTime_28Cycles5: Sample time equal to 28.5 cycles
 * @arg ADC_SampleTime_41Cycles5: Sample time equal to 41.5 cycles
 * @arg ADC_SampleTime_55Cycles5: Sample time equal to 55.5 cycles
 * @arg ADC_SampleTime_71Cycles5: Sample time equal to 71.5 cycles
 * @arg ADC_SampleTime_239Cycles5: Sample time equal to 239.5 cycles
 * @retval None
 */
void setADCSampleTime(uint8_t ADC_SampleTime)
{
  HAL_ADC_Set_Sample_Time(ADC_SampleTime);
}

/*
 * @brief Read the analog value of a pin.
 * Should return a 16-bit value, 0-65536 (0 = LOW, 65536 = HIGH)
 * Note: ADC is 12-bit. Currently it returns 0-4096
 */
int32_t analogRead(uint16_t pin)
{
  // Allow people to use 0-7 to define analog pins by checking to see if the values are too low.
  if(pin < FIRST_ANALOG_PIN)
  {
    pin = pin + FIRST_ANALOG_PIN;
  }

  // Safety check
  if( !pinAvailable(pin) ) {
    return LOW;
  }

  if(pin >= TOTAL_PINS || PIN_MAP[pin].adc_channel == NONE )
  {
    return LOW;
  }

  int32_t rv = HAL_ADC_Read(pin);
  return rv;
}

/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%.
 * TIM_PWM_FREQ is set at 500 Hz
 */
void analogWrite(uint16_t pin, uint8_t value)
{

  if(pin >= TOTAL_PINS || PIN_MAP[pin].timer_peripheral == NULL)
  {
    return;
  }

  // Safety check
  if( !pinAvailable(pin) ) {
    return;
  }

  if(PIN_MAP[pin].pin_mode != OUTPUT && PIN_MAP[pin].pin_mode != AF_OUTPUT_PUSHPULL)
  {
    return;
  }

  HAL_PWM_Write(pin, value);
}

/*
 * TIMING
 */

/*
 * @brief Should return the number of milliseconds since the processor started up.
 *      This is useful for measuring the passage of time.
 *      For now, let's not worry about what happens when this overflows (which should happen after 49 days).
 *      At some point we'll have to figure that out, though.
 */
system_tick_t millis(void)
{
  return GetSystem1MsTick();
}

/*
 * @brief Should return the number of microseconds since the processor started up.
 */
unsigned long micros(void)
{
  return HAL_Micro_Seconds();
}

/*
 * @brief This should block for a certain number of milliseconds and also execute spark_wlan_loop
 */
void delay(unsigned long ms)
{
#ifdef SPARK_WLAN_ENABLE
  volatile system_tick_t spark_loop_elapsed_millis = SPARK_LOOP_DELAY_MILLIS;
  spark_loop_total_millis += ms;
#endif

  volatile system_tick_t last_millis = GetSystem1MsTick();

  while (1)
  {
    KICK_WDT();

    volatile system_tick_t current_millis = GetSystem1MsTick();
    volatile long elapsed_millis = current_millis - last_millis;

    //Check for wrapping
    if (elapsed_millis < 0)
    {
      elapsed_millis = last_millis + current_millis;
    }

    if (elapsed_millis >= ms)
    {
      break;
    }

#ifdef SPARK_WLAN_ENABLE
    if (!SPARK_WLAN_SETUP || SPARK_WLAN_SLEEP)
    {
      //Do not yield for SPARK_WLAN_Loop()
    }
    else if ((elapsed_millis >= spark_loop_elapsed_millis) || (spark_loop_total_millis >= SPARK_LOOP_DELAY_MILLIS))
    {
      spark_loop_elapsed_millis = elapsed_millis + SPARK_LOOP_DELAY_MILLIS;
      //spark_loop_total_millis is reset to 0 in SPARK_WLAN_Loop()
      do
      {
        //Run once if the above condition passes
        SPARK_WLAN_Loop();
      }
      while (SPARK_FLASH_UPDATE);//loop during OTA update
    }
#endif
  }
}

/*
 * @brief This should block for a certain number of microseconds.
 */
void delayMicroseconds(unsigned int us)
{
  Delay_Microsecond(us);
}

long map(long value, long fromStart, long fromEnd, long toStart, long toEnd)
{
  return (value - fromStart) * (toEnd - toStart) / (fromEnd - fromStart) + toStart;
}

uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder) {
  uint8_t value = 0;
  uint8_t i;

  for (i = 0; i < 8; ++i) {
    digitalWrite(clockPin, HIGH);
    if (bitOrder == LSBFIRST)
      value |= digitalRead(dataPin) << i;
    else
      value |= digitalRead(dataPin) << (7 - i);
    digitalWrite(clockPin, LOW);
  }
  return value;
}

void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val)
{
  uint8_t i;

  for (i = 0; i < 8; i++)  {
    if (bitOrder == LSBFIRST)
      digitalWrite(dataPin, !!(val & (1 << i)));
    else
      digitalWrite(dataPin, !!(val & (1 << (7 - i))));

    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);
  }
}

