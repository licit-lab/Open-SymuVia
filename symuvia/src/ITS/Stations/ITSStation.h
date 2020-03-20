#pragma once 

#ifndef SYMUCOM_ITSStation_H
#define SYMUCOM_ITSStation_H

#include "SymubruitExports.h"

#include <Communication/Entity.h>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <xercesc/util/XercesDefs.hpp>

#include <boost/serialization/assume_abstract.hpp>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace boost {
    namespace serialization {
        class access;
    }
}

class ITSSensor;
class C_ITSApplication;
class Reseau;
struct Point;
class Vehicule;
class StaticElement;
class ITSStation;

struct ITSStationSnapshot {

    ITSStationSnapshot();
    ITSStationSnapshot(const ITSStation* pStation);

    SymuCom::Point coordinates;
    std::vector<SymuCom::Message> lstTravellingMessages;
    std::vector<SymuCom::Message> lstTreatedMessages;
    boost::posix_time::ptime dtLastBreakDown;
    bool bIsDown;

    int nIDDynamicParent;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
    class DOMDocument;
    class DOMElement;
}

class SYMUBRUIT_EXPORT ITSStation : public SymuCom::Entity{

public:
    ITSStation();
    ITSStation(const ITSStation &StationCopy);
    ITSStation(Reseau* pNetwork, const std::string& strID, const std::string& strType, const SymuCom::Point& pPoint, SymuCom::Graph* pGraph, bool bCanSend = true, bool bCanReceive = true,
               double dbEntityRange = 30, double dbAlphaDown = -1, double dbBetaDown = -1);
    virtual ~ITSStation();

    virtual void UpdateSensorsData(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration);

    std::string GetLabel() const;
    std::string GetType() const;

    ITSSensor *GetSensorByLabel(const std::string &strIDSensor);

    ITSSensor *GetSensorByType(const std::string &strTypeSensor);

    C_ITSApplication *GetApplicationByLabel(const std::string& strIDApplication);

    C_ITSApplication *GetApplicationByType(const std::string& strTypeApplication);

    virtual bool ProcessMessage(SymuCom::Message *pMessage);

    virtual bool TreatMessageData(SymuCom::Message *pMessage) = 0;

    virtual bool Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration) = 0;

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode) = 0;

    //Create a copy of the station with the same parameters as this, except for DynamicParent and StaticParent that will be NULL
    virtual ITSStation* CreateCopy(const std::string& strID) = 0;

    Reseau *GetNetwork() const;

    const std::vector<ITSSensor *>& GetSensors() const;

    const std::vector<C_ITSApplication *>& GetApplications() const;

    void AddSensor(ITSSensor* pSensor);

    void AddApplication(C_ITSApplication* pApplication);

    Vehicule *GetDynamicParent() const;
    void SetDynamicParent(Vehicule *pDynamicParent);

    StaticElement *GetStaticParent() const;
    void SetStaticParent(StaticElement *pStaticParent);

    virtual ITSStationSnapshot * TakeSnapshot() const;
    virtual void Restore(ITSStationSnapshot* pSnapshot);

protected:
    std::string                     m_strStationLabel; //Name of the ITS station

    std::string                     m_strStationType; //Type of the ITS station ex: ITSVehicle

    Reseau*                         m_pNetwork; //Link to the Network

    std::vector<ITSSensor*>         m_lstSensors; //List of all sensors owned by this station

    std::vector<C_ITSApplication*>  m_lstApplications; //List of all applications used by this station

private:
    Vehicule*                       m_pDynamicParent;//Dynamic parent

    StaticElement*                  m_pStaticParent;//static parent

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(ITSStation) // la classe est pure virtual

#pragma warning(pop)

#endif // SYMUCOM_ITSStation_H
