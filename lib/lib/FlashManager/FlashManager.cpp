#include <iostream>
#include "FlashManager.hpp"

bool FlashManager::readFlash(std::map<std::string, std::string>& data) {
    uint32_t* ptr = (uint32_t*)FLASH_SECTOR_ADDRESS;
    if (*ptr != MAGIC_NUMBER) return false;  // Нет данных

    uint8_t* dataPtr = (uint8_t*)(ptr + 1);
    while (*dataPtr != 0xFF) {
        std::string key = (char*)dataPtr;
        dataPtr += key.length() + 1;
        std::string value = (char*)dataPtr;
        dataPtr += value.length() + 1;
        data[key] = value;
    }
    return true;
}

bool FlashManager::writeFlash(const std::map<std::string, std::string>& data) {
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError;

    eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    eraseInit.Sector = FLASH_SECTOR_7;
    eraseInit.NbSectors = 1;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    HAL_FLASHEx_Erase(&eraseInit, &sectorError);
    
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_SECTOR_ADDRESS, MAGIC_NUMBER);
    uint32_t addr = FLASH_SECTOR_ADDRESS + 4;
    
    for (const auto& pair : data) {
        writeStringToFlash(addr, pair.first);
        writeStringToFlash(addr, pair.second);
    }
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr, 0xFF);  // Завершающий байт
    HAL_FLASH_Lock();
    return true;
}

void FlashManager::writeStringToFlash(uint32_t& addr, const std::string& str) {
    for (size_t i = 0; i <= str.length(); i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr++, str[i]);
    }
}