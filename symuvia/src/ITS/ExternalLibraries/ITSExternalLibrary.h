#pragma once
#ifndef ITSEXTERNALLIBRARYH
#define ITSEXTERNALLIBRARYH

#include <string>

#include <xercesc/util/XercesDefs.hpp>

namespace XERCES_CPP_NAMESPACE {
    class DOMDocument;
    class DOMNode;
}

namespace boost {
    namespace posix_time {
        class ptime;
    }
}

namespace boost {
    namespace serialization {
        class access;
    }
}


namespace SymuCom {
    class Message;
}

#ifdef WIN32
#ifndef _WINDEF_
class HINSTANCE__; // Forward or never
typedef HINSTANCE__* HINSTANCE;
#endif
#else
#include <dlfcn.h>
#endif

class Logger;
class Reseau;
class ITSStation;
class ITSExternalLibrary
{
public:
    ITSExternalLibrary();
    virtual ~ITSExternalLibrary();

    const std::string & GetID() const;

    bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument *pDoc, XERCES_CPP_NAMESPACE::DOMNode* pXMLNode, Logger * pLogger);

    // Sensors functions
    bool UpdateSensorData(void * Caller, Reseau * pNetwork, const std::string & customParams, const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime &dtTimeEnd, Logger * pLogger);

    // Stations functions
    bool TreatStationMessageData(void * Caller, Reseau * pNetwork, const std::string & customParams, SymuCom::Message *pMessage, Logger * pLogger);
    bool RunStation(void * Caller, Reseau * pNetwork, const std::string & customParams, const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration, Logger * pLogger);

    // Applications functions
    bool RunApplication(void * Caller, Reseau * pNetwork, const std::string & customParams, SymuCom::Message *pMessage, ITSStation * pStation, Logger * pLogger);

private:
    bool loadLibraryIfNeeded(Logger * pLogger);

protected:
    std::string m_strID;
    std::string m_strModule;

    bool m_bIsLoaded;

#ifdef WIN32
    HINSTANCE m_libHandle;
#else
    void * m_pLibHandle;
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif // ITSEXTERNALLIBRARYH
