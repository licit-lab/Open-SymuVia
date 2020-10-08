#pragma once

#ifndef _SYMUMASTER_XMLLOADER_
#define _SYMUMASTER_XMLLOADER_

#include "SymuMasterExports.h"

#include <string>

#include <xercesc/util/XercesVersion.hpp>

namespace XERCES_CPP_NAMESPACE {
    class DOMDocument;
    class XercesDOMParser;
}

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF XMLLoader {

    public:
        XMLLoader();
        virtual ~XMLLoader();

        bool Load(const std::string & xmlFilePath, const std::string & xsdFilePath);

        XERCES_CPP_NAMESPACE::DOMDocument * getDocument();

    private:

        XERCES_CPP_NAMESPACE::XercesDOMParser * m_pDOMParser;
    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_XMLLOADER_
