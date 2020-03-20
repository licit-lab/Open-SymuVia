#pragma once 

#ifndef SYMUCOM_C_ITSSCRIPTABLEAPPLICATION_H
#define SYMUCOM_C_ITSSCRIPTABLEAPPLICATION_H

#include "ITS/Applications/C_ITSApplication.h"
#include "ITS/ITSScriptableElement.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class C_ITSScriptableApplication : public C_ITSApplication, public ITSScriptableElement{

public:
    C_ITSScriptableApplication();
    C_ITSScriptableApplication(const std::string strID, const std::string strType);
    virtual ~C_ITSScriptableApplication();

    virtual bool RunApplication(SymuCom::Message* pMessage, ITSStation* pStation);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode, Reseau* pNetwork);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif // SYMUCOM_C_ITSSCRIPTABLEAPPLICATION_H
