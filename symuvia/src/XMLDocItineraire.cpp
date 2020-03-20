#include "stdafx.h"
#include "XMLDocItineraire.h"

#include "SerializeUtil.h"
#include "Xerces/XMLUtil.h"

#include "reseau.h"

#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMDocument.hpp>

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

// Constructeur par défaut
XMLDocItineraire::XMLDocItineraire()
{
	pXMLDoc = NULL;
    m_pNetwork = NULL;
}

// Constructeur
XMLDocItineraire::XMLDocItineraire(Reseau * pNetwork, XERCES_CPP_NAMESPACE::DOMDocument* pXMLDocument)
{
	pXMLDoc = pXMLDocument;
    m_pNetwork = pNetwork;
}

XMLDocItineraire::~XMLDocItineraire()
{
	pXMLDoc->release();
}

// Initialisation
void XMLDocItineraire::Create()
{
    // Noeud ORIGINES
	m_XmlNodeOrigines = (XERCES_CPP_NAMESPACE::DOMNode*)pXMLDoc->createElement(XS("ORIGINES"));
	XERCES_CPP_NAMESPACE::DOMElement * xmlnoderoot = pXMLDoc->getDocumentElement();
    xmlnoderoot->appendChild( m_XmlNodeOrigines);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void XMLDocItineraire::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void XMLDocItineraire::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void XMLDocItineraire::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pNetwork);

    SerialiseDOMDocument<Archive>(ar, "pXMLDoc", XS("RESEAU"), pXMLDoc, m_pNetwork->GetXMLUtil());

    // en cas de chargement, on réinitialise le pointeur vers le noeud m_XmlNodeOrigines
    if(Archive::is_loading::value)
    {
        m_XmlNodeOrigines = m_pNetwork->GetXMLUtil()->SelectSingleNode("ORIGINES", pXMLDoc);
    }
}