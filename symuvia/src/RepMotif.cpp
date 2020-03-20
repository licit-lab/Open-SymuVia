#include "stdafx.h"
#include "RepMotif.h"

#include "Motif.h"
#include "usage/SymuViaTripNode.h"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>


CMotifCoeff::CMotifCoeff()
{
    m_pOriginMotif = NULL;
    m_pDestinationMotif = NULL;
    m_dbCoeff = 0.0;
}

CMotifCoeff::CMotifCoeff(CMotif* pOrigin, CMotif* pDest, double dbCoeff)
{
    m_pOriginMotif = pOrigin;
    m_pDestinationMotif = pDest;
    m_dbCoeff = dbCoeff;
}

CMotifCoeff::~CMotifCoeff()
{

}

CMotif * CMotifCoeff::GetMotifOrigine() const
{
    return m_pOriginMotif;
}

CMotif * CMotifCoeff::GetMotifDestination() const
{
    return m_pDestinationMotif;
}

double CMotifCoeff::getCoeff() const
{
    return m_dbCoeff;
}

void CMotifCoeff::setCoeff(double dbCoeff)
{
    m_dbCoeff = dbCoeff;
}


CRepMotifDest::CRepMotifDest()
{
    m_pDestination = NULL;
}

CRepMotifDest::~CRepMotifDest()
{
}

void CRepMotifDest::setDestination(SymuViaTripNode * pDest)
{
    m_pDestination = pDest;
}

SymuViaTripNode * CRepMotifDest::getDestination() const
{
    return m_pDestination;
}

bool CRepMotifDest::addMotifCoeff(const CMotifCoeff & motifCoeff)
{
    for (size_t i = 0; i < m_LstCoeffs.size(); i++)
    {
        const CMotifCoeff & existingMotifCoeff = m_LstCoeffs[i];
        if (existingMotifCoeff.GetMotifDestination() == motifCoeff.GetMotifDestination()
            && existingMotifCoeff.GetMotifOrigine() == motifCoeff.GetMotifOrigine())
        {
            return false;
        }
    }
    m_LstCoeffs.push_back(motifCoeff);
    return true;
}

std::vector<CMotifCoeff> & CRepMotifDest::getCoeffs()
{
    return m_LstCoeffs;
}

CRepMotif::CRepMotif()
{

}

CRepMotif::~CRepMotif()
{

}

bool CRepMotif::addRepMotifDest(const CRepMotifDest & repMotifDest)
{
    std::map<SymuViaTripNode*, CRepMotifDest>::const_iterator iter = m_mapRepMotifs.find(repMotifDest.getDestination());
    if (iter == m_mapRepMotifs.end())
    {
        m_mapRepMotifs[repMotifDest.getDestination()] = repMotifDest;
        return true;
    }
    return false;
}

CRepMotifDest * CRepMotif::getRepMotifDest(SymuViaTripNode * pDestination)
{
    CRepMotifDest * pResult = NULL;
    std::map<SymuViaTripNode*, CRepMotifDest>::iterator iter = m_mapRepMotifs.find(pDestination);
    if (iter != m_mapRepMotifs.end())
    {
        pResult = &iter->second;
    }
    else
    {
        // On utilise la répartition générale
        iter = m_mapRepMotifs.find(NULL);
        if (iter != m_mapRepMotifs.end())
        {
            pResult = &iter->second;
        }
    }

    return pResult;
}


template void CMotifCoeff::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CMotifCoeff::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CMotifCoeff::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pOriginMotif);
    ar & BOOST_SERIALIZATION_NVP(m_pDestinationMotif);
    ar & BOOST_SERIALIZATION_NVP(m_dbCoeff);
}

template void CRepMotifDest::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CRepMotifDest::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CRepMotifDest::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pDestination);
    ar & BOOST_SERIALIZATION_NVP(m_LstCoeffs);
}

template void CRepMotif::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CRepMotif::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CRepMotif::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_mapRepMotifs);
}

