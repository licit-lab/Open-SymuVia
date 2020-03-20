#pragma once
#ifndef ITSSCRIPTABLEELEMENTH
#define ITSSCRIPTABLEELEMENTH

#pragma warning( disable : 4244 )
#include <boost/python/dict.hpp>
#pragma warning( default : 4244 )

#include "reseau.h"
#include "Logger.h"
#include <xercesc/util/XercesVersion.hpp>

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
}

namespace boost {
    namespace serialization {
        class access;
    }
}

static const std::string ITS_SCRIPTS_DIRECTORY = std::string("ITS");

class Reseau;

/**
* Abstract Class herited for all ITS scriptable class (associeted with a python class)
*/
class ITSScriptableElement
{
public:
    ITSScriptableElement();
    virtual ~ITSScriptableElement();

    virtual bool initialize(Reseau * pReseau, const std::string &strModuleName);

	const std::string &GetScriptName();
	boost::python::dict & GetParameters() { return m_dictParameters; }
	void SetParameters(boost::python::dict & context) { m_dictParameters = context; }

protected:
    // Pointeur vers le réseau
    Reseau* m_pScriptNetwork;
    // name of the file that contains all the function to execute. Also the type of the ITS element defined in the XML
    std::string m_strModuleName;
    // Python dictionnary for all parameters defined in the xml
    boost::python::dict m_dictParameters;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif // ITSSCRIPTABLEELEMENTH
