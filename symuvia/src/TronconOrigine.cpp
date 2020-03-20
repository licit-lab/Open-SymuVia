#include "stdafx.h"
#include "TronconOrigine.h"

#include "ZoneDeTerminaison.h"
#include "tuyau.h"
#include "vehicule.h"

#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>

TronconOrigine::TronconOrigine(Tuyau * pTuyau, ZoneDeTerminaison * pZoneParent):
SymuViaTripNode(pTuyau->GetLabel(), pTuyau->GetReseau())
{
    m_pZoneParent = pZoneParent;
    m_pTuyau = pTuyau;

    m_OutputPosition.SetLink(m_pTuyau);
}

TronconOrigine::~TronconOrigine()
{
}


// Identification de l'origine
std::string TronconOrigine::GetOutputID() const
{
    return m_pZoneParent ? (m_pZoneParent->GetOutputID() + " : " + SymuViaTripNode::GetOutputID()) : SymuViaTripNode::GetOutputID();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void TronconOrigine::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void TronconOrigine::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void TronconOrigine::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SymuViaTripNode);

    ar & BOOST_SERIALIZATION_NVP(m_pZoneParent);
    ar & BOOST_SERIALIZATION_NVP(m_pTuyau);
    ar & BOOST_SERIALIZATION_NVP(m_mapAssignment);
}