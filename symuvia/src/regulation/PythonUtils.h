#pragma once
#ifndef PythonUtilsH
#define PythonUtilsH

#include <xercesc/util/XercesVersion.hpp>

typedef struct _ts PyThreadState;

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
    class DOMElement;
}

namespace boost {
    namespace python {
        class dict;
        namespace api {
            class object;
        }
    }
}

class Logger;

#include <string>
#include <vector>

static const std::string SCRIPTS_DIRECTORY = std::string("scripts");
static const std::string SCRIPTS_SENSOR_MODULE_NAME = std::string("sensors");
static const std::string SCRIPTS_CONDITION_MODULE_NAME = std::string("conditions");
static const std::string SCRIPTS_ACTION_MODULE_NAME = std::string("actions");
static const std::string SCRIPTS_RESTITUTION_MODULE_NAME = std::string("restitutions");
static const std::string SCRIPTS_CAR_FOLLOWING_MODULE_NAME = std::string("car_following");

class PythonUtils
{
public:
    PythonUtils(void);
    virtual ~PythonUtils(void);

    void setLogger(Logger * pLogger);

    std::string getPythonErrorString();    

    void buildDictFromNode(XERCES_CPP_NAMESPACE::DOMNode * pNode, boost::python::dict & rDict);

    void buildNodeFromDict(XERCES_CPP_NAMESPACE::DOMElement* pElement, const boost::python::dict & rDict);

    void importModule(boost::python::api::object & globals, std::string moduleName, std::string modulePath);

    boost::python::api::object *getMainModule();

    // utilisé en cas d'interpreteur python enfant (si on lance les fonctions de SymuBruit depuis un interpreteur python parent)
    void enterChildInterpreter();
    void exitChildInterpreter();
    void lockChildInterpreter();
    void unlockChildInterpreter();

private:
    void initialize();

    void finalize();

private:
    // Contexte python global
    boost::python::api::object *m_pMainModule;

    // ThreadState associé à l'interpreteur python utilisé dans le cas où on lance SymuVia depuis python
    PyThreadState *m_pState;

    Logger * m_pLogger;
};

#endif // PythonUtilsH