#include "stdafx.h"
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMAttr.hpp>
#include "XMLDocReseauCirculation.h"
#include "SystemUtil.h"
#include "Xerces/XmlUtil.h"


// Initialisation
void		XMLDocReseauCirculation::Create(XERCES_CPP_NAMESPACE::DOMDocument *xmlDocReseau)
{
	DOMNode			*xmlnoderoot;                        

    // Noeud RESEAU_CIRCULATION (noeud root)
	xmlnoderoot = pXMLDoc->getDocumentElement();

    // Noeud EVE_TRONCONS
	m_XmlNodeTroncons = pXMLDoc->createElement(L"EVE_TRONCONS");
    xmlnoderoot->appendChild( m_XmlNodeTroncons );

    // Noeud EVE_ENTREES
    m_XmlNodeEntrees = pXMLDoc->createElement(L"EVE_ENTREES");
    xmlnoderoot->appendChild( m_XmlNodeEntrees );

    // Noeud EVE_SORTIES
    m_XmlNodeSorties = pXMLDoc->createElement(L"EVE_SORTIES");
    xmlnoderoot->appendChild( m_XmlNodeSorties );

    // Noeud FEUX
    m_XmlNodeFeux = pXMLDoc->createElement(L"FEUX");
    xmlnoderoot->appendChild( m_XmlNodeFeux );

    // Noeud ARRETS
    m_XmlNodeArrets = pXMLDoc->createElement(L"ARRETS");
    xmlnoderoot->appendChild( m_XmlNodeArrets );       
}

// Ajout d'un tronçon (de type EVE)
void XMLDocReseauCirculation::AddTroncon(std::wstring strID, Point* pPtAm, Point* pPtAv, std::deque<Point*> lstPtInterne, double dbLarg, double dbLong, std::wstring sSymTroncon, int nSymVoie, std::wstring sSymCnx)
{
	DOMElement* xmlnode;
    DOMNode* xmlnodePoints;
    DOMElement* xmlnodePoint;
	DOMAttr* xmlattr;

	xmlnode = pXMLDoc->createElement(L"EVE_TRONCON");
	
	// Libelle
	xmlattr = pXMLDoc->createAttribute( L"id" );
	xmlattr->setValue(strID.c_str());
	xmlnode->setAttributeNode(xmlattr);    

    // Largeur
    xmlattr = pXMLDoc->createAttribute( L"largeur" );
    xmlattr->setValue(SystemUtil::ToString(2, dbLarg).c_str());
	xmlnode->setAttributeNode(xmlattr);
             
    // Longueur
    xmlattr = pXMLDoc->createAttribute( L"longueur" );
    xmlattr->setValue(SystemUtil::ToString(2, dbLong).c_str());
	xmlnode->setAttributeNode(xmlattr);   

    // Sym. tronçon
    //if( sSymTroncon->Length>0)
    {
        xmlattr = pXMLDoc->createAttribute( L"sym_troncon" );
	    xmlattr->setValue(sSymTroncon.c_str());
	    xmlnode->setAttributeNode(xmlattr); 

        // Numéro de voie au sens Symubruit
        xmlattr = pXMLDoc->createAttribute( L"sym_voie" );
		xmlattr->setValue(SystemUtil::ToString(nSymVoie).c_str());
	    xmlnode->setAttributeNode(xmlattr); 
    }

    if( sSymCnx.length() > 0 )
    {
        xmlattr = pXMLDoc->createAttribute( L"sym_connexion" );
	    xmlattr->setValue(sSymCnx.c_str());
	    xmlnode->setAttributeNode(xmlattr);
    }

    // Points (à partir de la version 4.0.0.10, la géométrie d'un tronçon EVE est spécifiée à l'aide d'une unique liste de points ordonnés; la notion d'extrémité amont, aval
	// et points internes a disparu)

	xmlnodePoints = pXMLDoc->createElement(L"POINTS");
	xmlnode->appendChild(xmlnodePoints);

	// Extremité amont 
	xmlnodePoint = pXMLDoc->createElement( L"POINT" );
	xmlattr = pXMLDoc->createAttribute( L"coord" );
	xmlattr->setValue((SystemUtil::ToString(2, pPtAm->dbX) + L" " + SystemUtil::ToString(2, pPtAm->dbY) + L" " + SystemUtil::ToString(2, pPtAm->dbZ)).c_str());
	xmlnodePoint->setAttributeNode(xmlattr);
	xmlnodePoints->appendChild(xmlnodePoint);

    if(lstPtInterne.size())
    {            		        
        for(int i=0; i<(int)lstPtInterne.size(); i++)
        {
            xmlnodePoint = pXMLDoc->createElement( L"POINT" );
            
	        xmlattr = pXMLDoc->createAttribute( L"coord" );
			xmlattr->setValue((SystemUtil::ToString(2, lstPtInterne[i]->dbX) + L" " + SystemUtil::ToString(2, lstPtInterne[i]->dbY) + L" " + SystemUtil::ToString(2, lstPtInterne[i]->dbZ)).c_str());

			xmlnodePoint->setAttributeNode(xmlattr);
            xmlnodePoints->appendChild(xmlnodePoint);
        }
    }

	// Extremité aval 
	xmlnodePoint = pXMLDoc->createElement( L"POINT" );
	xmlattr = pXMLDoc->createAttribute( L"coord" );
	xmlattr->setValue((SystemUtil::ToString(2, pPtAv->dbX) + L" " + SystemUtil::ToString(2, pPtAv->dbY) + L" " + SystemUtil::ToString(2, pPtAv->dbZ)).c_str());

	xmlnodePoint->setAttributeNode(xmlattr);
    xmlnodePoints->appendChild(xmlnodePoint);

	// Append		
	m_XmlNodeTroncons->appendChild( xmlnode );		
}

// Ajout d'une entrée (de type EVE)
void XMLDocReseauCirculation::AddEntree(std::wstring strID, Point* pPt)
{
	DOMElement* xmlnode;        
	DOMAttr* xmlattr;

	xmlnode = pXMLDoc->createElement( L"EVE_ENTREE" );
	
	// Libelle
	xmlattr = pXMLDoc->createAttribute( L"id" );
	xmlattr->setValue(strID.c_str());
	xmlnode->setAttributeNode(xmlattr);

    // Coordonnées
	xmlattr = pXMLDoc->createAttribute( L"coord" );
	xmlattr->setValue( (SystemUtil::ToString(2, pPt->dbX) + L" " + SystemUtil::ToString(2, pPt->dbY) + L" " + SystemUtil::ToString(2, pPt->dbZ)).c_str());
	xmlnode->setAttributeNode(xmlattr);    

    // Append		
	m_XmlNodeEntrees->appendChild( xmlnode );		
}

// Ajout d'une sortie (de type EVE)
void XMLDocReseauCirculation::AddSortie(std::wstring strID, Point* pPt)
{
	DOMElement* xmlnode;        
	DOMAttr* xmlattr;

	xmlnode = pXMLDoc->createElement( L"EVE_SORTIE" );
	
	// Libelle
	xmlattr = pXMLDoc->createAttribute( L"id" );
	xmlattr->setValue(strID.c_str());
	xmlnode->setAttributeNode(xmlattr);

    // Coordonnées
	xmlattr = pXMLDoc->createAttribute( L"coord" );
	xmlattr->setValue( (SystemUtil::ToString(2, pPt->dbX) + L" " + SystemUtil::ToString(2, pPt->dbY) + L" " + SystemUtil::ToString(2, pPt->dbZ)).c_str());
	xmlnode->setAttributeNode(xmlattr);       

    // Append		
	m_XmlNodeSorties->appendChild( xmlnode );		
}

 // Ajout d'une description d'un arrêt de bus
void XMLDocReseauCirculation::AddArret(std::wstring strLibelle, std::wstring strIDTroncon, double dbPos, bool bHorsVoie )
{
	DOMElement* xmlnode;  
	DOMAttr* xmlattr; 

	xmlnode = pXMLDoc->createElement( L"ARRET" );
	
	// Libelle
	xmlattr = pXMLDoc->createAttribute( L"label" );
	xmlattr->setValue( strLibelle.c_str() );
	xmlnode->setAttributeNode(xmlattr);  

    // Identifiant du tronçon
    xmlattr = pXMLDoc->createAttribute( L"troncon" );
    xmlattr->setValue( strIDTroncon.c_str() );
    xmlnode->setAttributeNode(xmlattr);         

    // Position de l'arrêt par rapport au début du tronçon
    xmlattr = pXMLDoc->createAttribute( L"position" );
    xmlattr->setValue( SystemUtil::ToString(2, dbPos).c_str() );
	xmlnode->setAttributeNode(xmlattr);  

    // Hors voie ?
    xmlattr = pXMLDoc->createAttribute( L"hors_voie" );
	xmlattr->setValue( SystemUtil::ToString(bHorsVoie).c_str() );
	xmlnode->setAttributeNode(xmlattr);  
		
	// Append		
	m_XmlNodeArrets->appendChild( xmlnode );		
}

void XMLDocReseauCirculation::AddFeuOrigineDestination(std::wstring sIdOrigine, std::wstring sIdDestination)
{
    DOMNode* xmlFeunode;
    DOMElement* xmlTronconOriginenode;
    DOMNode* xmlTronconsDstnode;
    DOMElement* xmlTronconDstnode;
	DOMAttr* xmlattr; 
    std::wstring     sPath;        
    
    sPath = L"FEU/TRONCON_ORIGINE[@id=\"" + sIdOrigine + L"\"]";
	xmlTronconOriginenode = (DOMElement*)XMLUtil::SelectSingleNode( sPath, m_XmlNodeFeux->getOwnerDocument(), m_XmlNodeFeux);

    if(!xmlTronconOriginenode)
    {
		xmlFeunode = pXMLDoc->createElement( L"FEU" );
        m_XmlNodeFeux->appendChild( xmlFeunode );	

        xmlTronconOriginenode = pXMLDoc->createElement( L"TRONCON_ORIGINE" );
        xmlFeunode->appendChild( xmlTronconOriginenode );

        xmlTronconsDstnode = pXMLDoc->createElement( L"TRONCONS_DESTINATION" );
        xmlTronconOriginenode->appendChild( xmlTronconsDstnode );	
    }
    else
    {
		xmlTronconsDstnode = XMLUtil::SelectSingleNode(L"TRONCONS_DESTINATION", xmlTronconOriginenode->getOwnerDocument(), xmlTronconOriginenode);
    }

    // Identifiant du tronçon d'origine
	xmlattr = pXMLDoc->createAttribute( L"id" );
    xmlattr->setValue(sIdOrigine.c_str());
	xmlTronconOriginenode->setAttributeNode(xmlattr);             

	xmlTronconDstnode = pXMLDoc->createElement( L"TRONCON_DESTINATION" );
    xmlTronconsDstnode->appendChild( xmlTronconDstnode );	

    // Identifiant du tronçon destination
    xmlattr = pXMLDoc->createAttribute( L"id" );
    xmlattr->setValue(sIdDestination.c_str());
    xmlTronconDstnode->setAttributeNode(xmlattr);     
    
}

// Ajout d'un feu d'une LGP
void XMLDocReseauCirculation::AddFeuLGP(std::wstring sIdTr, std::wstring sIdCDF, double dbPos)
{
    DOMNode* xmlFeunode;
    DOMElement* xmlTronconLGPnode;        
    DOMAttr* xmlattr; 
   
    xmlFeunode = pXMLDoc->createElement( L"FEU" );
    m_XmlNodeFeux->appendChild( xmlFeunode );	       

    xmlTronconLGPnode = pXMLDoc->createElement( L"TRONCON_LGP" );
    xmlFeunode->appendChild( xmlTronconLGPnode );	

    // Identifiant du tronçon propre
    xmlattr = pXMLDoc->createAttribute( L"id" );
    xmlattr->setValue(sIdTr.c_str());
    xmlTronconLGPnode->setAttributeNode(xmlattr);     

    // Identifiant du CDF
    xmlattr = pXMLDoc->createAttribute( L"id_CDF" );
    xmlattr->setValue(sIdCDF.c_str());
    xmlTronconLGPnode->setAttributeNode(xmlattr);     

    // Position sur le tronçon
    xmlattr = pXMLDoc->createAttribute( L"position" );
	xmlattr->setValue(SystemUtil::ToString(2, dbPos).c_str());
    xmlTronconLGPnode->setAttributeNode(xmlattr);     
}

// Fin
void XMLDocReseauCirculation::Terminate()
{		        
	m_sFlux = XMLUtil::getOuterXml(pXMLDoc->getDocumentElement());
}
