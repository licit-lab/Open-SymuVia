#include "PublicTransportLine.h"

using namespace SymuCore;


PublicTransportLine::PublicTransportLine() : m_strName()
{
    //default constructor
}

PublicTransportLine::PublicTransportLine(const std::string& strName)
{
    m_strName = strName;
}

PublicTransportLine::~PublicTransportLine()
{
    
}

const std::string & PublicTransportLine::getStrName() const
{
    return m_strName;
}

//setters
void PublicTransportLine::setStrName(const std::string &strName)
{
    m_strName = strName;
}
