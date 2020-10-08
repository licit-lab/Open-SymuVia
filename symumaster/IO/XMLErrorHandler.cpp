#include "XMLErrorHandler.h"

#include "XMLStringUtils.h"

#include <xercesc/sax/SAXParseException.hpp>

#include <boost/log/trivial.hpp>

using namespace SymuMaster;

void XMLErrorHandler::warning(const xercesc::SAXParseException& ex)
{
    reportParseException(ex);
}

void XMLErrorHandler::error(const xercesc::SAXParseException& ex)
{
    reportParseException(ex);
}

void XMLErrorHandler::fatalError(const xercesc::SAXParseException& ex)
{
    reportParseException(ex);
}

void XMLErrorHandler::resetErrors()
{
}

void XMLErrorHandler::reportParseException(const xercesc::SAXParseException& ex)
{
    BOOST_LOG_TRIVIAL(error) << US(ex.getMessage()) << " at line " << ex.getLineNumber() << " column " << ex.getColumnNumber();
}