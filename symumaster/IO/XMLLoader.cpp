#include "XMLLoader.h"

#include "XMLStringUtils.h"
#include "XMLErrorHandler.h"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/dom/DOMException.hpp>

#include <boost/log/trivial.hpp>

using namespace SymuMaster;

XERCES_CPP_NAMESPACE_USE

XMLLoader::XMLLoader()
{
    m_pDOMParser = NULL;
    XMLPlatformUtils::Initialize();
}

XMLLoader::~XMLLoader()
{
    if (m_pDOMParser)
    {
        delete m_pDOMParser;
    }
    XMLPlatformUtils::Terminate();
}

bool XMLLoader::Load(const std::string & xmlFilePath, const std::string & xsdFilePath)
{
    try
    {
        if (m_pDOMParser)
        {
            delete m_pDOMParser;
        }
        m_pDOMParser = new XercesDOMParser();
        if (m_pDOMParser->loadGrammar(xsdFilePath.c_str(), Grammar::SchemaGrammarType) == NULL)
        {
            BOOST_LOG_TRIVIAL(error) << "Couldn't load schema '" << xsdFilePath << "'";
            return false;
        }

        XMLErrorHandler parserErrorHandler;

        m_pDOMParser->setErrorHandler(&parserErrorHandler);
        m_pDOMParser->setValidationScheme(XercesDOMParser::Val_Always);
        m_pDOMParser->setDoNamespaces(true);
        m_pDOMParser->setDoSchema(true);
        m_pDOMParser->setValidationSchemaFullChecking(true);

        m_pDOMParser->setExternalNoNamespaceSchemaLocation(xsdFilePath.c_str());

        m_pDOMParser->parse(xmlFilePath.c_str());

        return m_pDOMParser->getErrorCount() == 0;
    }
    catch (const XMLException& e)
    {
        BOOST_LOG_TRIVIAL(error) << US(e.getMessage()) << " at line " << e.getSrcLine();
        return false;
    }
    catch (const SAXException& e)
    {
        BOOST_LOG_TRIVIAL(error) << US(e.getMessage());
        return false;
    }
    catch (const DOMException& e)
    {
        BOOST_LOG_TRIVIAL(error) << US(e.getMessage());
        return false;
    }
    catch (...)
    {
        BOOST_LOG_TRIVIAL(error) << "Unknown error";
        return false;
    }
}

XERCES_CPP_NAMESPACE::DOMDocument * XMLLoader::getDocument()
{
    return m_pDOMParser->getDocument();
}
