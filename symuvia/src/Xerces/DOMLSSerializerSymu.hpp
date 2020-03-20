#pragma once
#if !defined(XERCESC_INCLUDE_GUARD_DOMLSSERIALIZERSYMU_HPP)
#define XERCESC_INCLUDE_GUARD_DOMLSSERIALIZERSYMU_HPP

/*
 * $Id: DOMLSSerializerSymu.hpp 695856 2008-09-16 12:52:32Z borisk $
 */

#include "XercesString.h"
#include <xercesc/dom/impl/DOMLSSerializerImpl.hpp>
#include <xercesc/dom/impl/DOMLSOutputImpl.hpp>
#include <deque>
#include <string>

XERCES_CPP_NAMESPACE_USE;

class DOMStringListImpl;

class DOMLSSerializerSymu : public DOMLSSerializerImpl
{

public:

    /** @name Constructor and Destructor */
    //@{

    /**
     * Constructor.
     */
	DOMLSSerializerSymu(XERCES_CPP_NAMESPACE::MemoryManager* const manager = XERCES_CPP_NAMESPACE::XMLPlatformUtils::fgMemoryManager);

    /**
     * Destructor.
     */
    ~DOMLSSerializerSymu();
    //@}

	/**
	 * Fonctions XmlReader
	 */

	//Initialisation du fichier XML à l'aide du chemin du fichier, de la version XML, de l'encodage, et du paramètre "standalone" optionnel
	bool initXmlWriter(const char * destinationFile,
						const XMLCh * Version,
						const XMLCh * Encoding,
						const XMLCh * Standalone = NULL);

	//Initialisation du fichier XML à l'aide d'un objet "DOMLSOutput", de la version XML, de l'encodage, et du paramètre "standalone" optionnel
	bool initXmlWriter(DOMLSOutput* const    destination,
						const XMLCh * Version,
						const XMLCh * Encoding,
						const XMLCh * Standalone = NULL);

	//Fermeture du fichier XML
	void close();

	//Positionne l'indentation
	void setIndent(bool bIndent);

	//Positionne la séquence de fin de ligne
	void setNewLineSeq(const XMLCh * Sequence);

	//Ecrit la déclaration XML
	bool writeXmlDeclaration();

	//Ecrit le début d'un élément XML
	bool writeStartElement(const XMLCh * ElementName);
	bool writeStartElement(const XMLCh * ElementName, const XMLCh * NS);
	bool writeStartElement(const XMLCh * Prefix, const XMLCh * ElementName, const XMLCh * NS);
						   
	//Ecrit un attribut avec sa valeur
	bool writeAttributeString(const XMLCh * AttrName, 
							 const XMLCh * AttrValue);

    //Ecrit le contenu d'un noeud texte
    bool writeText(const XMLCh * value);
							 
	//Ferme un élément XML
	bool writeEndElement();

    //Ferme un élément XML arbitraire
    bool writeEndElement(const XMLCh * endEltName, bool emptyNode = false);

	//Ecrit un noeud XML avec toute son arborescence
	bool writeNode(const XERCES_CPP_NAMESPACE::DOMNode*        nodeToWrite);

	//Renvoie si le flux XML est ouvert
	bool isOpen() { return m_bIsOpen; }

	//Pour corriger un problème de Xerces-C
    virtual bool write(const XERCES_CPP_NAMESPACE::DOMNode* nodeToWrite, DOMLSOutput* const    destination);

    // pour reprise des résultats après reprise d'un snapshot
    virtual bool write(std::string stringToWrite);

    virtual void setCurrentIndentLevel(int level) {flevel = level;}

    std::string getTargetFileName() const;

private:
	//Ecrit un noeud XML avec toute son arborescence
	bool writeNode2(const XERCES_CPP_NAMESPACE::DOMNode*        nodeToWrite);

private:
	bool m_bIsOpen;													//Signal si le fichier XML est ouvert
	XMLFormatTarget* pTarget;										//Objet de format XML
	const XMLCh * fStandalone;										//Information "standalone"
	unsigned int flevel;														//Niveau d'indentation
	std::deque<XercesString, std::allocator<XercesString>> fpile;	//Pile des Element XML
	XercesString fNodeBeingWritten;									//Dernier élément XML en traitement
	DOMLSOutputImpl * pOutputFile;									//Objet de sortie XML (fichier)
	bool m_bNewLineIsLacking;										//Une fin de ligne est manquante
    bool m_bTextNodeWritten;                                        // un noeud en mode texte a été écrit
    std::string m_strOutputFile;                                    // nom du fichier de sortie

private:
	void UpdateNewLine();											//Régularise les fins de lignes manquantes
};


#endif
