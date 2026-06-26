#pragma once

class BinaryWriter : public std::ofstream
{
public:
	BinaryWriter();
	BinaryWriter(const TCHAR *filepath);
	~BinaryWriter();

	void WriteUint32(uint32_t i);
};

