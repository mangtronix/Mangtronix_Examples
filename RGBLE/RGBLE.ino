/*
 * Read light readings from an RGB color sensor and send them over Bluetooth Low-Energy.
 * Uses example code from RedBearLab and Adafruit. See "Bluetooth Low-Energy with the Blend Micro"
 * for more information.
 * https://www.packtpub.com/books/content/bluetooth-low-energy-blend-micro
 *
 * Michael Ang <https://github.com/mangtronix>
 */

/*

Copyright (c) 2012, 2013 RedBearLab

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

// BLE module from RedBearLab
#include <SPI.h>
#include <Nordic_nRF8001.h>
#include <RBL_nRF8001.h>

#include <Wire.h>

// Autoranging color sensor from Adafruit
#include <Adafruit_TCS34725.h>
#include "tcs34725autorange.h"
tcs34725 rgb_sensor;

// Configure color sensor for longer (more accurate) integration time with low gain.
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);
 
#define DIGITAL_OUT_PIN    5
#define DIGITAL_IN_PIN     A4

void setup()
{
  // Turn off sensor LED (flashes briefly at startup)
  int ledPin = 5;
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, 1);
  
  // Turn off TX/RX LEDs - saves power
  TXLED1;
  RXLED1;
  
  // Set your BLE device name here, max. length 10
  ble_set_name("RGB Sensor");
  
  // Init. and start BLE library.
  ble_begin();
 
  // Set up pins
  pinMode(DIGITAL_OUT_PIN, OUTPUT);
  pinMode(DIGITAL_IN_PIN, INPUT);
  
  // Default to internally pull high, change it if you need
  digitalWrite(DIGITAL_IN_PIN, HIGH);
  //digitalWrite(DIGITAL_IN_PIN, LOW);
  
  // Autogain
  rgb_sensor.begin();
}

void loop()
{
  static boolean analog_enabled = false;
  static byte old_state = LOW;  
  
  // If data is ready
  while(ble_available())
  {
    // read out command and data
    byte data0 = ble_read();
    byte data1 = ble_read();
    byte data2 = ble_read();
    
    if (data0 == 0x01)  // Command is to control digital out pin
    {
      if (data1 == 0x01)
        digitalWrite(DIGITAL_OUT_PIN, HIGH);
      else
        digitalWrite(DIGITAL_OUT_PIN, LOW);
    }
    else if (data0 == 0xA0) // Command is to enable analog in reading
    {
      if (data1 == 0x01)
        analog_enabled = true;
      else
        analog_enabled = false;
    }
  }
  
  // Check if we're connected
  if (ble_connected())
  {
      // Take a light reading

      uint16_t r, g, b, c;
      
      // Raw sensor value
      // tcs.getRawData(&r, &g, &b, &c);
      
      // Sensor reading using automatic gain control
      rgb_sensor.getData();
      r = rgb_sensor.r_comp;
      g = rgb_sensor.g_comp;
      b = rgb_sensor.b_comp;
      c = rgb_sensor.c_comp;
      
      // Send reading as bytes.
      ble_write(0x0C); // red marker
      ble_write(r >> 8);
      ble_write(r);
      
      ble_write(0x0D); // green marker
      ble_write(g >> 8);
      ble_write(g);
      
      ble_write(0x0E); // blue marker
      ble_write(b >> 8);
      ble_write(b);
      
      ble_write(0x0F); // clear maker
      ble_write(c >> 8);
      ble_write(c);
      
      ble_write(0xA0); // lux marker
      ble_write(int(rgb_sensor.lux) >> 8); // Convert lux value to integer and send first byte
      ble_write(int(rgb_sensor.lux));      // send second byte of lux value
      
      ble_write(0xA1); // color temp marker
      ble_write(int(rgb_sensor.ct) >> 8);
      ble_write(int(rgb_sensor.ct));
      
  }

  
  // If digital in changes, report the state
  if (digitalRead(DIGITAL_IN_PIN) != old_state)
  {
    old_state = digitalRead(DIGITAL_IN_PIN);
    
    if (digitalRead(DIGITAL_IN_PIN) == HIGH)
    {
      ble_write(0x0A);
      ble_write(0x01);
      ble_write(0x00);    
    }
    else
    {
      ble_write(0x0A);
      ble_write(0x00);
      ble_write(0x00);
    }
  }
  
  if (!ble_connected())
  {
    analog_enabled = false;
    digitalWrite(DIGITAL_OUT_PIN, LOW);
  }
  
  // Allow BLE library to send/receive data
  ble_do_events();
  
} // end loop


