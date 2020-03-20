#include "stdafx.h"
#include "PublicTransportFleetParameters.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

PublicTransportFleetParameters::PublicTransportFleetParameters()
{
    m_nLastNbMontees = 0;
    m_nLastNbDescentes = 0;
}

PublicTransportFleetParameters::~PublicTransportFleetParameters()
{

}

void PublicTransportFleetParameters::SetLastNbMontees(int lastNbMontees)
{
    m_nLastNbMontees = lastNbMontees;
}
    
int PublicTransportFleetParameters::GetLastNbMontees()
{
    return m_nLastNbMontees;
}

void PublicTransportFleetParameters::SetLastNbDescentes(int lastNbDescentes)
{
    m_nLastNbDescentes = lastNbDescentes;
}
    
int PublicTransportFleetParameters::GetLastNbDescentes()
{
    return m_nLastNbDescentes;
}

AbstractFleetParameters * PublicTransportFleetParameters::Clone()
{
    PublicTransportFleetParameters * pResult = new PublicTransportFleetParameters();
    pResult->m_UsageParameters = m_UsageParameters;
    pResult->m_nLastNbMontees = m_nLastNbMontees;
    pResult->m_nLastNbDescentes = m_nLastNbDescentes;
    return pResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void PublicTransportFleetParameters::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void PublicTransportFleetParameters::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void PublicTransportFleetParameters::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractFleetParameters);

    ar & BOOST_SERIALIZATION_NVP(m_nLastNbMontees);
    ar & BOOST_SERIALIZATION_NVP(m_nLastNbDescentes);
}