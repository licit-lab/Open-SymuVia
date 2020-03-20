#pragma once

#include <xercesc/framework/XMLPScanToken.hpp>
#include <xercesc/sax/HandlerBase.hpp>

namespace XERCES_CPP_NAMESPACE {
    class DOMDocument;
    class DOMElement;
    class SAXParser;
};

#include <string>
#include <deque>

class DOMLSSerializerSymu;

class Attribut
{
public:
	Attribut() {}
	~Attribut() {}
	void Copy(const Attribut &a)
	{
		sName = a.sName;
		sValue = a.sValue;
	}
	Attribut(const Attribut &a)
	{
		Copy(a);
	}
	Attribut& operator=(const Attribut &a)
	{
		Copy(a);
		return *this;
	}
public:
	std::string sName;
	std::string sValue;
};

class PParseHandlers : public XERCES_CPP_NAMESPACE::HandlerBase
{
public:
    // -----------------------------------------------------------------------
    //  Constructeurs
    // -----------------------------------------------------------------------
    PParseHandlers();
    ~PParseHandlers();

	XERCES_CPP_NAMESPACE::DOMElement * CreateCurrentElement();
	void writeCurrentStartElement();
    void writeCurrentTextElement();
	void writeCurrentEndElement();
    // -----------------------------------------------------------------------
    //  Handlers de l'interface SAX "DocumentHandler"
    // -----------------------------------------------------------------------
    void startElement(const XMLCh* const name, XERCES_CPP_NAMESPACE::AttributeList& attributes);
	void endElement (const XMLCh *const name);
    void characters(const XMLCh* const chars, const XMLSize_t length);
    void ignorableWhitespace(const XMLCh* const chars, const XMLSize_t length);
    void resetDocument();


    // -----------------------------------------------------------------------
    //  Implementations de l'interface SAX "ErrorHandler"
    // -----------------------------------------------------------------------
    void warning(const XERCES_CPP_NAMESPACE::SAXParseException& exc);
    void error(const XERCES_CPP_NAMESPACE::SAXParseException& exc);
    void fatalError(const XERCES_CPP_NAMESPACE::SAXParseException& exc);

public:
	typedef enum {
		MODE_NORMAL,
		MODE_NODE_READER,
		MODE_NODE_WRITER
	} ModeType;

public:
	std::string m_sName;
	std::deque<Attribut> m_AttrList;
    std::string m_sTextContent;
	bool m_bStartElementRead;
	bool m_bEndElementRead;
	int m_iDepth;
	XERCES_CPP_NAMESPACE::DOMDocument * m_pdoc;
	DOMLSSerializerSymu * m_pwriter;
	ModeType m_Mode;
	std::deque<XERCES_CPP_NAMESPACE::DOMElement*> m_StackElem;
	
};


class XmlReaderSymuvia
{
public:
	XmlReaderSymuvia(void);
	virtual ~XmlReaderSymuvia(void);
	// Fermeture du fichier XML
	void Close(void);
	// Ouverture d'un fichier XML
	static XmlReaderSymuvia * Create(const std::string & sPath);
	// Lecture d'un d'noeud XML
	bool Read(void);
	// Lecture du prochain noeud XML avec le nom "sNodeName"
	bool ReadToFollowing(const std::string & sNodeName);
	// Lecture du prochain noeud XML avec le nom "sNodeName"
	bool ReadStartElement(const std::string & sNodeName);
	// Lecture de l'élément courant (avec son arborescence)
	XERCES_CPP_NAMESPACE::DOMElement * ReadNode(XERCES_CPP_NAMESPACE::DOMDocument * pdoc);
	// Ecriture de l'élément courant (avec son arborescence)
	void WriteNode(DOMLSSerializerSymu * pdoc, bool defattr);
	// Obtention des attributs courants
	bool MoveToNextAttribute(std::string &sAttrName, std::string &sAttrValue);
	// Pour le noeud courant, renvoie la valeur de l'attribut dont le nom est passé en paramètre
	std::string GetAttribute(const std::string & sNameAttr);
	void MoveToContent() {};
    void ResetStartEndFlags() {m_handler.m_bEndElementRead = false; m_handler.m_bStartElementRead = false;};

public:
	// Nom du noeud courant
	std::string Name;
	// Fin de fichier
	bool eof;

private:
    XERCES_CPP_NAMESPACE::SAXParser* m_parser;
    PParseHandlers m_handler;
	XERCES_CPP_NAMESPACE::XMLPScanToken m_token;
	XMLSize_t m_cptAttr;
	
private:
	// Lecture d'un début ou d'une fin de noeud XML
	bool ReadStartEnd(void);


};
