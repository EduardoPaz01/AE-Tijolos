#ifndef LCD_HPP
#define LCD_HPP

#include <Arduino.h>
#include <Wire.h>
#include <IPAddress.h>
#include <LiquidCrystal_I2C.h>

#define I2C_SDA 8
#define I2C_SCL 9

void setup_LCD();
void lcd_print_ip(const IPAddress ip);

#endif // LCD_HPP