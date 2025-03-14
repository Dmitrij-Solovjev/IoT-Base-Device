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
    *   5. 4б  -  резерв. (возможно использовать тип пакета: sett/ctrl/log/time)
    *   5. 8б  -  адресс зоны отправителя.
    *   6. 8б  -  id отправителя
    *   7. 0-60Б -сообщение
    *   8. 8б  -  хеш-сумма+цифровая подпись.
*/

#include <Arduino.h>
#include <CRC.h>
#include <RadioLib.h>
#include <STM32FreeRTOS.h>

#include <iostream>

#include "InterpreterManager.hpp"
#include "Logger.hpp"
#include "Messanger.hpp"
#include "Some_functions.hpp"

static Logger logger(&Serial, level_of_detail::ALL);
Messanger messanger(logger);

int transmissionState;

String serialData = "";  // Глобальная переменная для хранения данных
String onRecieveBuffer = "";
bool to_transmit_flag = false;
bool to_recieve_flag = false;

// InterpreterManager interpreter;
InterpreterManager interpreter(logger);

void vOnMessageReceive(void *pvParameters) {
  onRecieveBuffer = *((String *)pvParameters);
  to_transmit_flag = true;
}

void vMainTask(void *pvParameters) {
  UNUSED(pvParameters);

  while (true) {
    if (to_transmit_flag and !to_recieve_flag) {
      logger.log(level_of_detail::MAIN, "MAIN", "Recieved: " + onRecieveBuffer);
      interpreter.interpret_it_however_you_want(onRecieveBuffer);
      onRecieveBuffer = "";
      to_transmit_flag = false;
    }
    vTaskDelay(500);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(NIQR, INPUT_PULLUP);
  delay(5000);

  interpreter.initialRead();
  // interpreter.clear_event();

  CTinyJS *js = new CTinyJS();
  // Регистрируем функции
  interpreter.setJSInterpreter(js);
  add_native_functions(js, &logger, &interpreter);

  messanger.init(1, CS, NIQR, SDN);
  messanger.SetMessageReceivedAction(vOnMessageReceive);

  js->execute(("LED_PIN=" + String(LED_PIN) + ";").c_str());

  Serial1.println("Запуск задачи");
  xTaskCreate(vMainTask, "loop like vMainTask", 2048, NULL, 2, NULL);

  String message = "Ok!";
  messanger.ISend(message);

  vTaskStartScheduler();
}

void loop() {}