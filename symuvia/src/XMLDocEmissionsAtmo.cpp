#include "stdafx.h"
#include "XMLDocEmissionsAtmo.h"

#include "TimeUtil.h"
#include "SystemUtil.h"
#include "Xerces/XMLUtil.h"
#include "Xerces/DOMLSSerializerSymu.hpp"

#include "reseau.h"

#include <xercesc/dom/impl/DOMDocumentImpl.hpp>

XMLDocEmissionsAtmo::~XMLDocEmissionsAtmo()
{
	pXMLDoc->release();
}

// Initialisation
void XMLDocEmissionsAtmo::Init(const std::string & strFilename, const STime &dtDebut, const std::string & strVersionExe, const std::string & strTimeTrafic, const std::string & strVersionDB, const std::string & strTitre, const std::string & strDate)        
{
	m_strFilename = strFilename;

	// XmlWriter
	m_XmlWriter = new DOMLSSerializerSymu();
	m_XmlWriter->initXmlWriter(m_strFilename.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
	m_XmlWriter->setIndent(true);

	// Noeud déclaration
	m_XmlWriter->writeXmlDeclaration();

	// Noeud SIMULATION (noeud root)
	m_XmlNodeSimulation = pXMLDoc->getDocumentElement();

    //m_XmlWriter->WriteStartElement("SIMULATION");	
	m_XmlWriter->writeStartElement(XS("SIMULATION"));

	// attributs

    // Horodatage de lancement de la simulation   
	m_XmlWriter->writeAttributeString(XS("time"), XS(""));

    // Version de l'executable     
	m_XmlWriter->writeAttributeString( XS("version"), XS(("Symubruit " + strVersionExe).c_str()) );

    // Horodatage du lancement du trafic
	m_XmlWriter->writeAttributeString( XS("timeTrafic"), XS(strTimeTrafic.c_str()) );

    // heure de début de la simulation
	m_XmlWriter->writeAttributeString( XS("debut"), XS(dtDebut.ToString().c_str()) );

    // titre (si existe)
    if(strTitre.length()>0)
		m_XmlWriter->writeAttributeString( XS("titre"), XS(strTitre.c_str()) )		;

    // Date (si existe)
	if(strDate.length()>0)
		m_XmlWriter->writeAttributeString( XS("date"), XS("" )	)	;

    // Puis création du noeud INSTANTS		
	m_XmlNodeInstants = pXMLDoc->createElement(XS("INSTANTS"));
	m_XmlNodeSimulation->appendChild(m_XmlNodeInstants);

	m_XmlWriter->writeStartElement(XS("INSTANTS"));

	// Noeud VEHICULES		
	m_XmlNodeVehicules = pXMLDoc->createElement(XS("VEHS"));
	m_XmlNodeSimulation->appendChild(m_XmlNodeVehicules);

}

// Fin
void XMLDocEmissionsAtmo::Terminate()
{					
	m_XmlWriter->writeEndElement();						// End "INSTANTS"
	m_XmlWriter->writeNode(m_XmlNodeVehicules);	        // Ecriture noeud "VEHS"
	m_XmlWriter->writeEndElement();						// End "SIMULATION"            
	m_XmlWriter->close();
}

// Retourne le flux XML du dernier instant
DOMNode *	XMLDocEmissionsAtmo::GetLastInstant()
{
	DOMNode * xmlnode;
    xmlnode = m_pNetwork->GetXMLUtil()->SelectSingleNode("./INSTANTS", pXMLDoc)->getLastChild();

    return xmlnode;
}

// Ajout d'un instant
DOMNode *	XMLDocEmissionsAtmo::AddInstant(double dbInstant, int nNbVeh)
{
	DOMElement * xmlnode;
	DOMNode * xmlfathernode;
	DOMAttr * xmlattribute;
    
	// Création du noeud
	xmlnode = pXMLDoc->createElement(XS("INST"));
	xmlattribute = pXMLDoc->createAttribute(XS("val"));
	xmlattribute->setValue(XS(SystemUtil::ToString(2, dbInstant).c_str()));
	xmlnode->setAttributeNode(xmlattribute);

    // Noeud SOURCES PONCTUELLES
	DOMElement* xmlnodeSRCS;
	xmlnodeSRCS = pXMLDoc->createElement(XS("SRCS"));
	xmlnode->appendChild(xmlnodeSRCS);

	// Recherche du noeud père Instants
    xmlfathernode = m_pNetwork->GetXMLUtil()->SelectSingleNode("./INSTANTS", pXMLDoc);

	// Insertion
	xmlfathernode->appendChild(xmlnode);

	return xmlnode;		
}

// Sauvegarde des données acoustiques d'un instant
void XMLDocEmissionsAtmo::SaveLastInstant()
{
	DOMNode * xmlnode;

    xmlnode = m_pNetwork->GetXMLUtil()->SelectSingleNode("./INSTANTS", pXMLDoc)->getLastChild();

	m_XmlWriter->writeNode(xmlnode);
}

// libère le DOMNode associé au dernier instant (il est sauvegardé)
void XMLDocEmissionsAtmo::ReleaseLastInstant()
{
    DOMNode * xmlnode;
    xmlnode = m_pNetwork->GetXMLUtil()->SelectSingleNode("./INSTANTS", pXMLDoc)->getLastChild();

    // Ménage
    DOMNode* xmlChild = m_pNetwork->GetXMLUtil()->SelectSingleNode("./INSTANTS", pXMLDoc)->removeChild(xmlnode);
    m_pNetwork->GetXMLUtil()->ReleaseNodes(xmlChild);
}

void XMLDocEmissionsAtmo::Addval(double dbVCO2, double dbVNOx, double dbVPM, double dbCumCO2, double dbCumNOx, double dbCumPM )
{
	DOMAttr * xmlattr;    
	DOMElement * xmlelem;

	xmlattr = pXMLDoc->createAttribute( XS("ins_CO2") );        
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbVCO2).c_str()));
	xmlelem = (DOMElement *)m_XmlNodeInstants->getLastChild(); 
	xmlelem->setAttributeNode(xmlattr);

	xmlattr = pXMLDoc->createAttribute(XS("ins_NOx"));
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbVNOx).c_str()));
	xmlelem = (DOMElement *)m_XmlNodeInstants->getLastChild();
	xmlelem->setAttributeNode(xmlattr);       //Pas de notion de "InsertBefore"

	xmlattr = pXMLDoc->createAttribute( XS("ins_PM") );        
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbVPM).c_str()));
	xmlelem = (DOMElement *)m_XmlNodeInstants->getLastChild();
	xmlelem->setAttributeNode(xmlattr);       //Pas de notion de "InsertBefore"

	xmlattr = pXMLDoc->createAttribute( XS("cum_CO2") );        
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbCumCO2).c_str()));
	xmlelem = (DOMElement *)m_XmlNodeInstants->getLastChild();
	xmlelem->setAttributeNode(xmlattr);       //Pas de notion de "InsertBefore"

	xmlattr = pXMLDoc->createAttribute( XS("cum_NOx") );        
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbCumNOx).c_str()));
	xmlelem = (DOMElement *)m_XmlNodeInstants->getLastChild();
	xmlelem->setAttributeNode(xmlattr);       //Pas de notion de "InsertBefore"

	xmlattr = pXMLDoc->createAttribute( XS("cum_PM") );        
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbCumPM).c_str()));
	xmlelem = (DOMElement *)m_XmlNodeInstants->getLastChild();
	xmlelem->setAttributeNode(xmlattr);       //Pas de notion de "InsertBefore"


}

// Sauvegarde des donénes atmosphériques d'un véhicule sur l'instant courant
void XMLDocEmissionsAtmo::AddVehiculeInst(int nID, double dbCO2, double dbNOx, double dbPM)
{
    DOMElement* xmlnodeSrc;
	DOMAttr * xmlattr;
	std::string strVal; 

	xmlnodeSrc = pXMLDoc->createElement( XS("SRC") );

	// ID
	xmlattr = pXMLDoc->createAttribute( XS("id") );
	xmlattr->setValue(XS(SystemUtil::ToString(nID).c_str()));
	xmlnodeSrc->setAttributeNode(xmlattr);		

	// CO2
	xmlattr = pXMLDoc->createAttribute( XS("CO2") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbCO2).c_str()));
	xmlnodeSrc->setAttributeNode(xmlattr);		

    // NOx
	xmlattr = pXMLDoc->createAttribute( XS("NOx") );
	xmlattr->setValue(XS(SystemUtil::ToString(4, dbNOx).c_str()));
	xmlnodeSrc->setAttributeNode(xmlattr);	

    // PM
	xmlattr = pXMLDoc->createAttribute( XS("PM") );
	xmlattr->setValue(XS(SystemUtil::ToString(4, dbPM).c_str()));
	xmlnodeSrc->setAttributeNode(xmlattr);	

	// Append		
    m_pNetwork->GetXMLUtil()->SelectSingleNode("SRCS", m_XmlNodeInstants->getOwnerDocument(), (DOMElement*)m_XmlNodeInstants->getLastChild())->appendChild(xmlnodeSrc);
}

// Ajout de la description d'un véhicule (cumul)
void XMLDocEmissionsAtmo::AddVehicule(int nID, const std::string & strType, const std::string & strEntree, double dbInstCreation, const std::string & strSortie, double dbInstSortie, double dbCumCO2, double dbCumNOx, double dbCumPM, double dbDstParcourue, bool bAddToCreation)
{
	DOMElement* xmlnodeVeh;
	DOMAttr* xmlattr;

	xmlnodeVeh = pXMLDoc->createElement(XS("VEH"));

	// ID
	xmlattr = pXMLDoc->createAttribute( XS("id") );
	xmlattr->setValue( XS(SystemUtil::ToString(nID).c_str()) );
	xmlnodeVeh->setAttributeNode(xmlattr);	
	
	// Type de véhicule
	xmlattr = pXMLDoc->createAttribute( XS("type") );
	xmlattr->setValue( XS(strType.c_str()) );
	xmlnodeVeh->setAttributeNode(xmlattr);	

    // Entrée
	xmlattr = pXMLDoc->createAttribute( XS("entree") );
	xmlattr->setValue( XS(strEntree.c_str()) );
	xmlnodeVeh->setAttributeNode(xmlattr);

    // Instant d'entrée sur le réseau
    xmlattr = pXMLDoc->createAttribute( XS("instE") );		
	xmlattr->setValue(XS(SystemUtil::ToString(3, dbInstCreation).c_str()));
	xmlnodeVeh->setAttributeNode(xmlattr);

	// Sortie
	xmlattr = pXMLDoc->createAttribute( XS("sortie") );
	xmlattr->setValue( XS(strSortie.c_str()) );
	xmlnodeVeh->setAttributeNode(xmlattr);

    // Instant de sortie sur le réseau
    xmlattr = pXMLDoc->createAttribute( XS("instS") );		
	xmlattr->setValue(XS(SystemUtil::ToString(3, dbInstSortie).c_str()));
	xmlnodeVeh->setAttributeNode(xmlattr);

	// CO2
    xmlattr = pXMLDoc->createAttribute( XS("CO2") );		
	xmlattr->setValue(XS(SystemUtil::ToString(3, dbCumCO2).c_str()));
	xmlnodeVeh->setAttributeNode(xmlattr);

	// NOx
	xmlattr = pXMLDoc->createAttribute( XS("NOx") );		
	xmlattr->setValue(XS(SystemUtil::ToString(3, dbCumNOx).c_str()));
	xmlnodeVeh->setAttributeNode(xmlattr);

	// PM
	xmlattr = pXMLDoc->createAttribute( XS("PM") );		
	xmlattr->setValue(XS(SystemUtil::ToString(3, dbCumPM).c_str()));
	xmlnodeVeh->setAttributeNode(xmlattr);

	// Distance parcourue
	xmlattr = pXMLDoc->createAttribute( XS("dstParcourue") );		
	xmlattr->setValue(XS(SystemUtil::ToString(3, dbDstParcourue).c_str()));
	xmlnodeVeh->setAttributeNode(xmlattr);


    // Append		
	m_XmlNodeVehicules->appendChild( xmlnodeVeh );		
}
