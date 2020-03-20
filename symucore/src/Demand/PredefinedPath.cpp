#include "PredefinedPath.h"

using namespace SymuCore;


PredefinedPath::PredefinedPath()
{
    m_pJunction = NULL;
}

PredefinedPath::PredefinedPath(const std::vector<Pattern*>& listPattern, Node * pJunction, const std::string & name, double dbCoeff)
: m_Path(listPattern)
{
    m_pJunction = pJunction;
    m_strName = name;
    m_dbCoeff = dbCoeff;
}

PredefinedPath::~PredefinedPath()
{

}

double PredefinedPath::getCoeff() const
{
    return m_dbCoeff;
}

const Path & PredefinedPath::getPath() const
{
    return m_Path;
}

const std::string & PredefinedPath::getStrName() const
{
    return m_strName;
}

Node * PredefinedPath::getJunction() const
{
    return m_pJunction;
}
