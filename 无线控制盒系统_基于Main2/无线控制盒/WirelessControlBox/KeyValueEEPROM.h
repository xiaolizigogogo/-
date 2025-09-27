#ifndef KeyValueEEPROM_h
#define KeyValueEEPROM_h

#include <Arduino.h>
#include <EEPROM.h>

// 简单的键值对EEPROM存储类
class KeyValueEEPROM {
public:
    // 构造函数
    KeyValueEEPROM() {}
    
    // 写入字节值
    void writeByte(int address, byte value) {
        EEPROM.write(address, value);
    }
    
    // 读取字节值
    byte readByte(int address) {
        return EEPROM.read(address);
    }
    
    // 写入整数
    void writeInt(int address, int value) {
        EEPROM.write(address, value & 0xFF);
        EEPROM.write(address + 1, (value >> 8) & 0xFF);
    }
    
    // 读取整数
    int readInt(int address) {
        return EEPROM.read(address) | (EEPROM.read(address + 1) << 8);
    }
    
    // 写入长整数
    void writeLong(int address, long value) {
        EEPROM.write(address, value & 0xFF);
        EEPROM.write(address + 1, (value >> 8) & 0xFF);
        EEPROM.write(address + 2, (value >> 16) & 0xFF);
        EEPROM.write(address + 3, (value >> 24) & 0xFF);
    }
    
    // 读取长整数
    long readLong(int address) {
        return EEPROM.read(address) | 
               (EEPROM.read(address + 1) << 8) | 
               (EEPROM.read(address + 2) << 16) | 
               (EEPROM.read(address + 3) << 24);
    }
    
    // 写入浮点数
    void writeFloat(int address, float value) {
        byte* p = (byte*)(void*)&value;
        for (int i = 0; i < sizeof(value); i++) {
            EEPROM.write(address++, *p++);
        }
    }
    
    // 读取浮点数
    float readFloat(int address) {
        float value = 0.0;
        byte* p = (byte*)(void*)&value;
        for (int i = 0; i < sizeof(value); i++) {
            *p++ = EEPROM.read(address++);
        }
        return value;
    }
};

#endif
