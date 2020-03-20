#include "stdafx.h"
#include "DOMLSSerializerSymu.hpp"

#include "XMLUtil.h"

#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/util/TranscodingException.hpp>


//<?xml version="
static const XMLCh  gXMLDecl_VersionInfo[] =
{
    chOpenAngle, chQuestion, chLatin_x,     chLatin_m,  chLatin_l,  chSpace,
    chLatin_v,   chLatin_e,  chLatin_r,     chLatin_s,  chLatin_i,  chLatin_o,
    chLatin_n,   chEqual,    chDoubleQuote, chNull
};

// commenté car même nom de variable repris dans XMLUtil (àrenommer là bas si on a rebesoin de ca mais pas de raison.
/*static const XMLCh gXMLDecl_ver10[] =
{
    chDigit_1, chPeriod, chDigit_0, chNull
};*/

//encoding="
static const XMLCh  gXMLDecl_EncodingDecl[] =
{
    chLatin_e,  chLatin_n,  chLatin_c,  chLatin_o,      chLatin_d, chLatin_i,
    chLatin_n,  chLatin_g,  chEqual,    chDoubleQuote,  chNull
};

//standalone="
static const XMLCh  gXMLDecl_SDDecl[] =
{
    chLatin_s, chLatin_t, chLatin_a,   chLatin_n,    chLatin_d,   chLatin_a,
    chLatin_l, chLatin_o, chLatin_n,   chLatin_e,    chEqual,     chDoubleQuote,
    chNull
};

//"*Blank*
static const XMLCh  gXMLDecl_separator[] =
{
    chDoubleQuote, chSpace, chNull
};

//?>
static const XMLCh  gXMLDecl_endtag[] =
{
    chQuestion, chCloseAngle,  chNull
};

// default end-of-line sequence
static const XMLCh  gEOLSeq[] =
{
    chCR, chLF, chNull
};

//<
static const XMLCh  gXML_Begin[] =
{
    chOpenAngle, chNull
};

//</
static const XMLCh  gXML_BeginClose[] =
{
    chOpenAngle, chForwardSlash, chNull
};

//>
static const XMLCh gXML_EndSimple[] = 
{
    chCloseAngle,   chNull
};

//*blank*
static const XMLCh  gXML_separator1[] =
{
    chSpace, chNull
};

//="
static const XMLCh  gXML_separator2[] =
{
    chEqual, chDoubleQuote, chNull
};

//"
static const XMLCh  gXML_separator3[] =
{
    chDoubleQuote, chNull
};

//"
static const XMLCh  gXML_prefix[] =
{
	chColon, chNull
};

//"
static const XMLCh  gXML_xmlns[] =
{
	chLatin_x, chLatin_m, chLatin_l, chLatin_n, chLatin_s, chNull
};

// />
static const XMLCh  gXML_End[] =
{
    chForwardSlash, chCloseAngle,    chNull
};

//*CR**LF*
static const XMLCh  gXML_NewLine[] =
{
    chCR, chLF,    chNull
};


XERCES_CPP_NAMESPACE_USE

/**
  *		Constructeur, Destructeur
  **/

DOMLSSerializerSymu::DOMLSSerializerSymu(MemoryManager* const manager)
:DOMLSSerializerImpl(manager)
{
	m_bIsOpen = false;
	pTarget = NULL;
	fStandalone = NULL;
	flevel = 0;
	pOutputFile = NULL;
	setFeature(3, true);//FORMAT_PRETTY_PRINT_ID
	setNewLine(gXML_NewLine);//NewLine
	m_bNewLineIsLacking = false;
    m_bTextNodeWritten = false;
}

DOMLSSerializerSymu::~DOMLSSerializerSymu()
{
	flevel = 0;
	close();
}

/**
 *		fonction d'écriture
 **/

bool DOMLSSerializerSymu::initXmlWriter(const char * destinationFile,
										const XMLCh * Version,
										const XMLCh * Encoding,
										const XMLCh * Standalone)
{
	LocalFileFormatTarget * targetFile;
	try
	{
        m_strOutputFile = destinationFile;
		targetFile = new LocalFileFormatTarget(XS(destinationFile));
	}
	catch(...)
	{
		return false;
	}

	if (pOutputFile != NULL)
	{
		delete pOutputFile;
		pOutputFile = NULL;
	}
	pOutputFile = new DOMLSOutputImpl();
	
	pOutputFile->setByteStream(targetFile);

	bool bOk = initXmlWriter(pOutputFile, Version, Encoding, Standalone);

	return bOk;
}

std::string DOMLSSerializerSymu::getTargetFileName() const
{
    return m_strOutputFile;
}

bool DOMLSSerializerSymu::initXmlWriter(DOMLSOutput* const    destination,
										const XMLCh * Version,
										const XMLCh * Encoding,
										const XMLCh * Standalone)
{
    pTarget=destination->getByteStream();
    if(!pTarget)
    {
        const XMLCh* szSystemId=destination->getSystemId();
        if(!szSystemId)
        {
            //TODO: report error "missing target"
            return false;
        }
        pTarget=new LocalFileFormatTarget(szSystemId, fMemoryManager);
    }


    /**
     *  The end-of-line sequence of characters to be used in the XML being
     *  written out. The only permitted values are these:
     *     . null
     *
     *  Use a default end-of-line sequence. DOM implementations should choose
     * the default to match the usual convention for text files in the
     * environment being used. Implementations must choose a default
     * sequence that matches one of those allowed by  2.11 "End-of-Line
     * Handling".
     *
     *    CR    The carriage-return character (#xD)
     *    CR-LF The carriage-return and line-feed characters (#xD #xA)
     *    LF    The line-feed character (#xA)
     *
     *  The default value for this attribute is null
     */
    fNewLineUsed = (fNewLine && *fNewLine)? fNewLine : gEOLSeq;

    /**
     *  get Document Version
     */
    fDocumentVersion = Version; //XMLUni::fgVersion1_0

    fErrorCount = 0;

    fLineFeedInTextNodePrinted = false;
    fLastWhiteSpaceInTextNode = 0;

	fEncodingUsed = Encoding;

	fStandalone = Standalone;

    try
    {
        fFormatter = new (fMemoryManager) XMLFormatter( fEncodingUsed
                                                       ,fDocumentVersion
                                                       ,pTarget
                                                       ,XMLFormatter::NoEscapes
                                                       ,XMLFormatter::UnRep_CharRef
                                                       ,fMemoryManager);
    }
    catch (const TranscodingException& )
    {
		
        return false;
    }
	m_bIsOpen = true;
	return true;
}

bool DOMLSSerializerSymu::write(const DOMNode* nodeToWrite, DOMLSOutput* const destination)
{
	bool bOk;
	bOk = DOMLSSerializerImpl::write(nodeToWrite, destination);
	fFormatter = NULL;

	return bOk;
}

// pour reprise des résultats après reprise d'un snapshot
bool DOMLSSerializerSymu::write(std::string stringToWrite)
{
	bool bOk = true;
    XMLCh* str = XMLString::transcode(stringToWrite.c_str());
    *fFormatter << str;
    XMLString::release(&str);

	return bOk;
}

void DOMLSSerializerSymu::close()
{
	m_bIsOpen = false;
	if (pTarget != NULL)
	{
		pTarget->flush();
		delete pTarget;
		pTarget = NULL;
	}

	if (fFormatter != NULL)
	{
		delete fFormatter;
		fFormatter = NULL;
	}

	if (pOutputFile != NULL)
	{
		delete pOutputFile;
		pOutputFile = NULL;
	}
}

void DOMLSSerializerSymu::setIndent(bool bIndent)
{
	setFeature(3, bIndent);//FORMAT_PRETTY_PRINT_ID
}


void DOMLSSerializerSymu::setNewLineSeq(const XMLCh * Sequence)
{
	setNewLine(Sequence);//NewLine
    fNewLineUsed = (fNewLine && *fNewLine)? fNewLine : gEOLSeq;
}


bool DOMLSSerializerSymu::writeXmlDeclaration()
{
    *fFormatter << XMLFormatter::NoEscapes << gXMLDecl_VersionInfo << fDocumentVersion << gXMLDecl_separator;
    *fFormatter << gXMLDecl_EncodingDecl << fEncodingUsed << gXMLDecl_separator;

	if (fStandalone != NULL)
	{
		*fFormatter << gXMLDecl_SDDecl << fStandalone << gXMLDecl_separator;
	}

    *fFormatter << gXMLDecl_endtag;
	printNewLine();
	return true;
}

bool DOMLSSerializerSymu::writeStartElement(const XMLCh * ElementName)
{
	if (!fNodeBeingWritten.isEmpty())
	{
		//Fermer l'élément précédent
		fpile.push_back(fNodeBeingWritten);
		*fFormatter << XMLFormatter::NoEscapes << gXML_EndSimple;
		printNewLine();
		flevel++;
	}
	UpdateNewLine();
	//Initialiser l'élément courant
    XercesString tmpXercesString(ElementName);
	fNodeBeingWritten = tmpXercesString;
	//Ecrire le début de l'élément courant
	printIndent(flevel);
	*fFormatter << gXML_Begin << ElementName;
	return true;
}

bool DOMLSSerializerSymu::writeStartElement(const XMLCh * ElementName, const XMLCh * NS)
{
	if (!fNodeBeingWritten.isEmpty())
	{
		//Fermer l'élément précédent
		fpile.push_back(fNodeBeingWritten);
		*fFormatter << XMLFormatter::NoEscapes << gXML_EndSimple;
		printNewLine();
		flevel++;
	}
	UpdateNewLine();
	//Initialiser l'élément courant
	XercesString tmpXercesString(ElementName);
	fNodeBeingWritten = tmpXercesString;
	//Ecrire le début de l'élément courant
	printIndent(flevel);
	*fFormatter << XMLFormatter::NoEscapes << gXML_Begin << ElementName << gXML_separator1 << gXML_xmlns << gXML_separator2 << NS << gXML_separator3;
	return true;
}

bool DOMLSSerializerSymu::writeStartElement(const XMLCh * PrefixName, const XMLCh * ElementName, const XMLCh * NS)
{
	if (!fNodeBeingWritten.isEmpty())
	{
		//Fermer l'élément précédent
		fpile.push_back(fNodeBeingWritten);
		*fFormatter << XMLFormatter::NoEscapes << gXML_EndSimple;
		printNewLine();
		flevel++;
	}
	UpdateNewLine();
	//Initialiser l'élément courant
	XercesString tmpXercesString(ElementName);
	fNodeBeingWritten = tmpXercesString;
	//Ecrire le début de l'élément courant
	printIndent(flevel);
	*fFormatter << XMLFormatter::NoEscapes << gXML_Begin << PrefixName << gXML_prefix << ElementName << gXML_separator1 << gXML_xmlns << gXML_prefix << PrefixName << gXML_separator2 << NS << gXML_separator3;
	return true;
}

bool DOMLSSerializerSymu::writeAttributeString(const XMLCh * AttrName, 
											   const XMLCh * AttrValue)
{
    *fFormatter << XMLFormatter::CharEscapes << gXML_separator1 << AttrName << gXML_separator2 << AttrValue << gXML_separator3;
	return true;
}

bool DOMLSSerializerSymu::writeText(const XMLCh * value)
{
    *fFormatter << XMLFormatter::CharEscapes << gXML_EndSimple << value << gXML_BeginClose << fNodeBeingWritten;
    m_bTextNodeWritten = true;
    return true;
}

bool DOMLSSerializerSymu::writeEndElement()
{
	if (!fNodeBeingWritten.isEmpty())
	{
		//Fermer l'élément courant
        if(m_bTextNodeWritten)
        {
            *fFormatter << XMLFormatter::NoEscapes << gXML_EndSimple;
            m_bTextNodeWritten = false;
        }
        else
        {
		    *fFormatter << XMLFormatter::NoEscapes << gXML_End;
        }
		printNewLine();
		//if (flevel > 0) flevel--;
	}
	else
	{
		if (fpile.size() != 0)
		{//dépilé le dernier noeud empilé
			UpdateNewLine();
			fNodeBeingWritten = fpile.back();
			fpile.pop_back();
			if (flevel > 0) flevel--;
			printIndent(flevel);
			*fFormatter << XMLFormatter::NoEscapes << gXML_BeginClose << (const XMLCh*)fNodeBeingWritten << gXML_EndSimple;
			printNewLine();
		}
		//else
		//{//Ne rien faire : erreur
		//}
	}
	fNodeBeingWritten.Empty();

	return true;
}

bool DOMLSSerializerSymu::writeEndElement(const XMLCh * endEltName, bool emptyNode)
{
    if(emptyNode)
    {
        if(m_bTextNodeWritten)
        {
            *fFormatter << XMLFormatter::NoEscapes << gXML_EndSimple;
             m_bTextNodeWritten = false;
        }
        else
        {
            *fFormatter << XMLFormatter::NoEscapes << gXML_End;
        }
		printNewLine();
    }
    else
    {
        printIndent(flevel);
        *fFormatter << XMLFormatter::NoEscapes << gXML_BeginClose << (const XMLCh*)endEltName << gXML_EndSimple;
        printNewLine();
        flevel--;
    }

    return true;
}

bool DOMLSSerializerSymu::writeNode2(const DOMNode*        nodeToWrite)
{
	bool bNodeBeingWritten;

	bNodeBeingWritten = !fNodeBeingWritten.isEmpty();
	if (bNodeBeingWritten)
	{//Un élément courant est ouvert, écrire la fermeture de l'entête...
		*fFormatter << XMLFormatter::NoEscapes << gXML_EndSimple;
		flevel++;
		//Préparer la fermeture de l'élément courant
		fpile.push_back(fNodeBeingWritten);
		fNodeBeingWritten.Empty();
	}

	processNode(nodeToWrite, flevel);//On part sur le principe que la 1ere indentation est gérée par processNode
	m_bNewLineIsLacking = true;
	return true;
}
#include <iostream>
bool DOMLSSerializerSymu::writeNode(const DOMNode*        nodeToWrite)
{
    if (nodeToWrite->getNodeType() == DOMNode::TEXT_NODE)
    {
        // On ignore les textnodes dus aux éléments de type "any" (pramètres des fonctions des briques de régulation).
        // pas terrible mais pas trouvé de meilleure solution... peut poser problème si on doit pouvoir écrire des textnodes avec LF en premier charactère,
        // mais peu probable que ca arrive : pour l'instant les seuls textnodes qu'on écrit sont ceux du fichier pour Sirane et ne commencent pas par un LF.
        if(nodeToWrite->getTextContent()[0] != chLF)
        {
            *fFormatter << XMLFormatter::NoEscapes << gXML_EndSimple << nodeToWrite->getTextContent() << gXML_BeginClose << fNodeBeingWritten;
            m_bTextNodeWritten = true;
            return true;
        }
    }

    m_bTextNodeWritten = false;

	if (nodeToWrite->getNodeType() != DOMNode::ELEMENT_NODE)
		return false;

	const XMLCh * name = nodeToWrite->getNodeName();
	this->writeStartElement(name);


	DOMNamedNodeMap * attrList = nodeToWrite->getAttributes();
	
	XMLSize_t attrCount = attrList->getLength();

	for (XMLSize_t i = 0; i<attrCount; i++)
	{
		DOMAttr * attr = (DOMAttr*)attrList->item(i);
		
		this->writeAttributeString(attr->getNodeName(), attr->getNodeValue());
	}

	DOMNodeList * childList = nodeToWrite->getChildNodes();
	XMLSize_t childCount = childList->getLength();

	for (XMLSize_t j = 0; j<childCount; j++)
	{
		writeNode(childList->item(j));
	}

	this->writeEndElement();

	return true;
}

void DOMLSSerializerSymu::UpdateNewLine()
{
	if (m_bNewLineIsLacking)
	{
		m_bNewLineIsLacking = false;
		printNewLine();
	}
}

