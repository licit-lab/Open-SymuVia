#include "stdafx.h"
#include "Cryptage3DES.h"

extern "C"
{
#include "tdes.h"
#include <string.h>

#define _MAIN_H_
#include "tdesmain.h"
}


Cryptage3DES::Cryptage3DES(void)
{
}

Cryptage3DES::~Cryptage3DES(void)
{
}

unsigned char Cryptage3DES::m_key_ring[3][8]={
	{79, 211, 250, 224, 4, 103, 174, 238},
	{110, 231, 17, 127, 211, 46, 165, 32},
	{204, 20, 185, 116, 161, 149, 51, 81}
};

void Cryptage3DES::Init()
{
	tdes_init(m_key_ring);
}

void Cryptage3DES::Init(unsigned char key_ring[3][8])
{
	tdes_init(key_ring);
}

void Cryptage3DES::Crypter(const std::string &in, std::string &out)
{
	std::string sIn;
	sIn = in;
	size_t lg = sIn.length();
	size_t lgout;
	lgout = lg / 8;
	if (lg % 8 != 0) lgout++;
	lgout = lgout * 8;
	if (lg != lgout)
	{
		sIn.resize(lgout, '\0');
	}
	unsigned char * res = new unsigned char[lgout];
	memset(res, 0, lgout);
	tdes_encrypt((unsigned long)lgout, (unsigned char*)sIn.c_str(), res);
	out.clear();
	for (size_t i=0; i<lgout; i++)
	{
		out = out + (char)res[i];
	}
	delete [] res;
}

void Cryptage3DES::Decrypter(const std::string &in, std::string &out)
{
	size_t lg = in.length();
	if (lg % 8 != 0) //c'est une erreur
	{
		out.clear();
		return;
	}
	unsigned char * res = new unsigned char[lg];
	tdes_decrypt((unsigned long)lg, (unsigned char*)in.c_str(), res);
	out.clear();
	for (size_t i=0; i<lg; i++)
	{
		out = out + (char)res[i];
	}
	delete [] res;
}
