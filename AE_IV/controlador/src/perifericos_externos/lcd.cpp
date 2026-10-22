#include "lcd.hpp"

LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço I2C do display (se não acender, teste 0x3F)

static void lcd_print_linha(uint8_t linha, String texto) {
    if (texto.length() > 16) texto = texto.substring(0, 16);
    while (texto.length() < 16) texto += ' ';
    lcd.setCursor(0, linha);
    lcd.print(texto);
}

void setup_LCD() {
    Wire.begin(I2C_SDA, I2C_SCL);   // inicia o I2C nos pinos corretos do ESP32-S3
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd_print_linha(0, "IP:");
    lcd_print_linha(1, "Aguardando...");
}

void lcd_print_ip(const IPAddress ip) {
    lcd_print_linha(0, "IP:");
    lcd_print_linha(1, ip.toString());   // String temporária copiada, sem ponteiro solto
}