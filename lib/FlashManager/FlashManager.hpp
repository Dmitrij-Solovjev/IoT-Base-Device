#ifndef FLASH_MANAGER_HPP
#define FLASH_MANAGER_HPP

#include <Arduino.h>
#include <stm32f4xx_hal.h>
#include <map>
#include <cstring>

#define FLASH_SECTOR_ADDRESS  0x08060000  // Начало 7-го сектора (128КБ)
#define MAGIC_NUMBER          0xAABBCCDD  // Сигнатура для проверки данных

class FlashManager {
public:
    FlashManager() {}
    
    bool readFlash(std::map<std::string, std::string>& data);
    bool writeFlash(const std::map<std::string, std::string>& data);

private:
    void writeStringToFlash(uint32_t& addr, const std::string& str);
};

#endif