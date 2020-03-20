#include "stdafx.h"
#include "PythonUtils.h"

#include "../reseau.h"
#include "../vehicule.h"
#include "../DiagFonda.h"
#include "../tuyau.h"
#include "../TuyauMeso.h"
#include "../ControleurDeFeux.h"
#include "../Affectation.h"
#include "../voie.h"
#include "../ConnectionPonctuel.h"
#include "../carFollowing/CarFollowingContext.h"
#include "../carFollowing/ScriptedCarFollowing.h"
#include "../SystemUtil.h"
#include "../convergent.h"
#include "../Logger.h"
#include "../sensors/PonctualSensor.h"
#include "../sensors/SensorsManager.h"
#include "../loi_emission.h"
#include "../arret.h"
#include "../usage/Trip.h"
#ifdef USE_SYMUCOM
#include "ITS/Applications/C_ITSApplication.h"
#include "ITS/Applications/C_ITSScriptableApplication.h"
#include "ITS/Applications/DerivedClass/GLOSA.h"
#include "ITS/Stations/ITSStation.h"
#include "ITS/Stations/ITSScriptableStation.h"
#include "ITS/Stations/DerivedClass/ITSVehicle.h"
#include "ITS/Stations/DerivedClass/ITSCAF.h"
#include "ITS/Stations/DerivedClass/Simulator.h"
#include "ITS/Sensors/ITSSensor.h"
#include "ITS/Sensors/ITSScriptableDynamicSensor.h"
#include "ITS/Sensors/ITSScriptableStaticSensor.h"
#include "ITS/Sensors/DerivedClass/GPSSensor.h"
#include "ITS/Sensors/DerivedClass/BubbleSensor.h"
#include "ITS/Sensors/DerivedClass/InertialSensor.h"
#include "ITS/Sensors/DerivedClass/MagicSensor.h"
#include "ITS/Sensors/DerivedClass/TelemetrySensor.h"
#include "ITS/Sensors/DerivedClass/UBRSensor.h"
#endif //USE_SYMUCOM

#include "Xerces/XMLUtil.h"

#pragma warning( disable : 4244 )
#include <boost/python.hpp>
#pragma warning( default : 4244 )

#pragma warning( disable : 4267 )
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#pragma warning( default : 4267 )
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>

using namespace boost::python;

XERCES_CPP_NAMESPACE_USE

struct TronconWrap : Troncon, wrapper<Troncon>
{
    bool InitSimulation(bool bAcou, bool bSirane, std::string strName)
    { return this->get_override("InitSimulation")(bAcou, bSirane, strName); }
    void ComputeTraffic(double nInstant)
    { this->get_override("ComputeTraffic")(nInstant); }
    void ComputeTrafficEx(double nInstant)
    { this->get_override("ComputeTrafficEx")(nInstant); }
    void TrafficOutput()
    { this->get_override("TrafficOutput")(); }
    void EmissionOutput()
    { this->get_override("EmissionOutput")(); }
    void InitAcousticVariables()
    { this->get_override("InitAcousticVariables")(); }

};

struct TuyauWrap : Tuyau, wrapper<Tuyau>
{
    bool InitSimulation(bool bAcou, bool bSirane, std::string strName)
    { return this->get_override("InitSimulation")(bAcou, bSirane, strName); }
    void ComputeTraffic(double nInstant)
    { this->get_override("ComputeTraffic")(nInstant); }
    void TrafficOutput()
    { this->get_override("TrafficOutput")(); }
    void EmissionOutput()
    { this->get_override("EmissionOutput")(); }
    void InitVarSimu()
    { this->get_override("InitVarSimu")(); }
    bool Segmentation()
    { return this->get_override("Segmentation")(); }
    void ComputeNoise(Loi_emission* Loi)
    { this->get_override("ComputeNoise")(Loi); }
    double ComputeCost(double dbInst, TypeVehicule    *pTypeVeh, bool bUsePenalisation)
    { return this->get_override("ComputeCost")(dbInst, pTypeVeh, bUsePenalisation); }
    char GetLanesManagement()
    { return this->get_override("GetLanesManagement")(); }
    void DeleteLanes()
    { this->get_override("DeleteLanes")(); }
    ETuyauType GetType() const { return this->get_override("GetType")();}
    double GetTempsParcourRealise(TypeVehicule *pType) const
    { return this->get_override("GetTempsParcourRealise")(pType); }

};

struct StaticElementWrap : StaticElement, wrapper<StaticElement>
{
    Reseau* GetReseau()
    { return this->get_override("GetReseau")(); }
};

struct VoieWrap : Voie, wrapper<Voie>
{
    bool InitSimulation(bool bAcou, bool bSirane, std::string strName)
    { return this->get_override("InitSimulation")(bAcou, bSirane, strName); }
    void ComputeTraffic(double nInstant)
    { this->get_override("ComputeTraffic")(nInstant); }
    void ComputeTrafficEx(double nInstant)
    { this->get_override("ComputeTrafficEx")(nInstant); }
    void TrafficOutput()
    { this->get_override("TrafficOutput")(); }
     void InitAcousticVariables()
    { this->get_override("InitAcousticVariables")(); }
    void InitVarSimu()
    { this->get_override("InitVarSimu")(); }
    void EndComputeTraffic(double dbInstant)
    { this->get_override("EndComputeTraffic")(dbInstant); }
    void ComputeNoise(Loi_emission* Loi)
    { this->get_override("ComputeNoise")(Loi); }
    void ComputeOffer(double dbInstant)
    { this->get_override("ComputeOffer")(dbInstant); }
    void ComputeDemand(double dbInstSimu, double dbInstant)
    { this->get_override("ComputeDemand")(dbInstSimu, dbInstant); }
    bool Discretisation(int nNbVoie)
    { return this->get_override("Discretisation")(nNbVoie); }
};


#ifdef USE_SYMUCOM
struct StationWrap : ITSStation, wrapper<ITSStation>
{
    bool TreatMessageData(SymuCom::Message *pMessage)
    { return this->get_override("TreatMessageData")(pMessage); }
    bool Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration)
    { return this->get_override("Run")(dtTimeStart, dbTimeDuration); }
    bool LoadFromXML(DOMDocument * pDoc, DOMNode *pXMLNode)
    { return this->get_override("LoadFromXML")(pDoc, pXMLNode); }
    ITSStation* CreateCopy(const std::string& strID)
    { return this->get_override("CreateCopy")(strID); }
};

struct ApplicationWrap : C_ITSApplication, wrapper<C_ITSApplication>
{
    bool RunApplication(SymuCom::Message *pMessage, ITSStation* pStation)
    { return this->get_override("RunApplication")(pMessage, pStation); }
    bool LoadFromXML(DOMDocument * pDoc, DOMNode *pXMLNode, Reseau *pNetwork)
    { return this->get_override("LoadFromXML")(pDoc, pXMLNode, pNetwork); }
};

struct SensorWrap : ITSSensor, wrapper<ITSSensor>
{
    bool UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd)
    { return this->get_override("UpdateData")(dtTimeStart, dtTimeEnd); }
    bool LoadFromXML(DOMDocument * pDoc, DOMNode *pXMLNode)
    { return this->get_override("LoadFromXML")(pDoc, pXMLNode); }
    ITSSensor* CreateCopy(ITSStation *pNewOwner)
    { return this->get_override("CreateCopy")(pNewOwner); }
};

struct StaticSensorWrap : ITSStaticSensor, wrapper<ITSStaticSensor>
{
    bool UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd)
    { return this->get_override("UpdateData")(dtTimeStart, dtTimeEnd); }
    bool LoadFromXML(DOMDocument * pDoc, DOMNode *pXMLNode)
    { return this->get_override("LoadFromXML")(pDoc, pXMLNode); }
    ITSSensor* CreateCopy(ITSStation *pNewOwner)
    { return this->get_override("CreateCopy")(pNewOwner); }
};

struct DynamicSensorWrap : ITSDynamicSensor, wrapper<ITSDynamicSensor>
{
    bool UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd)
    { return this->get_override("UpdateData")(dtTimeStart, dtTimeEnd); }
    bool LoadFromXML(DOMDocument * pDoc, DOMNode *pXMLNode)
    { return this->get_override("LoadFromXML")(pDoc, pXMLNode); }
    ITSSensor* CreateCopy(ITSStation *pNewOwner)
    { return this->get_override("CreateCopy")(pNewOwner); }
};

struct EntityWrap : SymuCom::Entity, wrapper<SymuCom::Entity>
{
    bool ProcessMessage(SymuCom::Message* pMessage)
    { return this->get_override("ProcessMessage")(pMessage); }
};
#endif // USE_SYMUCOM

template <class T>
T* factory() { return new T(); }

template <class T, class P1, class P2>
T* factory(P1 p1, P2 p2) { return new T(p1, p2); }

template <class T, class P1, class P2, class P3, class P4, class P5>
T* factory(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) { return new T(p1, p2, p3, p4, p5); }

BOOST_PYTHON_MODULE(symuvia)
{
    ////////////////////////////////////////
    // Wrapper Python de la classe Reseau
    ////////////////////////////////////////

    class_<Reseau>("Network")
        .def_readwrite("LastVehicleID", &Reseau::m_nLastVehicleID)
        .def_readonly("LstVehicles", &Reseau::m_LstVehicles)
        .def_readonly("InstSimu", &Reseau::m_dbInstSimu)
        .def_readonly("LstLTP", &Reseau::m_LstLTP)
        .def("GetSimuStartTime", &Reseau::GetSimuStartTime, return_internal_reference<>())
        .def("GetTimeStep", &Reseau::GetTimeStep)
        .def("GetLinkFromLabel", &Reseau::GetLinkFromLabel, return_internal_reference<>())
        .def("GetConnectionFromID", &Reseau::GetConnectionFromID, return_internal_reference<>())
		.def("GetOrigineFromID", &Reseau::GetOrigineFromID, return_internal_reference<>())
        .def("GetBrickFromID", &Reseau::GetBrickFromID, return_internal_reference<>())
        .def("GetTrafficLightControllerFromID", &Reseau::GetTrafficLightControllerFromID, return_internal_reference<>())
        .def("GetVehicleTypeFromID", &Reseau::GetVehicleTypeFromID, return_internal_reference<>())
        .def("GetAffectationModule", &Reseau::GetAffectationModule, return_internal_reference<>())
        .def("GetSensors", &Reseau::GetGestionsCapteur, return_internal_reference<>())
        .def("GetMinKv", &Reseau::GetMinKv);

    // Wrapper pour la liste des véhicules du réseau
    class_<std::vector<boost::shared_ptr<Vehicule>>>("lstVehicles")
        .def(vector_indexing_suite<std::vector<boost::shared_ptr<Vehicule>>, true>());

    // Wrapper pour la liste des lignes de bus du réseau
    class_<std::deque<Trip*>>("lstTrips")
        .def(vector_indexing_suite<std::deque<Trip*>, true>());

    ////////////////////////////////////////
    // Wrapper Python de la classe Ligne_bus
    ////////////////////////////////////////
    class_<Trip, Trip*>("Trip")
        .def_readonly("LstBus", &Trip::m_AssignedVehicles)
        .def("GetLineName", &Trip::GetID, return_value_policy<copy_const_reference>())
        .def("GetLstStops", &Trip::GetTripNodes);

    // Wrapper pour la liste des arrêts de bus d'une ligne
    class_<std::vector<TripNode*>>("lstStops")
        .def(vector_indexing_suite<std::vector<TripNode*>, true>());

    // Wrapper pour la liste des arrêts de bus d'une ligne
    class_<std::deque<boost::shared_ptr<Vehicule>>>("dequeVehicles")
        .def(vector_indexing_suite<std::deque<boost::shared_ptr<Vehicule>>, true>());

    ////////////////////////////////////////
    // Wrapper Python de la classe Arret
    ////////////////////////////////////////
    class_<TripNode, TripNode*>("TripNode")
        .def("GetStopName", &TripNode::GetID, return_value_policy<copy_const_reference>());


    ////////////////////////////////////////
    // Wrapper Python de la classe Affectation
    ////////////////////////////////////////
    class_<Affectation>("Affectation")
        .def("Run", &Affectation::Run);

    ////////////////////////////////////////
    // Wrapper Python de la classe SensorsManagers
    ////////////////////////////////////////
    class_<SensorsManagers>("Sensors")
        .def("LstPonctualSensors", &SensorsManagers::GetCapteursPonctuels, return_internal_reference<>());

    // Wrapper pour la liste des capteurs ponctuels
    class_<std::vector<PonctualSensor*>>("lstPonctualSensors")
        .def(vector_indexing_suite<std::vector<PonctualSensor*>, true>());

    ///////////////////////////////////////////////
    // Wrapper Python de la classe CapteurPonctuel
    ///////////////////////////////////////////////
    class_<PonctualSensor, PonctualSensor*>("PonctualSensor")
        .def_readonly("strName", &PonctualSensor::GetUnifiedID)
        .def("GetLink", &PonctualSensor::GetTuyau, return_internal_reference<>())
        .def_readonly("Position", &PonctualSensor::GetPosition)
        .def("detectedVehiclesIDs", &PonctualSensor::GetNIDVehFranchissant, return_internal_reference<>())
        .def("detectedVehiclesTypes", &PonctualSensor::GetTypeVehFranchissant, return_internal_reference<>())
        .def("detectedVehiclesTimes", &PonctualSensor::GetInstVehFranchissant, return_internal_reference<>())
        .def("detectedVehiclesLanes", &PonctualSensor::GetVoieVehFranchissant, return_internal_reference<>());

    class_<std::deque<int>>("dequeInt").def(vector_indexing_suite<std::deque<int>>());
    class_<std::deque<char>>("dequechar").def(vector_indexing_suite<std::deque<char>>());
    class_<std::deque<double>>("dequedouble").def(vector_indexing_suite<std::deque<double>>());


    ////////////////////////////////////////
    // Wrapper Python de la classe Vehicule
    ////////////////////////////////////////
    class_<Vehicule, Vehicule*>("Vehicle")
        .add_property("ID", &Vehicule::GetID)
        .def_readonly("LstUsedLanes", &Vehicule::m_LstUsedLanes)
        .def("GetType", &Vehicule::GetType, return_internal_reference<>())
        .def("GetMaxSpeed", &Vehicule::GetVitMax)
        .def("GetLink", &Vehicule::GetLink, return_internal_reference<>())
        .def("GetPosition", &Vehicule::GetPos)
        .def("GetVoie", &Vehicule::GetVoie, return_internal_reference<>())
        .def("GetSpeed", &Vehicule::GetVit)
        .def("GetLength", &Vehicule::GetLength)
        .def("GetDiagFonda", &Vehicule::GetDiagFonda, return_internal_reference<>())
		.def("GetNetOccuringTime", &Vehicule::GetInstantEntree)
		.def("GetCreationTime", &Vehicule::GetInstantCreation)
        .def("GetExitTime", &Vehicule::GetExitInstant)
        .def("IsAtStop", &Vehicule::IsAtTripNode)
        .def("GetCurrentStop", &Vehicule::GetLastTripNode, return_internal_reference<>())
        .def("GetRemainingStopTime", &Vehicule::GetTpsArretRestant)
        .def("GetDesiredLane", &Vehicule::GetVoieDesiree)
        .def("SetDesiredLane", &Vehicule::SetVoieDesiree)
        .def("SetRemainingStopTime", &Vehicule::SetTpsArretRestant);

    register_ptr_to_python< boost::shared_ptr<Vehicule> >();

    class_<std::deque<Voie*>>("std::deque<Lane*>")
        .def(vector_indexing_suite<std::deque<Voie*>, true>());

    class_<std::deque<VoieMicro*>>("std::deque<LaneMicro*>")
        .def(vector_indexing_suite<std::deque<VoieMicro*>, true>());

    class_<std::vector<VoieMicro*>>("std::vector<LaneMicro*>")
        .def(vector_indexing_suite<std::vector<VoieMicro*>, true>());

    ////////////////////////////////////////
    // Wrapper Python de la classe StaticElement
    ////////////////////////////////////////
    class_<StaticElement, StaticElement*>("StaticElement")
        .def("GetReseau", &StaticElement::GetPos, return_value_policy<copy_const_reference>());

     ////////////////////////////////////////
    // Wrapper Python de la classe DiagFonda
    ////////////////////////////////////////
    class_<CDiagrammeFondamental>("FondamentalDiagram")
        .add_property("Kmax", &CDiagrammeFondamental::GetKMax)
        .add_property("Kx", &CDiagrammeFondamental::GetKCritique);

    ////////////////////////////////////////
    // Wrapper Python de la classe TypeVehicule
    ////////////////////////////////////////
    class_<TypeVehicule>("VehicleType")
        .add_property("Label", &TypeVehicule::GetLabel)
        .def("GetMaxAcc", &TypeVehicule::GetAccMax)
        .def("GetSpacingAtStop", &TypeVehicule::GetEspacementArret)
        .def("ComputeDeceleration", &TypeVehicule::CalculDeceleration)
        .def("GetDeceleration", &TypeVehicule::GetDeceleration);

    ////////////////////////////////////////
    // Wrapper Python de la classe Tuyau
    ////////////////////////////////////////
    class_<TronconWrap, boost::noncopyable>("Pipe")
        .def_readonly("Label", &Troncon::m_strLabel)
        .def("GetParent", &Troncon::GetParent, return_internal_reference<>())
        .def("GetLength", &Troncon::GetLength);
    class_<TuyauWrap, bases<Troncon>, boost::noncopyable>("Link")
        .def("GetLstLanes", &Tuyau::GetLstLanes, return_internal_reference<>())
        .def("GetLane", &Tuyau::GetVoie, return_internal_reference<>())
        .def("SendSpeedLimit", &Tuyau::SendSpeedLimit)
        .def("GetSpeedLimit", &Tuyau::GetVitRegByTypeVeh)
        .def("getNbVoies", &Tuyau::getNb_voies);
    class_<TuyauMicro, bases<Tuyau>>("LinkMicro");
	class_<CTuyauMeso, bases<TuyauMicro>>("LinkMeso");

    class_<std::vector<Voie*>>("std::vector<Lane*>")
        .def(vector_indexing_suite<std::vector<Voie*>, true>());

    ////////////////////////////////////////
    // Wrapper Python de la classe Voie
    ////////////////////////////////////////
    class_<VoieWrap, bases<Troncon>, boost::noncopyable>("Lane")
        .def("SetTargetLaneForbidden", &Voie::SetTargetLaneForbidden)
        .def("IsTargetLaneForbidden", &Voie::IsTargetLaneForbidden)
        .def("SetPullBackInInFrontOfGuidedVehicleForbidden", &Voie::SetPullBackInInFrontOfGuidedVehicleForbidden)
        .def("IsPullBackInInFrontOfGuidedVehicleForbidden", &Voie::IsPullBackInInFrontOfGuidedVehicleForbidden)
        .def("GetNum", &Voie::GetNum);
    class_<VoieMicro, VoieMicro*, bases<Voie>>("LaneMicro")
        .def("SetChgtVoieObligatoire", &VoieMicro::SetChgtVoieObligatoire)
        .def("SetNotChgtVoieObligatoire", &VoieMicro::SetNotChgtVoieObligatoire);

    class_<std::vector<TypeVehicule*>>("std::vector<TypeVehicule*>")
        .def(vector_indexing_suite<std::vector<TypeVehicule*>, true>());


    ////////////////////////////////////////
    // Wrapper Python de la classe ControleurDeFeux
    ////////////////////////////////////////
    class_<ControleurDeFeux>("TrafficLightController")
        .def("GetTrafficLightCycle", &ControleurDeFeux::GetTrafficLightCycle, return_internal_reference<>())
        .def("GetLstTrafficLightCycles", &ControleurDeFeux::GetLstTrafficLightCycles, return_internal_reference<>());

    // Wrapper de la liste des timevariations des plans de feux
    class_<ListOfTimeVariationEx<PlanDeFeux>>("ListOfTimeVariationEx<TrafficLightCycle>").
        def("AddVariation", &ListOfTimeVariationEx<PlanDeFeux>::AddVariation);
    

    ////////////////////////////////////////
    // Wrapper Python de la classe PlanDeFeux
    ////////////////////////////////////////
    class_<PlanDeFeux>("TrafficLightCycle", no_init)
        .def("GetStartTime", &PlanDeFeux::GetStartTime)
        .def("GetCycleLength", &PlanDeFeux::GetCycleLength)
        .def("GetLstSequences", &PlanDeFeux::GetLstSequences)
        .def("RemoveAllSequences", &PlanDeFeux::RemoveAllSequences)
        .def("SetCycleLength", &PlanDeFeux::SetCycleLength)
        .def("AddSequence", &PlanDeFeux::AddSequence);
    def("CreateTrafficLightCycle", factory<PlanDeFeux, char*, STime>, return_internal_reference<>() );
    class_<std::vector<CDFSequence*>>("lstSequences")
        .def(vector_indexing_suite<std::vector<CDFSequence*>, true>());

    ////////////////////////////////////////
    // Wrapper Python de la classe CDFSequence
    ////////////////////////////////////////
    class_<CDFSequence, CDFSequence*>("CDFSequence", no_init)
        .def("SetTotalLength", &CDFSequence::SetTotalLength)
        .def("GetTotalLength", &CDFSequence::GetTotalLength)
        .def("GetLstActiveSignals", &CDFSequence::GetLstActiveSignals)
        .def("AddActiveSignal", &CDFSequence::AddActiveSignal);
    def("CreateSequence", factory<CDFSequence, double, int>, return_internal_reference<>() );
    class_<std::vector<SignalActif*>>("lstActiveSignals")
        .def(vector_indexing_suite<std::vector<SignalActif*>, true>());

    ////////////////////////////////////////
    // Wrapper Python de la classe SignalActif
    ////////////////////////////////////////
    class_<SignalActif, SignalActif*>("ActiveSignal", no_init)
        .add_property("GreenDuration", &SignalActif::GetGreenDuration, &SignalActif::SetGreenDuration)
        .add_property("OrangeDuration", &SignalActif::GetOrangeDuration, &SignalActif::SetOrangeDuration)
        .add_property("ActivationDelay", &SignalActif::GetActivationDelay)
        .def("GetInputLink", &SignalActif::GetInputLink, return_internal_reference<>() )
        .def("GetOutputLink", &SignalActif::GetOutputLink, return_internal_reference<>() );
    def("CreateActiveSignal", factory<SignalActif, Tuyau*, Tuyau*, double, double, double>, return_internal_reference<>() );

    ////////////////////////////////////////
    // Wrapper Python de la classe Connexion
    ////////////////////////////////////////
    class_<Connexion>("Connection")
        .def("GetMovement", &Connexion::GetMovement, return_internal_reference<>() );

    // wrapper pour le container des coefficients d'un mouvement autorisé
    class_<std::map<int,boost::shared_ptr<MouvementsSortie>>>("std::map<int,OutputMovement>")
        .def(map_indexing_suite<std::map<int,boost::shared_ptr<MouvementsSortie>>, true>());
    // wrapper pour la classe MouvementsSortie
    class_<MouvementsSortie, boost::shared_ptr<MouvementsSortie>>("OutputMovement")
        .def("AddForbiddenType", &MouvementsSortie::AddTypeExclu)
        .def("RemoveForbiddenType", &MouvementsSortie::RemoveTypeExclu);

    //////////////////////////////////////////////////
    // Wrapper Python de la classe ConnexionPonctuel
    //////////////////////////////////////////////////
    class_<ConnectionPonctuel, bases<Connexion>>("PonctualConnection");

    //////////////////////////////////////////////////
    // Wrapper Python de la classe BriqueDeConnexion
    //////////////////////////////////////////////////
    class_<BriqueDeConnexion, bases<Connexion>, boost::noncopyable>("ConnectionBrick", no_init);

    //////////////////////////////////////////////////
    // Wrapper Python de la classe AbstractCarFollowing
    //////////////////////////////////////////////////
    class_<AbstractCarFollowing, boost::noncopyable>("AbstractCarFollowing", no_init)
        .def("GetVehicle", &AbstractCarFollowing::GetVehicle, return_internal_reference<>())
        .def("SetComputedTravelledDistance", &AbstractCarFollowing::SetComputedTravelledDistance)
        .def("SetComputedTravelSpeed", &AbstractCarFollowing::SetComputedTravelSpeed)
        .def("SetComputedTravelAcceleration", &AbstractCarFollowing::SetComputedTravelAcceleration);

    //////////////////////////////////////////////////
    // Wrapper Python de la classe AbstractCarFollowing
    //////////////////////////////////////////////////
    class_<ScriptedCarFollowing, bases<AbstractCarFollowing>>("ScriptedCarFollowing");

    //////////////////////////////////////////////////
    // Wrapper Python de la classe CarFollowingContext
    //////////////////////////////////////////////////
    class_<CarFollowingContext>("CarFollowingContext")
        .def("GetLstLanes",&CarFollowingContext::GetLstLanes, return_internal_reference<>())
        .def("GetLeaders",&CarFollowingContext::GetLeaders, return_internal_reference<>())
        .def("GetLeaderDistances",&CarFollowingContext::GetLeaderDistances, return_internal_reference<>())
        .def("GetStartPosition", &CarFollowingContext::GetStartPosition)
        .def("SetFreeFlow", &CarFollowingContext::SetFreeFlow);

    class_<std::vector<double>>("vectorDouble").def(vector_indexing_suite<std::vector<double>>());

    ////////////////////////////////////////
    // Wrapper Python de la classe SymuViaTripNode
    ////////////////////////////////////////
    class_<SymuViaTripNode, SymuViaTripNode*>("SymuViaTripNode")
        .def("SetLstRepVoie", &SymuViaTripNode::SetLstRepVoie);

    //////////////////////////////////////////////////
    // Wrapper Python de la classe PointDeConvergence
    //////////////////////////////////////////////////
    class_<PointDeConvergence>("ConvergencePoint");

    // Autres classes utilitaires
    class_<STime>("STime")
        .def_readwrite("hour", &STime::m_hour)
        .def_readwrite("minute", &STime::m_minute)
        .def_readwrite("second", &STime::m_second);
    class_<STimeSpan>("STimeSpan");

    ////////////////////////////////////////
    // Wrapper Python de la classe boost::posix_time::ptime
    ////////////////////////////////////////
    class_<boost::posix_time::ptime>("BoostPtime")
        .def("boostDate", &boost::posix_time::ptime::date)
        .def("boostTime", &boost::posix_time::ptime::time_of_day);

    class_<boost::posix_time::time_duration>("boostTime")
        .def("hours", &boost::posix_time::time_duration::hours)
        .def("minutes", &boost::posix_time::time_duration::minutes)
        .def("seconds", &boost::posix_time::time_duration::seconds)
        .def("totalMicroseconds", &boost::posix_time::time_duration::total_microseconds);

    class_<boost::gregorian::date>("boostDate")
        .def("year", &boost::gregorian::date::year)
        .def("month", &boost::gregorian::date::month)
        .def("day", &boost::gregorian::date::day)
        .def("totalDays", &boost::gregorian::date::day_count);

    //////////////////////////////////////////////////
    // Wrapper Python de la classe Point
    //////////////////////////////////////////////////
    class_<Point>("Point")
        .def_readwrite("X", &Point::dbX)
        .def_readwrite("Y", &Point::dbY)
        .def_readwrite("Z", &Point::dbZ)
        .def("DistanceTo", &Point::DistanceTo);

#ifdef USE_SYMUCOM
    //////////////////////////////////////////////////
    // Wrapper Python de la classe Message
    //////////////////////////////////////////////////
    class_<SymuCom::Message, SymuCom::Message*>("Message")
        .def("GetMessageID",&SymuCom::Message::GetMessageID)
        .def("GetOriginID",&SymuCom::Message::GetOriginID, return_value_policy<copy_const_reference>())
        .def("GetDestinationID",&SymuCom::Message::GetDestinationID, return_value_policy<copy_const_reference>())
        .def("GetStrType",&SymuCom::Message::GetStrType)
        .def("GetData",&SymuCom::Message::GetData, return_value_policy<copy_const_reference>())
        .def("GetTimeReceive",&SymuCom::Message::GetTimeReceive, return_internal_reference<>())
        .def("GetTimeSend",&SymuCom::Message::GetTimeSend, return_internal_reference<>());

    //////////////////////////////////////////////////
    // Wrapper Python de la classe Entity
    //////////////////////////////////////////////////
    class_<EntityWrap, EntityWrap*, boost::noncopyable>("Entity")
        .def("GetEntityID", &ITSStation::GetEntityID)
        .def("GetAlphaDown", &ITSStation::GetAlphaDown)
        .def("GetBetaDown", &ITSStation::GetBetaDown)
        .def("GetCanReceive", &ITSStation::GetCanReceive)
        .def("GetCanSend", &ITSStation::GetCanSend)
        .def("GetCoordinates", &ITSStation::GetCoordinates, return_internal_reference<>())
        .def("GetRange", &ITSStation::GetRange)
        .def("SendBroadcastMessage",&SymuCom::Entity::SendBroadcastMessage)
        .def("SendMultiMessages", &SymuCom::Entity::SendMultiMessages)
        .def("SendUniqueMessage", &SymuCom::Entity::SendUniqueMessage);

        enum_<SymuCom::Message::EMessageType>("EMessageType")
            .value("Unknown", SymuCom::Message::MT_Unknown)
            .value("Unicast", SymuCom::Message::MT_Unicast)
            .value("Broadcast", SymuCom::Message::MT_Broadcast)
            .value("Acknowledge", SymuCom::Message::MT_Acknowledge)
            .value("AckWaiting", SymuCom::Message::MT_AckWaiting)
            .value("AckReceive", SymuCom::Message::MT_AckReceive);

        class_<std::vector<unsigned int>>("lstID").def(vector_indexing_suite<std::vector<unsigned int>>());

    //////////////////////////////////////////////////
    // Wrapper Python de la classe ITSStation
    //////////////////////////////////////////////////
    class_<StationWrap, StationWrap*, bases<SymuCom::Entity>, boost::noncopyable>("ITSStation")
        .def("ProcessMessage", &ITSStation::ProcessMessage)
        .def("GetApplicationByLabel", &ITSStation::GetApplicationByLabel, return_internal_reference<>())
        .def("GetApplicationByType", &ITSStation::GetApplicationByType, return_internal_reference<>())
        .def("GetApplications", &ITSStation::GetApplications, return_internal_reference<>())
        .def("GetLabel", &ITSStation::GetLabel)
        .def("GetType", &ITSStation::GetType)
        .def("GetSensorByLabel", &ITSStation::GetSensorByLabel, return_internal_reference<>())
        .def("GetSensorByType", &ITSStation::GetSensorByType, return_internal_reference<>())
        .def("GetSensors", &ITSStation::GetSensors, return_internal_reference<>())
        .def("GetDynamicParent", &ITSStation::GetDynamicParent, return_internal_reference<>())
        .def("GetStaticParent", &ITSStation::GetStaticParent, return_internal_reference<>());

        class_<std::vector<C_ITSApplication *>>("lstApplis").def(vector_indexing_suite<std::vector<C_ITSApplication *>>());
        class_<std::vector<ITSSensor *>>("lstSensor").def(vector_indexing_suite<std::vector<ITSSensor *>>());

    //////////////////////////////////////////////////
    // Wrapper Python de la classe ITSSensor
    //////////////////////////////////////////////////
    class_<SensorWrap, SensorWrap*, boost::noncopyable>("ITSSensor")
        .def("GetLabel", &ITSSensor::GetLabel)
        .def("GetType", &ITSSensor::GetType)
        .def("GetOwner", &ITSSensor::GetOwner, return_internal_reference<>())
        .def("GetUpdateFrequency", &ITSSensor::GetUpdateFrequency)
        .def("GetPrecision", &ITSSensor::GetPrecision)
        .def("GetNoiseRate", &ITSSensor::GetNoiseRate)
        .def("IsDown", &ITSSensor::IsDown);

    class_<DynamicSensorWrap, DynamicSensorWrap*, bases<ITSSensor>, boost::noncopyable>("ITSDynamicSensor");
    class_<StaticSensorWrap, StaticSensorWrap*, bases<ITSSensor>, boost::noncopyable>("ITSStaticSensor");

    //////////////////////////////////////////////////
    // Wrapper Python de la classe C_ITSApplication
    //////////////////////////////////////////////////
    class_<ApplicationWrap, ApplicationWrap*, boost::noncopyable>("C_ITSApplication")
        .def("GetLabel", &C_ITSApplication::GetLabel)
        .def("GetType", &C_ITSApplication::GetType);

    //////////////////////////////////////////////////
    // Wrapper Python de la classe ITSScriptableElement
    //////////////////////////////////////////////////
    class_<ITSScriptableElement, ITSScriptableElement*>("ITSScriptableElement")
        .def("GetScriptName", &ITSScriptableElement::GetScriptName, return_value_policy<copy_const_reference>())
        .def("SetParameters", &ITSScriptableElement::SetParameters)
        .def("GetParameters", &ITSScriptableElement::GetParameters, return_internal_reference<>());

    //////////////////////////////////////////////////
    // Wrapper Python de la classe ITSScriptableStation
    //////////////////////////////////////////////////
    class_<ITSScriptableStation, ITSScriptableStation*, bases<ITSStation, ITSScriptableElement>>("ITSScriptableStation");

    //////////////////////////////////////////////////
    // Wrapper Python de la classe C_ITSScriptableApplication
    //////////////////////////////////////////////////
    class_<C_ITSScriptableApplication, C_ITSScriptableApplication*, bases<C_ITSApplication, ITSScriptableElement>>("C_ITSScriptableApplication");

    //////////////////////////////////////////////////
    // Wrapper Python de la classe ITSScriptableDynamicSensor
    //////////////////////////////////////////////////
    class_<ITSScriptableDynamicSensor, ITSScriptableDynamicSensor*, bases<ITSDynamicSensor, ITSScriptableElement>>("ITSScriptableDynamicSensor");

    //////////////////////////////////////////////////
    // Wrapper Python de la classe ITSScriptableStaticSensor
    //////////////////////////////////////////////////
    class_<ITSScriptableStaticSensor, ITSScriptableStaticSensor*, bases<ITSStaticSensor, ITSScriptableElement>>("ITSScriptableStaticSensor");

    //////////////////////////////////////////////////
    // Wrapper Python de la classe GLOSA
    //////////////////////////////////////////////////
    class_<GLOSA, GLOSA*, bases<C_ITSApplication>>("GLOSA");

    //////////////////////////////////////////////////
    // Wrapper Python de la classe ITSCAF
    //////////////////////////////////////////////////
    class_<ITSCAF, ITSCAF*, bases<ITSStation>>("ITSCAF")
        .def("GetSendFrequency", &ITSCAF::GetSendFrequency);

    //////////////////////////////////////////////////
    // Wrapper Python de la classe ITSVehicle
    //////////////////////////////////////////////////
    class_<ITSVehicle, ITSVehicle*, bases<ITSStation>>("ITSVehicle");

    //////////////////////////////////////////////////
    // Wrapper Python de la classe Simulator
    //////////////////////////////////////////////////
    class_<Simulator, Simulator*, bases<ITSStation>>("Simulator")
        .def("GetApplicationByTypeFromAll", &Simulator::GetApplicationByTypeFromAll, return_internal_reference<>())
        .def("CreateVehicleStationFromPattern", &Simulator::CreateDynamicStationFromPattern, return_internal_reference<>())
        .def("GetStations", &Simulator::GetStations, return_value_policy<copy_const_reference>());

    class_<std::vector<ITSStation *>>("lstAllStations").def(vector_indexing_suite<std::vector<ITSStation *>>());

    //////////////////////////////////////////////////
    // Wrapper Python de la classe GPSSensor
    //////////////////////////////////////////////////
    class_<GPSSensor, GPSSensor*, bases<ITSDynamicSensor>>("GPSSensor")
        .def("GetPointPosition", &GPSSensor::GetPointPosition)
        .def("GetDoublePosition", &GPSSensor::GetDoublePosition)
        .def("GetSpeed", &GPSSensor::GetSpeed)
        .def("GetLaneAtStart", &GPSSensor::GetLaneAtStart, return_internal_reference<>());

    //////////////////////////////////////////////////
    // Wrapper Python de la classe InertialSensor
    //////////////////////////////////////////////////
    class_<InertialSensor, InertialSensor*, bases<ITSDynamicSensor>>("InertialSensor")
        .def("GetOrientation", &InertialSensor::GetOrientation)
        .def("GetAcceleration", &InertialSensor::GetAcceleration);

    //////////////////////////////////////////////////
    // Wrapper Python de la classe TelemetrySensor
    //////////////////////////////////////////////////
    class_<TelemetrySensor, TelemetrySensor*, bases<ITSDynamicSensor>>("TelemetrySensor")
        .def("GetOrientation", &TelemetrySensor::GetStaticElementsSeen, return_value_policy<copy_const_reference>())
        .def("GetAcceleration", &TelemetrySensor::GetVehicleSeen, return_value_policy<copy_const_reference>());

#endif // USE_SYMUCOM
}

void PythonUtils::setLogger(Logger * pLogger)
{
    m_pLogger = pLogger;
}

PythonUtils::PythonUtils()
{
    m_pLogger = NULL;
    m_pMainModule = NULL;
    m_pState = NULL;

    initialize();
}

PythonUtils::~PythonUtils()
{
    if(m_pMainModule)
    {
        delete m_pMainModule;
    }

    finalize();
}

boost::python::api::object *PythonUtils::getMainModule()
{
    return m_pMainModule;
}

void PythonUtils::enterChildInterpreter()
{
    if(m_pState)
    {
        PyEval_AcquireThread(m_pState);
    }
}

void PythonUtils::exitChildInterpreter()
{
    if(m_pState)
    {
        PyEval_ReleaseThread(m_pState);
    }
}

void PythonUtils::lockChildInterpreter()
{
    if(m_pState)
    {
        PyThreadState_Swap(m_pState);
    }
}

void PythonUtils::unlockChildInterpreter()
{
    if(m_pState)
    {
        PyThreadState_Swap(NULL);
    }
}

void PythonUtils::initialize()
{
	PyObject* (*initsymuvia)(void);
    PyImport_AppendInittab( "symuvia", initsymuvia);

    if(!Py_IsInitialized())
    {
        // Cas classique : SymuVia n'est pas lancé depuis python
        Py_Initialize();
    }
    else
    {
        // Cas où SymuVia est lancé depuis python
        PyEval_AcquireLock();
        m_pState = Py_NewInterpreter();
        assert(m_pState);
    }

    try
    {
        // récupération du contexte python global
        m_pMainModule = new object(handle<>(borrowed(PyImport_AddModule("__main__"))));
        object globals = m_pMainModule->attr("__dict__");

        // Ajout au path du dossier scripts
        std::string scriptsPath = SystemUtil::GetFilePath(const_cast<char*>(SCRIPTS_DIRECTORY.c_str()));
        boost::python::str scriptsDir = scriptsPath.c_str();
        boost::python::object sys = boost::python::import("sys");
        sys.attr("path").attr("insert")(0, scriptsDir);

        // import des modules fournis avec Symubruit
        exec("import imp\n", globals);
        PythonUtils::importModule(globals, SCRIPTS_SENSOR_MODULE_NAME, scriptsPath + DIRECTORY_SEPARATOR + SCRIPTS_SENSOR_MODULE_NAME + ".py");
        PythonUtils::importModule(globals, SCRIPTS_CONDITION_MODULE_NAME, scriptsPath + DIRECTORY_SEPARATOR + SCRIPTS_CONDITION_MODULE_NAME + ".py");
        PythonUtils::importModule(globals, SCRIPTS_ACTION_MODULE_NAME, scriptsPath + DIRECTORY_SEPARATOR + SCRIPTS_ACTION_MODULE_NAME + ".py");
        PythonUtils::importModule(globals, SCRIPTS_RESTITUTION_MODULE_NAME, scriptsPath + DIRECTORY_SEPARATOR + SCRIPTS_RESTITUTION_MODULE_NAME + ".py");
        PythonUtils::importModule(globals, SCRIPTS_CAR_FOLLOWING_MODULE_NAME, scriptsPath + DIRECTORY_SEPARATOR + SCRIPTS_CAR_FOLLOWING_MODULE_NAME + ".py");
    }
    catch( error_already_set )
    {
        if (m_pLogger)
        {
            *m_pLogger << Logger::Error << PythonUtils::getPythonErrorString() << std::endl;
            *m_pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        }
        else
        {
            std::cerr << PythonUtils::getPythonErrorString() << std::endl;
        }
    }

    exitChildInterpreter();
}

void PythonUtils::finalize()
{
    if(!m_pState)
    {
        // Cas classique : SymuVia n'est pas lancé depuis python
        Py_Finalize();
    }
}

std::string PythonUtils::getPythonErrorString()
{
    PyObject *exc,*val,*tb;
    PyErr_Fetch(&exc,&val,&tb);
    PyErr_NormalizeException(&exc,&val,&tb);
    handle<> hexc(exc),hval(allow_null(val)),htb(allow_null(tb));
    if(!hval)
    {
        return extract<std::string>(str(hexc));
    }
    else
    {
        object traceback(import("traceback"));
        object format_exception(traceback.attr("format_exception"));
        object formatted_list(format_exception(hexc,hval,htb));
        object formatted(str("").join(formatted_list));
        return extract<std::string>(formatted);
    }
}

void PythonUtils::importModule(boost::python::object & globals, std::string moduleName, std::string modulePath)
{
    dict locals;
    locals["modulename"] = moduleName.c_str();
    locals["path"] = modulePath.c_str();
    exec((moduleName + " = imp.load_module(modulename,open(path),path,('py','U',imp.PY_SOURCE))\n").c_str(), globals, locals);
    globals[moduleName.c_str()] = locals[moduleName.c_str()];
}

void PythonUtils::buildDictFromNode(DOMNode * pNode, dict &rDict)
{
    // Ajout des éléments fils
    for(XMLSize_t iChild = 0; iChild < pNode->getChildNodes()->getLength(); iChild++)
    {
        DOMNode * pChild = pNode->getChildNodes()->item(iChild);
        if(pChild->getNodeType() == DOMNode::ELEMENT_NODE)
        {
            dict childDict;
            buildDictFromNode(pChild, childDict);

            // Si pas encore d'élément de ce nom dans le dict, on crée la liste et on ajoute l'élément à la liste
            if(!rDict.contains(std::string(US(pChild->getNodeName()))))
            {
                boost::python::list newList;
                newList.append(childDict);
                rDict[std::string(US(pChild->getNodeName()))] = newList;
            }
            else
            {
                boost::python::list existingList = extract<boost::python::list>(rDict[std::string(US(pChild->getNodeName()))]);
                existingList.append(childDict);
            }
        }
    }
    // Ajout des attributs
    DOMNamedNodeMap * pAttributes = pNode->getAttributes();
    if(pAttributes)
    {
        for(XMLSize_t iParam = 0; iParam < pAttributes->getLength(); iParam++)
        {
            DOMNode * pAttribute = pAttributes->item(iParam);
            assert(pAttribute->getNodeType() == DOMNode::ATTRIBUTE_NODE);
            rDict[std::string(US(pAttribute->getNodeName()))] = std::string(US(pAttribute->getNodeValue()));
        }
    }
}

void PythonUtils::buildNodeFromDict(DOMElement* pElement, const boost::python::dict & rDict)
{
    // Ajout des éléments fils
    for(boost::python::ssize_t iChild = 0; iChild < len(rDict.keys()); iChild++)
    {
        // Si l'élément fils est un dictionnaire, on crée un noeud fils correspondant à ce dictionnaire
        extract<boost::python::dict> get_dict(rDict[rDict.keys()[iChild]]);
        if (get_dict.check())
        {
            std::string attributeName = extract<std::string>(rDict.keys()[iChild]);
            DOMElement * pChild = pElement->getOwnerDocument()->createElement(XS(attributeName.c_str()));
            buildNodeFromDict(pChild, get_dict());
            pElement->appendChild(pChild);
        }
        else
        {
            // Sinon, on ajoute l'élément fils en tant qu'attribut
            std::string attributeValue = extract<std::string>(str(rDict[rDict.keys()[iChild]]));
            std::string attributeName = extract<std::string>(rDict.keys()[iChild]);
            pElement->setAttribute(XS(attributeName.c_str()), XS(attributeValue.c_str()));
        }
    }
}

