#pragma once

#ifndef _SYMUMASTER_XMLSTRINGUTILS_
#define _SYMUMASTER_XMLSTRINGUTILS_

#include <boost/shared_ptr.hpp>
#include <xercesc/util/XMLString.hpp>

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
}

namespace boost {
    namespace posix_time {
        class time_duration;
    }
}

namespace SymuMaster {

    template< class CharType >
    class ZStr      // Zero-terminated string.
    {
    private:
        boost::shared_ptr< CharType >   myArray;

    public:
        ZStr(CharType const* s, void(*deleter)(CharType*))
            : myArray(const_cast<CharType*>(s), deleter)
        {
            assert(deleter != 0);
        }

        CharType const* ptr() const { return myArray.get(); }
    };

    typedef ::XMLCh     Utf16Char;

    inline void dispose(Utf16Char* p) { xercesc::XMLString::release(&p); }
    inline void dispose(char* p)      { xercesc::XMLString::release(&p); }

    inline ZStr< Utf16Char > uStr(char const* s)
    {
        return ZStr< Utf16Char >(
            xercesc::XMLString::transcode(s), &dispose
            );
    }

    inline ZStr< char > cStr(Utf16Char const* s)
    {
        return ZStr< char >(
            xercesc::XMLString::transcode(s), &dispose
            );
    }


#define XS(x) uStr(x).ptr()
#define US(x) std::string(cStr(x).ptr())

    class XMLStringUtils {

    public:
        static bool GetAttributeValue(XERCES_CPP_NAMESPACE::DOMNode * pNode, const std::string & attributeName, std::string & attributeValue);
        static bool GetAttributeValue(XERCES_CPP_NAMESPACE::DOMNode * pNode, const std::string & attributeName, bool & attributeValue);
        static bool GetAttributeValue(XERCES_CPP_NAMESPACE::DOMNode * pNode, const std::string & attributeName, int & attributeValue);
        static bool GetAttributeValue(XERCES_CPP_NAMESPACE::DOMNode * pNode, const std::string & attributeName, double & attributeValue);
        static bool GetAttributeValue(XERCES_CPP_NAMESPACE::DOMNode * pNode, const std::string & attributeName, boost::posix_time::time_duration & attributeValue);
    };

}

#endif // _SYMUMASTER_XMLSTRINGUTILS_
