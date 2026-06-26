#include "pch.h"
#include "BinaryReader.h"

BinaryReader::BinaryReader() {

}

BinaryReader::BinaryReader(const TCHAR *filepath) : std::ifstream(filepath, std::ifstream::binary) {

}

uint8_t BinaryReader::ReadUint8() {
    uint8_t output;
    read(reinterpret_cast<char*>(&output), 1);
    return output;
}
uint16_t BinaryReader::ReadUint16() {
    uint16_t output;
    read(reinterpret_cast<char*>(&output), 2);
    return output;
}
uint32_t BinaryReader::ReadUint32() {
    uint32_t output;
    read(reinterpret_cast<char*>(&output), 4);
    return output;
}
uint64_t BinaryReader::ReadUint64() {
    uint64_t output;
    std::ifstream::read(reinterpret_cast<char*>(&output), 8);
    return output;
}

bool BinaryReader::readBlock(uint8_t *buffer, std::streamsize n){
    read(reinterpret_cast<char*>(buffer), n);
    return true;
}