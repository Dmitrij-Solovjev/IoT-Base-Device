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

void senderTask(void *pvParameters) {
  while (1) {
    for (auto i = 0; i < 10; ++i) {
      String led_change_trig =
          i % 2 == 0 ? "12344321e|button_slow$" : "12344321e|button_fast$";
      Serial1.println("Новое событие:" + led_change_trig);

      interpreter.interpret_it_however_you_want(led_change_trig);

      // vTaskDelay(pdMS_TO_TICKS(25));  // Отправляем команды каждые 50ms
    }
    while (1) vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

void SerialTask(void *pvParameters) {
  UNUSED(pvParameters);

  while (1) {
    if (Serial.available()) {
      to_recieve_flag = true;
      char receivedChar = Serial.read();
      serialData += receivedChar;
      // logger.log(level_of_detail::MAIN, "MAIN", "R: " + serialData);
      if (receivedChar == '\n') {  // Завершающий символ
        to_recieve_flag = false;
        logger.log(level_of_detail::MAIN, "SERTASK",
                   "Transfering: " + serialData);
        messanger.ISend(serialData);
        serialData = "";
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Небольшая задержка для оптимизации
  }
}

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
  //interpreter.clear_event();

  CTinyJS *js = new CTinyJS();
  // Регистрируем функции
  interpreter.setJSInterpreter(js);
  add_native_functions(js);

  messanger.init(1, CS, NIQR, SDN);
  messanger.SetMessageReceivedAction(vOnMessageReceive);

  // transmissionState = messanger.radio.startTransmit("Hello World!");
  String text =
      "Whether he's a cousin, be it a friend, until a man is in love freely to "
      "the lady enters it, and easily and techniques. But we need only him to "
      "fall in love he rarely visits the house. He... he even said with "
      "difficulty. He is timid, he is afraid.";

  // text = "Hello world!";
  //messanger.ISend(text);


  js->execute(("LED_PIN=" + String(LED_PIN) + ";").c_str());
/*
  Serial1.println("Попытка создать пару");

  String led_change_event = {
      "12344321+|button_fast$"
      "Serial_println(\"start thread_1\"); "
      "for (var i = 0; i < 20; i++) {"
      "  digitalWrite(LED_PIN,!digitalRead(LED_PIN)); "
      "  delay(100); "
      "}"
      "Serial_println(\"finish thread_1\");"};
  interpreter.interpret_it_however_you_want(led_change_event);

  led_change_event = {
      "12344321+|button_slow$"
      "Serial_println(\"start thread_2\"); "
      "for (var i = 0; i < 4; i++) {"
      "  digitalWrite(LED_PIN,!digitalRead(LED_PIN)); "
      "  delay(500); "
      "}"
      "Serial_println(\"finish thread_2\");"};

  interpreter.interpret_it_however_you_want(led_change_event);
xTaskCreate(senderTask, "SenderTask", 2048, NULL, 1, NULL);
  // led_change_event = {"12344321c|button_slow", ""};
  // interpreter.interpret_it_however_you_want(led_change_event);
*/
  
  Serial1.println("Запуск задачи");
  xTaskCreate(vMainTask, "loop like vMainTask", 2048, NULL, 2, NULL);
  xTaskCreate(SerialTask, "SerRead. and Trasfer Task", 2048, NULL, 2, NULL);

  vTaskStartScheduler();
}

void loop() {}

/*

void setup() {

  js->execute(("LED_PIN=" + String(LED_PIN) + ";").c_str());

  Serial1.println("Попытка создать пару");

  event led_change_event = {"12344321+|button_fast",
                            "Serial_println(\"start thread_1\"); "
                            "for (var i = 0; i < 20; i++) {"
                            "  digitalWrite(LED_PIN,!digitalRead(LED_PIN)); "
                            "  delay(100); "
                            "}"
                            "Serial_println(\"finish thread_1\");"};
  interpreter.interpret_it_however_you_want(led_change_event);

  led_change_event = {"12344321+|button_slow",
                      "Serial_println(\"start thread_2\"); "
                      "for (var i = 0; i < 4; i++) {"
                      "  digitalWrite(LED_PIN,!digitalRead(LED_PIN)); "
                      "  delay(500); "
                      "}"
                      "Serial_println(\"finish thread_2\");"};

  interpreter.interpret_it_however_you_want(led_change_event);

  //led_change_event = {"12344321c|button_slow", ""};
  //interpreter.interpret_it_however_you_want(led_change_event);

  Serial1.println("Запуск задачи");
  xTaskCreate(senderTask, "SenderTask", 2048, NULL, 1, NULL);

  // digitalWrite(LED_PIN, HIGH);

  vTaskStartScheduler();
}
void loop() {}
*/