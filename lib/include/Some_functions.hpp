#include <Arduino.h>
#include <iostream>

#include "InterpreterManager.hpp"

#define LED_PIN PB4
#define BUT1_PIN PB14
#define BUT2_PIN PB15
#define LED_MATRIX_PIN PB3



// Функция-обертка для digitalRead, совместимая с TinyJS
inline void js_digitalRead(CScriptVar *c, void *userdata) {
  int pin = c->getParameter("pin")->getInt();  // Получаем номер пина
  int value = digitalRead(pin);                // Читаем значение с пина
  c->getReturnVar()->setInt(value);  // Возвращаем результат в интерпретатор
}

// Функция-обертка для digitalWrite, совместимая с TinyJS
inline void js_digitalWrite(CScriptVar *c, void *userdata) {
  int pin = c->getParameter("pin")->getInt();      // Получаем номер пина
  int value = c->getParameter("value")->getInt();  // Получаем значение статуса
  digitalWrite(pin, value);                        // Записываем значение на пин
}

// Функция-обертка для delay, совместимая с TinyJS
inline void js_delay(CScriptVar *c, void *userdata) {
  const int ms = c->getParameter("ms")->getInt();  // Получаем время задержки
  vTaskDelay(pdMS_TO_TICKS(ms));  // Вызываем стандартную Arduino-функцию
}

// Функция-обертка для Serial.print, совместимая с TinyJS
inline void js_Serial_println(CScriptVar *c, void *userdata) {
  std::string text =
      c->getParameter("text")->getString();  // Получаем строку для печати
  Serial1.println(String(text.c_str()) + " at " + String(millis()));
}