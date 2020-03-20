#include "stdafx.h"
#include "ConnectionPonctuel.h"

#include "vehicule.h"

#include <xercesc/dom/DOMNode.hpp>

#include <boost/archive/xml_woarchive.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/shared_ptr.hpp>

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
// Constructeurs, destructeurs et assimilés
//---------------------------------------------------------------------------

// destructeur
ConnectionPonctuel::~ConnectionPonctuel()
{    
}

// Constructeur par défaut
ConnectionPonctuel::ConnectionPonctuel()
{   
}

// Constructeur normal
ConnectionPonctuel::ConnectionPonctuel(std::string sID, Reseau* pReseau)
:Connexion(sID, pReseau)
{     
}

void ConnectionPonctuel::DelInsVeh(boost::shared_ptr<Vehicule> pV)
{
    std::deque <boost::shared_ptr<Vehicule>>::iterator  beg, end, it;

    beg = m_LstInsVeh.begin();
    end = m_LstInsVeh.end();

    for (it = beg;it!=end;it++)
    {
        if(pV == (*it) )
        {
            m_LstInsVeh.erase(it);
            return;
        }
    }
}

bool ConnectionPonctuel::Init( DOMNode *pXmlNodeCnx, Logger *pofLog)
{    
    return true;
}

template void ConnectionPonctuel::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ConnectionPonctuel::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ConnectionPonctuel::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Connexion);

    ar & BOOST_SERIALIZATION_NVP(m_LstInsVeh);
}