#include "stdafx.h"
#include "ITSExternalLibrary.h"

#include "tools.h"
#include "SystemUtil.h"
#include "Logger.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

XERCES_CPP_NAMESPACE_USE


ITSExternalLibrary::ITSExternalLibrary()
{
    m_bIsLoaded = false;
#ifdef WIN32
    m_libHandle = NULL;
#else
    m_pLibHandle = NULL;
#endif
}


ITSExternalLibrary::~ITSExternalLibrary()
{
#ifdef WIN32
    if (m_libHandle) {
        FreeLibrary(m_libHandle);
    }
#else
    if (m_pLibHandle) {
        dlclose(m_pLibHandle);
    }
#endif
}

const std::string & ITSExternalLibrary::GetID() const
{
    return m_strID;
}

bool ITSExternalLibrary::LoadFromXML(DOMDocument *pDoc, DOMNode* pXMLNode, Logger * pLogger)
{
    GetXmlAttributeValue(pXMLNode, "id", m_strID, pLogger);
    GetXmlAttributeValue(pXMLNode, "module", m_strModule, pLogger);

    return true;
}

typedef bool(*f_UpdateSensor)(void*, void*, const char*, const char*, const char*);
bool ITSExternalLibrary::UpdateSensorData(void * Caller, Reseau * pNetwork, const std::string & customParams, const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime &dtTimeEnd, Logger * pLogger)
{
    if (!loadLibraryIfNeeded(pLogger))
    {
        return false;
    }

    std::string timeStart = boost::posix_time::to_iso_string(dtTimeStart);
    std::string timeEnd = boost::posix_time::to_iso_string(dtTimeEnd);
#ifdef WIN32
    f_UpdateSensor func = (f_UpdateSensor)GetProcAddress(m_libHandle, "UpdateSensorData");
    if (!func) {
        *pLogger << Logger::Error << "could not locate the function UpdateSensorData in library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }

    if (!func(Caller, pNetwork, customParams.c_str(), timeStart.c_str(), timeEnd.c_str()))
    {
        *pLogger << Logger::Error << "Error in function UpdateSensorData of library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }
#else
    f_UpdateSensor func = (f_UpdateSensor)dlsym(m_pLibHandle, "UpdateSensorData");
    char * error;
    if ((error = dlerror()) != NULL)  {
        *pLogger << Logger::Error << "could not locate the function UpdateSensorData in library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }

    if (!func(Caller, pNetwork, customParams.c_str(), timeStart.c_str(), timeEnd.c_str()))
    {
        *pLogger << Logger::Error << "Error in function UpdateSensorData of library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }
#endif

    return true;
}

typedef bool(*f_treatStationMessageData)(void*, void*, const char*, void *);
bool ITSExternalLibrary::TreatStationMessageData(void * Caller, Reseau * pNetwork, const std::string & customParams, SymuCom::Message *pMessage, Logger * pLogger)
{
    if (!loadLibraryIfNeeded(pLogger))
    {
        return false;
    }

#ifdef WIN32
    f_treatStationMessageData func = (f_treatStationMessageData)GetProcAddress(m_libHandle, "TreatStationMessageData");
    if (!func) {
        *pLogger << Logger::Error << "could not locate the function TreatStationMessageData in library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }

    if (!func(Caller, pNetwork, customParams.c_str(), pMessage))
    {
        *pLogger << Logger::Error << "Error in function TreatStationMessageData of library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }
#else
    f_treatStationMessageData func = (f_treatStationMessageData)dlsym(m_pLibHandle, "TreatStationMessageData");
    char * error;
    if ((error = dlerror()) != NULL)  {
        *pLogger << Logger::Error << "could not locate the function TreatStationMessageData in library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }

    if (!func(Caller, pNetwork, customParams.c_str(), pMessage))
    {
        *pLogger << Logger::Error << "Error in function TreatStationMessageData of library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }
#endif

    return true;
}


typedef bool(*f_runStation)(void*, void*, const char*, const char*, double);
bool ITSExternalLibrary::RunStation(void * Caller, Reseau * pNetwork, const std::string & customParams, const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration, Logger * pLogger)
{
    if (!loadLibraryIfNeeded(pLogger))
    {
        return false;
    }

    std::string timeStart = boost::posix_time::to_iso_string(dtTimeStart);

#ifdef WIN32
    f_runStation func = (f_runStation)GetProcAddress(m_libHandle, "RunStation");
    if (!func) {
        *pLogger << Logger::Error << "could not locate the function RunStation in library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }

    if (!func(Caller, pNetwork, customParams.c_str(), timeStart.c_str(), dbTimeDuration))
    {
        *pLogger << Logger::Error << "Error in function RunStation of library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }
#else
    f_runStation func = (f_runStation)dlsym(m_pLibHandle, "RunStation");
    char * error;
    if ((error = dlerror()) != NULL)  {
        *pLogger << Logger::Error << "could not locate the function RunStation in library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }

    if (!func(Caller, pNetwork, customParams.c_str(), timeStart.c_str(), dbTimeDuration))
    {
        *pLogger << Logger::Error << "Error in function RunStation of library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }
#endif

    return true;
}


typedef bool(*f_runApplication)(void*, void*, const char*, void*, void*);
bool ITSExternalLibrary::RunApplication(void * Caller, Reseau * pNetwork, const std::string & customParams, SymuCom::Message *pMessage, ITSStation * pStation, Logger * pLogger)
{
    if (!loadLibraryIfNeeded(pLogger))
    {
        return false;
    }

#ifdef WIN32
    f_runApplication func = (f_runApplication)GetProcAddress(m_libHandle, "RunApplication");
    if (!func) {
        *pLogger << Logger::Error << "could not locate the function RunApplication in library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }

    if (!func(Caller, pNetwork, customParams.c_str(), pMessage, pStation))
    {
        *pLogger << Logger::Error << "Error in function RunApplication of library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }
#else
    f_runApplication func = (f_runApplication)dlsym(m_pLibHandle, "RunApplication");
    char * error;
    if ((error = dlerror()) != NULL)  {
        *pLogger << Logger::Error << "could not locate the function RunApplication in library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }

    if (!func(Caller, pNetwork, customParams.c_str(), pMessage, pStation))
    {
        *pLogger << Logger::Error << "Error in function RunApplication of library '" << m_strModule << "'" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }
#endif

    return true;
}

bool ITSExternalLibrary::loadLibraryIfNeeded(Logger * pLogger)
{
    if (!m_bIsLoaded)
    {
        m_bIsLoaded = true;

#ifdef WIN32
        std::wstring moduleName = SystemUtil::ToWString(m_strModule);
        m_libHandle = LoadLibrary(moduleName.c_str());

        if (!m_libHandle) {
            *pLogger << Logger::Error << "could not load the dynamic library '" << m_strModule << "'" << std::endl;
            *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

            return false;
        }
#else
        m_pLibHandle = dlopen(m_strModule.c_str(), RTLD_LAZY);
        if (!m_pLibHandle) {
            *pLogger << Logger::Error << "could not load the dynamic library '" << m_strModule << "' : " << dlerror() << std::endl;
            *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

            return false;
        }
#endif
    }
    return true;
}

template void ITSExternalLibrary::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSExternalLibrary::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSExternalLibrary::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_strID);
    ar & BOOST_SERIALIZATION_NVP(m_strModule);
}

