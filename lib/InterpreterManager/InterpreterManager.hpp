/* Класс по обслуживанию входящих событий и запуску соответствующих задач.
 * В данном классе используется интерпретатор C++ для выполнения поступающих
 * задач, хранящихся в виде строки, чьё выполнение начинается после триггера в
 * виде поступившего события: нажатия кнопки, получения сообщения или иного
 * действия.
 * Также в этом классе хранится набор триггеров и связанный с ним набор задач.
 * Он содержит 3 публичных метода:
 *      1. Передача триггера, его обработка и запуск команды, если существует
 * такая привязка.
 *      2. Передача новой связки триггер-команда.
 *      3. Метод для удаления связки.
 */
#ifndef INTERPRETER_MANAGER_HPP
#define INTERPRETER_MANAGER_HPP

#include <Arduino.h>
#include <STM32FreeRTOS.h>

#include <map>

#include "FlashManager.hpp"
#include "Logger.hpp"
#include "TinyJS.h"
#include "TinyJS_Functions.h"

// TODO: Поничить многопоточное исполнение.
#define MAX_CONCURRENT_TASKS 1  // Макс кол-во одновременно работающих задач
#define QUEUE_SIZE 10           // Размер очереди команд

struct event {
  std::string trigger;
  std::string command;
};

class InterpreterManager {
  FlashManager flash;
  static CTinyJS *js;
  Logger logger;
  const char *pref = "IM";

 public:
  InterpreterManager(Logger &_logger): logger(_logger) {
    commandQueue = xQueueCreate(QUEUE_SIZE, sizeof(char *));
    taskCountSemaphore =
        xSemaphoreCreateCounting(MAX_CONCURRENT_TASKS, MAX_CONCURRENT_TASKS);
    xTaskCreate(taskManager, "TaskManager", 2048, this, 3, NULL);
  }

  void initialRead() {
    logger.log(level_of_detail::MAIN, pref, "Пытаюсь прочетать данные из Flash");
    if (flash.readFlash(subscribed_events)) {
      logger.log(level_of_detail::ALL, pref, "Найденный во flash словарь:");
      for (const auto &pair : subscribed_events) {
        logger.log(level_of_detail::ALL, pref, String(pair.first.c_str()) + " -> " + String(pair.second.c_str()));
      }
    }
  }

  void clear_event(){
    logger.log(level_of_detail::MAIN, pref, "Очистка памяти");
    subscribed_events.clear();
    flash.writeFlash(subscribed_events);
  }

  void setJSInterpreter(CTinyJS *interpreterJS){
    js = interpreterJS;
  }

  // По поступившему тириггеру нужно выполнить проверку
  // на наличие задачи по данному сигналу и если таковая присуствует,
  // запустить выполнение
  void enqueueTrigger(event new_event) {
    // TODO: проверить работоспособность
    logger.log(level_of_detail::MAIN, pref, "Проверка наличия триггера для исполнения");
    if (subscribed_events.count(new_event.trigger) > 0) {
      // Событие было найдено, нужно запустить команду на исполнение.
      std::string command = subscribed_events[new_event.trigger];
      char *commandCopy = (char *)pvPortMalloc(strlen(command.c_str()) + 1);
      if (commandCopy) {
        logger.log(level_of_detail::ALL, pref, "Отправка задачи в очередь на исполнение:" + String(new_event.trigger.c_str()));
        strcpy(commandCopy, command.c_str());
        xQueueSend(commandQueue, &commandCopy, portMAX_DELAY);
      }
    }
  }

  // Создание новой привязки триггер на событие + задача на исполнение.
  void createPair(event new_event) {
    if (subscribed_events.count(new_event.trigger) == 0) {
      logger.log(level_of_detail::ALL, pref, "Создание нового события");
      subscribed_events[new_event.trigger] = new_event.command;
      flash.writeFlash(subscribed_events);
    }
  }

  // Удаление привязки триггер-задача.
  void deletePair(event new_event) {
    auto iterat = subscribed_events.find(new_event.trigger);

    // Deleting the key-value pair using erase()
    if (iterat != subscribed_events.end()) {
      logger.log(level_of_detail::ALL, pref, "Удаление старого события");
      subscribed_events.erase(iterat);
      flash.writeFlash(subscribed_events);
    }
  }

 private:
  std::map<std::string, std::string> subscribed_events;

  // Очередь задач отправленных на выполенение после проверки.
  QueueHandle_t commandQueue;
  // Семафор, сигнализирующий о текущем количестве исполняемых задач.
  static SemaphoreHandle_t taskCountSemaphore;

  static void interpreterTask(void *pvParameters) {
    char *command = (char *)pvParameters;

    // Здесь выполняем интерпретацию команды
    js->execute(command);

    vPortFree(command);
    xSemaphoreGive(taskCountSemaphore);
    vTaskDelete(NULL);
  }

  static void taskManager(void *pvParameters) {
    InterpreterManager *self = static_cast<InterpreterManager *>(pvParameters);
    char *command;
    while (1) {
      if (xQueueReceive(self->commandQueue, &command, portMAX_DELAY) ==
          pdPASS) {
        if (xSemaphoreTake(self->taskCountSemaphore, portMAX_DELAY) == pdPASS) {
          xTaskCreate(interpreterTask, "InterpTask", 2048, command, 2, NULL);
        } else {
          vPortFree(command);
        }
      }
    }
  }
};

SemaphoreHandle_t InterpreterManager::taskCountSemaphore;
CTinyJS *InterpreterManager::js;
#endif