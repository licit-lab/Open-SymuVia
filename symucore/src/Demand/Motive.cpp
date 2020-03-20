#include "Motive.h"

using namespace SymuCore;


Motive::Motive() : m_strName()
{
    //default constructor
}

Motive::Motive(const std::string &strName)
{
    m_strName = strName;
}

Motive::~Motive()
{

}

const std::string & Motive::GetStrName() const
{
    return m_strName;
}

//setters
void Motive::SetStrName(const std::string &strName)
{
    m_strName = strName;
}
