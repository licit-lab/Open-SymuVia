#include "stdafx.h"
#include "XMLUtil.h"

#include "../SystemUtil.h"

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xqilla/xqilla-dom3.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMLSSerializer.hpp>
#include <xercesc/dom/DOMLSInput.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/dom/DOMLSParser.hpp>
#include <xercesc/dom/DOMLocator.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/sax/SAXException.hpp>
#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/parsers/SAXParser.hpp>


XERCES_CPP_NAMESPACE_USE

//------ Classe XMLUtil ----------------------------------------------------------------


// ---------------------------------------------------------------------------
//  DOMCountHandlers: Overrides of the DOM ErrorHandler interface
// ---------------------------------------------------------------------------
bool DOMReseauErrorHandler::handleError(const DOMError& domError)
{
    
	std::string sErrorMsg;

    if (domError.getSeverity() == DOMError::DOM_SEVERITY_WARNING)
	{
		m_WarningCount ++;
        sErrorMsg = "Warning ";
	}
    else if (domError.getSeverity() == DOMError::DOM_SEVERITY_ERROR)
	{
		m_ErrorCount ++;
		sErrorMsg = "Error ";
	}
    else//Erreur fatale
	{
		m_ErrorCount ++;
		sErrorMsg = "Fatal error ";
	}

	sErrorMsg = sErrorMsg + "\"" + US(domError.getMessage()) + "\""; 
	sErrorMsg = sErrorMsg + ", line ";
	sErrorMsg = sErrorMsg + SystemUtil::ToString((int)domError.getLocation()->getLineNumber());
	sErrorMsg = sErrorMsg + ", column ";
	sErrorMsg = sErrorMsg + SystemUtil::ToString((int)domError.getLocation()->getColumnNumber());

	if (m_pFicLog != NULL)
	{
		*m_pFicLog << sErrorMsg.c_str() << std::endl;
	}

    if (m_FirstErrorMessage.empty())
    {
        m_FirstErrorMessage = sErrorMsg;
    }

    return true;
}

void DOMReseauErrorHandler::resetErrors()
{
    m_ErrorCount = 0;
	m_WarningCount = 0;
}

//------ Classe XMLUtil ----------------------------------------------------------------

XMLUtil::XMLUtil(void)
{
    errorCode = 0;
    m_parser = NULL;
}

XMLUtil::~XMLUtil(void)
{
    CleanLoadDocument();
}

bool XMLUtil::XMLXercesActive = false;

void XMLUtil::XMLInit()
{
	if (XMLXercesActive)
		return;
	XMLXercesActive = true;
#ifdef REDEFINE_MEMORY_MANAGER
	if (m_MemoryManager == NULL)
	{
		m_MemoryManager = new MemoryManagerSymuvia();
	}
	#ifdef XQILLA
	XQillaPlatformUtils::initialize(m_MemoryManager);
	#else
	XMLPlatformUtils::Initialize(XMLUni::fgXercescDefaultLocale, NULL, NULL, m_MemoryManager);
	#endif
#else
	#ifdef XQILLA
	XQillaPlatformUtils::initialize();
	#else
	XMLPlatformUtils::Initialize();
	#endif
#endif
}
void XMLUtil::XMLEnd()
{
	if (!XMLXercesActive)
		return;
	XMLXercesActive = false;
#ifdef XQILLA
	XQillaPlatformUtils::terminate();
#else
	XMLPlatformUtils::Terminate();
#endif
#ifdef REDEFINE_MEMORY_MANAGER
	if (m_MemoryManager != NULL)
	{
		delete m_MemoryManager;
		m_MemoryManager = NULL;
	}
#endif
}

DOMDocument * XMLUtil::CreateXMLDocument(const XMLCh * xmlRootName, const XMLCh * xmlRootNamespace)
{
		XERCES_CPP_NAMESPACE::DOMDocument* doc = NULL;
		errorCode = 0;

        DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(DOM_IMPLEMENTATION);

		if (impl != NULL)
		{
		   try
		   {
			   doc = impl->createDocument(
						   xmlRootNamespace,     // root element namespace URI.
						   xmlRootName,          // root element name
						   0);                   // document type object (DTD).
		   }
		   catch (const OutOfMemoryException&)
		   {
			   errorCode = 1;
		   }
		   catch (const DOMException&)
		   {
			   errorCode = 2;
		   }
		   catch (...)
		   {
			   errorCode = 3;
		   }
		}
		else
		{
			errorCode = 4;
		}

	   return doc;
}

std::string XMLUtil::getOuterXml(DOMNode * node)
{
	std::string sres;
	char *res;

    if(node != NULL)
    {
	    // implémentation XML
	    DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(DOM_IMPLEMENTATION);

	    // création du Writer
	    DOMLSSerializer* myWriter = ((DOMImplementation*)impl)->createLSSerializer();

	    // serialisation du DOMNode
	    XMLCh * xml = myWriter->writeToString(node);

	    // conversion en char*
	    res = XMLString::transcode(xml);//theXMLString_Encoded;

	    // libération de la mémoire
	    XMLString::release(&xml);
	    myWriter->release();	

	    sres = res;
        
	    XMLString::release(&res); // Ajout CB 01/03/2011 (sinon Memory Leak !)
    }

	return sres;
}

// Renvoie la chaine XML correspondant au fichier de chemni spécifié
std::string XMLUtil::getOuterXml(const std::string &filePath)
{
    CleanLoadDocument();
	DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(DOM_IMPLEMENTATION);

#ifdef REDEFINE_MEMORY_MANAGER
	m_parser = impl->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0, XMLUtil::m_MemoryManager);
#else
	m_parser = impl->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
#endif

	DOMConfiguration  *config = m_parser->getDomConfig();

    config->setParameter(XMLUni::fgDOMNamespaces, true);
    config->setParameter(XMLUni::fgXercesSchema, false);
    config->setParameter(XMLUni::fgXercesHandleMultipleImports, true);
    config->setParameter(XMLUni::fgXercesSchemaFullChecking, false);
	config->setParameter(XMLUni::fgDOMComments, false);// Suppression des commentaires
	config->setParameter(XMLUni::fgDOMElementContentWhitespace, false);// Suppression des blancs

	config->setParameter(XMLUni::fgDOMValidateIfSchema, false); 
	config->setParameter(XMLUni::fgDOMValidate, false);

    // enable datatype normalization - default is off
    config->setParameter(XMLUni::fgDOMDatatypeNormalization, true);

	config->setParameter(XMLUni::fgXercesDOMHasPSVIInfo, true);

    XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *doc = NULL;

    // reset document pool
    m_parser->resetDocumentPool();

    doc = m_parser->parseURI(filePath.c_str());

    return getOuterXml(doc);
}

DOMNode * XMLUtil::SelectSingleNode(const std::string &XPath, XERCES_CPP_NAMESPACE::DOMDocument * pDoc, DOMElement * XmlRoot)
{
	DOMNode * xmlnode /*= NULL*/;

	if (XmlRoot == NULL)
		XmlRoot = pDoc->getDocumentElement();

	DOMNode * NodeRoot = (DOMNode *)XmlRoot;

	DOMXPathNSResolver* resolver=pDoc->createNSResolver(NodeRoot);

	DOMXPathResult * xpathResult = NULL;
	try
	{
		xpathResult = pDoc->evaluate(XS(XPath.c_str()), NodeRoot, resolver, DOMXPathResult::FIRST_RESULT_TYPE , NULL);
	}
	catch(DOMXPathException &e)	//catch(XQillaException &e)
	{
		std::string str = US(e.getMessage());
		str = "XPath 2.0 Error : \"" + str + "\"\nXPath : \"" + XPath + "\"\nXml node : \"" + US(XmlRoot->getTagName()) + "\"";
#ifdef WIN32
		::MessageBox(NULL, SystemUtil::ToWString(str).c_str(), L"SymuVia", 0);
#else
        std::cerr << str << std::endl;
#endif
		xpathResult = NULL;
	}
	catch(...)
	{
		xpathResult = NULL;
	}

    resolver->release();

	if (xpathResult != NULL)
	{
		const DOMTypeInfo * info = xpathResult->getTypeInfo();
		if (info == NULL)
		{
			xmlnode = NULL;
		}
		else
		{
			//const XMLCh * p = info->getTypeName();
			//DOMXPathResult::ResultType res = xpathResult->getResultType();
			//std::string s = SystemUtil::ToString(res);
			//s = s + std::string(p);
			//AfxMessageBox(s.c_str());

			xmlnode = xpathResult->getNodeValue();
		}

        xpathResult->release();
	}
	else
	{
		xmlnode = NULL;
	}

    return xmlnode;
}



DOMXPathResult * XMLUtil::SelectNodes(const std::string &XPath, XERCES_CPP_NAMESPACE::DOMDocument * pDoc, DOMElement * XmlRoot)
{
	if (XmlRoot == NULL)
		XmlRoot = pDoc->getDocumentElement();

	DOMNode * NodeRoot = (DOMNode *)XmlRoot;

	DOMXPathNSResolver* resolver=pDoc->createNSResolver(NodeRoot);

	DOMXPathResult * xpathResult = pDoc->evaluate(XS(XPath.c_str()), NodeRoot, resolver, DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE , NULL);

    resolver->release();

    return xpathResult;
}

DOMXPathResult * XMLUtil::SelectNodes(XMLCh * XPath, XERCES_CPP_NAMESPACE::DOMDocument * pDoc, DOMElement * XmlRoot)
{
	if (XmlRoot == NULL)
		XmlRoot = pDoc->getDocumentElement();

	DOMNode * NodeRoot = (DOMNode *)XmlRoot;

	DOMXPathNSResolver* resolver=pDoc->createNSResolver(NodeRoot);

	DOMXPathResult * xpathResult = pDoc->evaluate(XPath, NodeRoot, resolver, DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE , NULL);

    resolver->release();

    return xpathResult;
}

XERCES_CPP_NAMESPACE::DOMDocument * XMLUtil::LoadTrafic(const std::string & sPath, std::ofstream* pFicLog)
{
    XERCES_CPP_NAMESPACE::DOMDocument* pResult = NULL;

    // Vérification de l'existence du fichier XML de trafic
	if (!SystemUtil::FileExists(sPath))
    {      
		std::string sMsg;
		sMsg = "The file " + sPath + " doesn't exist !";
#ifdef WIN32
        ::MessageBox(NULL, SystemUtil::ToWString(sMsg).c_str(), L"SymuVia Error", 0);
#else
        std::cerr << sMsg << std::endl;
#endif
    }
    else
    {
        // Création et validation du document XML     
	    std::string sSchema = SystemUtil::GetFilePath("reseau.xsd");
        std::string firstError;
       
	    try
	    {
            pResult = XMLUtil::LoadDocument(sPath, sSchema, NULL, firstError);
	    }
        catch (const XMLException& e)
        {
		    std::string s = US(e.getMessage());
#ifdef WIN32
		    ::MessageBox(NULL,SystemUtil::ToWString("Validation error against the schema 'reseau.xsd' during input file loading 1: " + s).c_str(), L"SymuVia - Network loading", 0);        
#else
            std::cerr << "Validation error against the schema 'reseau.xsd' during input file loading 1: " << s << std::endl;
#endif
        }
        catch (const SAXException& e)
        {
		    std::string s = US(e.getMessage());
#ifdef WIN32
		    ::MessageBox(NULL,SystemUtil::ToWString("Validation error against the schema 'reseau.xsd' during input file loading 2: " + s).c_str(), L"SymuVia - Network loading", 0);        
#else
            std::cerr << "Validation error against the schema 'reseau.xsd' during input file loading 1: " << s << std::endl;
#endif     
        }
        catch (const DOMException& e)
        {
		    std::string s = US(e.getMessage());
#ifdef WIN32
		    ::MessageBox(NULL,SystemUtil::ToWString("Validation error against the schema 'reseau.xsd' during input file loading 3: " + s).c_str(), L"SymuVia - Network loading", 0);        
#else
            std::cerr << "Validation error against the schema 'reseau.xsd' during input file loading 1: " << s << std::endl;
#endif    
        }
        catch (...)
        {
		    //::MessageBox(NULL,"Validation error against the schema 'reseau.xsd' during input file loading 4", "SymuVia - Network loading", 0);        
        }

        if (pResult == NULL)
	    {
#ifdef WIN32
            ::MessageBox(NULL, SystemUtil::ToWString("Validation error against the schema 'reseau.xsd' during input file loading 5: " + firstError).c_str(), L"SymuVia - Network loading", 0);
#else
            std::cerr << "Validation error against the schema 'reseau.xsd' during input file loading 5: " << firstError << std::endl;
#endif      
	    }
        
    }

    return pResult;
}

XERCES_CPP_NAMESPACE::DOMDocument * XMLUtil::LoadDocument(const std::string & sPath, const std::string & xsdFile, std::ofstream* pFicLog, std::string & firstError)
{
	CleanLoadDocument();
	DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(DOM_IMPLEMENTATION);
#ifdef REDEFINE_MEMORY_MANAGER
	m_parser = impl->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0, XMLUtil::m_MemoryManager);
#else
	m_parser = impl->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
#endif


	DOMConfiguration  *config = m_parser->getDomConfig();

    config->setParameter(XMLUni::fgDOMNamespaces, true);
    config->setParameter(XMLUni::fgXercesSchema, true);
    config->setParameter(XMLUni::fgXercesHandleMultipleImports, true);
    config->setParameter(XMLUni::fgXercesSchemaFullChecking, true);
	config->setParameter(XMLUni::fgDOMComments, false);// Suppression des commentaires
	config->setParameter(XMLUni::fgDOMElementContentWhitespace, false);// Suppression des blancs

	config->setParameter(XMLUni::fgDOMValidateIfSchema, false); 
	config->setParameter(XMLUni::fgDOMValidate, true);

    // enable datatype normalization - default is off
    config->setParameter(XMLUni::fgDOMDatatypeNormalization, true);

	config->setParameter(XMLUni::fgXercesDOMHasPSVIInfo, true);

    // Création et installation du handler d'erreur
    DOMReseauErrorHandler errorHandler;
	errorHandler.setLogFile(pFicLog);
	errorHandler.resetErrors();
    config->setParameter(XMLUni::fgDOMErrorHandler, &errorHandler);

    XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *doc = NULL;

	m_parser->resetCachedGrammarPool();
    // reset document pool
    m_parser->resetDocumentPool();

	if (xsdFile.length() > 0)
	{
		Grammar* grammar;
		grammar = m_parser->loadGrammar(XS(xsdFile.c_str()), Grammar::SchemaGrammarType, true);
		if (grammar == NULL)
		{
			doc = NULL;
			return NULL;
		}
	}

    doc = m_parser->parseURI(XS(sPath.c_str()));

	if (errorHandler.getErrorCount() > 0)
	{
        firstError = errorHandler.getFirstErrorMsg();
		return NULL;
	}

	return doc;
}

XERCES_CPP_NAMESPACE::DOMDocument * XMLUtil::LoadDocument(const std::string & docStr)
{
	CleanLoadDocument();
	DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(DOM_IMPLEMENTATION);

    DOMLSInput* input = impl->createLSInput();
    XMLCh * str = XMLString::transcode(docStr.c_str());
    input->setStringData(str);

#ifdef REDEFINE_MEMORY_MANAGER
	m_parser = impl->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0, XMLUtil::m_MemoryManager);
#else
	m_parser = impl->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
#endif

	DOMConfiguration  *config = m_parser->getDomConfig();

    config->setParameter(XMLUni::fgDOMNamespaces, true);
    config->setParameter(XMLUni::fgXercesSchema, false);
    config->setParameter(XMLUni::fgXercesHandleMultipleImports, true);
    config->setParameter(XMLUni::fgXercesSchemaFullChecking, false);
	config->setParameter(XMLUni::fgDOMComments, false);// Suppression des commentaires
	config->setParameter(XMLUni::fgDOMElementContentWhitespace, false);// Suppression des blancs

	config->setParameter(XMLUni::fgDOMValidateIfSchema, false); 
	config->setParameter(XMLUni::fgDOMValidate, false);

    // enable datatype normalization - default is off
    config->setParameter(XMLUni::fgDOMDatatypeNormalization, true);

	config->setParameter(XMLUni::fgXercesDOMHasPSVIInfo, true);

    XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *doc = NULL;

    // reset document pool
    m_parser->resetDocumentPool();

    doc = m_parser->parse(input);

    input->release();
    XMLString::release(&str);

	return doc;
}

#ifdef REDEFINE_MEMORY_MANAGER
MemoryManagerSymuvia * XMLUtil::m_MemoryManager = NULL;
#endif

void XMLUtil::CleanLoadDocument()
{
	if (m_parser)
	{
		delete m_parser;
		m_parser = NULL;
	}
}

void XMLUtil::CleanLoadDocument(XERCES_CPP_NAMESPACE::DOMDocument * &pDocument)
{
	if (m_parser)
	{
		m_parser->release();
		m_parser = NULL;
		pDocument = NULL;//Le document est nettoyé par le parser.
	}
}

void XMLUtil::ReleaseChildNodes(DOMNode * node)
{
	if (node->hasChildNodes())
	{
		DOMNodeList * list = node->getChildNodes();
		for (XMLSize_t i = 0; i<list->getLength(); i++)
		{
			ReleaseChildNodes(list->item(i));
			DOMNode *n = node->removeChild(list->item(i));
			n->release();
			//delete n;
		}
	}
}

void XMLUtil::ReleaseNodes(DOMNode * node)
{
	ReleaseChildNodes(node);
	node->release();
}

const XMLCh  gXMLDecl_scenario[] =
{
    chLatin_S, chLatin_C, chLatin_E, chLatin_N, chLatin_A, chLatin_R, chLatin_I, chLatin_O, chNull
};

const XMLCh  gXMLDecl_id[] =
{
    chLatin_i, chLatin_d, chNull
};

const XMLCh  gXMLDecl_dirout[] =
{
    chLatin_d, chLatin_i, chLatin_r, chLatin_o, chLatin_u, chLatin_t, chNull
};

const XMLCh  gXMLDecl_prefout[] =
{
    chLatin_p, chLatin_r, chLatin_e, chLatin_f, chLatin_o, chLatin_u, chLatin_t, chNull
};


class MyScenarioSAXHandler : public HandlerBase {
public:

    void startElement(const XMLCh* const elementName, AttributeList& attrs)
    {
        if (XMLString::equals(elementName, gXMLDecl_scenario))
        {
            ScenarioElement scenar;

            const XMLCh * pId = attrs.getValue(gXMLDecl_id);
            if (pId)
            {
                scenar.id = US(pId);
                const XMLCh * pPrefOut = attrs.getValue(gXMLDecl_prefout);
                if (pPrefOut)
                {
                    scenar.prefout = US(pPrefOut);

                    const XMLCh * pDirOut = attrs.getValue(gXMLDecl_dirout);
                    if (pDirOut)
                    {
                        scenar.dirout = US(pDirOut);
                    }

                    m_Scenarios.push_back(scenar);
                }
            }
        }
    }

    const std::vector<ScenarioElement> & getScenarios() { return m_Scenarios; }

private:
    std::vector<ScenarioElement> m_Scenarios;
};

std::vector<ScenarioElement> XMLUtil::GetScenarioElements(const std::string & xmlFile)
{
    std::vector<ScenarioElement> results;

    SAXParser* parser = new SAXParser();
    parser->setValidationScheme(SAXParser::Val_Never);

    MyScenarioSAXHandler* docHandler = new MyScenarioSAXHandler();
    ErrorHandler* errHandler = (ErrorHandler*)docHandler;
    parser->setDocumentHandler(docHandler);
    parser->setErrorHandler(errHandler);

    try {
        parser->parse(xmlFile.c_str());
        results = docHandler->getScenarios();

        if (results.empty())
        {
#ifdef WIN32
            ::MessageBox(NULL, SystemUtil::ToWString("Node valid SCENARIO node found in input file.").c_str(), L"SymuVia - Network loading", 0);
#else
            std::cerr << "Node valid SCENARIO node found in input file." << std::endl;
#endif 
        }
    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
#ifdef WIN32
        ::MessageBox(NULL, SystemUtil::ToWString("Error loading input file 1: " + std::string(message)).c_str(), L"SymuVia - Network loading", 0);
#else
        std::cerr << "Error loading input file 1: " << message << std::endl;
#endif    
        XMLString::release(&message);
    }
    catch (const SAXParseException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
#ifdef WIN32
        ::MessageBox(NULL, SystemUtil::ToWString("Error loading input file 2: " + std::string(message)).c_str(), L"SymuVia - Network loading", 0);
#else
        std::cerr << "Error loading input file 2: " << message << std::endl;
#endif 
        XMLString::release(&message);
    }
    catch (...) {
        std::cerr << "Unexpected Exception \n";
    }

    delete parser;
    delete docHandler;

    return results;
}
