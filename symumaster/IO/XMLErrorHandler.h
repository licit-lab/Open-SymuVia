#pragma once

#ifndef _SYMUMASTER_XMLERRORHANDLER_
#define _SYMUMASTER_XMLERRORHANDLER_

#include <xercesc/sax/ErrorHandler.hpp>

#include "SymuMasterExports.h"

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF XMLErrorHandler : public XERCES_CPP_NAMESPACE::ErrorHandler {

    public:
        void warning(const xercesc::SAXParseException& ex);
        void error(const xercesc::SAXParseException& ex);
        void fatalError(const xercesc::SAXParseException& ex);
        void resetErrors();
    private:
        void reportParseException(const xercesc::SAXParseException& ex);
    };

}

#endif // _SYMUMASTER_XMLERRORHANDLER_
