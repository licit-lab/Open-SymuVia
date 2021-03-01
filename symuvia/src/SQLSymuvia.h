#pragma once

#include <string>
#include <deque>



//
// Classe d'exception
//
class ExceptionSymuvia
{
public:
	ExceptionSymuvia() {};
	ExceptionSymuvia(std::string sMsgError) { m_strError = sMsgError; };
	std::string m_strError;
};

//
//	Classe de gestion de la base de donn�es Symuvia
//
class DBSymuvia
{
public:
	DBSymuvia(void);
	virtual ~DBSymuvia(void);

	void Open(const std::string & sDSNName);

	void Close();

public:
	std::string m_sDSNName;			    // Nom du DSN
};

//
// Structure des buffers des param�tres des s�quences SQL
//
typedef struct
{
	enum {
		TYP_INT,
		TYP_STR
	} typ;
	union
	{
		long iValue;
		struct
		{
			char * sValue;
			int size;
		};
	};
	
} DataBuffer;

//
// Classe d'interrogation SQL
//
class SQLSymuvia
{
public:
	// Cr�ateurs, destructeurs
	SQLSymuvia(void);
	virtual ~SQLSymuvia(void);

	std::deque<DataBuffer> m_Buffer;	// Liste des buffers des param�tres

	// Connexion � une base de donn�es
	void sqlConnect(DBSymuvia &Database);
	// D�connexion de la base de donn�es
	void sqlDeconnect();
	// Ex�cution d'une s�quence SQL
	void sqlExecDirect(const char * cmdstr);
	// Pr�paration d'une s�quence SQL
	void sqlPrepare(const char * cmdstr);
	// Ex�cution d'une s�quence SQL qui a �t� pr�par�e
	void sqlExec();
	// Parcourir les donn�es (s'il y en a) jusque la fin
	void sqlSkipData();
	// Recherche la ligne suivante de donn�es
	bool sqlFetch();

	// Ajoute le nCol-i�me param�tre (entier), nCol commence � z�ro
	void AddParamInt(size_t nCol, long nValue);
	// Ajoute le nCol-i�me param�tre (chaine), nCol commence � z�ro
	void AddParamStr(size_t nCol, const std::string & s, int max_size);
	void AddParamStr(size_t nCol, char * s, int max_size);

	// Renvoie le nCol-i�me champ (chaine) d'une ligne de donn�es (nCol commence � 0)
	std::string getResultStr(int nCol);
	// Renvoie le nCol-i�me champ (double) d'une ligne de donn�es (nCol commence � 0)
	double getResultDouble(int nCol);
	// Renvoie le nCol-i�me champ (long) d'une ligne de donn�es (nCol commence � 0)
	long getResultInt(int nCol);

private:
	// Lib�re les buffers des param�tres
	void releaseBuffers();
};
