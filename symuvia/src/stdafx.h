// stdafx.h : fichier Include pour les fichiers Include système standard,
// ou les fichiers Include spécifiques aux projets qui sont utilisés fréquemment,
// et sont rarement modifiés
//

#ifdef WIN32

#pragma once

// Modifiez les définitions suivantes si vous devez cibler une plate-forme avant celles spécifiées ci-dessous.
// Reportez-vous à MSDN pour obtenir les dernières informations sur les valeurs correspondantes pour les différentes plates-formes.
#ifndef WINVER				// Autorise l'utilisation des fonctionnalités spécifiques à Windows XP ou version ultérieure.
#define WINVER 0x0501		// Attribuez la valeur appropriée à cet élément pour cibler d'autres versions de Windows.
#endif

#ifndef _WIN32_WINNT		// Autorise l'utilisation des fonctionnalités spécifiques à Windows XP ou version ultérieure.  
#if _MSC_VER >= 1900
#define _WIN32_WINNT 0x0601	// pour compiler sous visual 2015, upgrade de la version de boost en 1.66 qui ne cible plus par défaut la compatilbité avec windows XP (plus maintenu). SymuVia ne tournera donc plus sous windows XP avec cette version du compilateur
#else
#define _WIN32_WINNT 0x0501	// Attribuez la valeur appropriée à cet élément pour cibler d'autres versions de Windows.
#endif
#endif						

#ifndef _WIN32_WINDOWS		// Autorise l'utilisation des fonctionnalités spécifiques à Windows 98 ou version ultérieure.
#define _WIN32_WINDOWS 0x0410 // Attribuez la valeur appropriée à cet élément pour cibler Windows Me ou version ultérieure.
#endif

#ifndef _WIN32_IE			// Autorise l'utilisation des fonctionnalités spécifiques à Internet Explorer 6.0 ou version ultérieure.
#define _WIN32_IE 0x0600	// Attribuez la valeur appropriée à cet élément pour cibler d'autres versions d'Internet Explorer.
#endif

#define WIN32_LEAN_AND_MEAN		// Exclure les en-têtes Windows rarement utilisés
// Fichiers d'en-tête Windows :
#include <windows.h>



// TODO : faites référence ici aux en-têtes supplémentaires nécessaires au programme



#ifdef _DEBUG
#pragma warning(disable : 4996) // Pour suppression d'un warning lié à boost 1.58 en debug
#endif
#ifndef _WIN64
#pragma warning( disable : 4018 ) // Pour suppression d'un warning lié à boost 1.58 en 32 bits avec la serialisation des vector<bool>
#endif
#include <boost/serialization/vector.hpp>
#ifndef _WIN64
#pragma warning( default : 4018 )
#endif
#ifdef _DEBUG
#pragma warning(default : 4996)
#endif

#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

#pragma warning( disable : 4244 )
#include <boost/python.hpp>
#pragma warning( default : 4244 )

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemoryManager.hpp>
#include <xercesc/framework/XMLPScanToken.hpp>
#include <xercesc/util/XercesVersion.hpp>
#include <xercesc/util/Xerces_autoconf_config.hpp>
#include <xercesc/util/TranscodingException.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMAttr.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMXPathResult.hpp>
#include <xercesc/dom/DOMPSVITypeInfo.hpp>
#include <xercesc/dom/DOMLSSerializer.hpp>
#include <xercesc/dom/DOMLSInput.hpp>
#include <xercesc/dom/DOMLSParser.hpp>
#include <xercesc/dom/DOMLocator.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/dom/impl/DOMElementImpl.hpp>
#include <xercesc/dom/impl/DOMDocumentImpl.hpp>
#include <xercesc/dom/impl/DOMLSSerializerImpl.hpp>
#include <xercesc/dom/impl/DOMLSOutputImpl.hpp>
#include <xercesc/sax/SAXException.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/parsers/SAXParser.hpp>

#pragma warning(disable: 4005)
#include "ogr_spatialref.h"
#pragma warning(default: 4005)


#include <vector>
#include <list>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>

#endif // WIN32

