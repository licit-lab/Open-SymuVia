#pragma once
#ifndef _XERCESSTRING_H
#define _XERCESSTRING_H

/**************************************************************************
XercesString.h
***************************************************************************/

#include <xercesc/util/XMLString.hpp>

class XercesString
{
public:
	XMLCh *_wstr;
	XercesString() : _wstr(0L) { };
	XercesString(const char *str);
//	XercesString(XMLCh *wstr);
	XercesString(const XMLCh *wstr);
	XercesString(const XercesString &copy);
	~XercesString();
	bool append(const XMLCh *tail);
	bool erase(const XMLCh *head, const XMLCh *tail);
	const XMLCh* begin() const;
	const XMLCh* end() const;
	XMLSize_t size() const;
	XMLCh & operator [] (const int i);
	const XMLCh operator [] (const int i) const;
	operator const XMLCh * () const { return _wstr; };
	bool isEmpty() { return _wstr == 0L; };
	void Empty() { if (_wstr) { XERCES_CPP_NAMESPACE::XMLString::release(&_wstr); _wstr = 0L; } };
	XercesString& operator= (XercesString& xstr);
};

#define XX(str)	((const XMLCh*) XercesString(str))

#endif
