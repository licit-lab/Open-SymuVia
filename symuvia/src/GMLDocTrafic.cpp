#include "stdafx.h"
#include "GMLDocTrafic.h"

#include "symUtil.h"
#include "Xerces/DOMLSSerializerSymu.hpp"
#include "Xerces/XMLReaderSymuvia.h"
#include "SystemUtil.h"

#include "tuyau.h"
#include "voie.h"
#include "reseau.h"
#include "vehicule.h"
#include "CarrefourAFeuxEx.h"
#include "ControleurDeFeux.h"

#include <boost/serialization/map.hpp>

GMLDocTrafic::GMLDocTrafic()
{
    m_XmlWriterNetwork = NULL;
    m_XmlWriterTrafficLights = NULL;
    m_XmlWriterTrajsHeader = NULL;
    m_XmlWriterTrajsTemp = NULL;
    m_XmlWriterSensors = NULL;
    m_bReinitialized = false;
}

GMLDocTrafic::~GMLDocTrafic()
{
    if(m_XmlWriterNetwork)
    {
        delete m_XmlWriterNetwork;
    }
    if(m_XmlWriterTrafficLights)
    {
        delete m_XmlWriterTrafficLights;
    }
    if(m_XmlWriterTrajsHeader)
    {
        delete m_XmlWriterTrajsHeader;
    }
    if(m_XmlWriterTrajsTemp)
    {
        delete m_XmlWriterTrajsTemp;
    }
    if(m_XmlWriterSensors)
    {
        delete m_XmlWriterSensors;
    }
}


void GMLDocTrafic::Init(const std::string & strFilename, SDateTime dtDeb, SDateTime dtFin, const std::vector<Point> & enveloppe, bool bTmpDoc, const std::string & strFinalFilename,
    GMLDocTrafic * pOriginalDoc, bool bHasAtLeastOneCDF, bool bHasSensor)
{
    m_dtDeb = dtDeb;
    m_strFilenameNetwork = strFinalFilename.substr(0, strFinalFilename.find_last_of('.'));
    m_strFilenameNetwork.append("_net.xml");

    // *********************************************
    // FICHIER DE DESCRIPTION DU RESEAU CITYGML
    // *********************************************
    // Si enveloppe vide : cas des documents temporaires pour affectation dynamique convergente : on ne recrée pas le fichier _net qui n'aura pas changé par rapport à celui déjà écrit
    if(!bTmpDoc)
    {
	    // XmlWriter
	    m_XmlWriterNetwork = new DOMLSSerializerSymu();
	    m_XmlWriterNetwork->initXmlWriter(m_strFilenameNetwork.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
	    m_XmlWriterNetwork->setIndent(true);

	    // Noeud déclaration
	    m_XmlWriterNetwork->writeXmlDeclaration();

        m_XmlWriterNetwork->writeStartElement(XS("CityModel"));
    
	    // xmlns:xsi
        m_XmlWriterNetwork->writeAttributeString(XS("xmlns:xsi"), XS("http://www.w3.org/2001/XMLSchema-instance"));
        m_XmlWriterNetwork->writeAttributeString(XS("xsi:schemaLocation"), XS("http://www.opengis.net/citygml/2.0 CityGML.xsd"));
        m_XmlWriterNetwork->writeAttributeString(XS("xmlns"), XS("http://www.opengis.net/citygml/2.0"));
        m_XmlWriterNetwork->writeAttributeString(XS("xmlns:gml"), XS("http://www.opengis.net/gml"));
        m_XmlWriterNetwork->writeAttributeString(XS("xmlns:tran"), XS("http://www.opengis.net/citygml/transportation/2.0"));
        m_XmlWriterNetwork->writeAttributeString(XS("xmlns:frn"), XS("http://www.opengis.net/citygml/cityfurniture/2.0"));
        m_XmlWriterNetwork->writeAttributeString(XS("xmlns:gen"), XS("http://www.opengis.net/citygml/generics/2.0"));

        m_XmlWriterNetwork->writeAttributeString(XS("gml:id"), XS("CM_0001"));
        

        m_XmlWriterNetwork->writeStartElement(XS("gml:boundedBy"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:Envelope"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:pos"));
        m_XmlWriterNetwork->writeAttributeString(XS("srsDimension"), XS("3"));
        std::ostringstream woss;
        woss << SystemUtil::ToString(6, enveloppe[0].dbX) << " " << SystemUtil::ToString(6, enveloppe[1].dbY) << " " << SystemUtil::ToString(6, enveloppe[0].dbZ);
        m_XmlWriterNetwork->writeText(XS(woss.str().c_str()));
        m_XmlWriterNetwork->writeEndElement();
        m_XmlWriterNetwork->writeStartElement(XS("gml:pos"));
        m_XmlWriterNetwork->writeAttributeString(XS("srsDimension"), XS("3"));
        woss.str(std::string());
        woss << SystemUtil::ToString(6, enveloppe[1].dbX) << " " << SystemUtil::ToString(6, enveloppe[0].dbY) << " " << SystemUtil::ToString(6, enveloppe[0].dbZ);
        m_XmlWriterNetwork->writeText(XS(woss.str().c_str()));
        m_XmlWriterNetwork->writeEndElement();
        m_XmlWriterNetwork->writeEndElement(); // gml:Envelope
        m_XmlWriterNetwork->writeEndElement(); // gml:boundedBy
    }

    // *********************************************
    // FICHIER DES ETATS DES FEUX TRICOLORES
    // *********************************************
    m_strFilenameTrafficLights = strFilename.substr(0, strFilename.find_last_of('.'));
    m_strFilenameTrafficLights.append("_trafficlights.xml");
    if(bHasAtLeastOneCDF)
    {
	    // XmlWriter
	    m_XmlWriterTrafficLights = new DOMLSSerializerSymu();
	    m_XmlWriterTrafficLights->initXmlWriter(m_strFilenameTrafficLights.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
	    m_XmlWriterTrafficLights->setIndent(true);

	    // Noeud déclaration
	    m_XmlWriterTrafficLights->writeXmlDeclaration();

        m_XmlWriterTrafficLights->writeStartElement(XS("tls:TrafficLightsEvolution"));
    
	    // xmlns:xsi
        m_XmlWriterTrafficLights->writeAttributeString(XS("xmlns:xsi"), XS("http://www.w3.org/2001/XMLSchema-instance"));
        m_XmlWriterTrafficLights->writeAttributeString(XS("xmlns:tls"), XS("http://www.ifsttar.fr/licit/trafficLightsStatus/1.0"));
        m_XmlWriterTrafficLights->writeAttributeString(XS("xmlns:gml"), XS("http://www.opengis.net/gml/3.2"));
        m_XmlWriterTrafficLights->writeAttributeString(XS("xmlns:xlink"), XS("http://www.w3.org/1999/xlink"));

        m_XmlWriterTrafficLights->writeAttributeString(XS("gml:id"), XS("TLE_0001"));

        m_XmlWriterTrafficLights->writeStartElement(XS("gml:history"));
    }

    // *********************************************
    // FICHIER DES TRAJECTOIRES
    // *********************************************
    m_strFilenameTrajsHeader = strFilename.substr(0, strFilename.find_last_of('.'));
    m_strFilenameTrajsTemp = m_strFilenameTrajsHeader;
    m_strFilenameTrajsHeader.append("_trajs.xml");
    m_strFilenameTrajsTemp.append("_trajsTmp.xml");

	// XmlWriter
	m_XmlWriterTrajsHeader = new DOMLSSerializerSymu();
	m_XmlWriterTrajsHeader->initXmlWriter(m_strFilenameTrajsHeader.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
	m_XmlWriterTrajsHeader->setIndent(true);

	// Noeud déclaration
	m_XmlWriterTrajsHeader->writeXmlDeclaration();

    m_XmlWriterTrajsHeader->writeStartElement(XS("mf:MovingFeatures"));
    
	// xmlns:xsi
    m_XmlWriterTrajsHeader->writeAttributeString(XS("xmlns:xsi"), XS("http://www.w3.org/2001/XMLSchema-instance"));
    m_XmlWriterTrajsHeader->writeAttributeString(XS("xmlns:traj"), XS("http://www.ifsttar.fr/licit/trajectories/1.0"));
    m_XmlWriterTrajsHeader->writeAttributeString(XS("xmlns:gml"), XS("http://www.opengis.net/gml/3.2"));
    m_XmlWriterTrajsHeader->writeAttributeString(XS("xmlns:mf"), XS("http://schemas.opengis.net/mf-core/1.0"));
    m_XmlWriterTrajsHeader->writeAttributeString(XS("xmlns:xsd"), XS("http://www.w3.org/2001/XMLSchema"));

    m_XmlWriterTrajsHeader->writeStartElement(XS("mf:sTBoundedBy"));
    m_XmlWriterTrajsHeader->writeAttributeString(XS("offset"), XS("sec"));
    m_XmlWriterTrajsHeader->writeStartElement(XS("gml:EnvelopeWithTimePeriod"));
    m_XmlWriterTrajsHeader->writeStartElement(XS("gml:pos"));
    m_XmlWriterTrajsHeader->writeAttributeString(XS("srsDimension"), XS("3"));
    m_XmlWriterTrajsHeader->writeText(XS((SystemUtil::ToString(6, enveloppe[0].dbX) + " " + SystemUtil::ToString(6, enveloppe[1].dbY) + " " + SystemUtil::ToString(6, enveloppe[0].dbZ)).c_str()));
    m_XmlWriterTrajsHeader->writeEndElement();
    m_XmlWriterTrajsHeader->writeStartElement(XS("gml:pos"));
    m_XmlWriterTrajsHeader->writeAttributeString(XS("srsDimension"), XS("3"));
    m_XmlWriterTrajsHeader->writeText(XS((SystemUtil::ToString(6, enveloppe[1].dbX) + " " + SystemUtil::ToString(6, enveloppe[0].dbY) + " " + SystemUtil::ToString(6, enveloppe[0].dbZ)).c_str()));
    m_XmlWriterTrajsHeader->writeEndElement();
    m_XmlWriterTrajsHeader->writeStartElement(XS("gml:beginPosition"));
    m_XmlWriterTrajsHeader->writeText(XS(dtDeb.ToISOString().c_str()));
    m_XmlWriterTrajsHeader->writeEndElement(); // gml:beginPosition
    m_XmlWriterTrajsHeader->writeStartElement(XS("gml:endPosition"));
    m_XmlWriterTrajsHeader->writeText(XS(dtFin.ToISOString().c_str()));
    m_XmlWriterTrajsHeader->writeEndElement(); // gml:endPosition
    m_XmlWriterTrajsHeader->writeEndElement(); // gml:EnvelopeWithTimePeriod
    m_XmlWriterTrajsHeader->writeEndElement(); // mf:sTBoundedBy

    // Fichier pour les trajectoires, dont le contenu sera injecté dans le fichier complet à la fin
    m_XmlWriterTrajsTemp = new DOMLSSerializerSymu();
	m_XmlWriterTrajsTemp->initXmlWriter(m_strFilenameTrajsTemp.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
	m_XmlWriterTrajsTemp->setIndent(true);
    m_XmlWriterTrajsTemp->setCurrentIndentLevel(1);

    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:header"));
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:VaryingAttrDefs"));

    // direction
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:attrDef"));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("name"), XS("direction"));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("type"), XS("xsd:double"));
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:AttrAnnotation"));
    m_XmlWriterTrajsTemp->writeText(XS("The direction of the vehicle at the end of the trajectory: the attribute value is the angle between the north direction and the vehicle direction. Unit of angle is radian. For example, North is 0[rad], East is 1.57[rad] and West is -1.57[rad]."));
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:AttrAnnotation
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:attrDef

    // speed
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:attrDef"));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("name"), XS("speed"));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("type"), XS("xsd:double"));
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:AttrAnnotation"));
    m_XmlWriterTrajsTemp->writeText(XS("Vehicle speed in m/s during the trajectory"));
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:AttrAnnotation
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:attrDef

    // acceleration
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:attrDef"));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("name"), XS("acc"));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("type"), XS("xsd:double"));
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:AttrAnnotation"));
    m_XmlWriterTrajsTemp->writeText(XS("Vehicle acceleration in m/s² during the trajectory"));
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:AttrAnnotation
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:attrDef

    // link
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:attrDef"));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("name"), XS("link"));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("type"), XS("xsd:string"));
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:AttrAnnotation"));
    m_XmlWriterTrajsTemp->writeText(XS("Vehicle link at the end of the trajectory"));
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:AttrAnnotation
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:attrDef

    // pk
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:attrDef"));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("name"), XS("pk"));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("type"), XS("xsd:double"));
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:AttrAnnotation"));
    m_XmlWriterTrajsTemp->writeText(XS("Vehicle position on the link in meters at the end of the trajectory"));
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:AttrAnnotation
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:attrDef

    // lane
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:attrDef"));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("name"), XS("lane"));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("type"), XS("xsd:integer"));
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:AttrAnnotation"));
    m_XmlWriterTrajsTemp->writeText(XS("Vehicle lane at the end of the trajectory. Lane 1 is the right lane of the current link"));
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:AttrAnnotation
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:attrDef

    m_XmlWriterTrajsTemp->writeEndElement(); // mf:VaryingAttrDefs
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:header

    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:foliation"));

    if(pOriginalDoc)
    {
        m_mapLastVehiclePositions = pOriginalDoc->m_mapLastVehiclePositions;
    }

    // *********************************************
    // FICHIER DES CAPTEURS
    // *********************************************
    m_strFilenameSensors = strFilename.substr(0, strFilename.find_last_of('.'));
    m_strFilenameSensors.append("_sensors.xml");
    if(bHasSensor)
    {
	    // XmlWriter
	    m_XmlWriterSensors = new DOMLSSerializerSymu();
	    m_XmlWriterSensors->initXmlWriter(m_strFilenameSensors.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
	    m_XmlWriterSensors->setIndent(true);

	    // Noeud déclaration
	    m_XmlWriterSensors->writeXmlDeclaration();

        m_XmlWriterSensors->writeStartElement(XS("gml:Bag"));
    
	    // xmlns:xsi
        m_XmlWriterSensors->writeAttributeString(XS("xmlns:xsi"), XS("http://www.w3.org/2001/XMLSchema-instance"));
        m_XmlWriterSensors->writeAttributeString(XS("xmlns:sml"), XS("http://www.opengis.net/sensorml/2.0"));
        m_XmlWriterSensors->writeAttributeString(XS("xmlns:gml"), XS("http://www.opengis.net/gml/3.2"));
        m_XmlWriterSensors->writeAttributeString(XS("xmlns:xlink"), XS("http://www.w3.org/1999/xlink"));
        m_XmlWriterSensors->writeAttributeString(XS("xmlns:om"), XS("http://www.opengis.net/om/2.0"));
        m_XmlWriterSensors->writeAttributeString(XS("xmlns:swe"), XS("http://www.opengis.net/swe/2.0"));

        m_XmlWriterSensors->writeAttributeString(XS("gml:id"),XS("B_0001"));

        // Ajout de la définition du capteur global
        m_XmlWriterSensors->writeStartElement(XS("gml:member"));
        m_XmlWriterSensors->writeStartElement(XS("sml:PhysicalComponent"));
        m_XmlWriterSensors->writeAttributeString(XS("gml:id"), XS("GlobalSensor"));

        m_XmlWriterSensors->writeStartElement(XS("sml:featuresOfInterest"));
        m_XmlWriterSensors->writeStartElement(XS("sml:FeatureList"));
        m_XmlWriterSensors->writeStartElement(XS("sml:feature"));
        m_XmlWriterSensors->writeAttributeString(XS("xlink:href"), XS((SystemUtil::GetFileName(m_strFilenameNetwork) + "#CM_0001").c_str()));
        m_XmlWriterSensors->writeEndElement(); // sml:feature
        m_XmlWriterSensors->writeEndElement(); // sml:FeatureList
        m_XmlWriterSensors->writeEndElement(); // sml:featuresOfInterest

        // outputs
        //////////////////////
        m_XmlWriterSensors->writeStartElement(XS("sml:outputs"));
        m_XmlWriterSensors->writeStartElement(XS("sml:OutputList"));

        // vitesse
        writeSensorOutput(XS("speed"), XS("m/s"));
        writeSensorOutput(XS("aggregated_vehicles_count"), XS(""));
        writeSensorOutput(XS("total_travelled_distance"), XS("m"));
        writeSensorOutput(XS("total_travelled_time"), XS("s"));

        m_XmlWriterSensors->writeEndElement(); // sml:OutputList
        m_XmlWriterSensors->writeEndElement(); // sml:outputs

        m_XmlWriterSensors->writeEndElement(); // sml:PhysicalComponent
        m_XmlWriterSensors->writeEndElement(); // gml:member
    }
}

// Re-initialisation (après une désérialisation par exemple)
void GMLDocTrafic::ReInit(const std::string & trafficLights, const std::string & trajHeader, const std::string & trajTemp, const std::string & sensors)
{
    // ré-ouverture du writer
    if(!trafficLights.empty())
    {
        m_XmlWriterTrafficLights = new DOMLSSerializerSymu();
        m_XmlWriterTrafficLights->initXmlWriter(m_strFilenameTrafficLights.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
	    m_XmlWriterTrafficLights->setIndent(true);
        m_XmlWriterTrafficLights->write(trafficLights);
        m_XmlWriterTrafficLights->setCurrentIndentLevel(2);
    }

    m_XmlWriterTrajsHeader = new DOMLSSerializerSymu();
    m_XmlWriterTrajsHeader->initXmlWriter(m_strFilenameTrajsHeader.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
	m_XmlWriterTrajsHeader->setIndent(true);
    m_XmlWriterTrajsHeader->write(trajHeader);
    m_XmlWriterTrajsHeader->setCurrentIndentLevel(1);

    m_XmlWriterTrajsTemp = new DOMLSSerializerSymu();
    m_XmlWriterTrajsTemp->initXmlWriter(m_strFilenameTrajsTemp.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
	m_XmlWriterTrajsTemp->setIndent(true);
    m_XmlWriterTrajsTemp->write(trajTemp);
    m_XmlWriterTrajsTemp->setCurrentIndentLevel(2);

    if(!sensors.empty())
    {
        m_XmlWriterSensors = new DOMLSSerializerSymu();
        m_XmlWriterSensors->initXmlWriter(m_strFilenameSensors.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
	    m_XmlWriterSensors->setIndent(true);
        m_XmlWriterSensors->write(sensors);
        m_XmlWriterSensors->setCurrentIndentLevel(1);
    }

    m_bReinitialized = true;
}

void GMLDocTrafic::PostInit()
{
    // Le fichier réseau est terminé dès le début, on le termine donc dès le PostInit
    if(m_XmlWriterNetwork != NULL)
    {
        m_XmlWriterNetwork->writeEndElement();			// End "CityModel"       	        	        
	    m_XmlWriterNetwork->close();
        delete m_XmlWriterNetwork;
        m_XmlWriterNetwork = NULL;
    }
}

// Mise en veille (prise snapshot pour reprise plus tard)
void GMLDocTrafic::Suspend()
{
    if(m_XmlWriterTrafficLights!=NULL)
    { 
        m_XmlWriterTrafficLights->close();		
	    delete m_XmlWriterTrafficLights;
        m_XmlWriterTrafficLights = NULL;
    }
    if(m_XmlWriterTrajsHeader!=NULL)
    { 
        m_XmlWriterTrajsHeader->close();		
	    delete m_XmlWriterTrajsHeader;
        m_XmlWriterTrajsHeader = NULL;
    }
    if(m_XmlWriterTrajsTemp!=NULL)
    { 
        m_XmlWriterTrajsTemp->close();		
	    delete m_XmlWriterTrajsTemp;
        m_XmlWriterTrajsTemp = NULL;
    }
    if(m_XmlWriterSensors!=NULL)
    { 
        m_XmlWriterSensors->close();		
	    delete m_XmlWriterSensors;
        m_XmlWriterSensors = NULL;
    }
}

void GMLDocTrafic::Terminate()
{
    if(m_XmlWriterTrafficLights!=NULL)
    {
        // terminaison du fichier des états des feux
        if(!m_bReinitialized)
        {
	        m_XmlWriterTrafficLights->writeEndElement();    // End "gml:history"
            m_XmlWriterTrafficLights->writeEndElement();    // End "tls:TrafficLightsEvolution"
        }
        else
        {
            m_XmlWriterTrafficLights->setCurrentIndentLevel(1);
            m_XmlWriterTrafficLights->writeEndElement(XS("gml:history"), false);
            m_XmlWriterTrafficLights->writeEndElement(XS("tls:TrafficLightsEvolution"), false);
        }
	    m_XmlWriterTrafficLights->close();
        delete m_XmlWriterTrafficLights;
        m_XmlWriterTrafficLights = NULL;
    }

    if(m_XmlWriterTrajsTemp!=NULL)
    {
        // Terminaison du fichier temporaire des trajectoires
        if(!m_bReinitialized)
        {
            m_XmlWriterTrajsTemp->writeEndElement();    // End "mf:foliation"
        }
        else
        {
            m_XmlWriterTrajsTemp->setCurrentIndentLevel(1);
            m_XmlWriterTrajsTemp->writeEndElement(XS("mf:foliation"), false);
        }
	    m_XmlWriterTrajsTemp->close();
        delete m_XmlWriterTrajsTemp;
        m_XmlWriterTrajsTemp = NULL;
    }

    if(m_XmlWriterTrajsHeader!=NULL)
    {
        // Terminaison du fichier définitif des trajectoires
        m_XmlWriterTrajsHeader->close();
        delete m_XmlWriterTrajsHeader;
        m_XmlWriterTrajsHeader = NULL;
    }

    // Recopie du contenu du fichier temporaire dans le fichier des trajectoires définitif.
    std::ifstream  src(m_strFilenameTrajsTemp.c_str(), std::ios::in | std::ios::binary);
    std::ofstream  dst(m_strFilenameTrajsHeader.c_str(), std::ios::binary | std::ios::app);
    dst << src.rdbuf();
    src.close();

    // Fermeture du noeud mf:MovingFeatures
    dst << "</mf:MovingFeatures>";
    dst.close();

    // Suppression du fichier temporaire
    remove(m_strFilenameTrajsTemp.c_str());

    if(m_XmlWriterSensors!=NULL)
    {
        // terminaison du fichier des états des feux
        if(!m_bReinitialized)
        {
	        m_XmlWriterSensors->writeEndElement();    // End "gml:history"
        }
        else
        {
            m_XmlWriterSensors->setCurrentIndentLevel(0);
            m_XmlWriterSensors->writeEndElement(XS("gml:Bag"), false);
        }
	    m_XmlWriterSensors->close();
        delete m_XmlWriterSensors;
        m_XmlWriterSensors = NULL;
    }
}

// Suppression du fichier
void GMLDocTrafic::Remove()
{
    if (m_XmlWriterTrafficLights != NULL)
    {
        m_XmlWriterTrafficLights->close();
        delete m_XmlWriterTrafficLights;
        m_XmlWriterTrafficLights = NULL;
    }
    remove(m_strFilenameTrafficLights.c_str());

    if (m_XmlWriterTrajsTemp != NULL)
    {
        m_XmlWriterTrajsTemp->close();
        delete m_XmlWriterTrajsTemp;
        m_XmlWriterTrajsTemp = NULL;
    }
    remove(m_strFilenameTrajsTemp.c_str());

    if (m_XmlWriterTrajsHeader != NULL)
    {
        // Terminaison du fichier définitif des trajectoires
        m_XmlWriterTrajsHeader->close();
        delete m_XmlWriterTrajsHeader;
        m_XmlWriterTrajsHeader = NULL;
    }
    remove(m_strFilenameTrajsHeader.c_str());

    if (m_XmlWriterSensors != NULL)
    {
        m_XmlWriterSensors->close();
        delete m_XmlWriterSensors;
        m_XmlWriterSensors = NULL;
    }
    remove(m_strFilenameSensors.c_str());
}

void GMLDocTrafic::AddInstant(double dbInstant, double dbTimeStep, int nNbVeh)
{
    m_dbCurrentInstant = dbInstant;
    m_dbTimeStep = dbTimeStep;

    // Ecriture du début de la période pour les feux tricolores
    if(m_XmlWriterTrafficLights)
    {
        m_XmlWriterTrafficLights->writeStartElement(XS("tls:TrafficLightsStatus"));
        m_XmlWriterTrafficLights->writeAttributeString(XS("gml:id"),XS(("TLS_" + SystemUtil::ToString(2, dbInstant)).c_str()));
        m_XmlWriterTrafficLights->writeStartElement(XS("gml:validTime"));
        m_XmlWriterTrafficLights->writeStartElement(XS("gml:TimePeriod"));
        m_XmlWriterTrafficLights->writeAttributeString(XS("gml:id"),XS(("TP_" + SystemUtil::ToString(2, dbInstant)).c_str()));

        m_XmlWriterTrafficLights->writeStartElement(XS("gml:beginPosition"));
        STimeSpan tsInst((int)(dbInstant-dbTimeStep));
        STime debutPer = m_dtDeb.ToTime() + tsInst;
        m_XmlWriterTrafficLights->writeText(XS(debutPer.ToString().c_str()));
        m_XmlWriterTrafficLights->writeEndElement(); // gml:beginPosition

        m_XmlWriterTrafficLights->writeStartElement(XS("gml:endPosition"));
        STimeSpan tsInstFin((int)(dbInstant));
        STime finPer = m_dtDeb.ToTime() + tsInstFin;
        m_XmlWriterTrafficLights->writeText(XS(finPer.ToString().c_str()));
        m_XmlWriterTrafficLights->writeEndElement(); // gml:endPosition

        m_XmlWriterTrafficLights->writeEndElement(); // gml:TimePeriod
        m_XmlWriterTrafficLights->writeEndElement(); // gml:validTime
    }
}

void GMLDocTrafic::AddPeriodeCapteur(double dbDeb, double dbFin, const std::string & nodeName)
{
    m_dbSensorPeriodStart = dbDeb;
    m_dbSensorPeriodEnd = dbFin;
}

 DocTrafic::PCAPTEUR GMLDocTrafic::AddInfoCapteur(const std::string & sIdCpt, const std::string & sTypeVeh, const std::string & sVitGlob, const std::string & sNbCumGlob, const std::string & sVitVoie, const std::string & sNbCumVoie, const std::string & sFlow, bool bExtractVehicleList)
 {
     if(m_XmlWriterSensors)
     {
         // rmq. Pour l'instant, on ne sort pas les résultats par type de véhicule... à voir si il faut le faire et si le format GML/OM peut le gérer
         if(sTypeVeh.empty())
         {
             m_XmlWriterSensors->writeStartElement(XS("gml:member"));
             m_XmlWriterSensors->writeStartElement(XS("om:OM_Observation"));
             m_XmlWriterSensors->writeAttributeString(XS("gml:id"), XS((sIdCpt + "_" + SystemUtil::ToString(2, m_dbSensorPeriodEnd)).c_str()));

             writeCommonObservationNodes(sIdCpt);

             m_XmlWriterSensors->writeStartElement(XS("om:result"));
             m_XmlWriterSensors->writeStartElement(XS("gml:DataBlock"));
             m_XmlWriterSensors->writeStartElement(XS("gml:rangeParameters"));
             m_XmlWriterSensors->writeEndElement(); // gml:rangeParameters
             m_XmlWriterSensors->writeStartElement(XS("gml:tupleList"));
             m_XmlWriterSensors->writeText(XS((sVitGlob + "," + sNbCumGlob).c_str()));
             m_XmlWriterSensors->writeEndElement(); // gml:tupleList
             m_XmlWriterSensors->writeEndElement(); // gml:DataBlock
             m_XmlWriterSensors->writeEndElement(); // om:result

             m_XmlWriterSensors->writeEndElement(); // om:OM_Observation
             m_XmlWriterSensors->writeEndElement(); // gml:member
         }
     }

     return NULL;
 }

 DocTrafic::PCAPTEUR GMLDocTrafic::AddInfoCapteurLongitudinal(const std::string & sIdCpt, const std::string & sVoieOrigine, const std::string & sVoieDestination, const std::string & sCount)
 {
     return NULL;
 }

DocTrafic::PCAPTEUR GMLDocTrafic::AddInfoCapteurEdie(const std::string & sIdCpt, const std::string & sConcentrationCum, const std::string & sDebitCum, const std::string & sConcentration, const std::string & sDebit)
{
    if(m_XmlWriterSensors)
    {
        m_XmlWriterSensors->writeStartElement(XS("gml:member"));
        m_XmlWriterSensors->writeStartElement(XS("om:OM_Observation"));
        m_XmlWriterSensors->writeAttributeString(XS("gml:id"), XS((sIdCpt + "_" + SystemUtil::ToString(2, m_dbSensorPeriodEnd)).c_str()));

        writeCommonObservationNodes(sIdCpt);

        m_XmlWriterSensors->writeStartElement(XS("om:result"));
        m_XmlWriterSensors->writeStartElement(XS("gml:DataBlock"));
        m_XmlWriterSensors->writeStartElement(XS("gml:rangeParameters"));
        m_XmlWriterSensors->writeEndElement(); // gml:rangeParameters
        m_XmlWriterSensors->writeStartElement(XS("gml:tupleList"));
        m_XmlWriterSensors->writeText(XS((sConcentrationCum + "," + sDebitCum).c_str()));
        m_XmlWriterSensors->writeEndElement(); // gml:tupleList
        m_XmlWriterSensors->writeEndElement(); // gml:DataBlock
        m_XmlWriterSensors->writeEndElement(); // om:result

        m_XmlWriterSensors->writeEndElement(); // om:OM_Observation
        m_XmlWriterSensors->writeEndElement(); // gml:member
    }

    return NULL;
}

DocTrafic::PCAPTEUR GMLDocTrafic::AddInfoCapteurMFD(const std::string & sIdCpt, bool bIncludeStrictData, const std::string & sGeomLength1, const std::string & sGeomLength2, const std::string & sGeomLength3,
        const std::string & sDistanceTotaleParcourue, const std::string & sDistanceTotaleParcourueStricte, const std::string & sTempsTotalPasse, const std::string & sTempsTotalPasseStrict,
        const std::string & sDebitSortie, const std::string &  sIntDebitSortie, const std::string &  sTransDebitSortie, const std::string & sDebitSortieStrict, const std::string & sLongueurDeplacement, const std::string & sLongueurDeplacementStricte,
        const std::string & sVitesseSpatiale, const std::string & sVitesseSpatialeStricte, const std::string & sConcentration, const std::string & sDebit)
{
    if(m_XmlWriterSensors)
    {
        m_XmlWriterSensors->writeStartElement(XS("gml:member"));
        m_XmlWriterSensors->writeStartElement(XS("om:OM_Observation"));
        m_XmlWriterSensors->writeAttributeString(XS("gml:id"), XS((sIdCpt + "_" + SystemUtil::ToString(2, m_dbSensorPeriodEnd)).c_str()));

        writeCommonObservationNodes(sIdCpt);

        m_XmlWriterSensors->writeStartElement(XS("om:result"));
        m_XmlWriterSensors->writeStartElement(XS("gml:DataBlock"));
        m_XmlWriterSensors->writeStartElement(XS("gml:rangeParameters"));
        m_XmlWriterSensors->writeEndElement(); // gml:rangeParameters
        m_XmlWriterSensors->writeStartElement(XS("gml:tupleList"));
        std::string tupleList(sGeomLength1 + "," + sGeomLength2 + "," + sGeomLength3 + "," + sDistanceTotaleParcourue);
        if(bIncludeStrictData)
        {
            tupleList += "," + sDistanceTotaleParcourueStricte;
        }
        tupleList += "," + sTempsTotalPasse;
        if(bIncludeStrictData)
        {
            tupleList += "," + sTempsTotalPasseStrict;
        }
        tupleList += "," + sDebitSortie;
        if(bIncludeStrictData)
        {
            tupleList += "," + sDebitSortieStrict;
        }
        tupleList += "," + sLongueurDeplacement;
        if(bIncludeStrictData)
        {
            tupleList += "," + sLongueurDeplacementStricte;
        }
        tupleList += "," + sVitesseSpatiale;
        if(bIncludeStrictData)
        {
            tupleList += "," + sVitesseSpatialeStricte;
        }
        tupleList += "," + sConcentration;
		tupleList += "," + sDebit;

        m_XmlWriterSensors->writeText(XS(tupleList.c_str()));

        m_XmlWriterSensors->writeEndElement(); // gml:tupleList
        m_XmlWriterSensors->writeEndElement(); // gml:DataBlock
        m_XmlWriterSensors->writeEndElement(); // om:result

        m_XmlWriterSensors->writeEndElement(); // om:OM_Observation
        m_XmlWriterSensors->writeEndElement(); // gml:member
    }

    return NULL;
}

DocTrafic::PCAPTEUR GMLDocTrafic::AddInfoCapteurReservoir(const std::string & sIdCpt)
{
    return NULL;
}

 DocTrafic::PCAPTEUR GMLDocTrafic::AddInfoCapteurBlueTooth(const std::string & sIdCpt, const BlueToothSensorData& data)
 {
     return NULL;
 }

 DocTrafic::PCAPTEUR GMLDocTrafic::AddInfoCapteurGlobal( const std::string & sIdVeh, double TpsEcoulement, double DistanceParcourue)
 {
     return NULL;
 }

 DocTrafic::PCAPTEUR GMLDocTrafic::AddIndicateursCapteurGlobal(double dbTotalTravelledDistance, double dbTotalTravelledTime, double dbVit, int nNbVeh)
{
    if(m_XmlWriterSensors)
    {
        m_XmlWriterSensors->writeStartElement(XS("gml:member"));
        m_XmlWriterSensors->writeStartElement(XS("om:OM_Observation"));
        m_XmlWriterSensors->writeAttributeString(XS("gml:id"), XS(("GlobalSensor_" + SystemUtil::ToString(2, m_dbSensorPeriodEnd)).c_str()));

        writeCommonObservationNodes("GlobalSensor");

        m_XmlWriterSensors->writeStartElement(XS("om:result"));
        m_XmlWriterSensors->writeStartElement(XS("gml:DataBlock"));
        m_XmlWriterSensors->writeStartElement(XS("gml:rangeParameters"));
        m_XmlWriterSensors->writeEndElement(); // gml:rangeParameters
        m_XmlWriterSensors->writeStartElement(XS("gml:tupleList"));
        std::ostringstream oss;
        oss.precision(2);
        oss.setf(std::ios::fixed);
        oss << dbVit << "," << nNbVeh << "," << dbTotalTravelledDistance << "," << dbTotalTravelledTime;
        m_XmlWriterSensors->writeText(XS(oss.str().c_str()));
        m_XmlWriterSensors->writeEndElement(); // gml:tupleList
        m_XmlWriterSensors->writeEndElement(); // gml:DataBlock
        m_XmlWriterSensors->writeEndElement(); // om:result

        m_XmlWriterSensors->writeEndElement(); // om:OM_Observation
        m_XmlWriterSensors->writeEndElement(); // gml:member
    }

    return NULL;
}

void GMLDocTrafic::AddVehCpt(PCAPTEUR xmlNodeCpt, const std::string & sId, double dbInstPsg, int nVoie)
{
}

void GMLDocTrafic::AddTraverseeCpt(PCAPTEUR xmlNodeCpt, const std::string & vehId, const std::string & entryTime, const std::string & exitTime, const std::string & travelledDistance,
    const std::string & bCreatedInZone, const std::string & bDetroyedInZone)
{
}

void GMLDocTrafic::SaveLastInstant()
{
    // finalisation de la période du fichier des états des feux tricolores
    if(m_XmlWriterTrafficLights)
    {
        m_XmlWriterTrafficLights->writeEndElement(); // TrafficLightsStatus
    }
}

void GMLDocTrafic::RemoveLastInstant()
{
}

void GMLDocTrafic::Add(DocTrafic * docTraficSrc)
{
    // récupération des états des feux tricolores
    GMLDocTrafic* GmlDocTraficSrc = (GMLDocTrafic*)docTraficSrc;
    XmlReaderSymuvia * XmlReader;
    if(m_XmlWriterTrafficLights)
    {
	    XmlReader = XmlReaderSymuvia::Create(GmlDocTraficSrc->GetTrafficLightsFileName());
	    XmlReader->ReadToFollowing( "tls:TrafficLightsStatus" );	
        while( !XmlReader->Name.compare("tls:TrafficLightsStatus") )
	    {
            XmlReader->WriteNode(m_XmlWriterTrafficLights, false);		
	    }
        delete XmlReader;
    }

    // récupération des états des capteurs
    if(m_XmlWriterSensors)
    {
	    XmlReader = XmlReaderSymuvia::Create(GmlDocTraficSrc->GetSensorsFileName());
        bool bContinueOk = true;
        do
        {
            XmlReader->ReadToFollowing( "om:OM_Observation");	
            if( !XmlReader->Name.compare("om:OM_Observation"))
	        {
                m_XmlWriterSensors->writeStartElement(XS("gml:member"));
                XmlReader->WriteNode(m_XmlWriterSensors, false);		
                m_XmlWriterSensors->writeEndElement(); // gml:member
	        }
            else
            {
                bContinueOk = false;
            }
        }
        while(bContinueOk);
        delete XmlReader;
    }

    // récupération des nouveaux véhicules et nouvelles trajectoires
    XmlReader = XmlReaderSymuvia::Create(GmlDocTraficSrc->GetTrajHeaderFileName());
	XmlReader->ReadToFollowing( "mf:member");	
    XmlReader->ResetStartEndFlags();
    while( !XmlReader->Name.compare("mf:member"))
	{
        XmlReader->WriteNode(m_XmlWriterTrajsHeader, false);		
	}
    XmlReader->ReadToFollowing( "mf:LinearTrajectory");
    while( !XmlReader->Name.compare("mf:LinearTrajectory"))
	{
        XmlReader->WriteNode(m_XmlWriterTrajsTemp, false);		
	}
    delete XmlReader;

    m_mapLastVehiclePositions = GmlDocTraficSrc->m_mapLastVehiclePositions;
}

void GMLDocTrafic::AddCellule(int nID, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav, double dbYav, double dbZav)
{
}

void GMLDocTrafic::AddTroncon(const std::string & strLibelle, Point * pPtAm, Point * pPtAv, int nVoie, const std::string & ssBrique, std::deque<Point*> &lstPtInterne, double dbLong, std::vector<double> dbLarg, double dbStockMax, double dbStockInitial, Tuyau * pTuyau)
{
    if(ssBrique.empty())
    {
        m_XmlWriterNetwork->writeStartElement(XS("cityObjectMember"));
        m_XmlWriterNetwork->writeStartElement(XS("tran:Road"));
        m_XmlWriterNetwork->writeAttributeString(XS("gml:id"),XS(strLibelle.c_str()));

        // Nom
        m_XmlWriterNetwork->writeStartElement(XS("gml:name"));
        m_XmlWriterNetwork->writeText(XS(strLibelle.c_str()));
        m_XmlWriterNetwork->writeEndElement(); // gml:name

        // fonction du troncon
        m_XmlWriterNetwork->writeStartElement(XS("tran:function"));
        m_XmlWriterNetwork->writeText(XS("road"));
        m_XmlWriterNetwork->writeEndElement(); // tran:function

        // Types de véhicules autorisés
        std::set<std::string> gmlTypes;
        for(size_t i = 0; i < pTuyau->GetReseau()->GetLstTypesVehicule().size(); i++)
        {
            TypeVehicule * pTypeVeh = pTuyau->GetReseau()->GetLstTypesVehicule()[i];
            if(!pTuyau->IsInterdit(pTypeVeh))
            {
                gmlTypes.insert(pTypeVeh->GetGMLLabel());
            }
        }

        std::set<std::string>::const_iterator iterTypeVeh;
        for(iterTypeVeh = gmlTypes.begin(); iterTypeVeh != gmlTypes.end(); ++iterTypeVeh)
        {
            m_XmlWriterNetwork->writeStartElement(XS("tran:usage"));
            m_XmlWriterNetwork->writeText(XS((*iterTypeVeh).c_str()));
            m_XmlWriterNetwork->writeEndElement(); // tran:usage
        }

        double largeurTotale = 0;
        for(size_t i = 0; i < dbLarg.size(); i++)
        {
            largeurTotale += dbLarg[i];
        }

        // Ajout des surfaces des voies de circulation
        double dbRightOfLanePos = largeurTotale/2.0;
        for(size_t iLane = 0; iLane < pTuyau->GetLstLanes().size(); iLane++)
        {
            Voie * pLane = pTuyau->GetLstLanes()[iLane];
            m_XmlWriterNetwork->writeStartElement(XS("tran:trafficArea"));
            m_XmlWriterNetwork->writeStartElement(XS("tran:TrafficArea"));

            // nom de la voie
            m_XmlWriterNetwork->writeStartElement(XS("gml:name"));
            m_XmlWriterNetwork->writeText(XS(pLane->GetLabel().c_str()));
            m_XmlWriterNetwork->writeEndElement(); // gml:name
            
            // fonction de la voie
            m_XmlWriterNetwork->writeStartElement(XS("tran:function"));
            m_XmlWriterNetwork->writeText(XS("driving_lane"));
            m_XmlWriterNetwork->writeEndElement(); // tran:function

            // Types de véhicules autorisés
            gmlTypes.clear();
            for(size_t i = 0; i < pTuyau->GetReseau()->GetLstTypesVehicule().size(); i++)
            {
                TypeVehicule * pTypeVeh = pTuyau->GetReseau()->GetLstTypesVehicule()[i];
                if(!pTuyau->IsVoieInterdite(pTypeVeh, (int)iLane))
                {
                    gmlTypes.insert(pTypeVeh->GetGMLLabel());
                }
            }
            for(iterTypeVeh = gmlTypes.begin(); iterTypeVeh != gmlTypes.end(); ++iterTypeVeh)
            {
                m_XmlWriterNetwork->writeStartElement(XS("tran:usage"));
                m_XmlWriterNetwork->writeText(XS((*iterTypeVeh).c_str()));
                m_XmlWriterNetwork->writeEndElement(); // tran:usage
            }

            m_XmlWriterNetwork->writeStartElement(XS("tran:lod2MultiSurface"));
            m_XmlWriterNetwork->writeStartElement(XS("gml:MultiSurface"));
            m_XmlWriterNetwork->writeStartElement(XS("gml:surfaceMember"));
            m_XmlWriterNetwork->writeStartElement(XS("gml:Polygon"));
            m_XmlWriterNetwork->writeStartElement(XS("gml:exterior"));
            m_XmlWriterNetwork->writeStartElement(XS("gml:LinearRing"));
            m_XmlWriterNetwork->writeStartElement(XS("gml:posList"));

            std::deque<Point> pointsRight = pTuyau->GetLineString(dbRightOfLanePos);
            std::deque<Point> pointsLeft = pTuyau->GetLineString(dbRightOfLanePos-dbLarg[iLane]);
            int nbPoints;
            std::string posList = BuildSurfaceString(pointsRight, pointsLeft, nbPoints);

            m_XmlWriterNetwork->writeAttributeString(XS("count"),XS(SystemUtil::ToString(nbPoints).c_str()));
            m_XmlWriterNetwork->writeAttributeString(XS("srsDimension"),XS("3"));
            m_XmlWriterNetwork->writeText(XS(posList.c_str()));

            m_XmlWriterNetwork->writeEndElement(); // gml:posList
            m_XmlWriterNetwork->writeEndElement(); // gml:LinearRing
            m_XmlWriterNetwork->writeEndElement(); // gml:exterior
            m_XmlWriterNetwork->writeEndElement(); // gml:Polygon
            m_XmlWriterNetwork->writeEndElement(); // gml:surfaceMember
            m_XmlWriterNetwork->writeEndElement(); // gml:MultiSurface
            m_XmlWriterNetwork->writeEndElement(); // tran:lod2MultiSurface

            m_XmlWriterNetwork->writeEndElement(); // tran:trafficArea
            m_XmlWriterNetwork->writeEndElement(); // tran:TrafficArea

            dbRightOfLanePos -= dbLarg[iLane];
        }

        // lod0 - représentation filaire du tronçon
        m_XmlWriterNetwork->writeStartElement(XS("tran:lod0Network"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:CompositeCurve"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:curveMember"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:LineString"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:posList"));
        m_XmlWriterNetwork->writeAttributeString(XS("count"),XS(SystemUtil::ToString(lstPtInterne.size() + 2).c_str()));
        m_XmlWriterNetwork->writeAttributeString(XS("srsDimension"),XS("3"));

        std::ostringstream oss;
        oss.precision(2);
	    oss.setf(std::ios::fixed);
        oss << pPtAm->dbX << " " << pPtAm->dbY  << " " << pPtAm->dbZ;
        for(size_t i = 0; i < lstPtInterne.size(); i++)
        {    		
		    oss << " " << lstPtInterne[i]->dbX << " " << lstPtInterne[i]->dbY << " " << lstPtInterne[i]->dbZ;
        }
        oss << " " << pPtAv->dbX << " " << pPtAv->dbY  << " " << pPtAv->dbZ;

        m_XmlWriterNetwork->writeText(XS(oss.str().c_str()));
        
        m_XmlWriterNetwork->writeEndElement(); // gml:posList
        m_XmlWriterNetwork->writeEndElement(); // gml:LineString
        m_XmlWriterNetwork->writeEndElement(); // gml:curveMember
        m_XmlWriterNetwork->writeEndElement(); // gml:CompositeCurve
        m_XmlWriterNetwork->writeEndElement(); // tran:lod0Network

        
        // lod1 - représentation surfacique du tronçon
        m_XmlWriterNetwork->writeStartElement(XS("tran:lod1MultiSurface"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:MultiSurface"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:surfaceMember"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:Polygon"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:exterior"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:LinearRing"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:posList"));

        std::deque<Point> pointsRight = pTuyau->GetLineString(largeurTotale/2.0);
        std::deque<Point> pointsLeft = pTuyau->GetLineString(-largeurTotale/2.0);
        int nbPoints;
        std::string posList = BuildSurfaceString(pointsRight, pointsLeft, nbPoints);

        m_XmlWriterNetwork->writeAttributeString(XS("count"),XS(SystemUtil::ToString(nbPoints).c_str()));
        m_XmlWriterNetwork->writeAttributeString(XS("srsDimension"),XS("3"));
        m_XmlWriterNetwork->writeText(XS(posList.c_str()));

        m_XmlWriterNetwork->writeEndElement(); // gml:posList
        m_XmlWriterNetwork->writeEndElement(); // gml:LinearRing
        m_XmlWriterNetwork->writeEndElement(); // gml:exterior
        m_XmlWriterNetwork->writeEndElement(); // gml:Polygon
        m_XmlWriterNetwork->writeEndElement(); // gml:surfaceMember
        m_XmlWriterNetwork->writeEndElement(); // gml:MultiSurface
        m_XmlWriterNetwork->writeEndElement(); // tran:lod1MultiSurface
        m_XmlWriterNetwork->writeEndElement(); // tran:Road
        m_XmlWriterNetwork->writeEndElement(); // cityObjectMember
    }
}

void GMLDocTrafic::AddVoie(Point * pPtAm, Point * pPtAv, std::deque<Point*> &lstPtInterne, double dbLong)
{
}

void GMLDocTrafic::AddDefCapteur(const std::string & sId, const std::string & sT, double dbPos, Point ptCoord)
{
    m_XmlWriterSensors->writeStartElement(XS("gml:member"));
    m_XmlWriterSensors->writeStartElement(XS("sml:PhysicalComponent"));
    m_XmlWriterSensors->writeAttributeString(XS("gml:id"), XS(sId.c_str()));

    m_XmlWriterSensors->writeStartElement(XS("sml:featuresOfInterest"));
    m_XmlWriterSensors->writeStartElement(XS("sml:FeatureList"));
    m_XmlWriterSensors->writeStartElement(XS("sml:feature"));
    m_XmlWriterSensors->writeAttributeString(XS("xlink:href"), XS((SystemUtil::GetFileName(m_strFilenameNetwork) + "#" + sT).c_str()));
    m_XmlWriterSensors->writeEndElement(); // sml:feature
    m_XmlWriterSensors->writeEndElement(); // sml:FeatureList
    m_XmlWriterSensors->writeEndElement(); // sml:featuresOfInterest

    // outputs
    //////////////////////
    m_XmlWriterSensors->writeStartElement(XS("sml:outputs"));
    m_XmlWriterSensors->writeStartElement(XS("sml:OutputList"));

    // vitesse
    writeSensorOutput(XS("speed"),XS("m/s"));

    // nombre de véhicule cumulés
    writeSensorOutput(XS("aggregated_vehicles_count"),XS(""));

    m_XmlWriterSensors->writeEndElement(); // sml:OutputList
    m_XmlWriterSensors->writeEndElement(); // sml:outputs

    // parametres
    /////////////////////////
    m_XmlWriterSensors->writeStartElement(XS("sml:parameters"));
    m_XmlWriterSensors->writeStartElement(XS("sml:ParameterList"));
    m_XmlWriterSensors->writeStartElement(XS("sml:parameter"));
    m_XmlWriterSensors->writeAttributeString(XS("name"),XS("position"));
    m_XmlWriterSensors->writeStartElement(XS("swe:Quantity"));
    m_XmlWriterSensors->writeStartElement(XS("swe:label"));
    m_XmlWriterSensors->writeText(XS("position"));
    m_XmlWriterSensors->writeEndElement(); // swe:label
    m_XmlWriterSensors->writeStartElement(XS("swe:uom"));
    m_XmlWriterSensors->writeAttributeString(XS("code"),XS("m"));
    m_XmlWriterSensors->writeEndElement(); // swe:uom
    m_XmlWriterSensors->writeStartElement(XS("swe:value"));
    m_XmlWriterSensors->writeText(XS(SystemUtil::ToString(2, dbPos).c_str()));
    m_XmlWriterSensors->writeEndElement(); // swe:value
    m_XmlWriterSensors->writeEndElement(); // swe:Quantity
    m_XmlWriterSensors->writeEndElement(); // sml:parameter
    m_XmlWriterSensors->writeEndElement(); // sml:ParameterList
    m_XmlWriterSensors->writeEndElement(); // sml:parameters

    // position
    /////////////////
    m_XmlWriterSensors->writeStartElement(XS("sml:position"));
    m_XmlWriterSensors->writeStartElement(XS("gml:Point"));
    m_XmlWriterSensors->writeAttributeString(XS("gml:id"), XS(("Pt_" + sId).c_str()));
    m_XmlWriterSensors->writeStartElement(XS("gml:pos"));

    std::ostringstream oss;
    oss.precision(2);
    oss.setf(std::ios::fixed);
    oss << ptCoord.dbX << " " << ptCoord.dbY << " " << ptCoord.dbZ;

    m_XmlWriterSensors->writeText(XS(oss.str().c_str()));
    m_XmlWriterSensors->writeEndElement(); // gml:pos
    m_XmlWriterSensors->writeEndElement(); // gml:Point
    m_XmlWriterSensors->writeEndElement(); // sml:position


    m_XmlWriterSensors->writeEndElement(); // sml:PhysicalComponent
    m_XmlWriterSensors->writeEndElement(); // gml:member
}

void GMLDocTrafic::AddDefCapteurLongitudinal(const std::string & sId, const std::string & sT, double dbPosDebut, Point ptCoordDebut,
        double dbPosFin, Point ptCoordFin)
{
}

void GMLDocTrafic::AddDefCapteurEdie(const std::string & sId, const std::string & sT, double dbPosDebut, Point ptCoordDebut,
        double dbPosFin, Point ptCoordFin)
{
    m_XmlWriterSensors->writeStartElement(XS("gml:member"));
    m_XmlWriterSensors->writeStartElement(XS("sml:PhysicalComponent"));
    m_XmlWriterSensors->writeAttributeString(XS("gml:id"), XS(sId.c_str()));

    m_XmlWriterSensors->writeStartElement(XS("sml:featuresOfInterest"));
    m_XmlWriterSensors->writeStartElement(XS("sml:FeatureList"));
    m_XmlWriterSensors->writeStartElement(XS("sml:feature"));
    m_XmlWriterSensors->writeAttributeString(XS("xlink:href"), XS((SystemUtil::GetFileName(m_strFilenameNetwork) + "#" + sT).c_str()));
    m_XmlWriterSensors->writeEndElement(); // sml:feature
    m_XmlWriterSensors->writeEndElement(); // sml:FeatureList
    m_XmlWriterSensors->writeEndElement(); // sml:featuresOfInterest

    // outputs
    //////////////////////
    m_XmlWriterSensors->writeStartElement(XS("sml:outputs"));
    m_XmlWriterSensors->writeStartElement(XS("sml:OutputList"));

    // concentration
    writeSensorOutput(XS("concentration"),XS("veh/m"));

    // debit
    writeSensorOutput(XS("flow"),XS("veh/s"));

    m_XmlWriterSensors->writeEndElement(); // sml:OutputList
    m_XmlWriterSensors->writeEndElement(); // sml:outputs

    // parametres
    /////////////////////////
    m_XmlWriterSensors->writeStartElement(XS("sml:parameters"));
    m_XmlWriterSensors->writeStartElement(XS("sml:ParameterList"));

    // position début
    m_XmlWriterSensors->writeStartElement(XS("sml:parameter"));
    m_XmlWriterSensors->writeAttributeString(XS("name"),XS("beginPosition"));
    m_XmlWriterSensors->writeStartElement(XS("swe:Quantity"));
    m_XmlWriterSensors->writeStartElement(XS("swe:label"));
    m_XmlWriterSensors->writeText(XS("beginPosition"));
    m_XmlWriterSensors->writeEndElement(); // swe:label
    m_XmlWriterSensors->writeStartElement(XS("swe:uom"));
    m_XmlWriterSensors->writeAttributeString(XS("code"),XS("m"));
    m_XmlWriterSensors->writeEndElement(); // swe:uom
    m_XmlWriterSensors->writeStartElement(XS("swe:value"));
    m_XmlWriterSensors->writeText(XS(SystemUtil::ToString(2, dbPosDebut).c_str()));
    m_XmlWriterSensors->writeEndElement(); // swe:value
    m_XmlWriterSensors->writeEndElement(); // swe:Quantity
    m_XmlWriterSensors->writeEndElement(); // sml:parameter

    // position fin
    m_XmlWriterSensors->writeStartElement(XS("sml:parameter"));
    m_XmlWriterSensors->writeAttributeString(XS("name"),XS("endPosition"));
    m_XmlWriterSensors->writeStartElement(XS("swe:Quantity"));
    m_XmlWriterSensors->writeStartElement(XS("swe:label"));
    m_XmlWriterSensors->writeText(XS("endPosition"));
    m_XmlWriterSensors->writeEndElement(); // swe:label
    m_XmlWriterSensors->writeStartElement(XS("swe:uom"));
    m_XmlWriterSensors->writeAttributeString(XS("code"),XS("m"));
    m_XmlWriterSensors->writeEndElement(); // swe:uom
    m_XmlWriterSensors->writeStartElement(XS("swe:value"));
    m_XmlWriterSensors->writeText(XS(SystemUtil::ToString(2, dbPosFin).c_str()));
    m_XmlWriterSensors->writeEndElement(); // swe:value
    m_XmlWriterSensors->writeEndElement(); // swe:Quantity
    m_XmlWriterSensors->writeEndElement(); // sml:parameter

    m_XmlWriterSensors->writeEndElement(); // sml:ParameterList
    m_XmlWriterSensors->writeEndElement(); // sml:parameters

    m_XmlWriterSensors->writeEndElement(); // sml:PhysicalComponent
    m_XmlWriterSensors->writeEndElement(); // gml:member
}

void GMLDocTrafic::AddDefCapteurMFD(const std::string & sId, bool bIncludeStrictData, const std::vector<Tuyau*>& lstTuyaux)
{
    m_XmlWriterSensors->writeStartElement(XS("gml:member"));
    m_XmlWriterSensors->writeStartElement(XS("sml:PhysicalComponent"));
    m_XmlWriterSensors->writeAttributeString(XS("gml:id"), XS(sId.c_str()));

    m_XmlWriterSensors->writeStartElement(XS("sml:featuresOfInterest"));
    m_XmlWriterSensors->writeStartElement(XS("sml:FeatureList"));
    std::string netFileName = SystemUtil::GetFileName(m_strFilenameNetwork);
    for(size_t iTuy = 0; iTuy < lstTuyaux.size(); iTuy++)
    {
        m_XmlWriterSensors->writeStartElement(XS("sml:feature"));
        m_XmlWriterSensors->writeAttributeString(XS("xlink:href"), XS((netFileName + "#" + lstTuyaux[iTuy]->GetLabel()).c_str()));
        m_XmlWriterSensors->writeEndElement(); // sml:feature
    }
    m_XmlWriterSensors->writeEndElement(); // sml:FeatureList
    m_XmlWriterSensors->writeEndElement(); // sml:featuresOfInterest

    // outputs
    //////////////////////
    m_XmlWriterSensors->writeStartElement(XS("sml:outputs"));
    m_XmlWriterSensors->writeStartElement(XS("sml:OutputList"));

    writeSensorOutput(XS("geometric_length_barycentre"),XS("m"));
    writeSensorOutput(XS("geometric_length_downstream_move"),XS("m"));
    writeSensorOutput(XS("geometric_length_strict"),XS("m"));
    writeSensorOutput(XS("total_travelled_distance"),XS("m"));
    if(bIncludeStrictData)
    {
        writeSensorOutput(XS("total_travelled_distance_strict"),XS("m"));
    }
    writeSensorOutput(XS("total_travelled_time"),XS("s"));
    if(bIncludeStrictData)
    {
        writeSensorOutput(XS("total_travelled_time_strict"),XS("s"));
    }
    writeSensorOutput(XS("flow"),XS("veh/s"));
    if(bIncludeStrictData)
    {
        writeSensorOutput(XS("flow_strict"),XS("veh/s"));
    }
    writeSensorOutput(XS("travel_length"),XS("m.s/veh"));
    if(bIncludeStrictData)
    {
        writeSensorOutput(XS("travel_length_strict"),XS("m.s/veh"));
    }
    writeSensorOutput(XS("speed"),XS("m/s"));
    if(bIncludeStrictData)
    {
        writeSensorOutput(XS("speed_strict"),XS("m/s"));
    }

    writeSensorOutput(XS("concentration"),XS("veh/m"));

    m_XmlWriterSensors->writeEndElement(); // sml:OutputList
    m_XmlWriterSensors->writeEndElement(); // sml:outputs

    m_XmlWriterSensors->writeEndElement(); // sml:PhysicalComponent
    m_XmlWriterSensors->writeEndElement(); // gml:member
}

void GMLDocTrafic::AddDefEntree(const std::string & sId, Point ptCoord)
{
}

void GMLDocTrafic::AddDefSortie(const std::string & sId, Point ptCoord)
{
}

struct AngleTuyau {
    double dbAngle;
    Tuyau * pTuyau;
};

bool AngleTuyauCompare(AngleTuyau i,AngleTuyau j)
{
    return i.dbAngle<j.dbAngle;
}

void GMLDocTrafic::AddDefCAF(CarrefourAFeuxEx * pCAF)
{
    m_XmlWriterNetwork->writeStartElement(XS("cityObjectMember"));
    m_XmlWriterNetwork->writeStartElement(XS("tran:TransportationComplex"));
    m_XmlWriterNetwork->writeAttributeString(XS("gml:id"),XS(pCAF->GetID().c_str()));

    // Nom
    m_XmlWriterNetwork->writeStartElement(XS("gml:name"));
    m_XmlWriterNetwork->writeText(XS(pCAF->GetID().c_str()));
    m_XmlWriterNetwork->writeEndElement(); // gml:name

    // Fonction
    m_XmlWriterNetwork->writeStartElement(XS("tran:function"));
    m_XmlWriterNetwork->writeText(XS("junction"));
    m_XmlWriterNetwork->writeEndElement(); // tran:function

    // lod0 - représentation filaire du CAF
    m_XmlWriterNetwork->writeStartElement(XS("tran:lod0Network"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:CompositeCurve"));

    for(size_t iTuyau = 0; iTuyau < pCAF->GetReseau()->m_LstTuyaux.size(); iTuyau++)
    {
        Tuyau * pTuyau = pCAF->GetReseau()->m_LstTuyaux[iTuyau];
        if(pTuyau->GetBriqueParente() == pCAF)
        {
            m_XmlWriterNetwork->writeStartElement(XS("gml:curveMember"));
            m_XmlWriterNetwork->writeStartElement(XS("gml:LineString"));
            m_XmlWriterNetwork->writeAttributeString(XS("gml:id"),XS(pTuyau->GetLabel().c_str()));
            m_XmlWriterNetwork->writeStartElement(XS("gml:posList"));
            m_XmlWriterNetwork->writeAttributeString(XS("count"),XS(SystemUtil::ToString(pTuyau->GetLstPtsInternes().size() + 2).c_str()));
            m_XmlWriterNetwork->writeAttributeString(XS("srsDimension"),XS("3"));

            std::ostringstream oss;
            oss.precision(2);
	        oss.setf(std::ios::fixed);
            oss << pTuyau->GetExtAmont()->dbX << " " << pTuyau->GetExtAmont()->dbY  << " " << pTuyau->GetExtAmont()->dbZ;
            for(size_t i = 0; i < pTuyau->GetLstPtsInternes().size(); i++)
            {    		
		        oss << " " << pTuyau->GetLstPtsInternes()[i]->dbX << " " << pTuyau->GetLstPtsInternes()[i]->dbY << " " << pTuyau->GetLstPtsInternes()[i]->dbZ;
            }
            oss << " " << pTuyau->GetExtAval()->dbX << " " << pTuyau->GetExtAval()->dbY  << " " << pTuyau->GetExtAval()->dbZ;

            m_XmlWriterNetwork->writeText(XS(oss.str().c_str()));
        
            m_XmlWriterNetwork->writeEndElement(); // gml:posList
            m_XmlWriterNetwork->writeEndElement(); // gml:LineString
            m_XmlWriterNetwork->writeEndElement(); // gml:curveMember
        }
    }

    m_XmlWriterNetwork->writeEndElement(); // gml:CompositeCurve
    m_XmlWriterNetwork->writeEndElement(); // tran:lod0Network

    // lod1 - représentation surfacique du CAF
    m_XmlWriterNetwork->writeStartElement(XS("tran:lod1MultiSurface"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:MultiSurface"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:surfaceMember"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:Polygon"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:exterior"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:LinearRing"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:posList"));

    // Ensemble des tuyaux liés au CAF
    std::set<Tuyau*> lstTuyaux;
    lstTuyaux.insert(pCAF->m_LstTuyAm.begin(), pCAF->m_LstTuyAm.end());
    lstTuyaux.insert(pCAF->m_LstTuyAv.begin(), pCAF->m_LstTuyAv.end());

    // Calcul de l'angle entre l'extremité du tuyau qui touche le CAF et le barycentre du CAF
    std::vector<AngleTuyau> lstTuyauxTri;
    Point * pBaryCentre = pCAF->CalculBarycentre();
    std::set<Tuyau*>::iterator iter;
    for(iter = lstTuyaux.begin(); iter != lstTuyaux.end(); ++iter)
    {
        AngleTuyau angleTuyau;
        angleTuyau.pTuyau = *iter;

        if((*iter)->GetBriqueAmont() == pCAF)
        {
            angleTuyau.dbAngle = atan2((*iter)->GetExtAmont()->dbY - pBaryCentre->dbY, (*iter)->GetExtAmont()->dbX - pBaryCentre->dbX);
        }
        else
        {
            angleTuyau.dbAngle = atan2((*iter)->GetExtAval()->dbY - pBaryCentre->dbY, (*iter)->GetExtAval()->dbX - pBaryCentre->dbX);
        }
        lstTuyauxTri.push_back(angleTuyau);
    }

    delete pBaryCentre;

    // Tri des tuyaux par angle
    std::sort(lstTuyauxTri.begin(), lstTuyauxTri.end(), AngleTuyauCompare);

    // Construction de la liste des points constituant l'anneau exterieur de la surface
    std::deque<Point> points;
    for(size_t iTuyau = 0; iTuyau < lstTuyauxTri.size(); iTuyau++)
    {
        Tuyau * pTuyau = lstTuyauxTri[iTuyau].pTuyau;
        double largeurTotale = 0;
        for(size_t i = 0; i < pTuyau->getLargeursVoies().size(); i++)
        {
            largeurTotale += pTuyau->getLargeursVoies()[i];
        }
        // Calcul des points extrêmes du tuyau
        std::deque<Point> pointsRight = pTuyau->GetLineString(largeurTotale/2.0);
        std::deque<Point> pointsLeft = pTuyau->GetLineString(-largeurTotale/2.0);

        if(pTuyau->GetBriqueAmont() == pCAF)
        {
            points.push_back(pointsRight.front());
            points.push_back(pointsLeft.front());
        }
        else
        {
            points.push_back(pointsLeft.back());
            points.push_back(pointsRight.back());
        }
    }

    int nbPoints;
    std::string posList = BuildSurfaceString(points, std::deque<Point>(0), nbPoints);

    m_XmlWriterNetwork->writeAttributeString(XS("count"),XS(SystemUtil::ToString(nbPoints).c_str()));
    m_XmlWriterNetwork->writeAttributeString(XS("srsDimension"),XS("3"));
    m_XmlWriterNetwork->writeText(XS(posList.c_str()));

    m_XmlWriterNetwork->writeEndElement(); // gml:posList
    m_XmlWriterNetwork->writeEndElement(); // gml:LinearRing
    m_XmlWriterNetwork->writeEndElement(); // gml:exterior
    m_XmlWriterNetwork->writeEndElement(); // gml:Polygon
    m_XmlWriterNetwork->writeEndElement(); // gml:surfaceMember
    m_XmlWriterNetwork->writeEndElement(); // gml:MultiSurface
    m_XmlWriterNetwork->writeEndElement(); // tran:lod1MultiSurface

    m_XmlWriterNetwork->writeEndElement(); // tran:TransportationComplex
    m_XmlWriterNetwork->writeEndElement(); // cityObjectMember
}

void GMLDocTrafic::AddDefGir(Giratoire * pGir)
{
    m_XmlWriterNetwork->writeStartElement(XS("cityObjectMember"));
    m_XmlWriterNetwork->writeStartElement(XS("tran:TransportationComplex"));
    m_XmlWriterNetwork->writeAttributeString(XS("gml:id"),XS(pGir->GetID().c_str()));

    // Nom
    m_XmlWriterNetwork->writeStartElement(XS("gml:name"));
    m_XmlWriterNetwork->writeText(XS(pGir->GetID().c_str()));
    m_XmlWriterNetwork->writeEndElement(); // gml:name

    // Fonction
    m_XmlWriterNetwork->writeStartElement(XS("tran:function"));
    m_XmlWriterNetwork->writeText(XS("junction"));
    m_XmlWriterNetwork->writeEndElement(); // tran:function

    // lod0 - représentation filaire du Giratoire
    m_XmlWriterNetwork->writeStartElement(XS("tran:lod0Network"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:CompositeCurve"));

    std::ostringstream oss;
    oss.precision(2);
	oss.setf(std::ios::fixed);

    // Liste des points de l'anneau du giratoire
    double dbWidth = 0;
    const std::deque<TuyauMicro*>& tuyInt = pGir->GetTuyauxInternes();
    std::deque<Point> lstPoints;
    for(size_t iTuyau = 0; iTuyau < tuyInt.size(); iTuyau++)
    {
        Tuyau * pTuyau = tuyInt[iTuyau];
        m_XmlWriterNetwork->writeStartElement(XS("gml:curveMember"));
        m_XmlWriterNetwork->writeStartElement(XS("gml:LineString"));
        m_XmlWriterNetwork->writeAttributeString(XS("gml:id"),XS(pTuyau->GetLabel().c_str()));
        m_XmlWriterNetwork->writeStartElement(XS("gml:posList"));
        m_XmlWriterNetwork->writeAttributeString(XS("count"),XS(SystemUtil::ToString(pTuyau->GetLstPtsInternes().size() + 2).c_str()));
        m_XmlWriterNetwork->writeAttributeString(XS("srsDimension"),XS("3"));

        oss.str("");
        lstPoints.push_back(*pTuyau->GetExtAmont());
        oss << pTuyau->GetExtAmont()->dbX << " " << pTuyau->GetExtAmont()->dbY << " " << pTuyau->GetExtAmont()->dbZ;
        for(size_t i = 0; i < pTuyau->GetLstPtsInternes().size(); i++)
        {
            lstPoints.push_back(*pTuyau->GetLstPtsInternes()[i]);
            oss << " " << pTuyau->GetLstPtsInternes()[i]->dbX << " " << pTuyau->GetLstPtsInternes()[i]->dbY << " " << pTuyau->GetLstPtsInternes()[i]->dbZ;
        }
        lstPoints.push_back(*pTuyau->GetExtAval());
        oss << " " << pTuyau->GetExtAval()->dbX << " " << pTuyau->GetExtAval()->dbY << " " << pTuyau->GetExtAval()->dbZ;

        m_XmlWriterNetwork->writeText(XS(oss.str().c_str()));

        if(iTuyau == 0)
        {
            for(size_t iLane = 0; iLane < pTuyau->getLargeursVoies().size(); iLane++)
            {
                dbWidth += pTuyau->getLargeursVoies()[iLane];
            }
        }

        m_XmlWriterNetwork->writeEndElement(); // gml:posList
        m_XmlWriterNetwork->writeEndElement(); // gml:LineString
        m_XmlWriterNetwork->writeEndElement(); // gml:curveMember
    }

    m_XmlWriterNetwork->writeEndElement(); // gml:CompositeCurve
    m_XmlWriterNetwork->writeEndElement(); // tran:lod0Network

    // lod1 - représentation surfacique du Giratoire
    std::deque<Point> exteriorRing = BuildLineStringGeometry(lstPoints, dbWidth/2, dbWidth, pGir->GetID(), pGir->GetReseau()->GetLogger());
    std::deque<Point> interiorRing = BuildLineStringGeometry(lstPoints, -dbWidth / 2, dbWidth, pGir->GetID(), pGir->GetReseau()->GetLogger());

    m_XmlWriterNetwork->writeStartElement(XS("tran:lod1MultiSurface"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:MultiSurface"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:surfaceMember"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:Polygon"));

    // extérieur de l'anneau
    m_XmlWriterNetwork->writeStartElement(XS("gml:exterior"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:LinearRing"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:posList"));
    m_XmlWriterNetwork->writeAttributeString(XS("count"),XS(SystemUtil::ToString(exteriorRing.size()).c_str()));
    m_XmlWriterNetwork->writeAttributeString(XS("srsDimension"),XS("3"));

    oss.str("");
    for(size_t i = 0; i < exteriorRing.size(); i++)
    {    		
        if(i != 0)
        {
            oss << " ";
        }
		oss << exteriorRing[i].dbX << " " << exteriorRing[i].dbY << " " << exteriorRing[i].dbZ;
    }

    m_XmlWriterNetwork->writeText(XS(oss.str().c_str()));

    m_XmlWriterNetwork->writeEndElement(); // gml:posList
    m_XmlWriterNetwork->writeEndElement(); // gml:LinearRing
    m_XmlWriterNetwork->writeEndElement(); // gml:exterior

    // intérieur de l'anneau
    m_XmlWriterNetwork->writeStartElement(XS("gml:interior"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:LinearRing"));
    m_XmlWriterNetwork->writeStartElement(XS("gml:posList"));
    m_XmlWriterNetwork->writeAttributeString(XS("count"),XS(SystemUtil::ToString(interiorRing.size()).c_str()));
    m_XmlWriterNetwork->writeAttributeString(XS("srsDimension"),XS("3"));

    oss.str("");
    for(size_t i = 0; i < interiorRing.size(); i++)
    {    		
        if(i != 0)
        {
            oss << " ";
        }
		oss << interiorRing[i].dbX << " " << interiorRing[i].dbY << " " << interiorRing[i].dbZ;
    }

    m_XmlWriterNetwork->writeText(XS(oss.str().c_str()));

    m_XmlWriterNetwork->writeEndElement(); // gml:posList
    m_XmlWriterNetwork->writeEndElement(); // gml:LinearRing
    m_XmlWriterNetwork->writeEndElement(); // gml:interior

    m_XmlWriterNetwork->writeEndElement(); // gml:Polygon
    m_XmlWriterNetwork->writeEndElement(); // gml:surfaceMember
    m_XmlWriterNetwork->writeEndElement(); // gml:MultiSurface
    m_XmlWriterNetwork->writeEndElement(); // tran:lod1MultiSurface

    
    m_XmlWriterNetwork->writeEndElement(); // tran:TransportationComplex
    m_XmlWriterNetwork->writeEndElement(); // cityObjectMember
}

class LessPointPredicate
{
public:
	bool operator()(const Point i, const Point j) const
	{	
	  return (i.dbZ < j.dbZ)
        || (i.dbZ == j.dbZ && i.dbY < j.dbY)
        || (i.dbZ == j.dbZ && i.dbY == j.dbY && i.dbX < j.dbX);
	}
};
void GMLDocTrafic::AddDefCDF(ControleurDeFeux * pCDF)
{
    std::string sCF = pCDF->GetLabel();
    int placeHolder;

    // Par couple d'entrée sortie
    for(size_t i = 0; i < pCDF->GetLstCoupleEntreeSortie()->size(); i++)
    {            
        const CoupleEntreeSortie cplES = pCDF->GetLstCoupleEntreeSortie()->at(i);

        // un point pour chaque position différente en fonction de la voie amont et de la position des lignes de feux
        std::set<Point, LessPointPredicate> lstPoints;
        Connexion * pConnexion = cplES.pEntree->GetCnxAssAv();
        // Pour chaque voie du tuyau d'entrée
        for(size_t iUpLane = 0; iUpLane < cplES.pEntree->GetLstLanes().size(); iUpLane++)
        {
            if(pConnexion->m_mapMvtAutorises.find(cplES.pEntree->GetLstLanes()[iUpLane]) != pConnexion->m_mapMvtAutorises.end())
            {
                const std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau>  > & mapMoves = pConnexion->m_mapMvtAutorises[cplES.pEntree->GetLstLanes()[iUpLane]];
                if(mapMoves.find(cplES.pSortie) != mapMoves.end())
                {
                    const std::map<int, boost::shared_ptr<MouvementsSortie> > & mapMovesByDownLane = mapMoves.at(cplES.pSortie);
                    // Pour chaque voie aval accessible, détermination de la position du feu et ajout à l'ensemble
                    std::map<int, boost::shared_ptr<MouvementsSortie> >::const_iterator iter;
                    for(iter = mapMovesByDownLane.begin(); iter != mapMovesByDownLane.end(); ++iter)
                    {
                        // on détermine la voie sur laquelle se trouve le feu en tenant compte de la position de la ligne de feu
                        Voie * pVoie = NULL;
                        double dbPk = 0;
                        if(iter->second->m_dbLigneFeu > 0)
                        {
                            // Cas de la ligne de feu après la fin du tronçon amont
                            if(cplES.pEntree->GetBriqueAval())
                            {
                                std::vector<Tuyau*> lstTuyInt;
                                if(cplES.pEntree->GetBriqueAval()->GetTuyauxInternes(cplES.pEntree, (int)iUpLane, cplES.pSortie, iter->first, lstTuyInt))
                                {
                                    double dbRemainingLength = iter->second->m_dbLigneFeu;
                                    bool bFound = false;
                                    for(size_t iTuyInt = 0; iTuyInt < lstTuyInt.size(); iTuyInt++)
                                    {
                                        if(dbRemainingLength <= lstTuyInt[iTuyInt]->GetLength())
                                        {
                                            pVoie = lstTuyInt[iTuyInt]->GetLstLanes()[0];
                                            dbPk = dbRemainingLength;
                                            bFound = true;
                                            break;
                                        }
                                        else
                                        {
                                            dbRemainingLength -= lstTuyInt[iTuyInt]->GetLength();
                                        }
                                    }
                                    if(!bFound)
                                    {
                                        pVoie = lstTuyInt[lstTuyInt.size()-1]->GetLstLanes()[0];
                                        dbPk = pVoie->GetLength();
                                    }
                                }
                            }
                            else
                            {
                                // si la connexion n'est pas une brique, on positionne le feu sur la voie aval, en se ramenant à la fin de la voie aval si ca dépasse
                                pVoie = cplES.pSortie->GetLstLanes()[iter->first];
                                dbPk = std::min<double>(pVoie->GetLength(), iter->second->m_dbLigneFeu);
                            }
                        }
                        else
                        {
                            // Cas de la ligne de feu avant la fin du tronçon amont
                            pVoie = cplES.pEntree->GetLstLanes()[iUpLane];

                            // on n'autorise pas la ligne de feux à être encore plus en arrière que le début de la voie amont
                            dbPk = std::max<double>(0, pVoie->GetLength() + iter->second->m_dbLigneFeu);
                        }

                        Point pt;
                        CalculAbsCoords(pVoie, dbPk, false, pt.dbX, pt.dbY, pt.dbZ);
                        lstPoints.insert(pt);
                    }
                }
            }
        }

        // on fait ce test au cas où le scnéario soit mal défini (un couple entrée sortie du CDF
        // ne correspond à aucun mouvement autorisé produit une liste de points vide. Du coup, autant ne pas sortir
        // ce feu qui ne sert à rien)
        if(!lstPoints.empty())
        {
            const std::string & sTE = cplES.pEntree->GetLabel();
            const std::string & sTS = cplES.pSortie->GetLabel();

            m_XmlWriterNetwork->writeStartElement(XS("cityObjectMember"));
            m_XmlWriterNetwork->writeStartElement(XS("frn:CityFurniture"));
            m_XmlWriterNetwork->writeAttributeString(XS("gml:id"),XS(BuildTrafficLightName(sCF,sTE,sTS).c_str()));

            // Nom
            m_XmlWriterNetwork->writeStartElement(XS("gml:name"));
            m_XmlWriterNetwork->writeText(XS(BuildTrafficLightName(sCF,sTE,sTS).c_str()));
            m_XmlWriterNetwork->writeEndElement(); // gml:name

            // tronçon amont
            m_XmlWriterNetwork->writeStartElement(XS("gen:stringAttribute"));
            m_XmlWriterNetwork->writeAttributeString(XS("name"),XS("upstreamLink"));
            m_XmlWriterNetwork->writeStartElement(XS("gen:value"));
            m_XmlWriterNetwork->writeText(XS(sTE.c_str()));
            m_XmlWriterNetwork->writeEndElement(); // gen:value
            m_XmlWriterNetwork->writeEndElement(); // gen:stringAttribute

            // tronçon aval
            m_XmlWriterNetwork->writeStartElement(XS("gen:stringAttribute"));
            m_XmlWriterNetwork->writeAttributeString(XS("name"),XS("downstreamLink"));
            m_XmlWriterNetwork->writeStartElement(XS("gen:value"));
            m_XmlWriterNetwork->writeText(XS(sTS.c_str()));
            m_XmlWriterNetwork->writeEndElement(); // gen:value
            m_XmlWriterNetwork->writeEndElement(); // gen:stringAttribute

            // Classe
            m_XmlWriterNetwork->writeStartElement(XS("frn:class"));
            m_XmlWriterNetwork->writeText(XS("traffic"));
            m_XmlWriterNetwork->writeEndElement(); // frn:class

            // Fonction
            m_XmlWriterNetwork->writeStartElement(XS("frn:function"));
            m_XmlWriterNetwork->writeText(XS("traffic light"));
            m_XmlWriterNetwork->writeEndElement(); // frn:function

            // Géométrie
            m_XmlWriterNetwork->writeStartElement(XS("frn:lod1Geometry"));
            m_XmlWriterNetwork->writeStartElement(XS("gml:MultiPoint"));

            // écriture des positions pour le feu
            std::set<Point, LessPointPredicate>::iterator iter;
            for(iter = lstPoints.begin(); iter != lstPoints.end(); ++iter)
            {
                m_XmlWriterNetwork->writeStartElement(XS("gml:pointMember"));
                m_XmlWriterNetwork->writeStartElement(XS("gml:Point"));
                m_XmlWriterNetwork->writeStartElement(XS("gml:pos"));
                m_XmlWriterNetwork->writeAttributeString(XS("srsDimension"),XS("3"));
                std::deque<Point> pos(1, *iter);
                m_XmlWriterNetwork->writeText(XS(BuildSurfaceString(pos, std::deque<Point>(0), placeHolder).c_str()));
                m_XmlWriterNetwork->writeEndElement(); // gml:pos
                m_XmlWriterNetwork->writeEndElement(); // gml:Point
                m_XmlWriterNetwork->writeEndElement(); // gml:pointMember
            }

            m_XmlWriterNetwork->writeEndElement(); // gml:MultiPoint
            m_XmlWriterNetwork->writeEndElement(); // frn:lod1Geometry

            m_XmlWriterNetwork->writeEndElement(); // frn:CityFurniture
            m_XmlWriterNetwork->writeEndElement(); // cityObjectMember
        }
    }
}

void GMLDocTrafic::AddArret(const std::string & strLibelle, int numVoie, char * strNomLigne)
{
}

void GMLDocTrafic::AddVehicule(int nID, const std::string & strLibelle, const std::string & strType, const std::string & strGMLType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string & strZoneEntree, const std::string & strZoneSortie, const std::string & strPlaqueEntree, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif, int iLane, const std::string & sLine, const std::vector<std::string> & initialPath, const std::deque<PlageAcceleration> & plagesAcc, const std::string & motifOrigine, const std::string & motifDestination, bool bAddToCreationNode)
{
    m_XmlWriterTrajsHeader->writeStartElement(XS("mf:member"));
    m_XmlWriterTrajsHeader->writeStartElement(XS("traj:Vehicle"));
    m_XmlWriterTrajsHeader->writeAttributeString(XS("gml:id"), XS(("V_" + SystemUtil::ToString(nID)).c_str()));
    m_XmlWriterTrajsHeader->writeStartElement(XS("gml:name"));
    m_XmlWriterTrajsHeader->writeText(XS(SystemUtil::ToString(nID).c_str()));
    m_XmlWriterTrajsHeader->writeEndElement(); // gml:name
    m_XmlWriterTrajsHeader->writeStartElement(XS("traj:vehicleType"));
    m_XmlWriterTrajsHeader->writeText(XS(strGMLType.c_str()));
    m_XmlWriterTrajsHeader->writeEndElement(); // vehicleType
    if(!strEntree.empty())
    {
        m_XmlWriterTrajsHeader->writeStartElement(XS("traj:origin"));
        m_XmlWriterTrajsHeader->writeText(XS(strEntree.c_str()));
        m_XmlWriterTrajsHeader->writeEndElement(); // origin
    }
    if(!strSortie.empty())
    {
        m_XmlWriterTrajsHeader->writeStartElement(XS("traj:destination"));
        m_XmlWriterTrajsHeader->writeText(XS(strSortie.c_str()));
        m_XmlWriterTrajsHeader->writeEndElement(); // destination
    }
    m_XmlWriterTrajsHeader->writeEndElement(); // traj:Vehicle
    m_XmlWriterTrajsHeader->writeEndElement(); // mf:member
}

void GMLDocTrafic::AddVehiculeToCreationNode(int nID, const std::string & strLibelle, const std::string & strType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif)
{
}

void GMLDocTrafic::AddSimuRegulation(XERCES_CPP_NAMESPACE::DOMNode * pRestitutionNode)
{
}

void GMLDocTrafic::AddSimuEntree(const std::string & sEntree, int nVeh)
{
}

void GMLDocTrafic::AddSimuParking(const std::string & sParking, int nStock, int nVehAttente)
{
}

void GMLDocTrafic::AddSimuTronconStationnement(const std::string & sTroncon, double dbLongueur)
{
}

void GMLDocTrafic::UpdateInstEntreeVehicule(int nID, double dbInstEntree)
{
}

void GMLDocTrafic::UpdateInstSortieVehicule(int nID, double dbInstSortie, const std::string & sSortie, const std::string & sPlaqueSortie, double dstParcourue, const std::vector<Tuyau*> & itinerary, const std::map<std::string, std::string> & additionalAttributes)
{
}

void GMLDocTrafic::RemoveVehicules(int nFromID)
{
}
	
void GMLDocTrafic::AddSimFeux(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire)
{
    m_XmlWriterTrafficLights->writeStartElement(XS("tls:TrafficLightStatus"));
    m_XmlWriterTrafficLights->writeStartElement(XS("tls:TrafficLight"));
    m_XmlWriterTrafficLights->writeAttributeString(XS("xlink:href"), XS((SystemUtil::GetFileName(m_strFilenameNetwork) + "#" + BuildTrafficLightName(sCtrlFeux,sTE,sTS)).c_str()));
    m_XmlWriterTrafficLights->writeEndElement(); // TrafficLight
    m_XmlWriterTrafficLights->writeStartElement(XS("tls:status"));
    m_XmlWriterTrafficLights->writeText(XS(SystemUtil::ToString(bEtatFeu).c_str()));
    m_XmlWriterTrafficLights->writeEndElement(); // status
    m_XmlWriterTrafficLights->writeEndElement(); // TrafficLightStatus
}

void GMLDocTrafic::AddSimFeuxEVE(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire)
{
}

void GMLDocTrafic::AddCellSimu(int nID, double dbConc, double dbDebit, double dbVitAm, double dbAccAm, double bNAm, double dbVitAv, double dbAccAv, double dbNAv, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav, double dbYav, double dbZav)
{
}
    
void GMLDocTrafic::AddTrajectoire(int nID, Tuyau * pTuyau, const std::string & strTuyau, const std::string & strTuyauEx, const std::string & strNextTuyauEx, int nNumVoie, double dbAbs, double dbOrd, double dbZ, double dbAbsCur, double dbVit, double dbAcc, double dbDeltaN, const std::string & sTypeVehicule, double dbVitMax, double dbLongueur, const std::string & sLib, int nIDVehLeader, int nCurrentLoad,
        bool bTypeChgtVoie, TypeChgtVoie eTypeChgtVoie, bool bVoieCible, int nVoieCible, bool bPi, double dbPi, bool bPhi, double dbPhi, bool bRand, double dbRand, bool bDriven, const std::string & strDriveState, bool bDepassement, bool bRegimeFluide, const std::map<std::string, std::string> & additionalAttributes)
{
    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:LinearTrajectory"));
    std::ostringstream woss;
    woss << "V_" << nID << "_" << m_dbCurrentInstant;
    m_XmlWriterTrajsTemp->writeAttributeString(XS("gml:id"), XS(woss.str().c_str()));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("mfIdRef"), XS(("V_" + SystemUtil::ToString(nID)).c_str()));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("start"),XS(SystemUtil::ToString(2, m_dbCurrentInstant-m_dbTimeStep).c_str()));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("end"),XS(SystemUtil::ToString(2, m_dbCurrentInstant).c_str()));

    m_XmlWriterTrajsTemp->writeStartElement(XS("gml:posList"));
    int nbPoints = 1;
    woss.str("");
    woss.precision(2);
	woss.setf(std::ios::fixed);
    // Attention au cas du premier instant pour le véhicule : one ne met que la position à la fin du pas de temps(à améliorer ?)
    std::map<int,Point>::iterator iter = m_mapLastVehiclePositions.find(nID);
    if(iter != m_mapLastVehiclePositions.end())
    {
        nbPoints++;
        woss << iter->second.dbX << " " << iter->second.dbY << " " << iter->second.dbZ << " ";
    }

    m_XmlWriterTrajsTemp->writeAttributeString(XS("count"), XS(SystemUtil::ToString(nbPoints).c_str()));
    m_XmlWriterTrajsTemp->writeAttributeString(XS("srsDimension"), XS("3"));

    woss << dbAbs << " " << dbOrd << " " << dbZ;

    m_XmlWriterTrajsTemp->writeText(XS(woss.str().c_str()));
    
    m_XmlWriterTrajsTemp->writeEndElement(); // gml:posList

    m_XmlWriterTrajsTemp->writeStartElement(XS("mf:Attr"));
    woss.str("");
    woss << CalculAngleTuyau(pTuyau, dbAbsCur) << ',' << dbVit << ',' << dbAcc << ',' << strTuyau << ',' << dbAbsCur << ',' << nNumVoie;
    m_XmlWriterTrajsTemp->writeText(XS(woss.str().c_str()));
    m_XmlWriterTrajsTemp->writeEndElement(); // mf:Attr

    m_XmlWriterTrajsTemp->writeEndElement(); // mf:LinearTrajectory

    m_mapLastVehiclePositions[nID].dbX = dbAbs;
    m_mapLastVehiclePositions[nID].dbY = dbOrd;
    m_mapLastVehiclePositions[nID].dbZ = dbZ;
}

void GMLDocTrafic::AddStream(int nID, const std::string & strTuyau, const std::string & strTuyauEx)
{
}

void GMLDocTrafic::AddLink(const std::string &  strTuyau, double dbConcentration)
{

}

std::string GMLDocTrafic::BuildSurfaceString(const std::deque<Point> & line1, const std::deque<Point> & line2, int & nbPoints)
{
    std::string result;
    nbPoints = 0;

    std::ostringstream oss;
    oss.precision(2);
    oss.setf(std::ios::fixed);

    for(size_t i = 0; i < line1.size(); i++)
    {
        if(nbPoints != 0)
        {
            oss << " ";
        }
        oss << line1[i].dbX << " " << line1[i].dbY << " " << line1[i].dbZ;
        nbPoints++;
    }

    for(int i = (int)line2.size()-1; i >= 0; i--)
    {
        if(nbPoints != 0)
        {
            oss << " ";
        }
        oss << line2[i].dbX << " " << line2[i].dbY << " " << line2[i].dbZ;
        nbPoints++;
    }

    result = oss.str();
    return result;
}

std::string GMLDocTrafic::BuildTrafficLightName(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS)
{
    return sCtrlFeux + "_" + sTE + "to" + sTS;
}

void GMLDocTrafic::writeCommonObservationNodes(const std::string & sIdCpt)
{
        m_XmlWriterSensors->writeStartElement(XS("om:phenomenonTime"));
        m_XmlWriterSensors->writeStartElement(XS("gml:TimePeriod"));
        m_XmlWriterSensors->writeAttributeString(XS("gml:id"), XS((sIdCpt + "_tp_" + SystemUtil::ToString(2, m_dbSensorPeriodEnd)).c_str()));
        m_XmlWriterSensors->writeStartElement(XS("gml:beginPosition"));
        STimeSpan startPeriodOffset(0, 0, 0, (int)m_dbSensorPeriodStart);
        startPeriodOffset.RegulariseSomme(startPeriodOffset);
        STime periodStartTime = m_dtDeb.ToTime() + startPeriodOffset;
        SDateTime periodStart = m_dtDeb;
        periodStart.m_hour = periodStartTime.m_hour;
        periodStart.m_minute = periodStartTime.m_minute;
        periodStart.m_second = periodStartTime.m_second;
        m_XmlWriterSensors->writeText(XS(periodStart.ToISOString().c_str()));
        m_XmlWriterSensors->writeEndElement(); // gml:beginPosition
        m_XmlWriterSensors->writeStartElement(XS("gml:endPosition"));
        STimeSpan endPeriodOffset(0, 0, 0, (int)m_dbSensorPeriodEnd);
        endPeriodOffset.RegulariseSomme(endPeriodOffset);
        STime periodEndTime = m_dtDeb.ToTime() + endPeriodOffset;
        SDateTime periodEnd = m_dtDeb;
        periodEnd.m_hour = periodEndTime.m_hour;
        periodEnd.m_minute = periodEndTime.m_minute;
        periodEnd.m_second = periodEndTime.m_second;
        m_XmlWriterSensors->writeText(XS(periodEnd.ToISOString().c_str()));
        m_XmlWriterSensors->writeEndElement(); // gml:endPosition
        m_XmlWriterSensors->writeEndElement(); // gml:TimePeriod
        m_XmlWriterSensors->writeEndElement(); // om:phenomenonTime

        m_XmlWriterSensors->writeStartElement(XS("om:resultTime"));
        m_XmlWriterSensors->writeStartElement(XS("gml:TimeInstant"));
        m_XmlWriterSensors->writeAttributeString(XS("gml:id"), XS((sIdCpt + "_ti_" + SystemUtil::ToString(2, m_dbSensorPeriodEnd)).c_str()));
        m_XmlWriterSensors->writeStartElement(XS("gml:timePosition"));
        m_XmlWriterSensors->writeText(XS(periodEnd.ToISOString().c_str()));
        m_XmlWriterSensors->writeEndElement(); // gml:timePosition
        m_XmlWriterSensors->writeEndElement(); // gml:TimeInstant
        m_XmlWriterSensors->writeEndElement(); // om:resultTime

        m_XmlWriterSensors->writeStartElement(XS("om:procedure"));
        m_XmlWriterSensors->writeAttributeString(XS("xlink:href"), XS(("#" + sIdCpt).c_str()));
        m_XmlWriterSensors->writeEndElement(); // om:procedure

        m_XmlWriterSensors->writeStartElement(XS("om:observedProperty"));
        m_XmlWriterSensors->writeAttributeString(XS("xsi:nil"), XS("true"));
        m_XmlWriterSensors->writeEndElement(); // om:observedProperty

        m_XmlWriterSensors->writeStartElement(XS("om:featureOfInterest"));
        m_XmlWriterSensors->writeAttributeString(XS("xsi:nil"), XS("true"));
        m_XmlWriterSensors->writeEndElement(); // om:featureOfInterest
}

void GMLDocTrafic::writeSensorOutput(const XMLCh* sLabel, const XMLCh* sUnit)
{
    m_XmlWriterSensors->writeStartElement(XS("sml:output"));
    m_XmlWriterSensors->writeAttributeString(XS("name"), sLabel);
    m_XmlWriterSensors->writeStartElement(XS("swe:Quantity"));
    m_XmlWriterSensors->writeStartElement(XS("swe:label"));
    m_XmlWriterSensors->writeText(sLabel);
    m_XmlWriterSensors->writeEndElement(); // swe:label
    m_XmlWriterSensors->writeStartElement(XS("swe:uom"));
    if(sUnit[0] != chNull)
    {
        m_XmlWriterSensors->writeAttributeString(XS("code"), sUnit);
    }
    m_XmlWriterSensors->writeEndElement(); // swe:uom
    m_XmlWriterSensors->writeEndElement(); // swe:Quantity
    m_XmlWriterSensors->writeEndElement(); // sml:output
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void GMLDocTrafic::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void GMLDocTrafic::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void GMLDocTrafic::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(DocTrafic);

    // membres "classiques"
    ar & BOOST_SERIALIZATION_NVP(m_strFilenameNetwork);
    ar & BOOST_SERIALIZATION_NVP(m_strFilenameTrafficLights);
    ar & BOOST_SERIALIZATION_NVP(m_strFilenameTrajsHeader);
    ar & BOOST_SERIALIZATION_NVP(m_strFilenameTrajsTemp);
    ar & BOOST_SERIALIZATION_NVP(m_strFilenameSensors);
    ar & BOOST_SERIALIZATION_NVP(m_dtDeb);
    ar & BOOST_SERIALIZATION_NVP(m_dbCurrentInstant);
    ar & BOOST_SERIALIZATION_NVP(m_dbTimeStep);
    ar & BOOST_SERIALIZATION_NVP(m_dbSensorPeriodStart);
    ar & BOOST_SERIALIZATION_NVP(m_dbSensorPeriodEnd);
    ar & BOOST_SERIALIZATION_NVP(m_mapLastVehiclePositions);

    std::string trafficLightsStr;
    std::string trajHeaderStr;
    std::string trajTempStr;
    std::string sensorsStr;
    if(Archive::is_saving::value)
    {
        bool bHasTL = m_XmlWriterTrafficLights != NULL;
        bool bHasSensors = m_XmlWriterSensors != NULL;
        // fichier résultat : on ferme le fichier actuel et on récupère son contenu pour le sauvegarder dans 
        // le fichier de sérialisation
        Suspend();
        if(bHasTL)
        {
            std::ifstream ifs(m_strFilenameTrafficLights.c_str(), std::ios::in | std::ios::binary); // lecture en binaire sinon les CR LF deviennent LF
            std::stringstream ss;
            ss << ifs.rdbuf();
            ifs.close();
            trafficLightsStr = ss.str();
        }

        std::ifstream ifs2(m_strFilenameTrajsHeader.c_str(), std::ios::in | std::ios::binary); // lecture en binaire sinon les CR LF deviennent LF
        std::stringstream ss2;
        ss2 << ifs2.rdbuf();
        ifs2.close();
        trajHeaderStr = ss2.str();

        std::ifstream ifs3(m_strFilenameTrajsTemp.c_str(), std::ios::in | std::ios::binary); // lecture en binaire sinon les CR LF deviennent LF
        std::stringstream ss3;
        ss3 << ifs3.rdbuf();
        ifs3.close();
        trajTempStr = ss3.str();

        if(bHasSensors)
        {
            std::ifstream ifs4(m_strFilenameSensors.c_str(), std::ios::in | std::ios::binary); // lecture en binaire sinon les CR LF deviennent LF
            std::stringstream ss4;
            ss4 << ifs4.rdbuf();
            ifs4.close();
            sensorsStr = ss4.str();
        }
    }
    ar & BOOST_SERIALIZATION_NVP(trafficLightsStr);
    ar & BOOST_SERIALIZATION_NVP(trajHeaderStr);
    ar & BOOST_SERIALIZATION_NVP(trajTempStr);
    ar & BOOST_SERIALIZATION_NVP(sensorsStr);

    // on réinitialise les autres membres déductibles des autres et on rouvre le fichier de résultat
    if(Archive::is_loading::value)
    {
        ReInit(trafficLightsStr, trajHeaderStr, trajTempStr, sensorsStr);
    }
}
