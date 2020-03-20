#pragma once 

#ifndef SYMUCOM_ITSExternalStation_H
#define SYMUCOM_ITSExternalStation_H

#include "ITSStation.h"
#include "ITS/ITSScriptableElement.h"

class ITSExternalLibrary;
class ITSExternalStation : public ITSStation{

public:
    ITSExternalStation();
    ITSExternalStation(const ITSExternalStation &StationCopy);
    ITSExternalStation(Reseau* pNetwork, const std::string& strID, const std::string& strType, const SymuCom::Point& pPoint, SymuCom::Graph* pGraph, bool bCanSend = true, bool bCanReceive = true,
        double dbEntityRange = 10, double dbAlphaDown = -1, double dbBetaDown = -1, ITSExternalLibrary * pExtLib = NULL);
    virtual ~ITSExternalStation();

    virtual bool TreatMessageData(SymuCom::Message *pMessage);

    virtual bool Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode);

    virtual ITSStation* CreateCopy(const std::string& strID);

private:
    ITSExternalLibrary * m_pExtLib;

    std::string m_customParams;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif // SYMUCOM_ITSExternalStation_H
