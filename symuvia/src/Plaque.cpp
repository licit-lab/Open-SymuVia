#include "stdafx.h"
#include "Plaque.h"

#include "reseau.h"
#include "tuyau.h"
#include "Motif.h"
#include "SymuCoreManager.h"
#include "TronconDestination.h"
#include "TronconOrigine.h"
#include "ParkingParameters.h"
#include "usage/Position.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>



CSection::CSection()
{
    m_pTuyau = NULL;
    m_dbPositionDebut = 0;
    m_dbPositionFin = 0;
}

CSection::CSection(Tuyau * pTuyau, double dbStartPos, double dbEndPos)
{
    m_pTuyau = pTuyau;
    m_dbPositionDebut = dbStartPos;
    m_dbPositionFin = dbEndPos;
}

CSection::~CSection()
{
}

Tuyau * CSection::getTuyau() const
{
    return m_pTuyau;
}

double CSection::getStartPosition() const
{
    return m_dbPositionDebut;
}

double CSection::getEndPosition() const
{
    return m_dbPositionFin;
}

double CSection::getLength() const
{
    return m_dbPositionFin - m_dbPositionDebut;
}

CPlaque::CPlaque()
{
    m_pParentZone = NULL;

    m_pParkingParameters = NULL;
    m_bOwnsParkingParams = false;
}

CPlaque::CPlaque(ZoneDeTerminaison * pParentZone, const std::string & strID)
{
    m_pParentZone = pParentZone;
    m_strID = strID;

    m_pParkingParameters = NULL;
    m_bOwnsParkingParams = false;
}

CPlaque::~CPlaque()
{
    if (m_pParkingParameters && m_bOwnsParkingParams)
    {
        delete m_pParkingParameters;
    }
}

ZoneDeTerminaison * CPlaque::GetParentZone() const
{
    return m_pParentZone;
}

const std::string & CPlaque::GetID() const
{
    return m_strID;
}

const std::vector<CSection> & CPlaque::getSections() const
{
    return m_LstSections;
}

void CPlaque::SetParkingParameters(ParkingParameters * pParkingParams, bool bOwnsParkingParams)
{
    m_pParkingParameters = pParkingParams;
    m_bOwnsParkingParams = bOwnsParkingParams;
}

ParkingParameters * CPlaque::GetParkingParameters() const
{
    return m_pParkingParameters;
}

void CPlaque::AddSection(const CSection & section)
{
    m_LstSections.push_back(section);
    m_LstLinks.insert(section.getTuyau());

#ifdef USE_SYMUCORE
    // map pour permettre de moyenner les position d'origine et de destination des plaques
    std::map<Tuyau*, std::pair<int, std::pair<double, double> > > lstTuyaux;
    for (size_t iSection = 0; iSection < m_LstSections.size(); iSection++)
    {
        const CSection & section = m_LstSections[iSection];
        std::pair<int, std::pair<double, double> > & myPair = lstTuyaux[section.getTuyau()];
        myPair.first++;
        // rmq. : les véhicules partent au tiers du tronçon, même si c'est faux pour les plaques.... à reprendre si on rechange.
        myPair.second.first += section.getTuyau()->GetLength() / 3; // (section.getEndPosition() - section.getStartPosition()) / 2; // Position moyenne de départ pour la section
        myPair.second.second += section.getStartPosition(); // Position moyenne d'arrivée sur la section (tel que codé pour l'instant les véhicules sont détruits dès qu'ils arrivent dans la section)
    }
    for (std::map<Tuyau*, std::pair<int, std::pair<double, double> > >::const_iterator iter = lstTuyaux.begin(); iter != lstTuyaux.end(); ++iter)
    {
        std::pair<double, double> & myPair = m_AreaHelper.GetLstLinks()[iter->first];
        myPair.first = iter->second.second.first / iter->second.first;
        myPair.second = iter->second.second.second / iter->second.first;
    }
#endif // USE_SYMUCORE
}

void CPlaque::SetMotifCoeff(CMotif * pMotif, double dbCoeff)
{
    m_mapMotifsCoeffs[pMotif] = dbCoeff;
}

double CPlaque::GetMotifCoeff(CMotif * pMotif)
{
    std::map<CMotif*, double>::const_iterator iter = m_mapMotifsCoeffs.find(pMotif);
    if (iter != m_mapMotifsCoeffs.end())
    {
        return iter->second;
    }
    return 0;
}

const std::map<CMotif*, double> & CPlaque::GetMapMotifCoeff() const
{
    return m_mapMotifsCoeffs;
}

void CPlaque::ComputeBarycentre(Point & pt) const
{
    double x = 0, y = 0;
    int nbPts = 0;

    Tuyau * pLink;

    for (size_t sectionIdx = 0; sectionIdx < m_LstSections.size(); sectionIdx++)
    {
        const CSection & section = m_LstSections[sectionIdx];

        pLink = section.getTuyau();

        //rmq. approx car on ne prend pas en compte les points internes
        double dbX, dbY, dbZ;
        CalculAbsCoords(pLink->GetLstLanes().front(), section.getStartPosition(), false, dbX, dbY, dbZ);
        x += dbX;
        y += dbY;
        CalculAbsCoords(pLink->GetLstLanes().front(), section.getEndPosition(), false, dbX, dbY, dbZ);
        x += dbX;
        y += dbY;
        nbPts = nbPts + 2;
    }

    pt.dbX = x / nbPts;
    pt.dbY = y / nbPts;
    pt.dbZ = 0;
}

bool CPlaque::ContainsPosition(const Position & position) const
{
    assert(position.IsLink() && position.HasPosition()); // implémenté pour la recherche d'arrêt en plaque uniquement pour l'instant : à implémenter pour les autres types de position si besoin un jour ?
    bool bResult = false;
    for (size_t sectionIdx = 0; sectionIdx < m_LstSections.size() && !bResult; sectionIdx++)
    {
        const CSection & section = m_LstSections[sectionIdx];
        if (section.getTuyau() == position.GetLink())
        {
            if (position.GetPosition() >= section.getStartPosition()
                && position.GetPosition() <= section.getEndPosition())
            {
                bResult = true;
            }
        }
    }
    return bResult;
}

// Vérifie si le tuyau appartient à la plaque
bool CPlaque::HasLink(Tuyau * pLink) const
{
    return std::find(m_LstLinks.begin(), m_LstLinks.end(), pLink) != m_LstLinks.end();
}

// Vérifie si le tuyau appartient à la plaque ou s'il en est assez proche selon le radius défini pour les zones
bool CPlaque::CheckRadiusOK(Tuyau * pLink)
{
    if (m_LstLinksExtended.empty())
    {
        // Premier appel : constitution de la liste (sinon, impossible que m_LstLinksExtended soit vide car il contient les tuyaux de la plaque cad au moins 1 tuyau comme assuré par le XSD)
        m_LstLinksExtended = m_LstLinks;

        // Calcul du barycentre de la plaque
        Point baryCenter;
        ComputeBarycentre(baryCenter);

        // Test de distance
        double dbRadius = m_pParentZone->GetMaxDistanceToJunction();
        for (size_t iLink = 0; iLink < m_pParentZone->GetInputPosition().GetZone().size(); iLink++)
        {
            Tuyau * pCandidate = m_pParentZone->GetInputPosition().GetZone().at(iLink);
            Point pt;
            CalcCoordFromPos(pCandidate, 0, pt);
            if (GetDistance(baryCenter, pt) <= dbRadius)
            {
                m_LstLinksExtended.insert(pCandidate);
            }
            else
            {
                CalcCoordFromPos(pCandidate, pCandidate->GetLength(), pt);
                if (GetDistance(baryCenter, pt) <= dbRadius)
                {
                    m_LstLinksExtended.insert(pCandidate);
                }
            }
        }
    }

    return std::find(m_LstLinksExtended.begin(), m_LstLinksExtended.end(), pLink) != m_LstLinksExtended.end();
}

template void CSection::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CSection::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CSection::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pTuyau);
    ar & BOOST_SERIALIZATION_NVP(m_dbPositionDebut);
    ar & BOOST_SERIALIZATION_NVP(m_dbPositionFin);
}


template void CPlaque::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CPlaque::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CPlaque::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pParentZone);
    ar & BOOST_SERIALIZATION_NVP(m_strID);
    ar & BOOST_SERIALIZATION_NVP(m_LstSections);
    ar & BOOST_SERIALIZATION_NVP(m_LstLinks);
    ar & BOOST_SERIALIZATION_NVP(m_LstLinksExtended);
    ar & BOOST_SERIALIZATION_NVP(m_mapMotifsCoeffs);
    ar & BOOST_SERIALIZATION_NVP(m_pParkingParameters);
    ar & BOOST_SERIALIZATION_NVP(m_bOwnsParkingParams);
    
}

