#include "VehicleType.h"

using namespace SymuCore;


VehicleType::VehicleType() : m_strName()
{
    //default constructor
}

VehicleType::VehicleType(const std::string &strName)
{
    m_strName = strName;
}

VehicleType::~VehicleType()
{

}

const std::string & VehicleType::getStrName() const
{
    return m_strName;
}

//setters
void VehicleType::setStrName(const std::string &strName)
{
    m_strName = strName;
}
