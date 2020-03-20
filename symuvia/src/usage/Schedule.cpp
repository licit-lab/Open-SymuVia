#include "stdafx.h"
#include "Schedule.h"

#include "reseau.h"
#include "Logger.h"
#include "Xerces/XMLUtil.h"
#include "TimeUtil.h"
#include "ScheduleParameters.h"

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>

#include <boost/serialization/vector.hpp>

#include <boost/math/special_functions/trunc.hpp>

XERCES_CPP_NAMESPACE_USE

Schedule::Schedule()
{
    m_LastStartElementIndex = 0;
}

Schedule::~Schedule()
{
    for(size_t i = 0; i < m_LstElements.size(); i++)
    {
        delete m_LstElements[i];
    }
}

const std::vector<ScheduleElement*> & Schedule::GetLstElements()
{
    return m_LstElements;
}

int Schedule::GetLastStartElementIndex()
{
    return m_LastStartElementIndex;
}

void Schedule::SetLastStartElementIndex(int index)
{
    m_LastStartElementIndex = index;
}

bool Schedule::Load(DOMNode * pXMLNode, Reseau * pNetwork, Logger & loadingLogger, const std::string & strID,
                    const std::map<std::string, ScheduleParameters*> & scheduleParams, std::map<ScheduleElement*, ScheduleParameters*> & mapScheduleParams)
{
    XMLSize_t countj = pXMLNode->getChildNodes()->getLength();
	for(XMLSize_t j=0; j<countj;j++)
    {            
        // Chargement de la variante temporelle de la demande 
		DOMNode * xmlChildj = pXMLNode->getChildNodes()->item(j);
		if (xmlChildj->getNodeType() != DOMNode::ELEMENT_NODE) continue;

        std::string nodeName(US(xmlChildj->getNodeName()));
        ScheduleElement * pElement = NULL;
        if(!nodeName.compare("HORAIRE"))
        {
            pElement = new ScheduleInstant();
        }
        else if(!nodeName.compare("FREQUENCE"))
        {
            pElement = new ScheduleFrequency();
        }
        else
        {
            // Impossible car validé par XSD avant qui ne permet que ces deux possibilités
            assert(false);
        }

        if(pElement)
        {
            if(!pElement->Load(xmlChildj, pNetwork, loadingLogger, strID))
            {
                return false;
            }
            m_LstElements.push_back(pElement);

            // récupération des paramètres associés éventuellement
            std::string strParamsID;
            if(GetXmlAttributeValue(xmlChildj, "id_parametres", strParamsID, &loadingLogger))
            {
                std::map<std::string, ScheduleParameters*>::const_iterator iter = scheduleParams.find(strParamsID);
                if(iter != scheduleParams.end())
                {
                    mapScheduleParams[pElement] = iter->second;
                }
            }
        }
    }

    // Vérification de la chronologie temporelle du calendrier !
    for(size_t i = 1; i < m_LstElements.size(); i++)
    {
        ScheduleElement * pPrev = m_LstElements[i-1];
        ScheduleElement * pNext = m_LstElements[i];
        if(pNext->GetStartTime() < pPrev->GetStopTime())
        {
            loadingLogger << Logger::Error << " ERROR : the end time of a schedule element must be before or the same as the next element start time for element " << strID << std::endl;
            loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
            return false;
        }
    }

    return true;
}

bool Schedule::CheckTime(double dbInstant, double dbTimeStep, ScheduleElement ** pScheduleElement)
{
    // Si aucun élément programmé, on ne déclenche rien
    if(m_LstElements.empty())
        return false;

    // Sinon, on parcours les éléments à partir de l'élément courant
    bool bTrigger = false;
    bool bTooOld;
    while(m_LastStartElementIndex < (int)m_LstElements.size())
    {
        ScheduleElement * pScheduleElementCandidate = m_LstElements[m_LastStartElementIndex];

        // Pour gestion du recouvrement avec la plage temporelle définie dans l'élément suivant
        bool bHasNextConsecutiveElement = m_LastStartElementIndex + 1 < (int)m_LstElements.size() && m_LstElements[m_LastStartElementIndex + 1]->GetStartTime() == pScheduleElementCandidate->GetStopTime();

        // On regarde si l'élément se déclenche pendant le pas de temps courant
        bTrigger = pScheduleElementCandidate->CheckTime(dbInstant - dbTimeStep, dbInstant, bHasNextConsecutiveElement, bTooOld);

        // Si l'élément est dépassé, on passe au suivant
        if(bTooOld)
        {
            m_LastStartElementIndex++;
        }
        // Sinon, on reste sur l'élément courant, avec le résultat indiqué par bTrigger
        else
        {
            if(bTrigger)
            {
                *pScheduleElement = pScheduleElementCandidate;
            }
            break;
        }
    }

    return bTrigger;
}

double Schedule::GetInstantaneousFrequency(double dbInstant)
{
    ScheduleElement * pPreviousElement = NULL, * pPrevPreviousElement = NULL, * pCurrentElement = NULL, * pNextElement = NULL, *pNextNextElement = NULL;

    for (size_t iElement = 0; iElement < m_LstElements.size(); iElement++)
    {
        ScheduleElement * pScheduleElementCandidate = m_LstElements[iElement];

        if (pScheduleElementCandidate->GetStopTime() < dbInstant)
        {
            pPrevPreviousElement = pPreviousElement;
            pPreviousElement = pScheduleElementCandidate;
        }
        else if (pScheduleElementCandidate->GetStartTime() <= dbInstant
              && pScheduleElementCandidate->GetStopTime() >= dbInstant)
        {
            pCurrentElement = pScheduleElementCandidate;
        }
        else
        {
            assert(pScheduleElementCandidate->GetStartTime() > dbInstant);

            if (pNextElement)
            {
                pNextNextElement = pScheduleElementCandidate;
                break;
            }
            else
            {
                pNextElement = pScheduleElementCandidate;
            }
        }
    }

    ScheduleFrequency * pFreq = dynamic_cast<ScheduleFrequency*>(pCurrentElement);
    if (pFreq)
    {
        return pFreq->GetFrequency();
    }
    else
    {
        ScheduleElement * pElement1 = NULL, *pElement2 = NULL;

        if (pPreviousElement && pCurrentElement)
        {
            pElement1 = pPreviousElement;
            pElement2 = pCurrentElement;
        }
        else if (pCurrentElement && pNextElement)
        {
            pElement1 = pCurrentElement;
            pElement2 = pNextElement;
        }
        else if (pPreviousElement && pNextElement)
        {
            pElement1 = pPreviousElement;
            pElement2 = pNextElement;
        }
        else if (pPrevPreviousElement && pPreviousElement)
        {
            pElement1 = pPrevPreviousElement;
            pElement2 = pPreviousElement;
        }
        else if (pNextElement && pNextNextElement)
        {
            pElement1 = pNextElement;
            pElement2 = pNextNextElement;
        }

        if (pElement1 && pElement2)
        {
            double dbPrevDepartureTime;
            ScheduleFrequency * pFreq = dynamic_cast<ScheduleFrequency*>(pElement1);
            if (pFreq)
            {
                dbPrevDepartureTime = pFreq->GetStartTime() + pFreq->GetFrequency() * boost::math::trunc((pFreq->GetStopTime() - pFreq->GetStartTime()) / pFreq->GetFrequency());
            }
            else
            {
                dbPrevDepartureTime = pElement1->GetStopTime();
            }
            return pElement2->GetStartTime() - dbPrevDepartureTime;
        }
        else if (pNextElement)
        {
            // Cas ajouté le 07/01/2019 pour éviter les cas de cout infini d'attente à un instant alors qu'on a bien des bus qui vont partir plus tard.
            // Par cohérence avec le reste des calculs (la "fréquence" renvoyée est divisée par 2 pour avoir un temps d'attente moyen quelque soit l'instant considéré effectivement),
            // on prend ici le temps restant avant l'instant du prochain bus par rapport à l'instant courant, qu'on multiplie par deux.
            return (pNextElement->GetStartTime() - dbInstant) * 2;
        }
        else
        {
            return std::numeric_limits<double>::infinity();
        }
    }
}

bool ScheduleInstant::Load(XERCES_CPP_NAMESPACE::DOMNode * pXMLNode, Reseau * pNetwork, Logger & loadingLogger, const std::string & strID)
{
    std::string strTmp;
    GetXmlAttributeValue(pXMLNode, "heuredepart", strTmp, &loadingLogger);

	int nhd = atoi(strTmp.substr(0,2).c_str());
    int nmd = atoi(strTmp.substr(3,2).c_str());
    int nsd = atoi(strTmp.substr(6,2).c_str());              
                    
    // On se replace dans le référentiel temporel de la simulation
    double dbTime = nhd*3600+nmd*60+nsd;
    m_dbInstant = dbTime - (pNetwork->GetSimuStartTime().GetHour()*3600+pNetwork->GetSimuStartTime().GetMinute()*60+pNetwork->GetSimuStartTime().GetSecond());

    return true;
}

bool ScheduleInstant::CheckTime(double dbInstant, double dbEndTimeStep, bool bHasNextConsecutiveElement, bool & bTooOld)
{
    bTooOld = m_dbInstant < dbInstant || (m_dbInstant == dbInstant && bHasNextConsecutiveElement);
    return m_dbInstant >= dbInstant && m_dbInstant < dbEndTimeStep;
}

double ScheduleInstant::GetStartTime()
{
    return m_dbInstant;
}

double ScheduleInstant::GetStopTime()
{
    return m_dbInstant;
}

bool ScheduleFrequency::Load(XERCES_CPP_NAMESPACE::DOMNode * pXMLNode, Reseau * pNetwork, Logger & loadingLogger, const std::string & strID)
{
    std::string strTmp;
    GetXmlAttributeValue(pXMLNode, "heuredepart", strTmp, &loadingLogger);

	int nhd = atoi(strTmp.substr(0,2).c_str());
    int nmd = atoi(strTmp.substr(3,2).c_str());
    int nsd = atoi(strTmp.substr(6,2).c_str());              
                    
    // On se replace dans le référentiel temporel de la simulation
    double dbTime = nhd*3600+nmd*60+nsd;
    m_dbStartInstant = dbTime - (pNetwork->GetSimuStartTime().GetHour()*3600+pNetwork->GetSimuStartTime().GetMinute()*60+pNetwork->GetSimuStartTime().GetSecond());

    GetXmlAttributeValue(pXMLNode, "heurefin", strTmp, &loadingLogger);

	nhd = atoi(strTmp.substr(0,2).c_str());
    nmd = atoi(strTmp.substr(3,2).c_str());
    nsd = atoi(strTmp.substr(6,2).c_str());              
                    
    // On se replace dans le référentiel temporel de la simulation
    dbTime = nhd*3600+nmd*60+nsd;
    m_dbStopInstant = dbTime - (pNetwork->GetSimuStartTime().GetHour()*3600+pNetwork->GetSimuStartTime().GetMinute()*60+pNetwork->GetSimuStartTime().GetSecond());

    if(m_dbStopInstant < m_dbStartInstant)
    {
        loadingLogger << Logger::Error << " ERROR : the end time of a frequency must be after its start time in the schedule of element " << strID << std::endl;
        loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        return false;
    }

    GetXmlAttributeValue(pXMLNode, "frequence", m_dbFrequency, &loadingLogger);

    return true;
}

bool ScheduleFrequency::CheckTime(double dbInstant, double dbEndTimeStep, bool bHasNextConsecutiveElement, bool & bTooOld)
{
    bTooOld = m_dbStopInstant < dbInstant || (m_dbStopInstant == dbInstant && bHasNextConsecutiveElement);

    // On vérifie que le pas de temps a une emprise commune avec la plage temporelle définie pour la fréquence
    if(bTooOld || dbEndTimeStep < m_dbStartInstant)
        return false;

    // Cas du premier démarrage à l'heure de démarrage
    if(m_dbStartInstant >= dbInstant && m_dbStartInstant < dbEndTimeStep)
    {
        return true;
    }

    // Pour les démarrages suivants, on regarde si les bornes temporelles de l'instant encadrent un instant de lancement respectant la fréquence
    double nbStart = (dbInstant - m_dbStartInstant) / m_dbFrequency;
    double nbEnd = (dbEndTimeStep - m_dbStartInstant) / m_dbFrequency;
    return std::ceil(nbStart) != std::ceil(nbEnd);
}

double ScheduleFrequency::GetStartTime()
{
    return m_dbStartInstant;
}

double ScheduleFrequency::GetStopTime()
{
    return m_dbStopInstant;
}

double ScheduleFrequency::GetFrequency()
{
    return m_dbFrequency;
}

template void Schedule::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Schedule::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Schedule::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_LstElements);
    ar & BOOST_SERIALIZATION_NVP(m_LastStartElementIndex);
}

template void ScheduleElement::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ScheduleElement::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ScheduleElement::serialize(Archive & ar, const unsigned int version)
{
}

template void ScheduleFrequency::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ScheduleFrequency::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ScheduleFrequency::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ScheduleElement);

    ar & BOOST_SERIALIZATION_NVP(m_dbStartInstant);
    ar & BOOST_SERIALIZATION_NVP(m_dbStopInstant);
    ar & BOOST_SERIALIZATION_NVP(m_dbFrequency);
}

template void ScheduleInstant::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ScheduleInstant::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ScheduleInstant::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ScheduleElement);

    ar & BOOST_SERIALIZATION_NVP(m_dbInstant);
}