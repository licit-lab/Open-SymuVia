#pragma once

#ifndef SYMUCOM_SIMULATOR_H
#define SYMUCOM_SIMULATOR_H

#include "ITS/Stations/ITSStation.h"
#include "symUtil.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class Vehicule;
class ITSVehicle;
class ITSExternalLibrary;
class DOMLSSerializerSymu;
class SimulationSnapshot;
class TypeVehicule;

struct ITSSensorSnapshot;
struct SimulatorSnapshot {
    
    ITSStationSnapshot stationSnapshot;
    std::vector<ITSStationSnapshot*> lstStations;
    std::vector<ITSSensorSnapshot*> lstSensors;

    int iRandCount;

    unsigned int uiNextEntityID;

    int iLastMessageID;

    boost::posix_time::ptime dtBeginningTime;
    boost::posix_time::ptime dtCurrentTime;
    boost::posix_time::ptime dtRunEndTime;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class Simulator : public ITSStation{

    struct SensorParams {
        std::string strSensorType;
        std::string strPythonModule;
        std::string strExternalLibID;
        int iUpdateFrenquency;
        int iPrecision;
        double dbBreakDownRate;
        double dbNoiseRate;
        bool bStaticSensor;
        XERCES_CPP_NAMESPACE::DOMNode* pXMLSensorNode;
    };

    struct StationParams {
        std::string strStationID;
        std::string strStationType;
        double dbRange;
        bool canSend;
        bool canReceive;
        double dbAlphaDown;
        double dbBetaDown;
        SymuCom::Point ptEntityPos;
        XERCES_CPP_NAMESPACE::DOMDocument* pXMLDoc;
        XERCES_CPP_NAMESPACE::DOMNode* pXMLStationNode;
        XERCES_CPP_NAMESPACE::DOMNode* pXMLStationParamNode;
    };

    struct VehiclePattern {
        ITSStation* pDynamic;
        TypeVehicule* pTypeVeh;
        double dbPenetrationRate;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sérialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };

public:
    Simulator();
    Simulator(Reseau* pNetwork, const std::string& strID, const std::string& strType, const SymuCom::Point& pPoint, SymuCom::Graph* pGraph, SymuCom::CommunicationRunner* pCom, bool bCanSend = true, bool bCanReceive = true,
               double dbEntityRange = 10, double dbAlphaDown = -1, double dbBetaDown = -1);
    virtual ~Simulator();

    virtual void UpdateSensorsData(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration);

    virtual bool TreatMessageData(SymuCom::Message *pMessage);

    virtual bool Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration);

    virtual ITSStation* CreateCopy(const std::string& strID);

    Reseau *GetNetwork() const;

    const std::vector<ITSStation*> &GetStations() const;

    C_ITSApplication *GetApplicationByTypeFromAll(const std::string& strTypeApplication);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument *pDoc, XERCES_CPP_NAMESPACE::DOMNode* pXMLNode);

    ITSStation *CreateDynamicStationFromPattern(Vehicule *pVehicle);

    SymuCom::CommunicationRunner *GetCommunicationRunner() const;

    SymuCom::Graph *GetGraph() const;

    void AddStation(ITSStation* pStation);

    void ArbitrateVehicleChange();

    void RemoveConnectedVehicle(ITSStation* pStation);

    void WriteEntityToXML(ITSStation *pStation);

    void WriteMessageToXML(SymuCom::Message *pMessage);

    void InitSymuComXML(const std::string &strFile);

    void TerminateSymuComXML();

    SimulatorSnapshot * TakeSimulatorSnapshot() const;
    void Restore(SimulatorSnapshot* pSnapshot);

    // passage en mode d'�criture dans fichiers temporaires (affectation dynamique)
    void doUseTempFiles(SimulationSnapshot* pSimulationSnapshot, size_t snapshotIdx);
    // validation de la derni�re p�riode d'affectation et recopie des fichiers temporaires dans les fichiers finaux
    void converge(SimulationSnapshot * pSimulationSnapshot);

private:
    bool AddSpecificSensorsAndApplications(ITSStation *pStation, XERCES_CPP_NAMESPACE::DOMDocument *pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode);

    bool CreateCAFStations(StationParams lstParams);

    bool CreateDynamicPattern(ITSStation *DynPattern, XERCES_CPP_NAMESPACE::DOMNode* pXMLDynamicElement);

    ITSExternalLibrary * GetExternalLibraryFromID(const std::string & strExternalLibID);

    void InitSymuComXML(const std::string& strFile, DOMLSSerializerSymu** pWriter) const;

    void Suspend();
    void ReInit(const std::string & strXMLWriter);

private:
    std::vector<ITSStation*>                            m_lstAllStations;

    std::vector<ITSSensor*>                             m_lstAllSensors;

    std::vector<C_ITSApplication*>                      m_lstAllApplications;

    std::vector<ITSExternalLibrary*>                    m_lstExternalLibraries;

    SymuCom::Graph*                                     m_pGraph;

    SymuCom::CommunicationRunner*                       m_pCommunicationRunner;

    std::map<std::string, SensorParams>                 m_mapSensorsParams;

    std::vector<VehiclePattern>                         m_lstDynamicPatterns;

    std::map<std::string, std::vector<int> >            m_mapChangeRoute;

    std::map<std::string, std::vector<int> >            m_mapChangeLane;

    std::map<std::string, std::vector<double> >         m_mapSpeedTarget;

    std::map<std::string, std::vector<double> >         m_mapAccelerateDecelerate;

    DOMLSSerializerSymu*                                m_pXMLWriter;

    SimulationSnapshot*                                 m_pCurrentSimulationSnapshot;

    bool                                                m_bReinitialized;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif // SYMUCOM_SIMULATOR_H
