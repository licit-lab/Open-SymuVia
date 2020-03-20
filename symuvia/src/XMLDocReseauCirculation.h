#undef EOF

#ifndef XMLDocReseauCirculationH
#define XMLDocReseauCirculationH

#include <string>
#include <xercesc/dom/impl/DOMDocumentImpl.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include "SymUtil.h"

XERCES_CPP_NAMESPACE_USE

/*===========================================================================================*/
/* Classe de modélisation du document XML du réseau circulation 							 */
/*===========================================================================================*/
class XMLDocReseauCirculation 
{
private:
	// Le document XML
	DOMDocumentImpl * pXMLDoc;

public:	
	std::string		m_sFlux;

    // Constructeur
	XMLDocReseauCirculation(DOMDocumentImpl * pXMLDocument)
	{
		pXMLDoc = pXMLDocument;
	}

	~XMLDocReseauCirculation()
	{
		pXMLDoc->release();
	}

private:    	
    DOMElement			*m_XmlNodeTroncons;
    DOMElement			*m_XmlNodeEntrees;
    DOMElement			*m_XmlNodeSorties;
    DOMElement			*m_XmlNodeFeux;
    DOMElement			*m_XmlNodeArrets;

public:
    
    // Initialisation
	void		Create(XERCES_CPP_NAMESPACE::DOMDocument *xmlDocReseau);

    // Ajout d'un tronçon (de type EVE)
	void AddTroncon(std::wstring strID, Point* pPtAm, Point* pPtAv, std::deque<Point*> lstPtInterne, double dbLarg, double dbLong, std::wstring sSymTroncon, int nSymVoie, std::wstring sSymCnx);

    // Ajout d'une entrée (de type EVE)
	void AddEntree(std::wstring strID, Point* pPt);

    // Ajout d'une sortie (de type EVE)
	    void AddSortie(std::wstring strID, Point* pPt);

     // Ajout d'une description d'un arrêt de bus
	void AddArret(std::wstring strLibelle, std::wstring strIDTroncon, double dbPos, bool bHorsVoie );

	void AddFeuOrigineDestination(std::wstring sIdOrigine, std::wstring sIdDestination);

    // Ajout d'un feu d'une LGP
    void AddFeuLGP(std::wstring sIdTr, std::wstring sIdCDF, double dbPos);

    // Fin
	void Terminate();

};

#endif