#pragma once
#ifndef toolsH
#define toolsH

#include "symUtil.h"
#include "TimeUtil.h"

#include <xercesc/util/XercesVersion.hpp>

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

#include "SerializeUtil.h"

#include <deque>
#include <list>
#include <vector>
#include <sstream>
#include <map>

class Entree;
class Sortie;
class Reseau;
class Troncon;
class Tuyau;
class TuyauMicro;
class TypeVehicule;
class Connexion;
class Voie;
class Logger;

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
}

template<class T>
class LessPtr 
{
public:
	bool operator()(const T *a, const T *b) const
	{	
	  return (a->GetLabel() < b->GetLabel());	  	
	}
};

template<class T>
class LessOriginPtr 
{
public:
	bool operator()(const T *a, const T *b) const
	{	
	  return (a->GetOutputID() < b->GetOutputID());	  	
	}
};

template<class T>
class LessConnexionPtr 
{
public:
	bool operator()(const T *a, const T *b) const
	{	
	  return (a->GetID() < b->GetID());	  	
	}
};

template<class T>
class LessVehiculePtr 
{
public:
	bool operator()(const boost::shared_ptr<T> a, const boost::shared_ptr<T> b) const
	{	
	  return (a->GetID() < b->GetID());	  	
	}
};

template<class T>
class LessPairPtrIDRef
{
public:
	bool operator()(const T &a, const T &b) const
	{
		if (a.first < b.first)
		{
			return true;
		}
		else if (a.first == b.first)
		{
			if (a.second != NULL && b.second != NULL)
			{
				return a.second->GetID() < b.second->GetID();
			}
			else if (a.second == NULL && b.second != NULL)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
};

// Foncteur servant à libérer un pointeur - applicable à n'importe quel type
struct Delete 
{ 
   template <class T> void operator ()(T*& p) const 
   { 
      delete p;
      p = NULL;
   } 
}; 

// Structure de définition d'une plage temporelle
class PlageTemporelle {
public:

    double m_Debut;   // début (en secondes) de la plage temporelle
    double m_Fin;     // fin (en secondes) de la plage temporelle
    std::string m_ID; // Identifiant de la plage temporelle

    static PlageTemporelle * Set(double dStart, double dEnd , std::vector<PlageTemporelle * > & plages, double dDebutSimu )
    {
        // look for existing TimeRange with dStart and dEnd
        std::vector<PlageTemporelle *>::iterator it = plages.begin();
       
        while ( it != plages.end() )
        {
            if( (*it)->m_Debut == dStart- dDebutSimu && (*it)->m_Fin == dEnd -dDebutSimu)
            { 
           
                return (*it);
            }
            it++;
           
        }
        // create the temporal range
        PlageTemporelle* plage = new PlageTemporelle();
        plage->m_Debut = dStart -dDebutSimu;
        plage->m_Fin = dEnd - dDebutSimu;
        std::ostringstream str; // create name from end and start
        str <<STime::FromSecond(dStart).ToString() <<"_"<<STime::FromSecond(dEnd).ToString();
        plage->m_ID = str.str();
        // add to list of temporal Range
        plages.push_back(plage);
        return plage;
    }
    virtual ~PlageTemporelle()
    {
        int i = 0;
    }
     bool operator <(const PlageTemporelle& rhs) const
    {
        return m_ID < rhs.m_ID;
    }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// définition d'un type double qui peut être sérialisé dans un shared_ptr
BOOST_STRONG_TYPEDEF(double, tracked_double)
template<class Archive>
void serialize(Archive &ar, tracked_double& td, const unsigned int version){
    ar & BOOST_SERIALIZATION_NVP2("tracked_double",static_cast<double&>(td));
} 

// définition d'un type bool qui peut être sérialisé dans un shared_ptr
BOOST_STRONG_TYPEDEF(bool, tracked_bool)
template<class Archive>
void serialize(Archive &ar, tracked_bool& tb, const unsigned int version){
    ar & BOOST_SERIALIZATION_NVP2("tracked_bool",static_cast<bool&>(tb));
} 


// Structure template de stockage des variantes temporelles lorsque le caractère des variantes est exprimées à l'aide d'une durée d'application
// ou d'une plage temporelle (l'un ou l'autre exclusif)
template<class T> struct TimeVariation 
{
public:
    TimeVariation() : m_pPlage(NULL) { m_dbPeriod = 0; };
    double  m_dbPeriod;   // Durée de la variante
    PlageTemporelle* m_pPlage; // Plage temporelle associée
    boost::shared_ptr<T>    m_pData;      // Données

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
        ar & BOOST_SERIALIZATION_NVP(m_dbPeriod);
        ar & BOOST_SERIALIZATION_NVP(m_pPlage);
        ar & BOOST_SERIALIZATION_NVP(m_pData);
    }
};

// Structure template de stockage des variantes temporelles lorsque le caractère temporel des variantes est exprimées à l'aide d'une heure de début d'aplication
template<class T> struct TimeVariationEx 
{
	STime                   m_dtDebut;        // Heure de début de la variante
    boost::shared_ptr<T>    m_pData;          // Données

public:
    TimeVariationEx() {};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
        ar & BOOST_SERIALIZATION_NVP(m_dtDebut);
        ar & BOOST_SERIALIZATION_NVP(m_pData);
    }
};

bool CheckPlagesTemporellesEx(double dbDureeSimu, const std::vector<PlageTemporelle*> & plages);

// Fonctions template de manipulations des listes des variantes temporelles
// REMARQUES : le code des fonctions Template est ici sinon elles ne 
// sont pas reconnues à l'édition des liens (pas supporté par le compilo)



//================================================================
    template <class T> void AddVariation
//----------------------------------------------------------------
// Fonction  : Ajoute une variante à la liste des variantes
// Remarques :
// Version du: 20/06/2008
// Historique: 20/06/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(   
    double                          dbPeriod,   // Période de validité de la variante
    boost::shared_ptr<T>            pData,      // Variante
    std::deque<TimeVariation<T>>    *pLstTV     // Liste des variantes
)
{
    TimeVariation<T> TV;

    if(!pLstTV)
        return;

    if(dbPeriod<0)
        return;

    TV.m_dbPeriod = dbPeriod;
    TV.m_pData = pData;

    pLstTV->push_back(TV);
};

//================================================================
    template <class T> void AddVariation
//----------------------------------------------------------------
// Fonction  : Ajoute une variante à la liste des variantes
// Remarques :
// Version du: 06/09/2011
// Historique: 06/09/2011 (O.Tonck - Ipsis)
//             Création
//================================================================
(   
    PlageTemporelle                 *pPlage,    // Plage temporelle associée à la variante
    boost::shared_ptr<T>            pData,      // Variante
    std::deque<TimeVariation<T>>    *pLstTV     // Liste des variantes
)
{
    TimeVariation<T> TV;

    if(!pLstTV)
        return;

    if(pPlage == NULL)
        return;

    TV.m_pPlage = pPlage;
    TV.m_pData = pData;

    pLstTV->push_back(TV);
};


//================================================================
    template <class T> void InsertVariation
//----------------------------------------------------------------
// Fonction  : Insertion d'une variante à la liste des variantes
// Remarques :
// Version du: 27/07/2011
// Historique: 27/07/2011 (O. Tonck - IPSIS)
//             Création
//================================================================
(   
    double                          dbStartTime,    // Début de validité de la variante
    double                          dbEndTime,      // Fin de validité de la variante
    boost::shared_ptr<T>            pData,          // Variante
    std::deque<TimeVariation<T>>    *pLstTV         // Liste des variantes
)
{
    // **************************************************
    // Modification des variantes existantes si besoin
    // **************************************************

    // recherche de la première variante impactée
    double dbCumul = 0.0;
    typename std::deque<TimeVariation<T>>::iterator iter = pLstTV->begin();
    while(iter != pLstTV->end())
    {
        double currentDuree = (*iter).m_dbPeriod;

        // séparation en deux en cas de variante à cheval sur la nouvelle variante (début de période)
        if(dbStartTime > dbCumul && dbStartTime < (*iter).m_dbPeriod + dbCumul)
        {
            // modif de la variation existante
            (*iter).m_dbPeriod = dbStartTime - dbCumul;

            // nouvelle variation
            TimeVariation<T> tv;
            tv.m_pData = boost::shared_ptr<T>(new T(*pData));
            tv.m_dbPeriod = (*iter).m_dbPeriod + dbCumul - dbStartTime;
            iter = pLstTV->insert(iter + 1,  tv);
        }
        // cas des variantes intermédiaires
        else if(dbCumul >= dbStartTime && dbCumul+(*iter).m_dbPeriod <= dbEndTime) 
        {
            (*iter).m_pData.reset();
            (*iter).m_pData = boost::shared_ptr<T>(new T(*pData));
        }
        // séparation en deux en cas de variante à cheval sur la nouvelle variante (fin de période)
        else if(dbEndTime > dbCumul && dbEndTime < (*iter).m_dbPeriod + dbCumul)
        {
            // modif de la variation existante
            T oldValue = *((*iter).m_pData.get());
            (*iter).m_dbPeriod = dbEndTime - dbCumul;
            (*iter).m_pData = boost::shared_ptr<T>(new T(*pData));

            // nouvelle variation
            TimeVariation<T> tv;
            tv.m_pData = boost::shared_ptr<T>(new T(oldValue));
            tv.m_dbPeriod = (*iter).m_dbPeriod + dbCumul - dbEndTime;
            iter = pLstTV->insert(iter + 1,  tv);
        }

        dbCumul += currentDuree;
        iter++;
    }

    // *************************************************************************
    // Ajout de la variante sur la portion temporelle non précédemment couverte
    // *************************************************************************
    if(dbEndTime > dbCumul)
    {
        TimeVariation<T> tv;
        tv.m_pData = boost::shared_ptr<T>(new T(*pData));
        tv.m_dbPeriod = dbEndTime - dbCumul;
        pLstTV->push_back(tv);
    }
};

//================================================================
    template <class T> void EraseListOfVariation
//----------------------------------------------------------------
// Fonction  : Suppression de tous les éléments de la liste des 
//             variantes temporelles
// Remarques :
// Version du: 20/06/2008
// Historique: 20/06/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
( 
    std::deque<TimeVariation<T>> *pLstTV
)
{
    TimeVariation<T>    TV;

    if(!pLstTV)
        return;
    
    for(int i=0; i<(int)pLstTV->size(); i++)
    {
        TV = pLstTV->at(i); 
        TV.m_pData.reset();
    }

    pLstTV->clear();
}

//================================================================
    template <class T> void EraseVariation
//----------------------------------------------------------------
// Fonction  : Suppression les variations à partir d'un certain
//             instant
// Remarques :
// Version du: 23/06/2008
// Historique: 23/06/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
( 
    std::deque<TimeVariation<T>> *pLstTV,
    double  dbTime,
    double  dbLag
)
{
    int                 nIndex = 0;
    double              dbCumTime = - dbLag;
    double              dbCumTimePre = 0;
    int                 i = 0;
    TimeVariation<T>    TV;
    typename std::deque<TimeVariation<T>>::iterator iTV;
    double              dbPeriod;
    boost::shared_ptr<T>    pData;

    if(!pLstTV)
        return;

    if(dbTime<0)
        return;

    if(pLstTV->size()==0)
        return;

    // on commence par supprimer les variantes correspondantes à des plages temporelles
    iTV = pLstTV->begin();
    while(iTV != pLstTV->end())
    {
        if((*iTV).m_pPlage != NULL)
        {
            iTV = pLstTV->erase(iTV);
        }
        else
        {
            iTV++;
        }
    }

    TV = pLstTV->at(0);   
    dbCumTime += TV.m_dbPeriod;

    while(dbCumTime < dbTime && nIndex < (int)pLstTV->size() )
    {        
        TV = pLstTV->at(++nIndex);
        dbCumTimePre = dbCumTime;
        dbCumTime += TV.m_dbPeriod;
    }    
    
    if(nIndex < (int)pLstTV->size())
    {
        // Modification de la durée de la dernière variante afin que la nouvelle 
        // variante soit pris en compte dès le pas de temps courant
        dbPeriod = dbTime - dbCumTimePre - 0.1;
        pData = TV.m_pData;

        // Suppression des variantes suivantes
        for(i=nIndex; i<(int)pLstTV->size(); i++)
        {
            TV = pLstTV->at(i); 

            if(i!=nIndex)
                TV.m_pData.reset();
        }  
        
        i = 0;
        for ( iTV = pLstTV->begin( ); iTV != pLstTV->end( ); iTV++ )
        {
            if(i++ >= nIndex)
            {
                pLstTV->erase(iTV, pLstTV->end( ));

                // Ajout de la variation modifiée
                AddVariation(dbPeriod, pData, pLstTV);
                return;
            }            
        }        
    }
}

//================================================================
    template <class T> T* GetVariation
//----------------------------------------------------------------
// Fonction  : Retourne la variante correspondnat à l'instant
//             passé en paramètre
// Remarques : Si l'instant n'est pas défini, retourne la dernière
//             variante de la liste
// Version du: 20/06/2008
// Historique: 20/06/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
( 
    double dbTime, 
    std::deque<TimeVariation<T>> *pLstTV,
    double dbLag
)
{    
    double              dbCumTime = - dbLag;

    if(!pLstTV)
        return NULL;

    if(dbTime<0)
        return NULL;

    if(pLstTV->size()==0)
        return NULL;

    // priorité aux plages temporelles
    for(size_t i = 0; i < pLstTV->size(); i++)
    {
        const TimeVariation<T> & TV = pLstTV->at(i);
        if(TV.m_pPlage != NULL)
        {
            if(TV.m_pPlage->m_Debut <= dbTime && TV.m_pPlage->m_Fin >= dbTime)
            {
                return TV.m_pData.get();
            }
        }
    }
    
    // Si pas de plage temporelle correspondante, on regarde les durées
    size_t variationIdx = 0;
    while(variationIdx < pLstTV->size() && pLstTV->at(variationIdx).m_pPlage != NULL)
    {
        variationIdx++;
    }

    // si pas de durée définie non plus ...
    if(variationIdx >= pLstTV->size())
    {
        return NULL;
    }

    TimeVariation<T> * pTV = &pLstTV->at(variationIdx);
    dbCumTime += pTV->m_dbPeriod;
    variationIdx++;

    while(dbCumTime < dbTime && variationIdx < pLstTV->size() )
    {        
        // on ne prend pas en compte les plages temporelles
        if(pLstTV->at(variationIdx).m_pPlage == NULL)
        {
            pTV = &pLstTV->at(variationIdx);
            dbCumTime += pTV->m_dbPeriod;
        }
        variationIdx++;
    }

    return pTV->m_pData.get();
}

//================================================================
    template <class T> std::vector<std::pair<TimeVariation<T>, std::pair<double,double> > > GetVariations
//----------------------------------------------------------------
// Fonction  : Retourne les variantes ayant une emrpise commune
//             avec la plage temporelle passée en paramètre
// Version du: 19/01/2017
// Historique: 19/01/2017 (O.Tonck - Ipsis)
//             Création
//================================================================
(
    double dbTimeStart,
    double dbTimeEnd,
    std::deque<TimeVariation<T>> *pLstTV,
    double dbLag
)
{
    double              dbCumTime = -dbLag;

    std::vector<std::pair<TimeVariation<T>, std::pair<double, double> > >   result;

    if (!pLstTV)
        return result;

    if (pLstTV->size() == 0)
        return result;

    // priorité aux plages temporelles
    for (size_t i = 0; i < pLstTV->size(); i++)
    {
        const TimeVariation<T> & TV = pLstTV->at(i);
        if (TV.m_pPlage != NULL)
        {
            if (TV.m_pPlage->m_Debut <= dbTimeEnd && TV.m_pPlage->m_Fin >= dbTimeStart)
            {
                result.push_back(std::make_pair(TV, std::make_pair(
                    std::max<double>(TV.m_pPlage->m_Debut, dbTimeStart),
                    std::min<double>(TV.m_pPlage->m_Fin, dbTimeEnd)
                    )));
            }
        }
    }

    // Si pas de plage temporelle correspondante, on regarde les durées
    for (size_t variationIdx = 0; variationIdx < pLstTV->size(); variationIdx++)
    {
        const TimeVariation<T> & TV = pLstTV->at(variationIdx);
        if (TV.m_pPlage == NULL)
        {
            if (dbCumTime <= dbTimeEnd && (dbCumTime + TV.m_dbPeriod) >= dbTimeStart)
            {
                result.push_back(std::make_pair(TV, std::make_pair(
                    std::max<double>(dbCumTime, dbTimeStart),
                    std::min<double>(dbCumTime + TV.m_dbPeriod, dbTimeEnd)
                    )));
            }
            dbCumTime += TV.m_dbPeriod;

            if (dbCumTime > dbTimeEnd)
                break;
        }
    }

    return result;
}

//================================================================
    template <class T> std::deque<T*> GetVariations
//----------------------------------------------------------------
// Fonction  : Retourne les variantes correspondant à l'instant
//             passé en paramètre
// Remarques : Si l'instant n'est pas défini, retourne la dernière
//             variante de la liste
// Version du: 22/10/2014
// Historique: 20/10/2014 (ETS - IPSIS)
//             Création
//================================================================
( 
    double dbTime, 
    std::deque<TimeVariation<T>> *pLstTV,
    double dbLag
)
{    
    double              dbCumTime = - dbLag;

    if(!pLstTV)
        return std::deque<T*>();

    if(dbTime<0)
        return std::deque<T*>();

    if(pLstTV->size()==0)
        return std::deque<T*>();

    std::deque<T*> variations;
    // priorité aux plages temporelles
    for(size_t i = 0; i < pLstTV->size(); i++)
    {
        const TimeVariation<T> & TV = pLstTV->at(i);
        if(TV.m_pPlage != NULL)
        {
            if(TV.m_pPlage->m_Debut <= dbTime && TV.m_pPlage->m_Fin >= dbTime)
            {
                variations.push_back(TV.m_pData.get() );
            }
        }
    }
    if( variations.size()>0 )
    {
        return variations;
    }
    
    // Si pas de plage temporelle correspondante, on regarde les durées
    size_t variationIdx = 0;
    while(variationIdx < pLstTV->size() && pLstTV->at(variationIdx).m_pPlage != NULL)
    {
        variationIdx++;
    }

    // si pas de durée définie non plus ...
    if(variationIdx >= pLstTV->size())
    {
        return std::deque<T*>();
    }

    TimeVariation<T> * pTV = &pLstTV->at(variationIdx);
    dbCumTime += pTV->m_dbPeriod;
    variationIdx++;

    while(dbCumTime < dbTime && variationIdx < pLstTV->size() )
    {        
        // on ne prend pas en compte les plages temporelles
        if(pLstTV->at(variationIdx).m_pPlage == NULL)
        {
            pTV = &pLstTV->at(variationIdx);
            dbCumTime += pTV->m_dbPeriod;
        }
        variationIdx++;
    }
    variations.push_back(pTV->m_pData.get());
    return variations;
}
//================================================================
    template <class T> T* GetVariation
//----------------------------------------------------------------
// Fonction  : Retourne la valeur moyenne des variantes entre
//             les deux instants passés en paramètres
// Version du: 24/09/2012
// Historique: 24/09/2012 (O.Tonck - IPSIS)
//             Création
//================================================================
( 
    double dbTimeDebut, 
    double dbTimeFin, 
    std::deque<TimeVariation<T>> *pLstTV,
    double dbLag
)
{    
    double              dbCumTime = -dbLag;
    double              dbCurrentTime = dbTimeDebut;
    TimeVariation<T>   *pTV;

    if(!pLstTV)
        return NULL;

    if(pLstTV->size()==0)
        return NULL;

    T * pValue = new T;
    *pValue = 0;

    // Tant qu'on n'a pas couvert toute la plage temporelle demandée...
    while(dbCurrentTime < dbTimeFin)
    {
        TimeVariation<T>   *pCurrentTV = NULL;
        dbCumTime = -dbLag;

        // recherche de la plage correspondante à dbCurrentTime :

        // priorité aux plages temporelles
        for(size_t i = 0; i < pLstTV->size(); i++)
        {
            pTV = &pLstTV->at(i);
            if (pTV->m_pPlage != NULL)
            {
                if (pTV->m_pPlage->m_Debut <= dbCurrentTime && pTV->m_pPlage->m_Fin > dbCurrentTime)
                {
                    pCurrentTV = pTV;
                    dbCumTime = pTV->m_pPlage->m_Fin;
                }
            }
        }

        // Si pas de plage temporelle correspondante, on regarde les durées
        if(!pCurrentTV)
        {
            size_t variationIdx = 0;
            while(variationIdx < pLstTV->size() && pLstTV->at(variationIdx).m_pPlage != NULL)
            {
                variationIdx++;
            }

            // si pas de durée définie non plus ...
            if(variationIdx >= pLstTV->size())
            {
                return NULL;
            }

            pTV = &pLstTV->at(variationIdx);
            pCurrentTV = pTV;
            dbCumTime += pTV->m_dbPeriod;
            variationIdx++;

            while(dbCumTime <= dbCurrentTime && variationIdx < pLstTV->size() )
            {        
                // on ne prend pas en compte les plages temporelles
                if(pLstTV->at(variationIdx).m_pPlage == NULL)
                {
                    pTV = &pLstTV->at(variationIdx);
                    pCurrentTV = pTV;
                    dbCumTime += pTV->m_dbPeriod;
                }
                variationIdx++;
            }
        }

        // Ici, on a la premiere variation correspondante :
        // on pondère par la durée en commune entre la plage courante et la durée demandée
        double dbDuree = std::min<double>(dbCumTime, dbTimeFin)-dbCurrentTime;
        *pValue += (*pCurrentTV->m_pData.get())*dbDuree;
        dbCurrentTime = dbCumTime;
    }

    return pValue;
}

//================================================================
/*    template <class T> T* GetVariationEx
//----------------------------------------------------------------
// Fonction  : Retourne la variante correspondnat à l'instant
//             passé en paramètre
// Remarques : Si l'instant n'est pas défini, retourne la dernière
//             variante de la liste
// Version du: 20/06/2008
// Historique: 20/06/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
( 
    double dbTime, 
    std::deque<TimeVariationEx<T>> *pLstTV
)
{    
    int                 nIndex = 0;
    double              dbCumTime = 0;
    TimeVariationEx<T>    TV;

    if(!pLstTV)
        return NULL;

    if(pLstTV->size()==0)
        return NULL;
    
    TV = pLstTV[0];   
    for(int i=0; i<(int)pLstT->sie(); i++)
    {
        TV = pLstTV[i];   
        if( TV.m_dtDebut > dbTime)
            return pLstTV[i-1];
    }

    return TV.m_pData;
}*/

//================================================================
    template <class T> double GetCumTime
//----------------------------------------------------------------
// Fonction  : Retourne le temps passé dans les variantes précédentes
// Remarques : 
// Version du: 20/06/2008
// Historique: 20/06/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
( 
    double dbTime, 
    std::deque<TimeVariation<T>> *pLstTV
)
{    
    int                 nIndex = 0;
    double              dbCumTime = 0; 

    if(!pLstTV)
        return 0;

    if(dbTime<0)
        return 0;

    if(pLstTV->size()==0)
        return 0;    
    
    TimeVariation<T> * pTV = &pLstTV->at(0);

    while(dbCumTime < dbTime && nIndex < (int)pLstTV->size()-1 )
    {    
        // on ignore les plages temporelles ici
        if (pTV->m_pPlage == NULL)
        {
            dbCumTime += pTV->m_dbPeriod;
        }
        pTV = &pLstTV->at(++nIndex);
    }

    return dbCumTime;
}


// Classe template de stockage des listes des variantes temporelles
template<class T> class ListOfTimeVariation
{
    private:
        std::deque<TimeVariation<T>>            m_LstTV;      // Liste des variantes
        double                                  m_dbLag;        // Décalage (en seconde) entre la première variante de la liste par rapport au début de la simulation

    public:
        ListOfTimeVariation(){m_dbLag = 0;};
        ListOfTimeVariation(double dbLag);
        ~ListOfTimeVariation();

        void RemoveVariations() {EraseListOfVariation(&m_LstTV);};

		void Copy(ListOfTimeVariation *plstTV);

        void AddVariation(double dbPeriod, boost::shared_ptr<T> pData);
        void AddVariation(PlageTemporelle *pPlage, boost::shared_ptr<T> pData);
        T* GetVariationEx(double dbTime);
        T* GetVariationEx(double dbTime, double & dbEndVariationTime);

        void InsertVariation(double dbStartTime, double dbEndTime, boost::shared_ptr<T> pData);

        std::deque<TimeVariation<T>>*           GetLstTV(){return &m_LstTV;}

        double                                  GetLag(){return m_dbLag;}
        void                                    SetLag(double dbLag){m_dbLag = dbLag;}

        bool                                    CheckPlagesTemporelles(double dbDureeSimu);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
        ar & BOOST_SERIALIZATION_NVP(m_LstTV);
        ar & BOOST_SERIALIZATION_NVP(m_dbLag);
    }
};

    template<class T> ListOfTimeVariation<T>::ListOfTimeVariation(double dbLag)
    {
        m_dbLag = dbLag;
        m_LstTV.clear();
    }

    template<class T> ListOfTimeVariation<T>::~ListOfTimeVariation()
    {
    }

	template<class T> void ListOfTimeVariation<T>::Copy(ListOfTimeVariation *plstTV)
	{
		m_dbLag = plstTV->m_dbLag;			
		m_LstTV = plstTV->m_LstTV;		
	}

    template<class T> T* ListOfTimeVariation<T>::GetVariationEx(double dbTime)
    {        
        double dbPlaceHolder;
        return GetVariationEx(dbTime, dbPlaceHolder);
    }

    template<class T> T* ListOfTimeVariation<T>::GetVariationEx(double dbTime, double & endVariationTime)
    {
        double              dbCumTime = 0;
        endVariationTime = DBL_MAX;

        if (dbTime<0)
            return NULL;

        if (m_LstTV.size() == 0)
        {
            return NULL;
        }

        // récupération en priorité des valeurs définies au niveau des plages temporelles
        for (size_t i = 0; i < m_LstTV.size(); i++)
        {
            const TimeVariation<T> & TVref = m_LstTV[i];
            if (TVref.m_pPlage)
            {
                if (TVref.m_pPlage->m_Debut <= dbTime && TVref.m_pPlage->m_Fin >= dbTime)
                {
                    endVariationTime = TVref.m_pPlage->m_Fin;
                    return TVref.m_pData.get();
                }
            }
        }

        // Ensuite on regarde les séquences de durées
        dbTime += m_dbLag;

        // Si pas de plage temporelle correspondante, on regarde les durées
        size_t variationIdx = 0;
        while (variationIdx < m_LstTV.size() && m_LstTV.at(variationIdx).m_pPlage != NULL)
        {
            variationIdx++;
        }

        // si pas de durée définie non plus ...
        if (variationIdx >= m_LstTV.size())
        {
            return NULL;
        }

        TimeVariation<T> * pTV = &m_LstTV.at(variationIdx);
        dbCumTime = pTV->m_dbPeriod;
        variationIdx++;

        while (dbCumTime < dbTime && variationIdx < m_LstTV.size())
        {
            // on ne prend pas en compte les plages temporelles
            if (m_LstTV.at(variationIdx).m_pPlage == NULL)
            {
                pTV = &m_LstTV.at(variationIdx);
                dbCumTime += pTV->m_dbPeriod;
            }
            variationIdx++;
        }

        // rmq. : dans le cas de la dernière variation temporelle, elle termine à la fin de la simulation
        // même si la durée indiquée ne dure pas jusque là.
        endVariationTime = variationIdx == m_LstTV.size() ? DBL_MAX : dbCumTime;
        return pTV->m_pData.get();
    }

    template<class T> void ListOfTimeVariation<T>::AddVariation(double dbPeriod, boost::shared_ptr<T> pData)
    {
        ::AddVariation(dbPeriod, pData, &m_LstTV);
    }

    template<class T> void ListOfTimeVariation<T>::AddVariation(PlageTemporelle *pPlage, boost::shared_ptr<T> pData)
    {
        ::AddVariation(pPlage, pData, &m_LstTV);
    }

    template<class T> void ListOfTimeVariation<T>::InsertVariation(double dbStartTime, double dbEndTime, boost::shared_ptr<T> pData)
    {
        ::InsertVariation(dbStartTime, dbEndTime, pData, &m_LstTV);
    }

    template<class T> bool ListOfTimeVariation<T>::CheckPlagesTemporelles(double dbDureeSimu)
    {
        std::vector<PlageTemporelle*> plages;
        for(size_t i = 0 ; i < m_LstTV.size(); i++)
        {
            if(m_LstTV[i].m_pPlage)
            {
                plages.push_back(m_LstTV[i].m_pPlage);
            }
        }
        if(plages.size() == 0)
        {
            // dans le cas où il n'y a pas de plages temporelles, rien à vérifier (si on est quand même en mode 'horaire',
            // on suppose dans ce cas qu'une variation temporelle qui dure toute la simulation a été mise en place, conformément aux spécifications)
            return true;
        }
        else
        {
            return CheckPlagesTemporellesEx(dbDureeSimu, plages);
        }
    }


// Classe template de stockage des listes des variantes temporelles
template<class T> class ListOfTimeVariationEx
{
private:
    std::deque<TimeVariationEx<T>>            m_LstTV;      // Liste des variantes

public:
    ListOfTimeVariationEx();        
    ~ListOfTimeVariationEx();

    void AddVariation(STime  dtDebut, T* pData);
    T* GetVariationEx(STime  dtTime);
    T* GetFirstVariationEx();
    T* GetVariation(int n){return m_LstTV[n].m_pData.get();};

    std::deque<TimeVariationEx<T>>*           GetLstTV(){return &m_LstTV;};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(m_LstTV);
    }
};

    template<class T> ListOfTimeVariationEx<T>::ListOfTimeVariationEx()
    {                
        m_LstTV.clear();
    }

    template<class T> ListOfTimeVariationEx<T>::~ListOfTimeVariationEx()
    {
        EraseListOfVariationEx( &m_LstTV );
    }

	template<class T> T* ListOfTimeVariationEx<T>::GetVariationEx(STime dtTmp)
    {        
        int                 nIndex = 0;
        double              dbCumTime = 0;
        TimeVariationEx<T>  *pTV, *pTVEx;  
        size_t              nvars;

		if(dtTmp < STime())
            return NULL;

        nvars = m_LstTV.size();
        if(nvars==0)
            return NULL;

        pTVEx = &(m_LstTV[0]);
        for(size_t i=0; i< nvars; i++)
        {
            pTV = &(m_LstTV[i]);
            if( pTV->m_dtDebut > dtTmp ) 
                return pTVEx->m_pData.get(); 

            pTVEx = pTV;
        }
                       
        return pTV->m_pData.get();       // Non trouvé, on retourne la dernière 
    }

	template<class T> void ListOfTimeVariationEx<T>::AddVariation(STime dtDebut, T* pData)
    {
        AddVariationEx(dtDebut, pData, &m_LstTV);
    }

    template<class T> T* ListOfTimeVariationEx<T>::GetFirstVariationEx()
    {                           
        if( m_LstTV.size()>0 )
            return m_LstTV[0].m_pData.get();   
        else
            return NULL;
    }

// Fonctions template de manipulations des listes des variantes temporelles
// REMARQUES : le code des fonctions Template est ici sinon elles ne 
// sont pas reconnues à l'édition des liens (pas supporté par le compilo)



//================================================================
    template <class T> void AddVariationEx
//----------------------------------------------------------------
// Fonction  : Ajoute une variante à la liste des variantes
// Remarques :
// Version du: 23/10/2008
// Historique: 23/10/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(   
	STime                           dtDebut,    // Heure de début de la simulation
    T                               *pData,     // Variante
    std::deque<TimeVariationEx<T>>    *pLstTV   // Liste des variantes
)
{
    if(!pLstTV)
        return;

    TimeVariationEx<T> TV;
    TV.m_dtDebut = dtDebut;
    TV.m_pData = boost::shared_ptr<T>(pData);

    pLstTV->push_back(TV);
};

//================================================================
    template <class T> void EraseListOfVariationEx
//----------------------------------------------------------------
// Fonction  : Suppression de tous les éléments de la liste des 
//             variantes temporelles
// Remarques :
// Version du: 23/10/2008
// Historique: 23/10/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
( 
    std::deque<TimeVariationEx<T>> *pLstTV
)
{
    if(!pLstTV)
        return;
    
    TimeVariationEx<T>    TV;
    for(int i=0; i<(int)pLstTV->size(); i++)
    {
        TV = pLstTV->at(i); 
        TV.m_pData.reset();
    }

    pLstTV->clear();
}

////================================================================
//    template <class T> void EraseVariation
////----------------------------------------------------------------
//// Fonction  : Suppression les variations à partir d'un certain
////             instant
//// Remarques :
//// Version du: 23/10/2008
//// Historique: 23/10/2008 (C.Bécarie - Tinea)
////             Création
////================================================================
//( 
//    std::deque<TimeVariationEx<T>> *pLstTV,
//    System::DateTime  dtTime
//)
//{
//    int                 nIndex = 0;    
//    int                 i = 0;
//    TimeVariationEx<T>    TV;
//    std::deque<TimeVariationEx<T>>::iterator iTV;    
//    T*                  pData;
//    System::DateTime    dtTmp;
//
//    if(!pLstTV)
//        return;
//
//    if(dbTime<0)
//        return;
//
//    if(pLstTV->size()==0)
//        return;
//
//    TV = pLstTV->at(0);   
//    dtTmp = TV.m_dtDebut;
//
//    while(dtTmp < dtTime && nIndex < (int)pLstTV->size() )
//    {        
//        TV = pLstTV->at(++nIndex);
//        dtTmp = TV.m_dtDebut;
//    }    
//    
//    if(nIndex < (int)pLstTV->size())
//    {
//        // Modification de la durée de la dernière variante afin que la nouvelle 
//        // variante soit pris en compte dès le pas de temps courant        
//        pData = TV.m_pData;
//
//        // Suppression des variantes suivantes
//        for(i=nIndex; i<(int)pLstTV->size(); i++)
//        {
//            TV = pLstTV->at(i); 
//
//            if(i!=nIndex)
//                delete TV.m_pData;
//        }  
//        
//        i = 0;
//        for ( iTV = pLstTV->begin( ); iTV != pLstTV->end( ); iTV++ )
//        {
//            if(i++ >= nIndex)
//            {
//                pLstTV->erase(iTV, pLstTV->end( ));
//
//                // Ajout de la variation modifiée
//                AddVariation(dtTime, pData, pLstTV);
//                return;
//            }            
//        }        
//    }
//}


// ****************************************************************************
// Structure permettant de définir un paramètre pour une portion
// de tronçon.
// ****************************************************************************
template<class T> class Portion {
public:
    /// Constructeurs
    Portion() {};
    Portion(double debut, double fin, T data) :
      dbPosDebut(debut),
      dbPosFin(fin),
      data(data) {};

    /// Membres
public:
    double dbPosDebut; // position curviligne du début de la portion
    double dbPosFin; // position curviligne de la fin de la portion
    T data; // valeur du paramètre

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
        ar & BOOST_SERIALIZATION_NVP(dbPosDebut);
        ar & BOOST_SERIALIZATION_NVP(dbPosFin);
        ar & BOOST_SERIALIZATION_NVP(data);
    }
};


template<typename T> std::string to_string( const T & Value )
{
    // utiliser un flux de sortie pour créer la chaîne
    std::ostringstream oss;
    // écrire la valeur dans le flux
    oss << Value;
    // renvoyer une string
    return oss.str();
}

double Round (double dbNumber, double dbPrecision);

bool CalculIntersection( Troncon *pT1,Troncon *pT2, Point &ptInt, size_t &iPt1, size_t &iPt2);
bool CalculIntersection( Point pt11, Point pt12, Point pt21, Point pt22, Point &ptInt);
double GetDistance(Point pt1, Point pt2);
bool IsCounterClockWise(Point *pt1, Point *pt2, Point *pt3);
bool IsCounterClockWise(Point *V11, Point *V12, Point *V21, Point *V22);
double AngleOfView( double ViewPt_X, double ViewPt_Y, double Pt1_X, double Pt1_Y, double Pt2_X, double Pt2_Y );
std::deque<Point> BuildLineStringGeometry(const std::deque<Point> & lstPoints, double dbOffset, double dbTotalWidth, const std::string & strElementID, Logger * pLogger);
void CalculAbsCoords(Voie * pLane, double dbPos, bool bOutside, double & dbX, double & dbY, double & dbZ);
double CalculAngleTuyau(Tuyau * pTuyau, double dbPos);

bool GetXmlAttributeValue(XERCES_CPP_NAMESPACE::DOMNode *DOMElement, const std::string &strAttr, std::string &strVal, Logger* pLogger);
bool GetXmlAttributeValue(XERCES_CPP_NAMESPACE::DOMNode *DOMElement, const std::string &strAttr, bool &bVal, Logger* pLogger);
bool GetXmlAttributeValue(XERCES_CPP_NAMESPACE::DOMNode *DOMElement, const std::string &strAttr, double &dbVal, Logger* pLogger);
bool GetXmlAttributeValue(XERCES_CPP_NAMESPACE::DOMNode *DOMElement, const std::string &strAttr, char &cVal, Logger* pLogger);
bool GetXmlAttributeValue(XERCES_CPP_NAMESPACE::DOMNode *DOMElement, const std::string &strAttr, char &cVal, Logger* pLogger);
bool GetXmlAttributeValue(XERCES_CPP_NAMESPACE::DOMNode *DOMElement, const std::string &strAttr, int &nVal, Logger* pLogger);
bool GetXmlAttributeValue(XERCES_CPP_NAMESPACE::DOMNode *DOMElement, const std::string &strAttr, Point &pt, Logger* pLogger);
bool GetXmlAttributeValue(XERCES_CPP_NAMESPACE::DOMNode *DOMElement, const std::string &strAttr, SDateTime &dtTime, Logger* pLogger);
bool GetXmlAttributeValue(XERCES_CPP_NAMESPACE::DOMNode *DOMElement, const std::string &strAttr, unsigned int &uiVal, Logger* pLogger);

bool GetXmlDuree(XERCES_CPP_NAMESPACE::DOMNode *pXMLNode, Reseau * pReseau, double &dbVal, std::vector<PlageTemporelle*> & outPlages, Logger  *pLogger);

bool DecelerationProcedure(double dbTauxDec, Reseau* pReseau, const std::string & sTraficFile);
void TrajectoireProcedure(Reseau* pReseau, const std::string & sTraficFile);

void CalcCoordFromPos(Tuyau *pTuyau, double  dbPos, Point &ptCoord);

double CumulativeNormalDistribution(const double x);

Point CalculPositionTronconInsertion(const Point & ptTronconAval1, const Point & ptTronconAval2, std::vector<double> dbLargeurVoie, int nbVoieAmont, int nbVoieInsDroite);

Tuyau* AbsoluteToRelativePosition(Reseau * pReseau, double dbX, double dbY, double dbZ, int &nLane, double &dbRelativePosition);

// retourne les indices du vecteur dont les valeurs sont <= à valueToFind
template<class T> 
std::vector<int> FindLessEqual(std::vector<T> vectors, const T& valueToFind )
{
    std::vector<int> returnIndices;
    typename std::vector<T>::iterator it;
    int iLoop =0;
    for( it = vectors.begin(); it!= vectors.end(); it++)
    {
        if((*it )<= valueToFind )
        {
            returnIndices.push_back(iLoop);

        }
        iLoop++;

    }
    return returnIndices;
}

#endif