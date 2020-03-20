#include "stdafx.h"
#include "ITSScriptableDynamicSensor.h"

#include "reseau.h"
#include "Logger.h"
#include "Xerces/XMLUtil.h"
#include "ITS/Stations/ITSStation.h"
#include "regulation/PythonUtils.h"

#include <boost/python/exec.hpp>
#include <boost/python/extract.hpp>

XERCES_CPP_NAMESPACE_USE

using namespace boost::python;

ITSScriptableDynamicSensor::ITSScriptableDynamicSensor()
{

}

ITSScriptableDynamicSensor::ITSScriptableDynamicSensor(const ITSScriptableDynamicSensor &SensorCopy)
        : ITSDynamicSensor(SensorCopy.m_pNetwork, SensorCopy.m_strSensorID, SensorCopy.m_strSensorType, SensorCopy.GetOwner(), SensorCopy.m_iUpdateFrequency, SensorCopy.m_dbBreakDownRate,
                    SensorCopy.m_iPrecision, SensorCopy.m_dbNoiseRate)
{

}

ITSScriptableDynamicSensor::ITSScriptableDynamicSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation *pOwner, int dbUpdateFrequency,
                                   double dbBreakDownRate, int iPrecision, double dbNoiseRate)
                                   :ITSDynamicSensor(pNetwork, strID, strType, pOwner, dbUpdateFrequency, dbBreakDownRate, iPrecision, dbNoiseRate)
{

}

ITSScriptableDynamicSensor::~ITSScriptableDynamicSensor()
{

}

bool ITSScriptableDynamicSensor::UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime &dtTimeEnd)
{
    try
    {
        object globals = m_pScriptNetwork->GetPythonUtils()->getMainModule()->attr("__dict__");
        dict locals;

		locals["network"] = ptr(m_pScriptNetwork);
        locals["parameters"] = m_dictParameters;
        locals["TimeStart"] = dtTimeStart;
        locals["TimeEnd"] = dtTimeEnd;
        exec((m_strModuleName + ".UpdateData(network,parameters,TimeStart,TimeEnd)\n").c_str(), globals, locals);
    }
    catch( error_already_set )
    {
        m_pScriptNetwork->log() << Logger::Error << m_pScriptNetwork->GetPythonUtils()->getPythonErrorString() << std::endl;
        m_pScriptNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        return false;
    }

    return true;
}

bool ITSScriptableDynamicSensor::LoadFromXML(xercesc_3_1::DOMDocument *pDoc, xercesc_3_1::DOMNode *pXMLNode)
{
    // build a dictionnary with all parameters for the element
    if(pXMLNode)
        m_pScriptNetwork->GetPythonUtils()->buildDictFromNode(pXMLNode, m_dictParameters);
    m_dictParameters["this"] = ptr(this);

    return true;
}

ITSSensor *ITSScriptableDynamicSensor::CreateCopy(ITSStation *pNewOwner)
{
    ITSScriptableDynamicSensor * pNewSensor = new ITSScriptableDynamicSensor(m_pNetwork, m_strSensorID, m_strSensorType, pNewOwner, m_iUpdateFrequency, m_dbBreakDownRate, m_iPrecision, m_dbNoiseRate);
    pNewSensor->m_strModuleName = m_strModuleName;
    pNewSensor->m_pScriptNetwork = m_pScriptNetwork;
    pNewSensor->m_dictParameters = m_dictParameters.copy();
	pNewSensor->m_dictParameters["this"] = ptr(pNewSensor);
    return pNewSensor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ITSScriptableDynamicSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSScriptableDynamicSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSScriptableDynamicSensor::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSDynamicSensor);
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSScriptableElement);
}
