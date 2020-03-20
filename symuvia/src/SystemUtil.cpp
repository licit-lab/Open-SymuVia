#include "stdafx.h"
#include "SystemUtil.h"

#include "SymuviaVersion.h"

#include <algorithm>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <float.h>
#include <string.h>
#include <iostream>
#include <vector>

#ifndef WIN32
#include <dlfcn.h>
#endif

SystemUtil::SystemUtil(void)
{
}

SystemUtil::~SystemUtil(void)
{
}

#ifdef WIN32
HMODULE SystemUtil::m_hInstance = NULL;
#endif

std::string SystemUtil::GetModulePath()
{
#ifdef WIN32
	wchar_t lpBuff[512];
	DWORD size;

	size = ::GetModuleFileName(m_hInstance, lpBuff, sizeof(lpBuff)/sizeof(lpBuff[0]));

	if (size >= 0)
	{
		lpBuff[size] = 0;
	}

	return SystemUtil::ToString(std::wstring(lpBuff));
#else
    std::string strPath = "";
    Dl_info info;
    if(dladdr((const void *)(&SystemUtil::GetModulePath), &info))
    {
        strPath = info.dli_fname;
    }
    else
    {
        std::cerr << "Error in GetModulePath..." << std::endl;
    }
    return strPath;
#endif
}

std::string SystemUtil::GetFilePath(const char * sFileName)
{
	std::string sPath = GetModulePath();
	size_t idx = sPath.rfind(DIRECTORY_SEPARATOR);
	if (idx != std::string::npos)
	{
		sPath = sPath.substr(0, idx+1) + sFileName;
	}
	else
	{
		sPath = sFileName;
	}
	return sPath;
}

std::string SystemUtil::GetFileName(const std::string & filePath)
{
    std::string sName;

    // on enlève le chemin du répertoire
    size_t idx = filePath.rfind(DIRECTORY_SEPARATOR);
	if (idx != std::string::npos)
	{
		sName = filePath.substr(idx+1);
	}
	else
	{
		sName = filePath;
	}

    // on enlève le .py
    idx = sName.rfind(".py");
    if (idx != std::string::npos)
	{
		sName = sName.substr(0, idx);
	}

	return sName;
}

unsigned long SystemUtil::GetSmallFileSize(const std::string & sFilePath)
{
#ifdef WIN32
	WIN32_FIND_DATA findData;
	
	HANDLE handle = FindFirstFile(SystemUtil::ToWString(sFilePath).c_str(), &findData);

	if (handle == INVALID_HANDLE_VALUE) 
	{
		return 0;
	} 
	else 
	{
		FindClose(handle);
		return findData.nFileSizeLow;
	}
#else
    struct stat filestatus;
    if(stat( sFilePath.c_str(), &filestatus ) == 0)
    {
        return filestatus.st_size;
    }
    else
    {
        return 0;
    }
#endif
}

std::string SystemUtil::ReadSmallFile(const std::string & sFilePath)
{
	std::string sData;
	unsigned long FileSize = SystemUtil::GetSmallFileSize(sFilePath);

	if (FileSize == 0)
	{
		return std::string();
	}

	std::filebuf fb;	
	std::ifstream Fic(sFilePath.c_str(),std::ios::binary);


	char * Buffer = new char[FileSize];
	Fic.read(Buffer, FileSize);

	if (Fic.fail())
	{
		delete [] Buffer;
		fb.close();
		return std::string();
	}

	for (unsigned long i = 0; i<FileSize; i++)
	{
		sData = sData + Buffer[i];
	}
	delete[] Buffer;
	fb.close();
	return sData;
}

std::string SystemUtil::GetFileVersion(std::string sPathA)
{
#ifdef WIN32
	DWORD dwArg;
	DWORD dwInfoSize;
	std::string sPath;

	if (sPathA.size() == 0)
	{
		sPath = GetModulePath();
	}
	else
	{
		sPath = sPathA;
	}

	dwInfoSize = GetFileVersionInfoSize(SystemUtil::ToWString(sPath).c_str(), &dwArg);

	if(0 == dwInfoSize)
	{
		//"Pas d'informations de version disponibles"
	    return "";
	}

	BYTE* lpBuff = new BYTE[dwInfoSize];

	if(!lpBuff)
	{
	    //"Mémoire insuffisante"
	    return "";
	}

	if(0 == GetFileVersionInfo(SystemUtil::ToWString(sPath).c_str(), 0, dwInfoSize, lpBuff))
	{
	    //"Erreur lors de la récupération des informations de version"
	    return "";
	}

	VS_FIXEDFILEINFO *vInfo;

	UINT uInfoSize;

	if(0 == VerQueryValue(lpBuff, TEXT("\\"),
		    (LPVOID*)&vInfo,
		    &uInfoSize))
	{
	    //"Informations de version non disponibles"
	    delete lpBuff;
	    return "";
	}

	if(0 == uInfoSize)
	{
	    //"Informations de version non disponibles"
	    delete lpBuff;
	    return "";
	}

	std::string sres;
	char lpBufVer[512];
	sprintf(lpBufVer, "%d.%d.%d.%d",	HIWORD(vInfo->dwFileVersionMS), 
										LOWORD(vInfo->dwFileVersionMS), 
										HIWORD(vInfo->dwFileVersionLS),
										LOWORD(vInfo->dwFileVersionLS));

	delete [] lpBuff;

	sres = lpBufVer;

	return sres;
#else
    // version linux (qui fonctionnerait aussi sous Windows, d'ailleurs)
    return SYMUVIA_VERSION_STRING;
#endif
}

#ifdef WIN32
std::wstring SystemUtil::ToWString(const std::string &s)
{
	std::wstring sw;
	int n = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, s.c_str(), -1, NULL, 0);
	wchar_t *lpsW;
	lpsW = new wchar_t[n];
	int nb = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, s.c_str(), -1, lpsW, n);	
	if (nb > 0)
	{
		sw = lpsW;
	}
	else
	{
		sw = L"";
	}
	delete[] lpsW;
	return sw;
}

std::string SystemUtil::ToString(const std::wstring &sw)
{
	std::string s;
	int n = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DISCARDNS, sw.c_str(), -1, NULL, 0, NULL, NULL);
	char *lps;
	lps = new char[n];
	int nb = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DISCARDNS, sw.c_str(), -1, lps, n, NULL, NULL);
	if (nb > 0)
	{
		s = lps;
	}
	else
	{
		s = "";
	}
	delete[] lps;
	return s;
}

#endif

std::string SystemUtil::ToString(int precision, double d)
{
	std::string sres;

	if (d < 0)
	{
		sres = "-";
		d = -d;
	}

	if (d > DBL_MAX)
	{
		sres = sres + "Infini";
		return sres;
	}
	
	char buf[512];
	sprintf(buf, "%.*f", precision, d);

	sres += buf;

	return sres;
}

double SystemUtil::ToDouble(const std::string &s)
{
	return atof(s.c_str());
}

int SystemUtil::ToInt32(const std::string &s)
{
	return atoi(s.c_str());
}

void SystemUtil::trim(std::string &sw)
{
	size_t idxd, idxf;

	if (sw.size() == 0)
		return;

	idxd = sw.find_first_not_of(" \t");

	if (idxd == std::string::npos)
	{
		sw.clear();
		return;
	}

	idxf = sw.find_last_not_of(" \t");

	std::string s;

	s = sw.substr(idxd, idxf - idxd + 1);
	sw = s;
}


void SystemUtil::split(const std::string & src, const char delim, std::deque<std::string> &splt)
{
	size_t p = 0;
	size_t q = src.find (delim, p);
	splt.clear();
	while (q != std::string::npos) 
	{
		std::string s = src.substr(p, q-p);
		splt.push_back (s);
		p = q;
		p++;
		q = src.find (delim, p);
	}
	if (p < src.size())
		splt.push_back (src.substr(p));
}

std::deque<std::string> SystemUtil::split(const std::string & src, const char delim)
{
	std::deque<std::string> v;
	std::string::const_iterator p = src.begin ();
	std::string::const_iterator q =	find (p, src.end (), delim);
	while (q != src.end ())
	{
		v.push_back (std::string (p, q));
		p = q;
		q = find (++p, src.end (), delim);
	}
	if (p != src.end ())
		v.push_back (std::string (p, src.end ()));
	return v;
}

std::vector<std::string> SystemUtil::split2vct(const std::string & src, const char delim)
{
	std::vector<std::string> v;
	std::string::const_iterator p = src.begin();
	std::string::const_iterator q = find(p, src.end(), delim);
	while (q != src.end())
	{
		v.push_back(std::string(p, q));
		p = q;
		q = find(++p, src.end(), delim);
	}
	if (p != src.end())
		v.push_back(std::string(p, src.end()));
	return v;
}

bool SystemUtil::FileExists(const std::string & sPath)
{
#ifdef WIN32
	struct stat buffer;
	int   iRetTemp = 0;

	memset ((void*)&buffer, 0, sizeof(buffer));

	iRetTemp = stat(sPath.c_str(), &buffer);

	if (iRetTemp == 0)
	{
		if (buffer.st_mode & _S_IFREG)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
#else
    struct stat filestatus;
    if(stat( sPath.c_str(), &filestatus ) == 0)
    {
        return S_ISREG(filestatus.st_mode);
    }
    else
    {
        return false;
    }
#endif
}

bool SystemUtil::FolderExists(const std::string & sPath)
{
#ifdef WIN32
	struct stat buffer;
	int   iRetTemp = 0;

	memset ((void*)&buffer, 0, sizeof(buffer));

	iRetTemp = stat(sPath.c_str(), &buffer);

	if (iRetTemp == 0)
	{
		if (buffer.st_mode & _S_IFDIR)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
#else
    struct stat filestatus;
    if(stat( sPath.c_str(), &filestatus ) == 0)
    {
        return S_ISDIR(filestatus.st_mode);
    }
    else
    {
        return false;
    }
#endif
}

std::string SystemUtil::ConcatPath(const std::string & path1, const std::string & path2)
{
	std::string st1, st2;
	size_t s1, s2;

	s1 = path1.size();
	if (s1 == 0)
	{
		st1.clear();
	}
	else
	{
		if (path1[s1-1] == DIRECTORY_SEPARATOR)
		{
			st1 = path1.substr(0, s1-1);
		}
		else
		{
			st1 = path1;
		}
	}

	s2 = path2.size();
	if (s2 == 0)
	{
		st2.clear();
	}
	else
	{
		if (path2[s2-1] == DIRECTORY_SEPARATOR)
		{
			st2 = path2.substr(1);
		}
		else
		{
			st2 = path2;
		}
	}

	return st1 + DIRECTORY_SEPARATOR + st2;
}


