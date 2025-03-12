#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <Arduino.h>
enum level_of_detail { ALL = 3, MAIN = 2, IMPORTANT = 1, NOTHING = 0 };
class Logger {
 private:
  Stream* output;
  

  level_of_detail treshold;

 public:
  // Конструктор: принимает поток для логирования и уровень фильтрации сообщений
  Logger(Stream* stream = &Serial, level_of_detail _treshold = ALL) {
    output = stream;
    treshold = _treshold;
  }

  void setTreshold(level_of_detail _treshold) { treshold = _treshold; }

  // Метод для изменения потока вывода
  void setOutput(Stream* stream) { output = stream; }

  // Метод для вывода сообщений с префиксом
  void log(level_of_detail _treshold, const String& prefix, const String& message) {
    if (treshold >= _treshold) {
      output->println("[" + prefix + "] " + message);
    }
  }
};

#endif