#pragma once
#ifndef TronconOrigineH
#define TronconOrigineH

#include "usage/SymuViaTripNode.h"
#include "Connection.h"

class ZoneDeTerminaison;
class TypeVehicule;

class TronconOrigine : public SymuViaTripNode
{
public:
   TronconOrigine() {m_pZoneParent=NULL; m_pTuyau=NULL;}
    TronconOrigine(Tuyau * pTuyau, ZoneDeTerminaison * pZoneParent);
    virtual ~TronconOrigine();

    // ************************************
    // Traitements publics
    // ************************************
    virtual std::string GetOutputID() const;

    ZoneDeTerminaison * GetZoneParent() {return m_pZoneParent;}

    Tuyau * GetTuyau() {return m_pTuyau;}

    virtual std::map<TypeVehicule*, std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> > > & GetMapAssignment() {return m_mapAssignment;}

    // ************************************
    // Membres privés
    // ************************************
private:
    ZoneDeTerminaison * m_pZoneParent;

    Tuyau * m_pTuyau;

    // Variable de stockage des données d'affectation
	std::map<TypeVehicule*, std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> > >		m_mapAssignment;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif //TronconOrigineH
