#include "stdafx.h"
#include "C_ITSExternalApplication.h"

#include "reseau.h"

#include "ITS/ExternalLibraries/ITSExternalLibrary.h"

#include "Xerces/XMLUtil.h"

XERCES_CPP_NAMESPACE_USE


C_ITSExternalApplication::C_ITSExternalApplication()
{
    m_pExtLib = NULL;
    m_pNetwork = NULL;
}

C_ITSExternalApplication::C_ITSExternalApplication(const std::string strID, const std::string strType, ITSExternalLibrary * pExtLib, Reseau * pNetwork) : C_ITSApplication(strID, strType)
{
    m_pExtLib = pExtLib;
    m_pNetwork = pNetwork;
}

C_ITSExternalApplication::~C_ITSExternalApplication()
{

}

bool C_ITSExternalApplication::RunApplication(SymuCom::Message *pMessage, ITSStation *pStation)
{
    return m_pExtLib->RunApplication(this, m_pNetwork, m_customParams, pMessage, pStation, m_pNetwork->GetLogger());
}

bool C_ITSExternalApplication::LoadFromXML(DOMDocument *pDoc, DOMNode *pXMLNode, Reseau *pNetwork)
{
    m_customParams = XMLUtil::getOuterXml(pXMLNode);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void C_ITSExternalApplication::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void C_ITSExternalApplication::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void C_ITSExternalApplication::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(C_ITSApplication);
    ar & BOOST_SERIALIZATION_NVP(m_pExtLib);
    ar & BOOST_SERIALIZATION_NVP(m_pNetwork);
    ar & BOOST_SERIALIZATION_NVP(m_customParams);
}
