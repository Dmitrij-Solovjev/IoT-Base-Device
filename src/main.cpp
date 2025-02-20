/*
  Базовая реализация устройства. Пытаюсь "подружить" интепретатор (с обвязкой)
  и пакетный режим передачи сообщений, а также интегрировать дистанционное
  программирование.

  Используется Serial1 для логов, PB4 для светодиода, PB3 для
  матрицы и PB14+PB15 для кнопок. Для реализации протокола используются si4432
  на частоте 433МГц.

  В этом проекте:
    * Используются ненастойчивый CSMA
  https://ru.wikipedia.org/wiki/Carrier_Sense_Multiple_Access
    * Пакет состоит из
    *   1. 1б  -  с подтверждением или без
    *   2. 1б  -  приоритетное или нет
    *   3. 2б  -  тип пакета (10 - начало,
    *                         00 - середина,
    *                         01 - конец,
    *                         11 - сообщение из одного пакета)
    *   5. 4б  -  резерв.
    *   5. 8б  -  адресс зоны отправителя.
    *   6. 8б  -  id отправителя
    *   7. 0-60Б -сообщение
    *   8. 8б  -  хеш-сумма+цифровая подпись.
*/

#include <Arduino.h>

#include <iostream>

#include "InterpreterManager.hpp"
#include "Logger.hpp"
#include "Some_functions.hpp"

int counter = 0;

// Глобальный логгер
Logger logger(&Serial1, level_of_detail::ALL);

// InterpreterManager interpreter;
InterpreterManager interpreter(logger);

void senderTask(void *pvParameters) {
  while (1) {
    for (auto i = 0; i < 7; ++i) {
      event led_change_trigger = {"button" + std::to_string(i % 3 + 1), ""};
      Serial1.println("Новое событие:" +
                      String(led_change_trigger.trigger.c_str()));
      interpreter.enqueueTrigger(led_change_trigger);
      vTaskDelay(pdMS_TO_TICKS(25));  // Отправляем команды каждые 50ms
    }
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(10000));
    };
  }
}

void setup() {
  Serial1.begin(115200);
  delay(5000);

  interpreter.initialRead();
  interpreter.clear_event();

  pinMode(LED_PIN, OUTPUT);

  CTinyJS *js = new CTinyJS();
  // Регистрируем функции
  interpreter.setJSInterpreter(js);

  js->addNative("function digitalRead(pin)", js_digitalRead, 0);
  js->addNative("function digitalWrite(pin, value)", js_digitalWrite, 0);
  js->addNative("function delay(ms)", js_delay, 0);
  js->addNative("function Serial_println(text)", js_Serial_println, 0);

  js->execute(("LED_PIN=" + String(LED_PIN) + ";").c_str());

  Serial1.println("Попытка создать пару");

  event led_change_event = {"button1",
                            "Serial_println(\"start thread_1\"); "
                            "digitalWrite(LED_PIN,!digitalRead(LED_PIN)); "
                            " delay(1000); "
                            "Serial_println(\"finish thread_1\");"};
  interpreter.createPair(led_change_event);

  led_change_event = {"button2",
                      "Serial_println(\"start thread_2\"); "
                      "digitalWrite(LED_PIN,!digitalRead(LED_PIN)); "
                      " delay(500); "
                      "Serial_println(\"finish thread_2\");"};
  interpreter.createPair(led_change_event);

  led_change_event = {"button3",
                      "Serial_println(\"start thread_3\"); "
                      "digitalWrite(LED_PIN,!digitalRead(LED_PIN)); "
                      " delay(50); "
                      "Serial_println(\"finish thread_3\");"};

  interpreter.createPair(led_change_event);

  Serial1.println("Запуск задачи");
  xTaskCreate(senderTask, "SenderTask", 2048, NULL, 1, NULL);

  // digitalWrite(LED_PIN, HIGH);

  vTaskStartScheduler();
}
void loop() {}