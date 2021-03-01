#include "stdafx.h"
#include "SQLSymuvia.h"

#include "SystemUtil.h"

#include <sstream>
#include <vector>

#ifndef WIN32
#include <string.h>
#endif

#define MYSQLSUCCESS(rc) ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))

DBSymuvia::DBSymuvia(void)
{

}

DBSymuvia::~DBSymuvia(void)
{
	Close();
}

void DBSymuvia::Open(const std::string &sDSNName)
{
	m_sDSNName = sDSNName;
}

void DBSymuvia::Close()
{

}

SQLSymuvia::SQLSymuvia(void)
{
	
}

SQLSymuvia::~SQLSymuvia(void)
{
	// Libérer la connexion et la mémoire
	sqlDeconnect();
}

// Libère les buffers des paramètres
void SQLSymuvia::releaseBuffers()
{
	size_t count;
	size_t i;

	count = m_Buffer.size();
	for (i = 0; i < count; i++)
	{
		if (m_Buffer[i].typ == DataBuffer::TYP_STR)
		{
			delete m_Buffer[i].sValue;
			m_Buffer[i].sValue = NULL;
		}
	}
	m_Buffer.clear();
}

// Connexion à la base de données
void SQLSymuvia::sqlConnect(DBSymuvia &Database)
{

}

// Déconnexion
void SQLSymuvia::sqlDeconnect()
{
	releaseBuffers();
}

// Exécution d'une séquence SQL
void SQLSymuvia::sqlExecDirect(const char *cmdstr)
{

}

// Préparation d'une séquence SQL
void SQLSymuvia::sqlPrepare(const char *cmdstr)
{

}

// Exécution d'une séquence SQL qui a été préparée
void SQLSymuvia::sqlExec()
{

}

// Recherche une ligne de données
bool SQLSymuvia::sqlFetch()
{
	return false;
}

void SQLSymuvia::sqlSkipData()
{
	while (sqlFetch())
	{
	}
}

// Renvoie la NCol-ième colonne (chaine) d'une ligne sous forme d'une chaine
std::string SQLSymuvia::getResultStr(int nCol)
{
	char szData[512];

	std::string sValue = szData;
	return sValue;
}

// Renvoie la NCol-ième colonne (double) d'une ligne sous forme d'un double
double SQLSymuvia::getResultDouble(int nCol)
{
	double dValue;
	
	return dValue;
}

// Renvoie la NCol-ième colonne (long) d'une ligne sous forme d'un double
long SQLSymuvia::getResultInt(int nCol)
{
	long dValue;
	
	return dValue;
}

// Ajoute le "nCol-ième" paramètre sous forme entière
void SQLSymuvia::AddParamInt(size_t nCol, long nValue)
{
	if (nCol <= m_Buffer.size())
	{
		DataBuffer db;
		db.typ = DataBuffer::TYP_INT;
		db.iValue = 0;
		//db.realSize = 0;
		m_Buffer.resize(nCol + 1, db);
	}

	if (m_Buffer[nCol].typ == DataBuffer::TYP_STR)
	{
		if (m_Buffer[nCol].sValue)
		{
			delete[] m_Buffer[nCol].sValue;
		}
	}

	m_Buffer[nCol].typ = DataBuffer::TYP_INT;
	m_Buffer[nCol].iValue = nValue;

	/* SqlBindParameter */
}

void SQLSymuvia::AddParamStr(size_t nCol, char *s, int max_size)
{
	std::string ws = s;
	AddParamStr(nCol, ws, max_size);
}

// Ajoute le "nCol-ième" paramètre sous forme de chaine
void SQLSymuvia::AddParamStr(size_t nCol, const std::string &s, int max_size)
{
	if (nCol <= m_Buffer.size())
	{
		DataBuffer db;
		db.typ = DataBuffer::TYP_INT;
		db.iValue = 0;
		//db.realSize = 0;
		m_Buffer.resize(nCol + 1, db);
	}

	if (m_Buffer[nCol].typ == DataBuffer::TYP_STR)
	{
		if (m_Buffer[nCol].sValue)
		{
			delete[] m_Buffer[nCol].sValue;
		}
	}

	m_Buffer[nCol].typ = DataBuffer::TYP_STR;
	m_Buffer[nCol].size = max_size;
	m_Buffer[nCol].sValue = new char[s.size() + 1];
	//wcscpy(m_Buffer[nCol].sValue, s.c_str());
#ifdef WIN32
	strcpy_s(m_Buffer[nCol].sValue, s.size() + 1, s.c_str());
#else
	strcpy(m_Buffer[nCol].sValue, s.c_str());
#endif

	/* SqlBindParameter */
}
