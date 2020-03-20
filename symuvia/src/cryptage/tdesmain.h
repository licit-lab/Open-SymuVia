#pragma once
////////////////////////////////////////////////////////////////////////////////
//
// tdesmain.h - Easy Triple-DES
// Copyright (C) 2009  Mehter Tariq <mehtertariq@integramicro.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////
#ifdef _MAIN_H_
////////////////////////////////////////////////////////////////////////////////
//
// Key: Key1, Key2, Key3
// Key1: 01 01 01 01 01 01 01 01 (odd parity set)
// Key2: 01 01 01 01 01 01 01 01 (odd parity set)
// Key3: 01 01 01 01 01 01 01 01 (odd parity set)
//
////////////////////////////////////////////////////////////////////////////////
unsigned char key[3][64]={
	{
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1
	},
	{
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1
	},
	{
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1,
	 0, 0, 0, 0, 0, 0, 0, 1
	}	
};
////////////////////////////////////////////////////////////////////////////////
//
// Plain Text: 80 00 00 00 00 00 00 00
//
////////////////////////////////////////////////////////////////////////////////
unsigned char pt[64]={
	1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};
////////////////////////////////////////////////////////////////////////////////
//
// Cipher Text: 95 F8 A5 E5 DD 31 D9 00
//
////////////////////////////////////////////////////////////////////////////////
unsigned char et[64];
////////////////////////////////////////////////////////////////////////////////
#else
////////////////////////////////////////////////////////////////////////////////
extern unsigned char key[3][64];
extern unsigned char pt[64];
extern unsigned char et[64];
////////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////////
