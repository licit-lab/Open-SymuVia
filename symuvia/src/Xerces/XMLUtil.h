#pragma once

// Définir ce paramètre pour redéfinir la classe "MemoryManager" par défaut de Xerces-C
// WARNING : probably not thread safe to have ust one static memory manager like this ?
//#define REDEFINE_MEMORY_MANAGER 1

#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#ifdef REDEFINE_MEMORY_MANAGER
#include "MemoryManagerSymuvia.hpp"
#endif

namespace XERCES_CPP_NAMESPACE {
    class DOMDocumentImpl;
    class DOMXPathResult;
    class DOMNode;
    class DOMDocument;
    class DOMElement;
    class DOMLSParser;
};

#include <string>
#include <vector>
#include <iostream>
#include <fstream>


///////////////////////////////////////////////////////////////////
// Bloc de définition des macro de conversion char * vers XMLCh *
///////////////////////////////////////////////////////////////////

#include <boost/shared_ptr.hpp>
#include <xercesc/util/XMLString.hpp>

template< class CharType >
class ZStr      // Zero-terminated string.
{
private:
    boost::shared_ptr< CharType >   myArray;
 
public:
    ZStr( CharType const* s, void (*deleter)( CharType* ) )
        : myArray( const_cast< CharType* >( s ), deleter )
    { assert( deleter != 0 ); }
 
    CharType const* ptr() const { return myArray.get(); }
};
 
typedef ::XMLCh     Utf16Char;
 
inline void dispose( Utf16Char* p ) { xercesc::XMLString::release( &p ); }
inline void dispose( char* p )      { xercesc::XMLString::release( &p ); }
 
inline ZStr< Utf16Char > uStr( char const* s )
{
    return ZStr< Utf16Char >(
        xercesc::XMLString::transcode( s ), &dispose
        );
}
 
inline ZStr< char > cStr( Utf16Char const* s )
{
    return ZStr< char >(
        xercesc::XMLString::transcode( s ), &dispose
        );
}


#define XS(x) uStr(x).ptr()
#define US(x) cStr(x).ptr()

///////////////////////////////////////////////////////////////////
// Fin du bloc
///////////////////////////////////////////////////////////////////


#define XQILLA	1

#ifdef XQILLA
	#define	DOM_IMPLEMENTATION					XS("XPath2 3.0")
	//#define	DOM_IMPLEMENTATION2					"XPath2 3.0"
#else
	#ifdef XERCES_CORE
		#define	DOM_IMPLEMENTATION					"Core"
		//#define	DOM_IMPLEMENTATION2					"Core"
	#else
		#define	DOM_IMPLEMENTATION					"LS"
		//#define	DOM_IMPLEMENTATION2					"LS"
	#endif
#endif


static const XMLCh gXMLDecl_ver10[] =
{
    XERCES_CPP_NAMESPACE::chDigit_1, XERCES_CPP_NAMESPACE::chPeriod, XERCES_CPP_NAMESPACE::chDigit_0, XERCES_CPP_NAMESPACE::chNull
};

static const XMLCh gXMLDecl_encodingASCII[] =
{
    XERCES_CPP_NAMESPACE::chLatin_A, XERCES_CPP_NAMESPACE::chLatin_S, XERCES_CPP_NAMESPACE::chLatin_C, XERCES_CPP_NAMESPACE::chLatin_I,
    XERCES_CPP_NAMESPACE::chLatin_I, XERCES_CPP_NAMESPACE::chNull
};

struct ScenarioElement {
    std::string id;
    std::string dirout;
    std::string prefout;
};

// ---------------------------------------------------------------------------
//  Classe handler d'error pour le parser du reseau
// ---------------------------------------------------------------------------
class DOMReseauErrorHandler : public XERCES_CPP_NAMESPACE::DOMErrorHandler
{
public:
    // -----------------------------------------------------------------------
    //  Constructeurs and Destructeur
    // -----------------------------------------------------------------------
    DOMReseauErrorHandler()
	{
		m_ErrorCount = 0;
		m_WarningCount = 0;
		m_pFicLog = NULL;
	};
	~DOMReseauErrorHandler() {};



    // -----------------------------------------------------------------------
    //  Implementation de l'interface du DOM ErrorHandler
    // -----------------------------------------------------------------------
    bool handleError(const XERCES_CPP_NAMESPACE::DOMError& domError);
    void resetErrors();

    // -----------------------------------------------------------------------
    //  Compte-rendu d'erreur
    // -----------------------------------------------------------------------
	int getErrorCount() { return m_ErrorCount; };
	int getWarningCount() { return m_WarningCount; };
    const std::string & getFirstErrorMsg() { return m_FirstErrorMessage; }
	

    // -----------------------------------------------------------------------
    //  Initialisation du fichier de log
    // -----------------------------------------------------------------------
	void setLogFile(std::ofstream* pFicLog) { m_pFicLog = pFicLog; };

private :
    // -----------------------------------------------------------------------
    //  Constructors et Operators non implémenté
    // -----------------------------------------------------------------------
    DOMReseauErrorHandler(const DOMReseauErrorHandler&);
    void operator=(const DOMReseauErrorHandler&);


    // -----------------------------------------------------------------------
    //  Membres de données privées
    //
    //  m_ErrorCount
    //      Compteur du nombre d'erreurs fatales
    //  m_WarningCount
    //      Compteur du nombre d'erreurs non fatales
    // -----------------------------------------------------------------------
    int				m_ErrorCount;
	int				m_WarningCount;
	std::ofstream*	m_pFicLog;

    std::string     m_FirstErrorMessage;
};


class XMLUtil
{
public:
	XMLUtil(void);
	virtual ~XMLUtil(void);

    // Static stuff here for global xerces initialization / termination
	static bool XMLXercesActive;

    static void XMLInit();
    static void XMLEnd();
    // end of static stuff

	int errorCode;
	
	XERCES_CPP_NAMESPACE::DOMDocument * CreateXMLDocument(const XMLCh * xmlRootName, const XMLCh * xmlRootNamespace = 0);
	static std::string getOuterXml(XERCES_CPP_NAMESPACE::DOMNode * node); // still static for conveninence (avoid to change all GetXmlAttributeValue calls
    std::string getOuterXml(const std::string &filePath);
	XERCES_CPP_NAMESPACE::DOMNode * SelectSingleNode(const std::string &XPath, XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMElement * XmlRoot = NULL);
	XERCES_CPP_NAMESPACE::DOMXPathResult * SelectNodes(const std::string &XPath, XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMElement * XmlRoot = NULL);
	XERCES_CPP_NAMESPACE::DOMXPathResult * SelectNodes(XMLCh * XPath, XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMElement * XmlRoot = NULL);
    XERCES_CPP_NAMESPACE::DOMDocument * LoadDocument(const std::string & sPath, const std::string & xsdFile, std::ofstream* pFicLog, std::string & firstError);
    XERCES_CPP_NAMESPACE::DOMDocument * LoadTrafic(const std::string & sPath, std::ofstream* pFicLog = NULL);
    XERCES_CPP_NAMESPACE::DOMDocument * LoadDocument(const std::string & docStr);
	void CleanLoadDocument(XERCES_CPP_NAMESPACE::DOMDocument * &pDocument);
	void ReleaseNodes(XERCES_CPP_NAMESPACE::DOMNode * node);
    std::vector<ScenarioElement> GetScenarioElements(const std::string & xmlFile);

private:
	XERCES_CPP_NAMESPACE::DOMLSParser *m_parser;
#ifdef REDEFINE_MEMORY_MANAGER
	static MemoryManagerSymuvia * m_MemoryManager;
#endif

private:
	void ReleaseChildNodes(XERCES_CPP_NAMESPACE::DOMNode * node); // Libération d'un noeud xml avec tous ses fils

    void CleanLoadDocument();
};

