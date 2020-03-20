#include "stdafx.h"
#include "XMLDocSirane.h"

#include "Xerces/DOMLSSerializerSymu.hpp"
#include "SystemUtil.h"
#include "symUtil.h"

#include "Xerces/XMLUtil.h"

#include "reseau.h"

#include <xercesc/dom/impl/DOMDocumentImpl.hpp>

#include "SerializeUtil.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

#include <sstream>
#include <fstream>


// Constructeurs
XMLDocSirane::XMLDocSirane() : m_bReinitialized(false)
{
    pXMLDoc = NULL;
    m_pNetwork = NULL;
    m_nNbCells = 0;
}
XMLDocSirane::XMLDocSirane(Reseau * pNetwork, XERCES_CPP_NAMESPACE::DOMDocument* pXMLDocument) : m_bReinitialized(false)
{
    m_pNetwork = pNetwork;
	pXMLDoc = pXMLDocument;
    m_nNbCells = 0;
}
XMLDocSirane::~XMLDocSirane()
{
	pXMLDoc->release();
}


// Initialisation
void XMLDocSirane::Init(const std::string & strFilename, const std::vector<Point> & boundingRect)        
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

    m_XmlWriter->writeStartElement(XS("ogr:FeatureCollection"));
    
	// xmlns:xsi
    m_XmlWriter->writeAttributeString(XS("xmlns:xsi"), XS("http://www.w3.org/2001/XMLSchema-instance"));
    m_XmlWriter->writeAttributeString(XS("xsi:schemaLocation"), XS("http://ogr.maptools.org/ Symuvia.xsd"));
    m_XmlWriter->writeAttributeString(XS("xmlns:ogr"), XS("http://ogr.maptools.org/"));
    m_XmlWriter->writeAttributeString(XS("xmlns:gml"), XS("http://www.opengis.net/gml"));

    m_nNbCells = 0;

    // rectangle englobant
    DOMElement * nodeBoundedBy = pXMLDoc->createElement(XS("gml:boundedBy"));
    DOMElement * nodeBox = pXMLDoc->createElement(XS("gml:Box"));

    // Coordonnées Xmin, Ymin
    DOMElement * nodeCoord = pXMLDoc->createElement(XS("gml:coord"));
    DOMElement * nodeX = pXMLDoc->createElement(XS("gml:X"));
    nodeX->setTextContent(XS(SystemUtil::ToString(2, boundingRect[0].dbX).c_str()));
    nodeCoord->appendChild(nodeX);
    DOMElement * nodeY = pXMLDoc->createElement(XS("gml:Y"));
    nodeY->setTextContent(XS(SystemUtil::ToString(2, boundingRect[1].dbY).c_str()));
    nodeCoord->appendChild(nodeY);
    nodeBox->appendChild(nodeCoord);

    // Coordonnées Xmax, Ymax
    nodeCoord = pXMLDoc->createElement(XS("gml:coord"));
    nodeX = pXMLDoc->createElement(XS("gml:X"));
    nodeX->setTextContent(XS(SystemUtil::ToString(2, boundingRect[1].dbX).c_str()));
    nodeCoord->appendChild(nodeX);
    nodeY = pXMLDoc->createElement(XS("gml:Y"));
    nodeY->setTextContent(XS(SystemUtil::ToString(2, boundingRect[0].dbY).c_str()));
    nodeCoord->appendChild(nodeY);
    nodeBox->appendChild(nodeCoord);

    nodeBoundedBy->appendChild(nodeBox);

    m_XmlWriter->writeNode(nodeBoundedBy);
}

// Re-initialisation (après une désérialisation par exemple)
void XMLDocSirane::ReInit(const std::string & results)
{
	// Noeud SIMULATION
    m_XmlNodeSimulation = pXMLDoc->getDocumentElement();

    // ré-ouverture du writer
    m_XmlWriter = new DOMLSSerializerSymu();
    m_XmlWriter->initXmlWriter(m_strFilename.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
    m_XmlWriter->setIndent(true);
    m_XmlWriter->write(results);
    m_XmlWriter->setCurrentIndentLevel(1);

    m_bReinitialized = true;
}

// Fin
void XMLDocSirane::Terminate()
{		
    if(m_XmlWriter != NULL)
    {
        if(!m_bReinitialized)
        {
	        m_XmlWriter->writeEndElement();			// End "ogr:FeatureCollection"       	        	        
        }
        else
        {
            m_XmlWriter->setCurrentIndentLevel(0);
            m_XmlWriter->writeEndElement(XS("ogr:FeatureCollection"));	
        }				     
	    m_XmlWriter->close();
    }
}

// Mise en veille (prise snapshot pour reprise plus tard)
void XMLDocSirane::Suspend()
{
    if(m_XmlWriter != NULL)
    {
        m_XmlWriter->close();		
	    delete m_XmlWriter;
        m_XmlWriter = NULL;
    }
}

// Ajout de la description d'une cellule sirane
void XMLDocSirane::AddCellule(const std::string & strLibelle, eTypeCelluleSirane type, const std::vector<Point*> & lstPts, ePhaseCelluleSirane iPhase)
{
    DOMElement * nodeFeatureMember = pXMLDoc->createElement(XS("gml:featureMember"));
    DOMElement * nodeSymuVia = pXMLDoc->createElement(XS("ogr:Symuvia"));
    
	// identifiant gml de la cellule (fid)
    DOMAttr* xmlattr = pXMLDoc->createAttribute( XS("fid" ));
    std::ostringstream oss;
    oss << "F" << m_nNbCells;
	xmlattr->setValue(XS(oss.str().c_str()));
	nodeSymuVia->setAttributeNode(xmlattr);

    // géométrie
    DOMElement * nodeGeometryProperty = pXMLDoc->createElement(XS("ogr:geometryProperty"));
    DOMElement * nodeLineString = pXMLDoc->createElement(XS("gml:LineString"));
    DOMElement* xmlnodeCoords = pXMLDoc->createElement(XS("gml:coordinates"));
    oss.str(""); // reset du buffer
    oss.precision(2);
	oss.setf(std::ios::fixed);
    for(size_t i = 0; i < lstPts.size(); i++)
    {
        if(i != 0)
        {
            oss << " ";
        }       		
		oss << lstPts[i]->dbX << "," << lstPts[i]->dbY;
    }
    xmlnodeCoords->setTextContent(XS(oss.str().c_str()));	
    nodeLineString->appendChild(xmlnodeCoords);
    nodeGeometryProperty->appendChild(nodeLineString);
    nodeSymuVia->appendChild(nodeGeometryProperty);


    // ogr:id
    DOMElement* xmlnodeOGRID = pXMLDoc->createElement(XS("ogr:id"));
    xmlnodeOGRID->setTextContent(XS(strLibelle.c_str()));
    nodeSymuVia->appendChild(xmlnodeOGRID);

    // ogr:type
    DOMElement* xmlnodeOGRType = pXMLDoc->createElement(XS("ogr:type"));
    if(type == CELL_BRIN)
    {
        xmlnodeOGRType->setTextContent(XS("link"));
    }
    else
    {
        xmlnodeOGRType->setTextContent(XS("node"));
    }
    nodeSymuVia->appendChild(xmlnodeOGRType);

    // ogr:phase
    DOMElement* xmlnodeOGRPhase = pXMLDoc->createElement(XS("ogr:phase"));
    xmlnodeOGRPhase->setTextContent(XS(SystemUtil::ToString(iPhase).c_str()));
    nodeSymuVia->appendChild(xmlnodeOGRPhase);

    nodeFeatureMember->appendChild(nodeSymuVia);
    m_XmlNodeSimulation->appendChild(nodeFeatureMember);

    m_XmlWriter->writeNode(nodeFeatureMember);
    
    m_nNbCells++;
}

// Ajout d'une nouvelle période d'agrégation
void XMLDocSirane::AddPeriod(double dbInstantDebut, double dbInstantFin)
{
	DOMAttr* xmlattr;

	m_XmlNodePeriode = pXMLDoc->createElement(XS("PERIODE"));

    // debut
	xmlattr = pXMLDoc->createAttribute( XS("debut" ));
    xmlattr->setValue(XS(SystemUtil::ToString(2, dbInstantDebut).c_str()));
	m_XmlNodePeriode->setAttributeNode(xmlattr);

    // fin
	xmlattr = pXMLDoc->createAttribute( XS("fin" ));
    xmlattr->setValue(XS(SystemUtil::ToString(2, dbInstantFin).c_str()));
	m_XmlNodePeriode->setAttributeNode(xmlattr);

    m_XmlNodeSimulation->appendChild(m_XmlNodePeriode);
}

// Fin de la sauvegarde de la période d'agrégation courante
void XMLDocSirane::ClosePeriod()
{
    m_XmlWriter->writeNode(m_XmlNodePeriode);
}

// Ajout des résultats d'une cellule sur la période
void XMLDocSirane::AddCellSimu(const std::string & strLibelle)
{
	DOMAttr* xmlattr;

	m_XmlNodeCell = pXMLDoc->createElement(XS("CELL"));

	// Libelle
	xmlattr = pXMLDoc->createAttribute( XS("lib" ));
	xmlattr->setValue(XS(strLibelle.c_str()));
	m_XmlNodeCell->setAttributeNode(xmlattr);

	// Append		
	m_XmlNodePeriode->appendChild( m_XmlNodeCell );	
}

// Ajout des résultats d'une cellule sur la période pour un type de véhicule
void XMLDocSirane::AddCellSimuForType(const std::string & strType, const std::vector<double> & vitMins, const std::vector<double> & vitMaxs, const std::vector<double> & durees)
{
    DOMElement* xmlnodeType;
    DOMElement* xmlnodePlage;
	DOMAttr* xmlattr;

	xmlnodeType = pXMLDoc->createElement(XS("TYPE_VEHICULE"));

	// Libelle du type de véhiule
	xmlattr = pXMLDoc->createAttribute( XS("lib" ));
	xmlattr->setValue(XS(strType.c_str()));
	xmlnodeType->setAttributeNode(xmlattr);

    // plages de vitesse
    for(size_t i = 0; i < durees.size(); i++)
    {
        xmlnodePlage = pXMLDoc->createElement(XS("PLAGE"));

        // vitesse minimum
        xmlattr = pXMLDoc->createAttribute( XS("vit_min") );
        xmlattr->setValue(XS(SystemUtil::ToString(0, vitMins[i]).c_str()));
	    xmlnodePlage->setAttributeNode(xmlattr);

        // vitesse maximum
        xmlattr = pXMLDoc->createAttribute( XS("vit_max" ));
        xmlattr->setValue(XS(SystemUtil::ToString(0, vitMaxs[i]).c_str()));
	    xmlnodePlage->setAttributeNode(xmlattr);

        // durée cumulée
        xmlattr = pXMLDoc->createAttribute( XS("cumul" ));
        xmlattr->setValue(XS(SystemUtil::ToString(2, durees[i]).c_str()));
	    xmlnodePlage->setAttributeNode(xmlattr);

        xmlnodeType->appendChild(xmlnodePlage);
    }

	// Append		
	m_XmlNodeCell->appendChild( xmlnodeType );
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void XMLDocSirane::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void XMLDocSirane::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void XMLDocSirane::serialize(Archive & ar, const unsigned int version)
{
    // membres "classiques"
    ar & BOOST_SERIALIZATION_NVP(m_strFilename);
    ar & BOOST_SERIALIZATION_NVP(m_nNbCells);
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