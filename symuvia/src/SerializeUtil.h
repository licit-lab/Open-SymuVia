#pragma once
#ifndef SerializeUtilH
#define SerializeUtilH

//rmq. positionnement de cet include avant le reste sinon erreur de compilation sou MAC OS X
#pragma warning( disable : 4244 )
#include <boost/python/dict.hpp>
#pragma warning( default : 4244 )

#define XML_MODE

#include <xercesc/util/Xerces_autoconf_config.hpp>
#include <xercesc/util/XercesVersion.hpp>

#include <boost/serialization/nvp.hpp>

#include <string>
#include <deque>

#ifdef MICROSOFT
#pragma warning(disable : 4996)
#endif

namespace XERCES_CPP_NAMESPACE {
    class DOMDocument;
}

class XMLUtil;
class Reseau;


#ifndef XML_MODE
#undef BOOST_SERIALIZATION_NVP
#define BOOST_SERIALIZATION_NVP(n)	n
#define BOOST_SERIALIZATION_NVP2(name, value)	value
#else
#define BOOST_SERIALIZATION_NVP2(name, value)	boost::serialization::make_nvp(name, value)
#endif

static const std::string NullPtr = "@NULL@";

////////////////////////////////////////////////////////////////////////////////

// Inscription des types et casts
template <class Archive>
void registerClasses(Archive & ar);

// enregistrer une collection dans un fichier
template <class Archive, class Objet>
void saveIntoFile(const char * nom, const Objet& d, const char* file);

// charger une collection depuis un fichier
template <class Archive, class Objet>
void getFromFile(const char * nom, Objet& d, const char* file);

////////////////////////////////////////////////////////////////////////////////

// serialize un tableau 1D
template <class Archive, class Objet>
void SerialiseTab(Archive & ar, const char * nom, Objet * &tab, int &nbItem)
{
    char buf[512];

    // cas de la sauvegarde d'un tableau vide
    if(!tab && Archive::is_saving::value)
    {
	    snprintf(buf, sizeof(buf), "%s_count", nom);
        int zero = 0;
	    ar & BOOST_SERIALIZATION_NVP2(buf, zero);
        return;
    }

	snprintf(buf, sizeof(buf), "%s_count", nom);
	ar & BOOST_SERIALIZATION_NVP2(buf, nbItem);
    if(Archive::is_loading::value)
    {
        if (tab != NULL)
	    {
		    delete [] tab;
	    }
	    tab = new Objet[nbItem];
    }
	for (int i=0; i<nbItem; i++)
	{
		snprintf(buf, sizeof(buf), "%s_%d", nom, i);
		ar & BOOST_SERIALIZATION_NVP2(buf, tab[i]);
	}
}

// serialize un tableau 2D
template <class Archive, class Objet>
void SerialiseTab2D(Archive & ar, const char * nom, Objet ** &tab, int &nbItemi, int &nbItemj);

// serialize un deque de pointeurs de type de base
template <class Archive, class Object>
void SerialiseDeque2D(Archive & ar, const char * nom, std::deque<Object*> &pDeque, size_t nbItemj);

// serialize un pointeur de DOMDocument XERCES
template <class Archive>
void SerialiseDOMDocument(Archive & ar, const char * nom, const XMLCh* rootName, XERCES_CPP_NAMESPACE::DOMDocument* &pDoc, XMLUtil * pXMLUtil);

////////////////////////////////////////////////////////////////////////////////
/// Implémentation de la sérialisation avec boost des structures DOM de xerces
////////////////////////////////////////////////////////////////////////////////

namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive & ar, XERCES_CPP_NAMESPACE::DOMDocument &doc, const unsigned int version);

    } // namespace serialization
} // namespace boost


////////////////////////////////////////////////////////////////////////////////
/// Implémentation de la sérialisation avec boost des boost::python::dict
////////////////////////////////////////////////////////////////////////////////

namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive & ar, boost::python::dict &obj, const unsigned int version);
    } // namespace serialization
} // namespace boost



class SerializeUtil
{
public:
	SerializeUtil(void);
	~SerializeUtil(void);

public:
	static void Save(Reseau * pNetwork, char * sFilePath);
    static void Load(Reseau * pNetwork, char * sFilePath);
};

#endif // SerializeUtilH