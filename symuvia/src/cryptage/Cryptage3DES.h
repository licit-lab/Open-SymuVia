#pragma once

#include <string>

class Cryptage3DES
{
public:
	Cryptage3DES(void);
	~Cryptage3DES(void);

public:
	static unsigned char m_key_ring[3][8];
	static void Init();
	static void Init(unsigned char key_ring[3][8]);
	static void Crypter(const std::string &in, std::string &out);
	static void Decrypter(const std::string &in, std::string &out);
};
