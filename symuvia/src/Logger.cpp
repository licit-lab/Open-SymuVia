#include "stdafx.h"
#include "Logger.h"

#include "TimeUtil.h"
#include "regulation/PythonUtils.h"

#include <iostream>
#include <fstream>

Logger::Logger(const std::string & logFile, bool bEnableFileLog, LoggerSeverity severityFilter)
{
    m_pNetwork = NULL;
    m_eNewLine = NewLine;
    m_CurrentSeverityLevel = Info;
    m_FilterSeverityLevel = severityFilter;
    m_fileName = logFile;
    m_bEnableFileLog = bEnableFileLog;
    if(bEnableFileLog && !m_fileName.empty())
    {
        m_pLogStream = new std::ofstream(logFile.c_str());

        SDateTime	tNow;
	    tNow = SDateTime::Now();
        (*this) << tNow.GetDay()<<"/"<<tNow.GetMonth()<<"/"<<tNow.GetYear()<<std::endl;  // inscrit l'heure et la date
	    (*this) << tNow.GetHour()<<":"<<tNow.GetMinute()<<":"<<tNow.GetSecond()<<std::endl<<std::endl;
    }
    else
    {
        m_pLogStream = NULL;
    }
}

Logger::~Logger()
{
    if(m_pLogStream && m_pLogStream->is_open())
    {
        m_pLogStream->close();
    }
    if(m_pLogStream)
    {
        delete m_pLogStream;
    }
}

void Logger::SetNetwork(Reseau * pNetwork)
{
    m_pNetwork = pNetwork;
}

const std::string & Logger::getFileName()
{
    return m_fileName;
}