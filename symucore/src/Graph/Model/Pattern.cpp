#include "Pattern.h"

using namespace SymuCore;

unsigned int Pattern::s_uniqueIDCounter = 0;

Pattern::Pattern() : m_uniqueID(s_uniqueIDCounter++), m_pLink(NULL), m_ePatternType(PT_Undefined), m_dbFuncClass(0.0)
{
    //Default constructor
}

Pattern::Pattern(Link* pLink, PatternType ePatternType) :
m_uniqueID(s_uniqueIDCounter++)
{
    m_pLink = pLink;
    m_ePatternType = ePatternType;
    m_dbFuncClass = 0.0;
}

Pattern::~Pattern()
{
    
}

Link* Pattern::getLink() const
{
    return m_pLink;
}

PatternType Pattern::getPatternType() const
{
    return m_ePatternType;
}

int Pattern::getUniqueID() const
{
    return m_uniqueID;
}

double Pattern::getFuncClass() const
{
    return m_dbFuncClass;
}

void Pattern::setFuncClass(double dbFuncClass)
{
    m_dbFuncClass = dbFuncClass;
}


