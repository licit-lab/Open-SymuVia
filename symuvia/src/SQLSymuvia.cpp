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
	m_henv = NULL;
	m_hdbc = NULL;
}

DBSymuvia::~DBSymuvia(void)
{
	Close();
}

void DBSymuvia::Open(const std::string &sDSNName)
{
	m_sDSNName = sDSNName;
	SQLAllocEnv(&m_henv);
	SQLAllocConnect(m_henv, &m_hdbc);
#ifdef WIN32
	m_rc = SQLConnect(m_hdbc, (SQLWCHAR *)SystemUtil::ToWString(sDSNName).c_str(), SQL_NTS, NULL, 0, NULL, 0);
#else
	m_rc = SQLConnect(m_hdbc, (SQLCHAR *)sDSNName.c_str(), SQL_NTS, NULL, 0, NULL, 0);
#endif

	if (!MYSQLSUCCESS(m_rc))
	{
		// Désallouer les handles, et quitter.
		Close();
		throw ExceptionSymuvia("Unable to connect to DSN : '" + sDSNName + "'");
	}
}

void DBSymuvia::Close()
{
	if (m_hdbc)
	{
		SQLDisconnect(m_hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, m_hdbc);
		m_hdbc = NULL;
	}
	if (m_henv)
	{
		SQLFreeHandle(SQL_HANDLE_ENV, m_henv);
		m_henv = NULL;
	}
}

SQLSymuvia::SQLSymuvia(void)
{
	m_hstmt = NULL;
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
	m_rc = SQLAllocStmt(Database.m_hdbc, &m_hstmt);
	if (!MYSQLSUCCESS(m_rc))
	{
		// Désallouer les handles, et quitter.
		sqlDeconnect();
		std::ostringstream oss;
		oss << "Unable to connect to database of DSN : '" << Database.m_sDSNName.c_str() << "'";
		throw ExceptionSymuvia(oss.str());
	}
}

// Déconnexion
void SQLSymuvia::sqlDeconnect()
{
	if (m_hstmt)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
		m_hstmt = NULL;
	}
	releaseBuffers();
}

// Exécution d'une séquence SQL
void SQLSymuvia::sqlExecDirect(const char *cmdstr)
{
#ifdef WIN32
	m_rc = SQLExecDirect(m_hstmt, (SQLWCHAR *)SystemUtil::ToWString(cmdstr).c_str(), SQL_NTS);
#else
	m_rc = SQLExecDirect(m_hstmt, (SQLCHAR *)cmdstr, SQL_NTS);
#endif
	if (!MYSQLSUCCESS(m_rc)) //Error
	{
		// Désallouer les handles, et quitter.
		sqlDeconnect();

		std::ostringstream oss;
		oss << "Unable to process request : '" << cmdstr << "'";

		throw ExceptionSymuvia(oss.str());
	}
}

// Préparation d'une séquence SQL
void SQLSymuvia::sqlPrepare(const char *cmdstr)
{
#ifdef WIN32
	m_rc = SQLPrepare(m_hstmt, (SQLWCHAR *)SystemUtil::ToWString(cmdstr).c_str(), SQL_NTS);
#else
	m_rc = SQLPrepare(m_hstmt, (SQLCHAR *)cmdstr, SQL_NTS);
#endif
	if (!MYSQLSUCCESS(m_rc)) //Error
	{
		// Désallouer les handles, et quitter.
		sqlDeconnect();

		std::ostringstream oss;
		oss << "Unable to prepare request : '" << cmdstr << "'";

		throw ExceptionSymuvia(oss.str());
	}
}

// Exécution d'une séquence SQL qui a été préparée
void SQLSymuvia::sqlExec()
{
	m_rc = SQLExecute(m_hstmt);
	//if (!MYSQLSUCCESS(m_rc))   //Error
	//{
	//   //error_out();
	//   // Deallocate handles and disconnect.
	//   sqlDeconnect();

	//  throw ExceptionSymuvia("Impossible d'exécuter une requête SQL");
	//}
}

// Recherche une ligne de données
bool SQLSymuvia::sqlFetch()
{
	m_rc = SQLFetch(m_hstmt);
	return MYSQLSUCCESS(m_rc);
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
	SQLLEN cbData;
	SQLGetData(m_hstmt, nCol + 1, SQL_C_CHAR, szData, sizeof(szData), &cbData);
	std::string sValue = szData;
	return sValue;
}

// Renvoie la NCol-ième colonne (double) d'une ligne sous forme d'un double
double SQLSymuvia::getResultDouble(int nCol)
{
	double dValue;
	SQLLEN cbData;
	SQLGetData(m_hstmt, nCol + 1, SQL_C_DOUBLE, &dValue, sizeof(dValue), &cbData);
	return dValue;
}

// Renvoie la NCol-ième colonne (long) d'une ligne sous forme d'un double
long SQLSymuvia::getResultInt(int nCol)
{
	long dValue;
	SQLLEN cbData;
	SQLGetData(m_hstmt, nCol + 1, SQL_C_SLONG, &dValue, sizeof(dValue), &cbData);
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
		db.realSize = 0;
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

	SQLBindParameter(m_hstmt,
					 (SQLUSMALLINT)(nCol + 1),
					 SQL_PARAM_INPUT,
					 SQL_C_SLONG,
					 SQL_INTEGER,
					 0,
					 0,
					 &m_Buffer[nCol].iValue,
					 0,
					 nullptr);
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
		db.realSize = 0;
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
	m_Buffer[nCol].realSize = SQL_NTS;

	SQLBindParameter(m_hstmt,
					 (SQLUSMALLINT)(nCol + 1),
					 SQL_PARAM_INPUT,
					 SQL_C_CHAR,
					 SQL_VARCHAR,
					 max_size,
					 0,
					 m_Buffer[nCol].sValue,
					 m_Buffer[nCol].size,
					 nullptr);
}
