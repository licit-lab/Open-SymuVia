#pragma once 

#ifndef SYMUCOM_C_ITSEXTERNALAPPLICATION_H
#define SYMUCOM_C_ITSEXTERNALAPPLICATION_H

#include "ITS/Applications/C_ITSApplication.h"

class ITSExternalLibrary;
class C_ITSExternalApplication : public C_ITSApplication {

public:
    C_ITSExternalApplication();
    C_ITSExternalApplication(const std::string strID, const std::string strType, ITSExternalLibrary * pExtLib, Reseau * pNetwork);
    virtual ~C_ITSExternalApplication();

    virtual bool RunApplication(SymuCom::Message* pMessage, ITSStation* pStation);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode, Reseau* pNetwork);

private:
    ITSExternalLibrary * m_pExtLib;
    Reseau * m_pNetwork;

    std::string m_customParams;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

};

#endif // SYMUCOM_C_ITSEXTERNALAPPLICATION_H
