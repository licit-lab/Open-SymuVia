#pragma once 

#ifndef SYMUCOM_C_ITSAPPLICATION_H
#define SYMUCOM_C_ITSAPPLICATION_H

#include <vector>
#include <string>

#include <xercesc/util/XercesDefs.hpp>

namespace boost {
    namespace serialization {
        class access;
    }
}

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCom {
    class Message;
}

class ITSStation;
class Reseau;

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
    class DOMDocument;
}

class C_ITSApplication{

public:
    C_ITSApplication();
    C_ITSApplication(const std::string strID, const std::string strType);
    virtual ~C_ITSApplication();

    virtual bool RunApplication(SymuCom::Message* pMessage, ITSStation* pStation) = 0;

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode, Reseau* pNetwork) = 0;

    std::string GetLabel() const;
    std::string GetType() const;

protected:
    static std::vector<std::string> split(const std::string &str, char delimiter);

    bool ReadAction(Reseau *pNetwork, const std::string &Action, std::string &Command, std::string &Value);

protected:
    std::string m_strApplicationID; // Name of the C_ITS application

    std::string m_strApplicationType; // Type of the C-ITS application ex: GLOSA

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif // SYMUCOM_C_ITSAPPLICATION_H
