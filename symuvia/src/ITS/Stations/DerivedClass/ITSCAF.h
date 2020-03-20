#pragma once 

#ifndef SYMUCOM_ITSCAF_H
#define SYMUCOM_ITSCAF_H

#include "ITS/Stations/ITSStation.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class ITSCAF : public ITSStation{

public:
    ITSCAF();
    ITSCAF(StaticElement *pParent, Reseau *pNetwork, const std::string &strID, const std::string &strType, const SymuCom::Point &pPoint, SymuCom::Graph *pGraph, int iSendFrequency, bool bCanSend = true,
           bool bCanReceive = true, double dbEntityRange = 10, double dbAlphaDown = -1, double dbBetaDown = -1);
    virtual ~ITSCAF();

    virtual bool Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration);

    virtual bool TreatMessageData(SymuCom::Message *pMessage);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode);

    virtual ITSStation* CreateCopy(const std::string& strID);

    int GetSendFrequency() const;

private:
    int     m_iSendFrequency;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif // SYMUCOM_ITSCAF_H
