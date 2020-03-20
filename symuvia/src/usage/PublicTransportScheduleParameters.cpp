#include "stdafx.h"
#include "PublicTransportScheduleParameters.h"

#include "tools.h"
#include "SystemUtil.h"
#include "arret.h"
#include "Trip.h"
#include "TripLeg.h"
#include "Logger.h"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

PublicTransportScheduleParameters::PublicTransportScheduleParameters()
    : ScheduleParameters()
{
}

PublicTransportScheduleParameters::~PublicTransportScheduleParameters()
{
}

bool PublicTransportScheduleParameters::Load(XERCES_CPP_NAMESPACE::DOMNode * pNodeParams, Logger & loadingLogger)
{
    GetXmlAttributeValue(pNodeParams, "id", m_strID, &loadingLogger);

    std::string strTmp;
    GetXmlAttributeValue(pNodeParams, "dureearrets", strTmp, &loadingLogger);
	std::deque<std::string> split = SystemUtil::split(strTmp, ' ');
    for(size_t iSplit = 0; iSplit < split.size(); iSplit++)
    {
        m_StopTimes.push_back(atof(split[iSplit].c_str()));
    }

    return true;
}

bool PublicTransportScheduleParameters::AssignStopTimesToStops(Trip * pLigne, Logger & loadingLogger)
{
    size_t iStopTime = 0;

    // Gestion de l'origine de la ligne, s'il s'agit d'un arrêt
    Arret * pStop = dynamic_cast<Arret*>(pLigne->GetOrigin());
    if(pStop)
    {
        if(iStopTime >= m_StopTimes.size())
        {
            loadingLogger << Logger::Error << " ERROR : the number of values for 'dureearrets' for line " << pLigne->GetID() << " is different from the actual stations number for this line." << std::endl;
            loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
            return false;
        }
        m_mapStopTimes[pStop] = m_StopTimes[iStopTime++];
    }

    // Gestion des arrêts suivants
    for(size_t i = 0; i < pLigne->GetLegs().size(); i++)
    {
        pStop = dynamic_cast<Arret*>(pLigne->GetLegs()[i]->GetDestination());
        if(pStop)
        {
            if(iStopTime >= m_StopTimes.size())
            {
                loadingLogger << Logger::Error << " ERROR : the number of values for 'dureearrets' for line " << pLigne->GetID() << " is different from the actual stations number for this line." << std::endl;
                loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                return false;
            }
            m_mapStopTimes[pStop] = m_StopTimes[iStopTime++];
        }
    }

    // rmq. : condition !(iStopTime == 0 && m_StopTimes.size() == 1) pour ne pas renvoyer d'erreur si
    // aucun arrêt pour la ligne mais une durée d'arrêt (valeur par défaut dans le xsd)
    if(iStopTime != m_StopTimes.size() && !(iStopTime == 0 && m_StopTimes.size() == 1))
    {
        loadingLogger << Logger::Error << " ERREUR : the number of values for 'dureearrets' for line " << pLigne->GetID() << " is different from the actual stations number for this line." << std::endl;
        loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        return false;
    }

    return true;
}

const std::map<Arret*, double> & PublicTransportScheduleParameters::GetStopTimes()
{
    return m_mapStopTimes;
}

template void PublicTransportScheduleParameters::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void PublicTransportScheduleParameters::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void PublicTransportScheduleParameters::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ScheduleParameters);

    ar & BOOST_SERIALIZATION_NVP(m_StopTimes);
    ar & BOOST_SERIALIZATION_NVP(m_mapStopTimes);
}
