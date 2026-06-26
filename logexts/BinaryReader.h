#pragma once

class BinaryReader :
    public std::ifstream
{
public:
    BinaryReader();
    BinaryReader(const TCHAR *filepath);

    uint8_t ReadUint8();
    uint16_t ReadUint16();
    uint32_t ReadUint32();
    uint64_t ReadUint64();

    bool readBlock(uint8_t *buffer, std::streamsize n);
};

