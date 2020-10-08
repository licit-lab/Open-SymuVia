#include "XMLStringUtils.h"

#include <xercesc/dom/DOMAttr.hpp>
#include <xercesc/dom/DOMElement.hpp>

#include <boost/date_time/posix_time/time_parsers.hpp>

#include <limits>

XERCES_CPP_NAMESPACE_USE

using namespace SymuMaster;

bool XMLStringUtils::GetAttributeValue(DOMNode * pNode, const std::string & attributeName, std::string & attributeValue)
{
    DOMAttr * pAttr = ((DOMElement*)pNode)->getAttributeNode(XS(attributeName.c_str()));

    if (!pAttr)
        return false;

    attributeValue = US(pAttr->getValue());

    return true;
}

bool XMLStringUtils::GetAttributeValue(DOMNode * pNode, const std::string & attributeName, bool & attributeValue)
{
    DOMAttr * pAttr = ((DOMElement*)pNode)->getAttributeNode(XS(attributeName.c_str()));

    if (!pAttr)
        return false;

    const std::string & attrStrValue = US(pAttr->getValue());

	attributeValue = (attrStrValue == "1" || attrStrValue == "true");

	return true;
}

bool XMLStringUtils::GetAttributeValue(DOMNode * pNode, const std::string & attributeName, int & attributeValue)
{
    DOMAttr * pAttr = ((DOMElement*)pNode)->getAttributeNode(XS(attributeName.c_str()));

    if (!pAttr)
        return false;

    attributeValue = atoi(US(pAttr->getValue()).c_str());

    return true;
}

bool XMLStringUtils::GetAttributeValue(DOMNode * pNode, const std::string & attributeName, double & attributeValue)
{
    DOMAttr * pAttr = ((DOMElement*)pNode)->getAttributeNode(XS(attributeName.c_str()));

    if (!pAttr)
        return false;

    const std::string & attrStrValue = US(pAttr->getValue());

    // patch pour gestion de la valeur INF (XSD indique INF alors que atof ne connait que +INF)
    if (!attrStrValue.compare("INF"))
    {
        attributeValue = std::numeric_limits<double>::infinity();
    }
    // cas général
    else
    {
        attributeValue = atof(attrStrValue.c_str());
    }

    return true;
}

bool XMLStringUtils::GetAttributeValue(XERCES_CPP_NAMESPACE::DOMNode * pNode, const std::string & attributeName, boost::posix_time::time_duration & attributeValue)
{
    DOMAttr * pAttr = ((DOMElement*)pNode)->getAttributeNode(XS(attributeName.c_str()));

    if (!pAttr)
        return false;

    const std::string & attrStrValue = US(pAttr->getValue());

    attributeValue = boost::posix_time::duration_from_string(attrStrValue);

    return !attributeValue.is_not_a_date_time();
}

