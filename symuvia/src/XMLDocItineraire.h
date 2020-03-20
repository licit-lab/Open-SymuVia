#pragma once
#ifndef XMLDocItineraireH
#define XMLDocItineraireH

#include <xercesc/util/XercesVersion.hpp>

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
    class DOMDocument;
};

namespace boost {
    namespace serialization {
        class access;
    }
}

class Reseau;

/*===========================================================================================*/
/* Classe de modélisation du document XML des itinéraires   								 */
/*===========================================================================================*/
class XMLDocItineraire 
{
public:	
	// Le document XML
	XERCES_CPP_NAMESPACE::DOMDocument * pXMLDoc;

    // Constructeur par défaut
	XMLDocItineraire();

    // Constructeur
	XMLDocItineraire(Reseau * pNetwork, XERCES_CPP_NAMESPACE::DOMDocument* pXMLDocument);

	~XMLDocItineraire();

private:
	XERCES_CPP_NAMESPACE::DOMNode			*m_XmlNodeOrigines;

    Reseau * m_pNetwork;

public:
	XERCES_CPP_NAMESPACE::DOMNode*        GetXmlNodeOrigines()
	{
		return m_XmlNodeOrigines;
	};

    // Initialisation
	void Create();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif