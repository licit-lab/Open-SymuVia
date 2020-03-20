#include "stdafx.h"
#include "SerializeUtil.h"

#include "Symubruit.h"

#include "reseau.h"
#include "entree.h"
#include "ControleurDeFeux.h"
#include "CarrefourAFeuxEx.h"
#include "repartiteur.h"
#include "divergent.h"
#include "vehicule.h"
#include "tuyau.h"
#include "TuyauMeso.h"
#include "Segment.h"
#include "convergent.h"
#include "voie.h"
#include "frontiere.h"
#include "XMLDocSirane.h"
#include "XMLDocTrafic.h"
#include "GMLDocTrafic.h"
#include "TraceDocTrafic.h"
#include "XMLDocAcoustique.h"
#include "Parking.h"
#include "ZoneDeTerminaison.h"
#include "TronconOrigine.h"
#include "regulation/RegulationModule.h"
#include "regulation/RegulationBrique.h"
#include "regulation/RegulationCapteur.h"
#include "regulation/RegulationCondition.h"
#include "regulation/RegulationAction.h"
#include "regulation/RegulationRestitution.h"
#include "sensors/SensorsManager.h"
#include "sortie.h"
#include "arret.h"
#include "DiagFonda.h"
#include "Xerces/XMLUtil.h"
#include "regulation/PythonUtils.h"
#include "carFollowing/NewellCarFollowing.h"
#include "carFollowing/NewellContext.h"
#include "carFollowing/ScriptedCarFollowing.h"
#include "carFollowing/GippsCarFollowing.h"
#include "carFollowing/IDMCarFollowing.h"
#include "sensors/EdieSensor.h"
#include "sensors/MFDSensor.h"
#include "sensors/GlobalSensor.h"
#include "sensors/PonctualSensor.h"
#include "sensors/LongitudinalSensor.h"
#include "sensors/BlueToothSensor.h"
#include "sensors/TankSensor.h"
#include "usage/PublicTransportFleetParameters.h"
#include "usage/SymuViaFleetParameters.h"
#include "usage/PublicTransportFleet.h"
#include "usage/logistic/DeliveryFleet.h"
#include "usage/SymuViaFleet.h"
#include "usage/Schedule.h"
#include "usage/PublicTransportScheduleParameters.h"
#include "usage/logistic/PointDeLivraison.h"
#include "usage/logistic/DeliveryFleetParameters.h"
#include "usage/SymuViaVehicleToCreate.h"
#include "usage/PublicTransportLine.h"

#ifdef USE_SYMUCOM
#include "ITS/Stations/ITSStation.h"
#include "ITS/Stations/DerivedClass/Simulator.h"
#include "ITS/Stations/ITSScriptableStation.h"
#include "ITS/Stations/DerivedClass/ITSVehicle.h"
#include "ITS/Stations/DerivedClass/ITSCAF.h"
#include "ITS/Stations/ITSExternalStation.h"
#include "ITS/Applications/C_ITSExternalApplication.h"
#include "ITS/Applications/C_ITSScriptableApplication.h"
#include "ITS/Applications/DerivedClass/GLOSA.h"
#include "ITS/Sensors/ITSExternalDynamicSensor.h"
#include "ITS/Sensors/ITSExternalStaticSensor.h"
#include "ITS/Sensors/ITSScriptableStaticSensor.h"
#include "ITS/Sensors/ITSScriptableDynamicSensor.h"
#include "ITS/Sensors/DerivedClass/BubbleSensor.h"
#include "ITS/Sensors/DerivedClass/GPSSensor.h"
#include "ITS/Sensors/DerivedClass/InertialSensor.h"
#include "ITS/Sensors/DerivedClass/MagicSensor.h"
#include "ITS/Sensors/DerivedClass/TelemetrySensor.h"
#include "ITS/Sensors/DerivedClass/UBRSensor.h"
#endif // USE_SYMUCOM

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMElement.hpp>

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>
#include <boost/serialization/deque.hpp>

#include <boost/python/import.hpp>
#include <boost/python/extract.hpp>

using namespace std;

XERCES_CPP_NAMESPACE_USE


SerializeUtil::SerializeUtil(void)
{
}

SerializeUtil::~SerializeUtil(void)
{
}

void SerializeUtil::Save(Reseau * pNetwork, char * sFilePath)
{
    saveIntoFile<boost::archive::xml_woarchive, Reseau>("Reseau", *pNetwork, sFilePath);
}

void SerializeUtil::Load(Reseau * pNetwork, char * sFilePath)
{
    getFromFile<boost::archive::xml_wiarchive, Reseau>("Reseau", *pNetwork, sFilePath);
}

// Inscription des types et casts
template <class Archive>
void registerClasses(Archive & ar)
{
    // marche pas avec ca car Tuyau est pure virtual 
  /*  ar.register_type(static_cast<Tuyau *>(NULL));
    boost::serialization::void_cast_register(static_cast<Tuyau *>(NULL),static_cast<Troncon *>(NULL));*/

    ar.register_type(static_cast<TuyauMacro *>(NULL));
    boost::serialization::void_cast_register(static_cast<TuyauMacro *>(NULL),static_cast<Tuyau *>(NULL));

    ar.register_type(static_cast<TuyauMicro *>(NULL));
    boost::serialization::void_cast_register(static_cast<TuyauMicro *>(NULL),static_cast<Tuyau *>(NULL));

    ar.register_type(static_cast<CTuyauMeso*>(NULL));
    boost::serialization::void_cast_register(static_cast<CTuyauMeso *>(NULL),static_cast<TuyauMicro *>(NULL));

    ar.register_type(static_cast<SymuViaTripNode *>(NULL));
    boost::serialization::void_cast_register(static_cast<SymuViaTripNode *>(NULL),static_cast<TripNode *>(NULL));

    ar.register_type(static_cast<PointDeLivraison *>(NULL));
    boost::serialization::void_cast_register(static_cast<PointDeLivraison *>(NULL),static_cast<TripNode *>(NULL));

    ar.register_type(static_cast<ArretSnapshot *>(NULL));
    boost::serialization::void_cast_register(static_cast<ArretSnapshot *>(NULL),static_cast<TripNodeSnapshot *>(NULL));

    ar.register_type(static_cast<PublicTransportLineSnapshot *>(NULL));
    boost::serialization::void_cast_register(static_cast<PublicTransportLineSnapshot *>(NULL), static_cast<TripSnapshot *>(NULL));

    ar.register_type(static_cast<PublicTransportLine*>(NULL));
    boost::serialization::void_cast_register(static_cast<PublicTransportLine *>(NULL), static_cast<Trip*>(NULL));

    ar.register_type(static_cast<ParkingSnapshot *>(NULL));
    boost::serialization::void_cast_register(static_cast<ParkingSnapshot *>(NULL),static_cast<TripNodeSnapshot *>(NULL));

    ar.register_type(static_cast<PublicTransportFleet *>(NULL));
    boost::serialization::void_cast_register(static_cast<PublicTransportFleet *>(NULL),static_cast<AbstractFleet*>(NULL));

    ar.register_type(static_cast<DeliveryFleet *>(NULL));
    boost::serialization::void_cast_register(static_cast<DeliveryFleet *>(NULL),static_cast<AbstractFleet*>(NULL));

    ar.register_type(static_cast<SymuViaFleet *>(NULL));
    boost::serialization::void_cast_register(static_cast<SymuViaFleet *>(NULL),static_cast<AbstractFleet*>(NULL));

    ar.register_type(static_cast<PublicTransportFleetParameters *>(NULL));
    boost::serialization::void_cast_register(static_cast<PublicTransportFleetParameters *>(NULL),static_cast<AbstractFleetParameters*>(NULL));

    ar.register_type(static_cast<DeliveryFleetParameters *>(NULL));
    boost::serialization::void_cast_register(static_cast<DeliveryFleetParameters *>(NULL),static_cast<AbstractFleetParameters*>(NULL));

    ar.register_type(static_cast<SymuViaFleetParameters *>(NULL));
    boost::serialization::void_cast_register(static_cast<SymuViaFleetParameters *>(NULL),static_cast<AbstractFleetParameters*>(NULL));

    ar.register_type(static_cast<SymuViaVehicleToCreate*>(NULL));
    boost::serialization::void_cast_register(static_cast<SymuViaVehicleToCreate *>(NULL),static_cast<VehicleToCreate*>(NULL));

    ar.register_type(static_cast<PonctualSensor *>(NULL));
    boost::serialization::void_cast_register(static_cast<PonctualSensor *>(NULL),static_cast<AbstractSensor *>(NULL));

    ar.register_type(static_cast<PonctualSensorSnapshot *>(NULL));
    boost::serialization::void_cast_register(static_cast<PonctualSensorSnapshot *>(NULL),static_cast<AbstractSensorSnapshot *>(NULL));
    ar.register_type(static_cast<BlueToothSensorSnapshot *>(NULL));
    boost::serialization::void_cast_register(static_cast<BlueToothSensorSnapshot *>(NULL),static_cast<AbstractSensorSnapshot *>(NULL));
    ar.register_type(static_cast<LongitudinalSensorData *>(NULL));
    boost::serialization::void_cast_register(static_cast<LongitudinalSensorData *>(NULL),static_cast<AbstractSensorSnapshot *>(NULL));
    ar.register_type(static_cast<EdieSensorSnapshot *>(NULL));
	boost::serialization::void_cast_register(static_cast<EdieSensorSnapshot *>(NULL), static_cast<AbstractSensorSnapshot *>(NULL));
    ar.register_type(static_cast<MFDSensorSnapshot *>(NULL));
    boost::serialization::void_cast_register(static_cast<MFDSensorSnapshot *>(NULL),static_cast<AbstractSensorSnapshot *>(NULL));
    ar.register_type(static_cast<TankSensorSnapshot *>(NULL));
    boost::serialization::void_cast_register(static_cast<TankSensorSnapshot *>(NULL), static_cast<AbstractSensorSnapshot *>(NULL));
    ar.register_type(static_cast<GlobalSensorSnapshot *>(NULL));
    boost::serialization::void_cast_register(static_cast<GlobalSensorSnapshot *>(NULL),static_cast<AbstractSensorSnapshot *>(NULL));

    ar.register_type(static_cast<LongitudinalSensor *>(NULL));
    boost::serialization::void_cast_register(static_cast<LongitudinalSensor *>(NULL),static_cast<AbstractSensor *>(NULL));

    ar.register_type(static_cast<BlueToothSensor *>(NULL));
    boost::serialization::void_cast_register(static_cast<BlueToothSensor *>(NULL),static_cast<AbstractSensor *>(NULL));

    ar.register_type(static_cast<EdieSensor *>(NULL));
    boost::serialization::void_cast_register(static_cast<EdieSensor *>(NULL),static_cast<LongitudinalSensor *>(NULL));

    ar.register_type(static_cast<MFDSensor *>(NULL));
    boost::serialization::void_cast_register(static_cast<MFDSensor *>(NULL),static_cast<AbstractSensor *>(NULL));

    ar.register_type(static_cast<TankSensor *>(NULL));
    boost::serialization::void_cast_register(static_cast<TankSensor *>(NULL), static_cast<AbstractSensor *>(NULL));

    ar.register_type(static_cast<GlobalSensor *>(NULL));
    boost::serialization::void_cast_register(static_cast<GlobalSensor *>(NULL),static_cast<AbstractSensor *>(NULL));

    ar.register_type(static_cast<ConnectionPonctuel *>(NULL));
    boost::serialization::void_cast_register(static_cast<ConnectionPonctuel *>(NULL),static_cast<Connexion *>(NULL));

    ar.register_type(static_cast<Repartiteur *>(NULL));
    boost::serialization::void_cast_register(static_cast<Repartiteur *>(NULL),static_cast<ConnectionPonctuel *>(NULL));

    ar.register_type(static_cast<Entree *>(NULL));
    boost::serialization::void_cast_register(static_cast<Entree *>(NULL),static_cast<SymuViaTripNode *>(NULL));
    boost::serialization::void_cast_register(static_cast<Entree *>(NULL),static_cast<ConnectionPonctuel *>(NULL));

    ar.register_type(static_cast<Parking*>(NULL));
    boost::serialization::void_cast_register(static_cast<Parking*>(NULL),static_cast<SymuViaTripNode *>(NULL));

    ar.register_type(static_cast<Arret *>(NULL));
    boost::serialization::void_cast_register(static_cast<Arret *>(NULL),static_cast<TripNode *>(NULL));

    ar.register_type(static_cast<Convergent *>(NULL));
    boost::serialization::void_cast_register(static_cast<Convergent *>(NULL),static_cast<ConnectionPonctuel *>(NULL));

    ar.register_type(static_cast<Divergent *>(NULL));
    boost::serialization::void_cast_register(static_cast<Divergent *>(NULL),static_cast<ConnectionPonctuel *>(NULL));

    ar.register_type(static_cast<Sortie *>(NULL));
    boost::serialization::void_cast_register(static_cast<Sortie *>(NULL),static_cast<SymuViaTripNode *>(NULL));
    boost::serialization::void_cast_register(static_cast<Sortie *>(NULL),static_cast<ConnectionPonctuel *>(NULL));

    ar.register_type(static_cast<ZoneDeTerminaison *>(NULL));
    boost::serialization::void_cast_register(static_cast<ZoneDeTerminaison *>(NULL),static_cast<SymuViaTripNode *>(NULL));

    ar.register_type(static_cast<TronconOrigine *>(NULL));
    boost::serialization::void_cast_register(static_cast<TronconOrigine *>(NULL),static_cast<SymuViaTripNode *>(NULL));

    ar.register_type(static_cast<Giratoire *>(NULL));
    boost::serialization::void_cast_register(static_cast<Giratoire *>(NULL),static_cast<BriqueDeConnexion *>(NULL));

    ar.register_type(static_cast<CarrefourAFeuxEx *>(NULL));
    boost::serialization::void_cast_register(static_cast<CarrefourAFeuxEx *>(NULL),static_cast<BriqueDeConnexion *>(NULL));

    ar.register_type(static_cast<Connexion*>(NULL));
    boost::serialization::void_cast_register(static_cast<Connexion*>(NULL),static_cast<CMesoNode *>(NULL));

    ar.register_type(static_cast<SegmentMacro *>(NULL));
    boost::serialization::void_cast_register(static_cast<SegmentMacro *>(NULL),static_cast<Segment *>(NULL));

    ar.register_type(static_cast<VoieMacro *>(NULL));
    boost::serialization::void_cast_register(static_cast<VoieMacro *>(NULL),static_cast<Voie *>(NULL));

    ar.register_type(static_cast<VoieMicro *>(NULL));
    boost::serialization::void_cast_register(static_cast<VoieMicro *>(NULL),static_cast<Voie *>(NULL));

    ar.register_type(static_cast<XMLDocTrafic *>(NULL));
    boost::serialization::void_cast_register(static_cast<XMLDocTrafic *>(NULL),static_cast<DocTrafic *>(NULL));

    ar.register_type(static_cast<GMLDocTrafic *>(NULL));
    boost::serialization::void_cast_register(static_cast<GMLDocTrafic *>(NULL),static_cast<DocTrafic *>(NULL));

    ar.register_type(static_cast<TraceDocTrafic *>(NULL));
    boost::serialization::void_cast_register(static_cast<TraceDocTrafic *>(NULL),static_cast<DocTrafic *>(NULL));

    ar.register_type(static_cast<XMLDocAcoustique *>(NULL));
    boost::serialization::void_cast_register(static_cast<XMLDocAcoustique *>(NULL),static_cast<DocAcoustique *>(NULL));

    ar.register_type(static_cast<RegulationCapteur *>(NULL));
    boost::serialization::void_cast_register(static_cast<RegulationCapteur *>(NULL),static_cast<RegulationElement *>(NULL));

    ar.register_type(static_cast<RegulationCondition *>(NULL));
    boost::serialization::void_cast_register(static_cast<RegulationCondition*>(NULL),static_cast<RegulationElement *>(NULL));

    ar.register_type(static_cast<RegulationAction*>(NULL));
    boost::serialization::void_cast_register(static_cast<RegulationAction*>(NULL),static_cast<RegulationElement *>(NULL));

    ar.register_type(static_cast<RegulationRestitution*>(NULL));
    boost::serialization::void_cast_register(static_cast<RegulationRestitution*>(NULL),static_cast<RegulationAction *>(NULL));

    ar.register_type(static_cast<NewellCarFollowing*>(NULL));
    boost::serialization::void_cast_register(static_cast<NewellCarFollowing*>(NULL),static_cast<AbstractCarFollowing *>(NULL));

    ar.register_type(static_cast<NewellContext*>(NULL));
    boost::serialization::void_cast_register(static_cast<NewellContext*>(NULL),static_cast<CarFollowingContext *>(NULL));

    ar.register_type(static_cast<ScriptedCarFollowing*>(NULL));
    boost::serialization::void_cast_register(static_cast<ScriptedCarFollowing*>(NULL),static_cast<AbstractCarFollowing *>(NULL));

    ar.register_type(static_cast<GippsCarFollowing*>(NULL));
    boost::serialization::void_cast_register(static_cast<GippsCarFollowing*>(NULL),static_cast<AbstractCarFollowing *>(NULL));

    ar.register_type(static_cast<IDMCarFollowing*>(NULL));
    boost::serialization::void_cast_register(static_cast<IDMCarFollowing*>(NULL),static_cast<AbstractCarFollowing *>(NULL));

    ar.register_type(static_cast<ScheduleInstant*>(NULL));
    boost::serialization::void_cast_register(static_cast<ScheduleInstant*>(NULL),static_cast<ScheduleElement *>(NULL));

    ar.register_type(static_cast<ScheduleFrequency*>(NULL));
    boost::serialization::void_cast_register(static_cast<ScheduleFrequency*>(NULL),static_cast<ScheduleElement *>(NULL));

    ar.register_type(static_cast<PublicTransportScheduleParameters*>(NULL));
    boost::serialization::void_cast_register(static_cast<PublicTransportScheduleParameters*>(NULL),static_cast<ScheduleParameters *>(NULL));

    // Classes ITS
#ifdef USE_SYMUCOM
    ar.register_type(static_cast<Simulator*>(NULL));
    boost::serialization::void_cast_register(static_cast<Simulator*>(NULL), static_cast<ITSStation *>(NULL));

    ar.register_type(static_cast<ITSCAF*>(NULL));
    boost::serialization::void_cast_register(static_cast<ITSCAF*>(NULL), static_cast<ITSStation *>(NULL));

    ar.register_type(static_cast<ITSVehicle*>(NULL));
    boost::serialization::void_cast_register(static_cast<ITSVehicle*>(NULL), static_cast<ITSStation *>(NULL));

    ar.register_type(static_cast<ITSScriptableStation*>(NULL));
    boost::serialization::void_cast_register(static_cast<ITSScriptableStation*>(NULL), static_cast<ITSStation *>(NULL));
    boost::serialization::void_cast_register(static_cast<ITSScriptableStation*>(NULL), static_cast<ITSScriptableElement *>(NULL));

    ar.register_type(static_cast<ITSExternalStation*>(NULL));
    boost::serialization::void_cast_register(static_cast<ITSExternalStation*>(NULL), static_cast<ITSStation *>(NULL));

    ar.register_type(static_cast<C_ITSExternalApplication *>(NULL));
    boost::serialization::void_cast_register(static_cast<C_ITSExternalApplication *>(NULL), static_cast<C_ITSApplication  *>(NULL));

    ar.register_type(static_cast<GLOSA *>(NULL));
    boost::serialization::void_cast_register(static_cast<GLOSA *>(NULL), static_cast<C_ITSApplication  *>(NULL));

    ar.register_type(static_cast<C_ITSScriptableApplication*>(NULL));
    boost::serialization::void_cast_register(static_cast<C_ITSScriptableApplication*>(NULL), static_cast<C_ITSApplication *>(NULL));
    boost::serialization::void_cast_register(static_cast<C_ITSScriptableApplication*>(NULL), static_cast<ITSScriptableElement *>(NULL));

    ar.register_type(static_cast<ITSScriptableDynamicSensor  *>(NULL));
    boost::serialization::void_cast_register(static_cast<ITSScriptableDynamicSensor*>(NULL), static_cast<ITSSensor  *>(NULL));
    boost::serialization::void_cast_register(static_cast<ITSScriptableDynamicSensor*>(NULL), static_cast<ITSScriptableElement *>(NULL));

    ar.register_type(static_cast<ITSScriptableStaticSensor  *>(NULL));
    boost::serialization::void_cast_register(static_cast<ITSScriptableStaticSensor*>(NULL), static_cast<ITSSensor *>(NULL));
    boost::serialization::void_cast_register(static_cast<ITSScriptableStaticSensor*>(NULL), static_cast<ITSScriptableElement *>(NULL));

    ar.register_type(static_cast<ITSExternalDynamicSensor  *>(NULL));
    boost::serialization::void_cast_register(static_cast<ITSExternalDynamicSensor*>(NULL), static_cast<ITSDynamicSensor *>(NULL));

    ar.register_type(static_cast<ITSExternalStaticSensor  *>(NULL));
    boost::serialization::void_cast_register(static_cast<ITSExternalStaticSensor*>(NULL), static_cast<ITSStaticSensor *>(NULL));

    ar.register_type(static_cast<BubbleSensor*>(NULL));
    boost::serialization::void_cast_register(static_cast<BubbleSensor *>(NULL), static_cast<ITSDynamicSensor *>(NULL));

    ar.register_type(static_cast<GPSSensor*>(NULL));
    boost::serialization::void_cast_register(static_cast<GPSSensor*>(NULL), static_cast<ITSDynamicSensor *>(NULL));

    ar.register_type(static_cast<InertialSensor*>(NULL));
    boost::serialization::void_cast_register(static_cast<InertialSensor*>(NULL), static_cast<ITSDynamicSensor *>(NULL));

    ar.register_type(static_cast<MagicSensor *>(NULL));
    boost::serialization::void_cast_register(static_cast<MagicSensor *>(NULL), static_cast<ITSDynamicSensor *>(NULL));

    ar.register_type(static_cast<TelemetrySensor*>(NULL));
    boost::serialization::void_cast_register(static_cast<TelemetrySensor *>(NULL), static_cast<ITSDynamicSensor *>(NULL));

    ar.register_type(static_cast<UBRSensor *>(NULL));
    boost::serialization::void_cast_register(static_cast<UBRSensor  *>(NULL), static_cast<ITSStaticSensor *>(NULL));
#endif // USE_SYMUCOM
}

// enregistrer une collection dans un fichier
template <class Archive, class Objet>
void saveIntoFile(const char * nom, const Objet& d, const char* file)
{
	wofstream ofile(file);
	Archive ar(ofile);

    registerClasses(ar);

	ar << BOOST_SERIALIZATION_NVP2(nom, d);
}

// charger une collection depuis un fichier
template <class Archive, class Objet>
void getFromFile(const char * nom, Objet& d, const char* file)
{
	wifstream ifile(file);
	Archive ar(ifile);

    registerClasses(ar);

	ar >> BOOST_SERIALIZATION_NVP2(nom, d);
}

template void SerialiseTab2D(boost::archive::xml_woarchive & ar, const char * nom, int ** &tab, int &nbItemi, int &nbItemj);
template void SerialiseTab2D(boost::archive::xml_woarchive & ar, const char * nom, double ** &tab, int &nbItemi, int &nbItemj);
template void SerialiseTab2D(boost::archive::xml_wiarchive & ar, const char * nom, int ** &tab, int &nbItemi, int &nbItemj);
template void SerialiseTab2D(boost::archive::xml_wiarchive & ar, const char * nom, double ** &tab, int &nbItemi, int &nbItemj);

// serialize un tableau 2D
template <class Archive, class Objet>
void SerialiseTab2D(Archive & ar, const char * nom, Objet ** &tab, int &nbItemi, int &nbItemj)
{
    bool bIsLoading = Archive::is_loading::value;
    if(bIsLoading)
    {
        if (tab != NULL)
	    {
            for(int i = 0; i < nbItemi; i++)
            {
		        delete tab[i];
            }
            delete tab;
	    }
    }

	char buf[512];
	snprintf(buf, sizeof(buf), "%s_counti", nom);
	ar & BOOST_SERIALIZATION_NVP2(buf, nbItemi);
    snprintf(buf, sizeof(buf), "%s_countj", nom);
	ar & BOOST_SERIALIZATION_NVP2(buf, nbItemj);
    if(bIsLoading)
    {
        tab = new Objet*[nbItemi];
    }
	for (int i=0; i<nbItemi; i++)
	{
        if(bIsLoading)
        {
            tab[i] = new Objet[nbItemj];
        }
        for (int j=0; j<nbItemj; j++)
	    {
		    snprintf(buf, sizeof(buf), "%s_%d_%d", nom, i, j);
		    ar & BOOST_SERIALIZATION_NVP2(buf, tab[i][j]);
        }
	}
}

template void SerialiseDeque2D(boost::archive::xml_woarchive & ar, const char * nom, std::deque<double*> &pDeque, size_t nbItemj);
template void SerialiseDeque2D(boost::archive::xml_woarchive & ar, const char * nom, std::deque<int*> &pDeque, size_t nbItemj);
template void SerialiseDeque2D(boost::archive::xml_wiarchive & ar, const char * nom, std::deque<double*> &pDeque, size_t nbItemj);
template void SerialiseDeque2D(boost::archive::xml_wiarchive & ar, const char * nom, std::deque<int*> &pDeque, size_t nbItemj);

// serialize un deque de pointeurs de type de base
template <class Archive, class Object>
void SerialiseDeque2D(Archive & ar, const char * nom, std::deque<Object*> &pDeque, size_t nbItemj)
{
    bool bIsLoading = Archive::is_loading::value;
	char buf[512];
	snprintf(buf, sizeof(buf), "%s_count", nom);
    if(bIsLoading)
    {
        size_t dequeSize;
        ar & BOOST_SERIALIZATION_NVP2(buf, dequeSize);
        typename std::deque<Object*>::iterator it = pDeque.begin();
        typename std::deque<Object*>::iterator itEnd = pDeque.end();
        while(it != itEnd)
        {
            delete (*it);
            it++;
        }
        pDeque.clear();
        for(size_t i = 0; i < dequeSize; i++)
        {
            pDeque.push_back(new Object[nbItemj]);
        }
    }
    else
    {
        size_t dequeSize = pDeque.size();
        ar & BOOST_SERIALIZATION_NVP2(buf, dequeSize);
    }
	
	for (size_t i=0; i<pDeque.size(); i++)
	{
        for (size_t j=0; j<nbItemj; j++)
	    {
#if defined _MSC_VER && _MSC_VER >= 1900
		    snprintf(buf, sizeof(buf), "%s_%zu_%zu", nom, i, j);
#else
			snprintf(buf, sizeof(buf), "%s_%lu_%lu", nom, i, j);
#endif
		    ar & BOOST_SERIALIZATION_NVP2(buf, pDeque[i][j]);
        }
	}
}

template void SerialiseDOMDocument(boost::archive::xml_woarchive & ar, const char * nom, const XMLCh* rootName, XERCES_CPP_NAMESPACE::DOMDocument* &pDoc, XMLUtil * pXMLUtil);
template void SerialiseDOMDocument(boost::archive::xml_wiarchive & ar, const char * nom, const XMLCh* rootName, XERCES_CPP_NAMESPACE::DOMDocument* &pDoc, XMLUtil * pXMLUtil);

// serialize un pointeur de DOMDocument XERCES
template <class Archive>
void SerialiseDOMDocument(Archive & ar, const char * nom, const XMLCh* rootName, XERCES_CPP_NAMESPACE::DOMDocument* &pDoc, XMLUtil * pXMLUtil)
{
    // instanciation éventuelle du document
    if(!pDoc)
    {
        pDoc = pXMLUtil->CreateXMLDocument(rootName);
    }
    ar & BOOST_SERIALIZATION_NVP2(nom, *pDoc);
}

namespace boost {
    namespace serialization {
        template<class Archive>
        void serialize(Archive & ar, XERCES_CPP_NAMESPACE::DOMDocument &doc, const unsigned int version)
        {
            XMLUtil xmlUtils;

            string docStr;
            if(Archive::is_saving::value)
            {
                // en cas de sauvegarde, on doit au préalable générer la string correspondant au document
                docStr = xmlUtils.getOuterXml(doc.getDocumentElement());
            }
            ar & BOOST_SERIALIZATION_NVP(docStr);
            if(Archive::is_loading::value)
            {
                // en cas de chargement, on doit parser la string lue pour reconstituer le document
                DOMDocument * pLoadedDoc = xmlUtils.LoadDocument(docStr);
                doc.replaceChild(doc.importNode(pLoadedDoc->getDocumentElement(),true), doc.getDocumentElement());
            }
        }
    }
}

namespace boost {
    namespace serialization {
        template void serialize(boost::archive::xml_woarchive & ar, boost::python::dict &obj, const unsigned int version);
        template void serialize(boost::archive::xml_wiarchive & ar, boost::python::dict &obj, const unsigned int version);

        template<class Archive>
        void serialize(Archive & ar, boost::python::dict &obj, const unsigned int version)
        {
            string docStr;
            boost::python::object pickleModule = boost::python::import("pickle");
            if(Archive::is_saving::value)
            {
                boost::python::object dumpsFunction = pickleModule.attr("dumps");
                boost::python::str s = boost::python::extract<boost::python::str>(dumpsFunction(obj));
                docStr = boost::python::extract<std::string>(s);
            }
            ar & BOOST_SERIALIZATION_NVP(docStr);
            if(Archive::is_loading::value)
            {
                boost::python::object loadsFunction = pickleModule.attr("loads");
                obj = boost::python::extract<boost::python::dict>(loadsFunction(docStr));
            }
        }
    }
}