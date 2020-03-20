#include "stdafx.h"
#include "XMLDocAcoustique.h"

#include "Xerces/XMLUtil.h"
#include "SystemUtil.h"
#include "TimeUtil.h"
#include "Xerces/DOMLSSerializerSymu.hpp"
#include "SerializeUtil.h"
#include "reseau.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

#include <sstream>

XMLDocAcoustique::~XMLDocAcoustique()
{
	pXMLDoc->release();
}

// Initialisation
void XMLDocAcoustique::Init(const std::string &strFilename, const STime &dtDebut, const std::string &strVersionExe, const std::string &strTimeTrafic, const std::string &strVersionDB, const std::string &strTitre, const std::string &strDate)        
{
	m_strFilename = strFilename;

    if(m_bSave)
    {
		m_XmlWriter = new DOMLSSerializerSymu();
		m_XmlWriter->initXmlWriter(m_strFilename.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
		m_XmlWriter->setIndent(true);
    }

	// Noeud déclaration
    if(m_bSave)
		m_XmlWriter->writeXmlDeclaration();

	// Noeud SIMULATION (noeud root)
	m_XmlNodeSimulation = pXMLDoc->getDocumentElement();

    if(m_bSave)
		m_XmlWriter->writeStartElement(XS("SIMULATION"));	

	// attributs

    // Horodatage de lancement de la simulation   
    if(m_bSave)
		m_XmlWriter->writeAttributeString( XS("time"), XS(SDateTime::Now().ToString().c_str() ));

    // Version de l'executable     
	std::string strVersionExe2 = "Symubruit " + strVersionExe;
    if(m_bSave)
		m_XmlWriter->writeAttributeString( XS("version"), XS(strVersionExe2.c_str()) );

    // Horodatage du lancement du trafic
    if(m_bSave)
		m_XmlWriter->writeAttributeString( XS("timeTrafic"), XS(strTimeTrafic.c_str()) );

    // heure de début de la simulation
    if(m_bSave)		      
		m_XmlWriter->writeAttributeString( XS("debut"), XS(dtDebut.ToString().c_str()) );

    // version de la base de données  
    if(m_bSave)
        m_XmlWriter->writeAttributeString( XS("versionDB"), XS(strVersionDB.c_str()) );

    // titre (si existe)
    if(m_bSave)
		if(strTitre.length()>0)
            m_XmlWriter->writeAttributeString( XS("titre"), XS(strTitre.c_str()) );

    // Date (si existe)
    if(m_bSave)
		if(strDate.length()>0)
            m_XmlWriter->writeAttributeString( XS("date"), XS(strDate.c_str()) );

    // Puis création du noeud INSTANTS		
	m_XmlNodeInstants = pXMLDoc->createElement(XS("INSTANTS"));
	m_XmlNodeSimulation->appendChild(m_XmlNodeInstants);		

    if(m_bSave)
		m_XmlWriter->writeStartElement(XS("INSTANTS"));


	// Noeud CELLULES		
	m_XmlNodeCellules = pXMLDoc->createElement(XS("CELDEFS"));
	m_XmlNodeSimulation->appendChild(m_XmlNodeCellules);		
};

// Re-initialisation (après une désérialisation par exemple)
void XMLDocAcoustique::ReInit(std::string results)
{
	// Noeud SIMULATION
    m_XmlNodeSimulation = pXMLDoc->getDocumentElement();

    // Noeud VEHICULES		
	m_XmlNodeInstants = (DOMElement*)m_pNetwork->GetXMLUtil()->SelectSingleNode("INSTANTS", pXMLDoc, (DOMElement*)m_XmlNodeSimulation);

	// Noeud CELLULES		
    m_XmlNodeCellules = m_pNetwork->GetXMLUtil()->SelectSingleNode("CELDEFS", pXMLDoc, (DOMElement*)m_XmlNodeSimulation);

    if(m_bSave)
    {
        // ré-ouverture du writer
        m_XmlWriter = new DOMLSSerializerSymu();
	    m_XmlWriter->initXmlWriter(m_strFilename.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
	    m_XmlWriter->setIndent(true);
        m_XmlWriter->write(results);
        m_XmlWriter->setCurrentIndentLevel(2);
    }

    m_bReinitialized = true;
}

// Fin
void XMLDocAcoustique::Terminate()
{					
    if(m_bSave && m_XmlWriter != NULL)
    {
        if(!m_bReinitialized)
        {
	        m_XmlWriter->writeEndElement();				        	        
        }
        else
        {
            m_XmlWriter->setCurrentIndentLevel(1);
            m_XmlWriter->writeEndElement(XS("INSTANTS"), !m_bAtLeastOneInstant);	
        }
		m_XmlWriter->writeNode(m_XmlNodeCellules);
         if(!m_bReinitialized)
        {
	        m_XmlWriter->writeEndElement();				        	        
        }
        else
        {
            m_XmlWriter->setCurrentIndentLevel(0);
            m_XmlWriter->writeEndElement(XS("SIMULATION"));	
        }
	    m_XmlWriter->close();
    }
}

// Mise en veille (prise snapshot pour reprise plus tard)
void XMLDocAcoustique::Suspend()
{
    if(m_bSave && m_XmlWriter!=NULL)
    { 
        m_XmlWriter->close();		
	    delete m_XmlWriter;
        m_XmlWriter = NULL;
    }
}

// Retourne le flux XML du dernier instant
DOMNode *	XMLDocAcoustique::GetLastInstant()
{
    DOMNode * xmlnode;
    xmlnode = m_pNetwork->GetXMLUtil()->SelectSingleNode("./INSTANTS", pXMLDoc)->getLastChild();

    return xmlnode;
}

// libère le DOMNode associé au dernier instant (il est sauvegardé)
void    XMLDocAcoustique::ReleaseLastInstant()
{
    DOMNode * xmlnode;
    xmlnode = m_pNetwork->GetXMLUtil()->SelectSingleNode("./INSTANTS", pXMLDoc)->getLastChild();

    // Ménage
    DOMNode* xmlChild = m_pNetwork->GetXMLUtil()->SelectSingleNode("./INSTANTS", pXMLDoc)->removeChild(xmlnode);
    m_pNetwork->GetXMLUtil()->ReleaseNodes(xmlChild);
}

// Ajout d'un instant
/*DOMNode**/XMLDocAcoustique::PINSTANT	XMLDocAcoustique::AddInstant(double dbInstant, int nNbVeh)
{
	DOMElement* xmlnode;
	DOMNode* xmlfathernode;
	DOMAttr* xmlattribute;
    
	// Création du noeud
	xmlnode = pXMLDoc->createElement(XS("INST"));
	xmlattribute = pXMLDoc->createAttribute( XS("val") );
	xmlattribute->setValue(XS(SystemUtil::ToString(2, dbInstant).c_str()));
	xmlnode->setAttributeNode(xmlattribute);

	// Noeud SOURCES PONCTUELLES
	m_XmlNodeSRCS = pXMLDoc->createElement(XS("SRCS"));
	xmlnode->appendChild(m_XmlNodeSRCS);

	// Noeuds CELLULES
	m_XmlNodeCELLS = pXMLDoc->createElement(XS("CELS"));
	xmlnode->appendChild(m_XmlNodeCELLS);

	// Recherche du noeud père Instants
    xmlfathernode = m_pNetwork->GetXMLUtil()->SelectSingleNode("./INSTANTS", pXMLDoc);

	// Insertion
	xmlfathernode->appendChild(xmlnode);

	return (XMLDocAcoustique::PINSTANT)xmlnode;		
}

// Sauvegarde des données acoustiques d'un instant
void XMLDocAcoustique::SaveLastInstant()
{
	DOMNode * xmlnode;
    xmlnode = m_pNetwork->GetXMLUtil()->SelectSingleNode("./INSTANTS", pXMLDoc)->getLastChild();

	if(m_bSave)
	{
		if (m_XmlWriter->isOpen())
		{
			m_XmlWriter->writeNode(xmlnode);
            m_bAtLeastOneInstant = true;
		}
	}
}

// Ajout de la description d'une cellule acoustique
void XMLDocAcoustique::AddCellule(int nID, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav,  double dbYav, double dbZav)
{
	DOMElement* xmlnodeCell;
	DOMAttr* xmlattr;

	xmlnodeCell = pXMLDoc->createElement(XS("CELDEF"));

	// ID
	xmlattr = pXMLDoc->createAttribute( XS("id" ));
	xmlattr->setValue(XS(SystemUtil::ToString(nID).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

	// Libelle
	xmlattr = pXMLDoc->createAttribute( XS("lib") );
	xmlattr->setValue(XS(strLibelle.c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

	// Tuyau
	xmlattr = pXMLDoc->createAttribute( XS("tron") );
	xmlattr->setValue(XS(strTuyau.c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

	// X amont
	xmlattr = pXMLDoc->createAttribute( XS("Xam") );        
    xmlattr->setValue(XS(SystemUtil::ToString(2, dbXam).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

	// X aval
	xmlattr = pXMLDoc->createAttribute( XS("Xav") );
    xmlattr->setValue(XS(SystemUtil::ToString(2, dbXav).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

	// Y amont
	xmlattr = pXMLDoc->createAttribute( XS("Yam") );
    xmlattr->setValue(XS(SystemUtil::ToString(2, dbYam).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

	// Y aval
	xmlattr = pXMLDoc->createAttribute( XS("Yav") );
    xmlattr->setValue(XS(SystemUtil::ToString(2, dbYav).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

    // Z amont
	xmlattr = pXMLDoc->createAttribute( XS("Zam") );
    xmlattr->setValue(XS(SystemUtil::ToString(2, dbZam).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

	// Z aval
	xmlattr = pXMLDoc->createAttribute( XS("Zav") );
    xmlattr->setValue(XS(SystemUtil::ToString(2, dbZav).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

	// Append		
	m_XmlNodeCellules->appendChild( xmlnodeCell );		
}

// Ajout des données acoustique d'une source ponctuelle à l'instant considéré
void XMLDocAcoustique::AddSourcePonctuelle(int nID, double dbAbsDeb, double dbOrdDeb, double dbHautDeb, double dbAbsFin, double dbOrdFin, double dbHautFin, double * arEmissions)
{
	DOMElement* xmlnodeSrc;
	DOMAttr * xmlattr;
	std::string strVal; 

	xmlnodeSrc = pXMLDoc->createElement( XS("SRC") );

	// ID
	xmlattr = pXMLDoc->createAttribute( XS("id") );
	xmlattr->setValue(XS(SystemUtil::ToString(nID).c_str()));
	xmlnodeSrc->setAttributeNode(xmlattr);		

	// Abscisse de la source au début du pas de temps
	xmlattr = pXMLDoc->createAttribute( XS("ad") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbAbsDeb).c_str()));
	xmlnodeSrc->setAttributeNode(xmlattr);		

    // Ordonnée de la source au début du pas de temps
	xmlattr = pXMLDoc->createAttribute( XS("od") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbOrdDeb).c_str()));
	xmlnodeSrc->setAttributeNode(xmlattr);		

    // Hauteur de la source au début du pas de temps
	xmlattr = pXMLDoc->createAttribute( XS("hd") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbHautDeb).c_str()));
	xmlnodeSrc->setAttributeNode(xmlattr);		

    // Abscisse de la source à la fin du pas de temps
	xmlattr = pXMLDoc->createAttribute( XS("af") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbAbsFin).c_str()));
	xmlnodeSrc->setAttributeNode(xmlattr);		

    // Ordonnée de la source à la fin du pas de temps
	xmlattr = pXMLDoc->createAttribute( XS("of") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbOrdFin).c_str()));
	xmlnodeSrc->setAttributeNode(xmlattr);		

    // Hauteur de la source à la fin du pas de temps
	xmlattr = pXMLDoc->createAttribute( XS("hf") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbHautFin).c_str()));
	xmlnodeSrc->setAttributeNode(xmlattr);		

    // Emissions        	
	xmlattr = pXMLDoc->createAttribute( XS("val") );
	for (int i=0; i<9; i++)
	{
		strVal = strVal + SystemUtil::ToString(2, arEmissions[i]);
		if(i!=8)
         strVal = strVal + " ";
	}
	xmlattr->setValue(XS(strVal.c_str()));
	xmlnodeSrc->setAttributeNode(xmlattr);		

	// Append		
	m_XmlNodeSRCS->appendChild( xmlnodeSrc );
};

// Ajout des données acoustiques d'une cellule à l'instant considéré
void XMLDocAcoustique::AddCellSimu(int nID, double * arEmissions ) 
{
	DOMElement * xmlnodeCell;
	DOMAttr * xmlattr;    
	std::string strVal;

	xmlnodeCell = pXMLDoc->createElement( XS("CEL") );

	// ID
	xmlattr = pXMLDoc->createAttribute( XS("id") );
	xmlattr->setValue( XS(SystemUtil::ToString(nID).c_str() ));
	xmlnodeCell->setAttributeNode(xmlattr);	

	xmlattr = pXMLDoc->createAttribute( XS("val") );
    for(int i=0; i<9; i++)
    {                 
		strVal = strVal + SystemUtil::ToString(2, arEmissions[i]);
       if(i!=8)
         strVal = strVal + " ";
    }	
	xmlattr->setValue( XS(strVal.c_str()) );
	xmlnodeCell->setAttributeNode(xmlattr);	


	// Append		
	m_XmlNodeCELLS->appendChild( xmlnodeCell );
}

// Ajout des données acoustiques d'une cellule à l'instant considéré
void XMLDocAcoustique::AddCellSimuEx(int nID, double * arEmissions, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav,  double dbYav, double dbZav )
{
	DOMElement* xmlnodeCell;
	DOMAttr* xmlattr;    
	std::string strVal;

	xmlnodeCell = pXMLDoc->createElement( XS("CEL") );

	// ID
	xmlattr = pXMLDoc->createAttribute( XS("id") );
	xmlattr->setValue( XS(SystemUtil::ToString(nID).c_str() ));
	xmlnodeCell->setAttributeNode(xmlattr);	

    // Vecteurs des émissions
    xmlattr = pXMLDoc->createAttribute( XS("val") );
    for(int i=0; i<9; i++)
    {                 
		strVal = strVal + SystemUtil::ToString(2, arEmissions[i]);
		if(i!=8)
         strVal = strVal + " ";
    }	
    xmlattr->setValue( XS(strVal.c_str()) );
	xmlnodeCell->setAttributeNode(xmlattr);      

    // Tuyau
	xmlattr = pXMLDoc->createAttribute( XS("tron") );
	xmlattr->setValue( XS(strTuyau.c_str() ));
	xmlnodeCell->setAttributeNode(xmlattr);

	// X amont
	xmlattr = pXMLDoc->createAttribute( XS("Xam") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbXam).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

	// X aval
	xmlattr = pXMLDoc->createAttribute( XS("Xav") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbXav).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

	// Y amont
	xmlattr = pXMLDoc->createAttribute( XS("Yam") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbYam).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

	// Y aval
	xmlattr = pXMLDoc->createAttribute( XS("Yav") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbYav).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

    // Z amont
	xmlattr = pXMLDoc->createAttribute( XS("Zam") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbZam).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);

	// Z aval
	xmlattr = pXMLDoc->createAttribute( XS("Zav") );
	xmlattr->setValue(XS(SystemUtil::ToString(2, dbZav).c_str()));
	xmlnodeCell->setAttributeNode(xmlattr);


	// Append		
	m_XmlNodeCELLS->appendChild( xmlnodeCell );
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void XMLDocAcoustique::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void XMLDocAcoustique::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void XMLDocAcoustique::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(DocAcoustique);

    // membres "classiques"
    ar & BOOST_SERIALIZATION_NVP(m_strFilename);
    ar & BOOST_SERIALIZATION_NVP(m_bSave);
    ar & BOOST_SERIALIZATION_NVP(m_bAtLeastOneInstant);
    ar & BOOST_SERIALIZATION_NVP(m_pNetwork);

    // structure DOM
    SerialiseDOMDocument<Archive>(ar, "pXMLDoc", XS("SIMULATION"), pXMLDoc, m_pNetwork->GetXMLUtil());

    std::string resultStr;
    if(Archive::is_saving::value)
    {
        // fichier résultat : on ferme le fichier actuel et on récupère son contenu pour le sauvegarder dans 
        // le fichier de sérialisation
        Suspend();
        std::ifstream ifs(m_strFilename.c_str(), std::ios::in | std::ios::binary); // lecture en binaire sinon les CR LF deviennent LF
        std::stringstream ss;
        ss << ifs.rdbuf();
        ifs.close();
        resultStr = ss.str();
    }
    ar & BOOST_SERIALIZATION_NVP(resultStr);


    // on réinitialise les autres membres déductibles des autres et on rouvre le fichier de résultat
    if(Archive::is_loading::value)
    {
        ReInit(resultStr);
    }
}