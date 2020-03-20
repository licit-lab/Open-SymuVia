#include "stdafx.h"
#include "XMLReaderSymuvia.h"

#include "DOMLSSerializerSymu.hpp"
#include "XMLUtil.h"

#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/dom/impl/DOMDocumentImpl.hpp>


XERCES_CPP_NAMESPACE_USE

///----------------------- Classe Handler des évènements


// ---------------------------------------------------------------------------
//  PParseHandlers: Constructeur and Destructeur
// ---------------------------------------------------------------------------
PParseHandlers::PParseHandlers()
{
	m_bStartElementRead = false;
	m_bEndElementRead = false;
	m_iDepth = 0;
	m_Mode = MODE_NORMAL;
	m_pdoc = NULL;
	m_pwriter = NULL;
}

PParseHandlers::~PParseHandlers()
{
}

DOMElement * PParseHandlers::CreateCurrentElement()
{
	DOMElement * element = m_pdoc->createElement(XS(this->m_sName.c_str()));

	for (XMLSize_t i = 0; i < m_AttrList.size(); i++)
	{
		Attribut a = m_AttrList.at(i);
		element->setAttribute(XS(a.sName.c_str()), XS(a.sValue.c_str()));
	}

	return element;
}

void PParseHandlers::writeCurrentStartElement()
{
	m_pwriter->writeStartElement(XS(m_sName.c_str()));

	//std::string s;
	//s = m_sName + " : " + SystemUtil::ToString(m_iDepth);
	//::MessageBox(NULL, s.c_str(), "PParseHandlers::writeCurrentStartElement()", 0);

	for (XMLSize_t i = 0; i < m_AttrList.size(); i++)
	{
		Attribut a = m_AttrList.at(i);
		m_pwriter->writeAttributeString(XS(a.sName.c_str()), XS(a.sValue.c_str()));
	}
}

void PParseHandlers::writeCurrentTextElement()
{
    if(!m_sTextContent.empty() && m_sTextContent[0] != chLF)
    {
        m_pwriter->writeText(XS(m_sTextContent.c_str()));
    }
}

void PParseHandlers::writeCurrentEndElement()
{
    m_pwriter->writeEndElement();
}

// ---------------------------------------------------------------------------
//  PParseHandlers: Overrides de l'interface SAX "DocumentHandler"
// ---------------------------------------------------------------------------
void PParseHandlers::startElement(const   XMLCh* const      name 
                                    ,       AttributeList&  attributes)
{
	Attribut a;
	m_bStartElementRead = true;
	m_sName = US(name);
	XMLSize_t lg = attributes.getLength();
	m_AttrList.clear();
    m_sTextContent.clear();
	for (XMLSize_t i=0; i<lg; i++)
	{
		a.sName = US(attributes.getName(i));
		a.sValue = US(attributes.getValue(i));
		m_AttrList.push_back(a);
	}
	switch(m_Mode)
	{
	case MODE_NODE_READER:
		{
			m_iDepth ++;

			DOMElement * element = CreateCurrentElement();
			m_StackElem.push_back(element);
			break;
		}
	case MODE_NODE_WRITER:
		{
			m_iDepth ++;

			//std::string s;
			//s = m_sName + " : " + SystemUtil::ToString(m_iDepth);
			//::MessageBox(NULL, s.c_str(), "PParseHandlers::startElement", 0);

			this->writeCurrentStartElement();
			break;
		}
    case MODE_NORMAL:
        break;
	}
}

void PParseHandlers::endElement (const XMLCh *const name)
{
	m_bEndElementRead = true;
	m_sName = US(name);
	switch(m_Mode)
	{
	case MODE_NODE_READER:
		{
			m_iDepth --;
			if (m_StackElem.size() > 1)
			{
				DOMElement * element = m_StackElem.at(m_StackElem.size()-1);
				m_StackElem.pop_back();
				DOMElement * rootElement = m_StackElem.at(m_StackElem.size()-1);
				rootElement->appendChild(element);
			}
			break;
		}
	case MODE_NODE_WRITER:
		{
			m_iDepth --;

			//std::string s;
			//s = std::string(name) + " : " + SystemUtil::ToString(m_iDepth);
			//::MessageBox(NULL, s.c_str(), "PParseHandlers::endElement", 0);

			if (m_iDepth >= 0)
			{
                this->writeCurrentTextElement();
				this->writeCurrentEndElement();
			}
			break;
		}
    case MODE_NORMAL:
        break;
	}
    m_sTextContent.clear();
}

void PParseHandlers::characters(  const   XMLCh* const    chars 
								    , const XMLSize_t    length)
{
    m_sTextContent.append(US(chars));
}

void PParseHandlers::ignorableWhitespace( const   XMLCh* const chars
										    , const XMLSize_t length)
{
}

void PParseHandlers::resetDocument()
{
}


// ---------------------------------------------------------------------------
//  PParseHandlers: Overrides de l'interface SAX "ErrorHandler"
// ---------------------------------------------------------------------------
void PParseHandlers::error(const SAXParseException& e)
{
   // XERCES_STD_QUALIFIER cerr << "\nError at file " << StrX(e.getSystemId())
		 //<< ", line " << e.getLineNumber()
		 //<< ", char " << e.getColumnNumber()
   //      << "\n  Message: " << StrX(e.getMessage()) << XERCES_STD_QUALIFIER endl;
}

void PParseHandlers::fatalError(const SAXParseException& e)
{
   // XERCES_STD_QUALIFIER cerr << "\nFatal Error at file " << StrX(e.getSystemId())
		 //<< ", line " << e.getLineNumber()
		 //<< ", char " << e.getColumnNumber()
   //      << "\n  Message: " << StrX(e.getMessage()) << XERCES_STD_QUALIFIER endl;
}

void PParseHandlers::warning(const SAXParseException& e)
{
   // XERCES_STD_QUALIFIER cerr << "\nWarning at file " << StrX(e.getSystemId())
		 //<< ", line " << e.getLineNumber()
		 //<< ", char " << e.getColumnNumber()
   //      << "\n  Message: " << StrX(e.getMessage()) << XERCES_STD_QUALIFIER endl;
}



///----------------------- Classe XmlReaderSymuvia

XmlReaderSymuvia::XmlReaderSymuvia(void)
{
	eof = false;
	m_parser = NULL;
	m_cptAttr = 0;
}

XmlReaderSymuvia::~XmlReaderSymuvia(void)
{
	Close();
}

// Fermeture du fichier XML
void XmlReaderSymuvia::Close(void)
{
	if (m_parser)
	{
		m_parser->parseReset(m_token);
		delete m_parser;
		m_parser = NULL;
		eof = true;
		m_cptAttr = 0;
	}
}

// Ouverture d'un fichier XML
XmlReaderSymuvia * XmlReaderSymuvia::Create(const std::string & sPath)
{
	XmlReaderSymuvia * pReader = new XmlReaderSymuvia();

    pReader->m_parser = new SAXParser();
    pReader->m_parser->setDocumentHandler(&pReader->m_handler);
    pReader->m_parser->setErrorHandler(&pReader->m_handler);
    //pReader->m_parser->setValidationScheme(valScheme);
    pReader->m_parser->setDoNamespaces(true);
    pReader->m_parser->setDoSchema(true);
    pReader->m_parser->setHandleMultipleImports (true);
    pReader->m_parser->setValidationSchemaFullChecking(true);

	bool bOk = pReader->m_parser->parseFirst(XS(sPath.c_str()), pReader->m_token);

	pReader->eof = !bOk;

	return pReader;
}

// Lecture d'un d'noeud XML
bool XmlReaderSymuvia::Read(void)
{
	bool bOk;

	if (eof)
		return false;

	m_handler.m_bStartElementRead = false;
	m_handler.m_bEndElementRead = false;
	do
	{
		bOk = m_parser->parseNext(m_token);
	}
	while (bOk && !m_handler.m_bStartElementRead);

	eof = !bOk;

	if (bOk)
	{
		Name = m_handler.m_sName;		
	}
	else
	{
		Name = "";
	}

	m_cptAttr = 0;

	return bOk;
}

// Lecture d'un début ou d'une fin de noeud XML
bool XmlReaderSymuvia::ReadStartEnd(void)
{
	bool bOk;

	if (eof)
		return false;

	m_handler.m_bStartElementRead = false;
	m_handler.m_bEndElementRead = false;
	do
	{
		bOk = m_parser->parseNext(m_token);
	}
	while (bOk && !m_handler.m_bStartElementRead && !m_handler.m_bEndElementRead);

	eof = !bOk;

	if (bOk)
	{
		Name = m_handler.m_sName;		
	}
	else
	{
		Name = "";
	}

	m_cptAttr = 0;

	return bOk;
}

// Recherche et lecture d'un d'noeud XML
bool XmlReaderSymuvia::ReadToFollowing(const std::string & sNodeName)
{
	bool bOk;
	bool bNotFound = true;

	if (eof)
		return false;

	do
	{
		m_handler.m_bStartElementRead = false;
		m_handler.m_bEndElementRead = false;
		do
		{
			bOk = m_parser->parseNext(m_token);
		}
		while (bOk && !m_handler.m_bStartElementRead);

		if (bOk && m_handler.m_bStartElementRead)
		{
			Name = m_handler.m_sName;
			if (Name == sNodeName)
			{
				bNotFound = false;
			}
		}
	}
	while(bNotFound && bOk);

	m_cptAttr = 0;
	eof = !bOk;
	return bOk;
}

bool XmlReaderSymuvia::ReadStartElement(const std::string & sNodeName)
{
	return ReadToFollowing(sNodeName);
}

DOMElement * XmlReaderSymuvia::ReadNode(DOMDocument* pdoc)
{
	m_handler.m_Mode = PParseHandlers::MODE_NODE_READER;
	m_handler.m_pdoc = pdoc;
	m_handler.m_StackElem.clear();

	m_handler.m_iDepth = 0;
	DOMElement * element = m_handler.m_pdoc->createElement(XS(this->Name.c_str()));

	for (XMLSize_t i = 0; i < m_handler.m_AttrList.size(); i++)
	{
		Attribut a = m_handler.m_AttrList.at(i);
		element->setAttribute(XS(a.sName.c_str()), XS(a.sValue.c_str()));
	}

	m_handler.m_StackElem.push_back(element);

	bool bOk;
	do
	{
		bOk = this->ReadStartEnd();
	}
	while (bOk && (m_handler.m_iDepth >= 0));

	m_handler.m_Mode = PParseHandlers::MODE_NORMAL;
	m_handler.m_pdoc = NULL;
	m_handler.m_StackElem.clear();

	this->Read();

	return element;
}

void XmlReaderSymuvia::WriteNode(DOMLSSerializerSymu * pWriter, bool defattr)
{
	m_handler.m_Mode = PParseHandlers::MODE_NODE_WRITER;
	m_handler.m_pwriter = pWriter;

	m_handler.m_iDepth = 0;

	m_handler.writeCurrentStartElement();
	
    if(!m_handler.m_bEndElementRead)
    {
	    bool bOk;
	    do
	    {
		    bOk = this->ReadStartEnd();
	    }
	    while (bOk && (m_handler.m_iDepth >= 0));
    }

	m_handler.writeCurrentEndElement();

	m_handler.m_Mode = PParseHandlers::MODE_NORMAL;
	m_handler.m_pwriter = NULL;
	this->Read();
}

// Pour le noeud courant, renvoie la valeur de l'attribut dont le nom est passé en paramètre
std::string XmlReaderSymuvia::GetAttribute(const std::string & sNameAttr)
{
	XMLSize_t lg = m_handler.m_AttrList.size();
	bool bCont = true;
	std::string sVal = "";

	for (XMLSize_t i=0; bCont && (i<lg); i++)
	{
		if (m_handler.m_AttrList.at(i).sName == sNameAttr)
		{
			bCont = false;
			sVal = m_handler.m_AttrList.at(i).sValue;
		}
	}
	return sVal;
}

bool XmlReaderSymuvia::MoveToNextAttribute(std::string & sAttrName, std::string & sAttrValue)
{
		XMLSize_t lg = m_handler.m_AttrList.size();

		if (m_cptAttr < lg)
		{
			sAttrName = m_handler.m_AttrList.at(m_cptAttr).sName;
			sAttrValue = m_handler.m_AttrList.at(m_cptAttr).sValue;
			m_cptAttr ++;
			return true;
		}
		else
		{
			return false;
		}
}

