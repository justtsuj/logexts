#include <fstream>
#include <Windows.h>
#include "BinaryWriter.h"

BinaryWriter::BinaryWriter()
{
}

BinaryWriter::BinaryWriter(const TCHAR* filepath) : std::ofstream(filepath, std::ofstream::binary)
{
}

BinaryWriter::~BinaryWriter()
{
}

void BinaryWriter::WriteUint32(uint32_t i)
{
	operator<<(i);
}