#pragma once

#include <xercesc/dom/impl/DOMDocumentImpl.hpp>

XERCES_CPP_NAMESPACE_USE

class DOMDocumentSymuvia :
	public DOMDocumentImpl
{
public:
    DOMDocumentSymuvia(DOMImplementation* domImpl, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    DOMDocumentSymuvia(const XMLCh*     namespaceURI,     //DOM Level 2
                    const XMLCh*     qualifiedName,
                    DOMDocumentType* doctype,
                    DOMImplementation* domImpl,
                    MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	virtual ~DOMDocumentSymuvia(void);

	virtual DOMElement*          createDeclaration();

	virtual void release();

private:
	void release(DOMNode * node);

};
