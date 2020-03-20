#pragma once 

#ifndef SYMUCOM_GLOSA_H
#define SYMUCOM_GLOSA_H

#include "ITS/Applications/C_ITSApplication.h"
#include "symUtil.h"
#include "ControleurDeFeux.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

class ITSStation;
class GPSSensor;

class GLOSA : public C_ITSApplication{

public:
    GLOSA();
    GLOSA(const std::string strID, const std::string strType);
    virtual ~GLOSA();

    virtual bool RunApplication(SymuCom::Message* pMessage, ITSStation* pStation);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode, Reseau* pNetwork);

private:
    bool m_bUseSpeedAsTarget;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif // SYMUCOM_GLOSA_H
