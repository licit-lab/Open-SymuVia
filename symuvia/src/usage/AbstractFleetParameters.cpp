#include "stdafx.h"
#include "AbstractFleetParameters.h"

#include "ScheduleParameters.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

AbstractFleetParameters::AbstractFleetParameters()
{
    m_pScheduleParameters = NULL;
}

AbstractFleetParameters::~AbstractFleetParameters()
{
}

VehicleTypeUsageParameters & AbstractFleetParameters::GetUsageParameters()
{
    return m_UsageParameters;
}

void AbstractFleetParameters::SetScheduleParameters(ScheduleParameters * pScheduleParams)
{
    m_pScheduleParameters = pScheduleParams;
}
    
ScheduleParameters * AbstractFleetParameters::GetScheduleParameters()
{
    return m_pScheduleParameters;
}

AbstractFleetParameters * AbstractFleetParameters::Clone()
{
    AbstractFleetParameters * pResult = new AbstractFleetParameters();
    pResult->m_UsageParameters = m_UsageParameters;
    pResult->m_pScheduleParameters = m_pScheduleParameters;
    return pResult;
}

template void AbstractFleetParameters::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void AbstractFleetParameters::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void AbstractFleetParameters::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_UsageParameters);
    ar & BOOST_SERIALIZATION_NVP(m_pScheduleParameters);
}