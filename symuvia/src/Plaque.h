#ifndef _PLAQUE_H__
#define _PLAQUE_H__

#include <string>
#include <vector>
#include <map>
#include <set>

#ifdef USE_SYMUCORE
#include "SymuCoreAreaHelper.h"
#endif // USE_SYMUCORE

namespace boost {
    namespace serialization {
        class access;
    }
}

class Tuyau;
class CMotif;
class Connexion;
class ZoneDeTerminaison;
class Position;
class ParkingParameters;
struct Point;

//! Classe définissant une section de tronçon (une plaque = un ensemble de sections de tronçons)
class CSection
{
public:
    //! Default constructor
    CSection(void);
    //! Constructor
    CSection(Tuyau * pTuyau, double dbStartPos, double dbEndPos);
    //! Destructor
    virtual ~CSection(void);

public:

    Tuyau * getTuyau() const;

    double getStartPosition() const;
    double getEndPosition() const;

    // Cacule la longueur de la section
    double getLength() const;

private:

    Tuyau*      m_pTuyau;
    double      m_dbPositionDebut;    // Positionnement en m du début de la section (à partir du début du tuyau)
    double      m_dbPositionFin;      // Positionnement en m de la fin de la section (à partir du début du tuyau)

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sérialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

//! Classe définissant une plaque d'une zone de terminaison
class CPlaque
{

public:
    //! Default constructor
    CPlaque(void);
    //! Constructor
    CPlaque(ZoneDeTerminaison * pParentZone, const std::string & strID);
    //! Destructor
    virtual ~CPlaque(void);


public:

    ZoneDeTerminaison * GetParentZone() const;

    const std::string & GetID() const;
    const std::vector<CSection> & getSections() const;

    void AddSection(const CSection & section);

    void SetMotifCoeff(CMotif * pMotif, double dbCoeff);

    double GetMotifCoeff(CMotif * pMotif);

    const std::map<CMotif*, double> & GetMapMotifCoeff() const;

    void SetParkingParameters(ParkingParameters * pParkingParams, bool bOwnsParkingParams);
    ParkingParameters * GetParkingParameters() const;
        
    void ComputeBarycentre(Point & pt) const;

    // Vérifie si la position considérée appartient à la plaque
    bool ContainsPosition(const Position & position) const;

    // Vérifie si le tuyau appartient à la plaque
    bool HasLink(Tuyau * pLink) const;

    // Vérifie si le tuyau appartient à la plaque ou s'il en est assez proche selon le radius défini pour les zones
    bool CheckRadiusOK(Tuyau * pLink);

private:

    ZoneDeTerminaison * m_pParentZone;

    std::string m_strID;

    std::vector<CSection> m_LstSections;
    std::set<Tuyau*> m_LstLinks;
    std::set<Tuyau*> m_LstLinksExtended;

    std::map<CMotif*, double> m_mapMotifsCoeffs;

    ParkingParameters * m_pParkingParameters;
    bool m_bOwnsParkingParams;

#ifdef USE_SYMUCORE
public:
    SymuCoreAreaHelper m_AreaHelper;
#endif // USE_SYMUCORE

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sérialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif // _PLAQUE_H__