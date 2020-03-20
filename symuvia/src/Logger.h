#pragma once
#ifndef LoggerH
#define LoggerH

#include "reseau.h"

#include <iostream>

class Logger
{
public:

    // ne pas hésiter à compléter le jour où on aura besoin de plus de niveaux
    enum LoggerSeverity {
        Info = 3,
        Warning = 4,
        Error = 5
    };

    enum LineState {
        NoNewLine,
        NewLineInPipe,
        NewLine
    };

public:
    Logger(const std::string & logFile, bool bEnableFileLog, LoggerSeverity severityFilter);
    virtual ~Logger();

    void SetNetwork(Reseau * pNetwork);

    template<typename T>
    Logger& operator << (const T& object)
    {
        if(m_pLogStream)
        {
            if(m_CurrentSeverityLevel >= m_FilterSeverityLevel)
            {
                if((m_eNewLine == NewLine) && (m_pNetwork != NULL) && (m_CurrentSeverityLevel >= Logger::Warning))
                {
                    (*m_pLogStream) << "[" << m_pNetwork->m_dbInstSimu << "s] ";
                }
                (*m_pLogStream) << object;
            }
        }
        //rmq. : si pas de fichier de log encore défini, on écrit les messages dans la console. En principe, ne devrait pas arriver !
        else if(m_bEnableFileLog && m_fileName.empty())
        {
            if(m_CurrentSeverityLevel >= m_FilterSeverityLevel)
            {
                std::cout << object;
            }
        }

        if(m_eNewLine == NewLineInPipe)
        {
            m_eNewLine = NewLine;
        } else if(m_eNewLine == NewLine)
        {
            m_eNewLine = NoNewLine;
        }
        return *this;
    }

    typedef std::ostream& (*ostream_manipulator)(std::ostream&);
    Logger& operator<<(ostream_manipulator pf)
    {
       if(pf == (std::basic_ostream<char>& (*)(std::basic_ostream<char>&)) &std::endl)
       {
           m_eNewLine = NewLineInPipe;
       }
       return operator<< <ostream_manipulator> (pf);
    }

    Logger& operator<<(LoggerSeverity severity)
    {
        m_CurrentSeverityLevel = severity;
        return *this;
    }

    const std::string & getFileName();

private:
    std::ofstream*  m_pLogStream;
    std::string     m_fileName;
    bool            m_bEnableFileLog;

    LoggerSeverity  m_FilterSeverityLevel;
    LoggerSeverity  m_CurrentSeverityLevel;

    LineState       m_eNewLine;

    Reseau         *m_pNetwork;
};

#endif // LoggerH