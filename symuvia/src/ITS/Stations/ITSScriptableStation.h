#pragma once 

#ifndef SYMUCOM_ITSScriptableStation_H
#define SYMUCOM_ITSScriptableStation_H

#include "ITSStation.h"
#include "ITS/ITSScriptableElement.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class ITSScriptableStation : public ITSStation, public ITSScriptableElement{

public:
    ITSScriptableStation();
    ITSScriptableStation(const ITSScriptableStation &StationCopy);
    ITSScriptableStation(Reseau* pNetwork, const std::string& strID, const std::string& strType, const SymuCom::Point& pPoint, SymuCom::Graph* pGraph, bool bCanSend = true, bool bCanReceive = true,
               double dbEntityRange = 10, double dbAlphaDown = -1, double dbBetaDown = -1);
    virtual ~ITSScriptableStation();

    virtual bool TreatMessageData(SymuCom::Message *pMessage);

    virtual bool Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode);

    virtual ITSStation* CreateCopy(const std::string& strID);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

};

#pragma warning(pop)

#endif // SYMUCOM_ITSScriptableStation_H
