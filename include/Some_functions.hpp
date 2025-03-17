#include <Arduino.h>

#include <iostream>

#include "InterpreterManager.hpp"

#define LED_PIN PB4
#define BUT1_PIN PB14
#define BUT2_PIN PB15
#define LED_MATRIX_PIN PB3

// Si4432 has the following connections:
#define CS PA4    // chip select (nSEL)
#define NIQR PA3  // прерывание при получении сообщения
#define SDN PA2   // Режим сна (0 - сон)

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
  Logger *logger = (Logger *)userdata;
  std::string text =
      c->getParameter("text")->getString();  // Получаем строку для печати
  // Serial1.println(String(text.c_str()) + " at " + String(millis()));
  logger->log(level_of_detail::MAIN, "INTERPR:" + String(millis()), text.c_str());
}

inline void js_delete_all(CScriptVar *c, void *userdata) {
  InterpreterManager *interpreter = (InterpreterManager *)userdata;
  interpreter->clear_event();
}

void add_native_functions(CTinyJS *js, Logger *logger,
                          InterpreterManager *interpreter) {
  js->addNative("function digitalRead(pin)", js_digitalRead, 0);
  js->addNative("function digitalWrite(pin, value)", js_digitalWrite, 0);
  js->addNative("function delay(ms)", js_delay, 0);
  js->addNative("function Serial_println(text)", js_Serial_println,
                (void *)logger);
  js->addNative("function DELETE_ALL()", js_delete_all, (void *)interpreter);
  pinMode(LED_PIN, OUTPUT);
  pinMode(NIQR, INPUT_PULLUP);
  pinMode(BUT1_PIN, INPUT);
  pinMode(BUT2_PIN, INPUT);
}