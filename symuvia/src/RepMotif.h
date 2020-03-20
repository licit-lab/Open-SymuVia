#ifndef _REPMOTIF_H__
#define _REPMOTIF_H__

class SymuViaTripNode;
class CMotif;

namespace boost {
    namespace serialization {
        class access;
    }
}

#include <vector>
#include <map>

//! Classe définissant un coefficient pour un couple de motifs de déplacement
class CMotifCoeff
{

public:
    //! Default constructor
    CMotifCoeff(void);
    //! Constructor
    CMotifCoeff(CMotif* pOrigin, CMotif* pDest, double dbCoeff);
    //! Destructor
    virtual ~CMotifCoeff(void);

public:
    CMotif * GetMotifOrigine() const;
    CMotif * GetMotifDestination() const;

    double getCoeff() const;
    void setCoeff(double dbCoeff);

private:

    CMotif * m_pOriginMotif;
    CMotif * m_pDestinationMotif;
    double   m_dbCoeff;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sérialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

//! Classe définissant une répartition de motifs de déplacement
class CRepMotifDest
{

public:
    //! Default constructor
    CRepMotifDest(void);
    //! Destructor
    virtual ~CRepMotifDest(void);

public:

    void setDestination(SymuViaTripNode * pDest);
    SymuViaTripNode * getDestination() const;

    bool addMotifCoeff(const CMotifCoeff & motifCoeff);
    std::vector<CMotifCoeff> & getCoeffs();

private:

    // Destination (optionnelle) associée à la répartition des motifs. Si NULL, la répartition
    // est la même pour toutes les destinations
    SymuViaTripNode * m_pDestination;

    // Liste des couples de motifs OD pour lesquels on a un coefficient non nul
    std::vector<CMotifCoeff> m_LstCoeffs;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sérialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

//! Classe définissant une liste de répartitions de motifs de déplacement pour différentes destinations
class CRepMotif
{

public:
    //! Default constructor
    CRepMotif(void);
    //! Destructor
    virtual ~CRepMotif(void);

public:

    bool addRepMotifDest(const CRepMotifDest & repMotifDest);

    CRepMotifDest * getRepMotifDest(SymuViaTripNode * pDestination);

    std::map<SymuViaTripNode*, CRepMotifDest> & getMapRepMotifs() { return m_mapRepMotifs; }

private:

    // Map des répartition des motifs en fonction de la destination
    std::map<SymuViaTripNode*, CRepMotifDest> m_mapRepMotifs;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sérialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif // _REPMOTIF_H__