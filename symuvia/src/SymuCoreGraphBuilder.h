#pragma once

#ifndef SymuCoreGraphBuilder_H
#define SymuCoreGraphBuilder_H

#include "SymuCoreDrivingGraphBuilder.h"
#include "SymuCorePublicTransportGraphBuilder.h"

#include <map>
#include <set>

class Reseau;
class Connexion;
class Tuyau;
class TypeVehicule;
class ZoneDeTerminaison;
class SymuViaTripNode;

namespace SymuCore {
    class Graph;
    class MultiLayersGraph;
    class Link;
    class Node;
    class VehicleType;
    class MacroType;
    class Pattern;
    class DrivingPattern;
    class AbstractPenalty;
    class Populations;
}

namespace boost {
    namespace posix_time {
        class ptime;
        class time_duration;
    }
}

class SymuCoreGraphBuilder
{
public:

    SymuCoreGraphBuilder();

	//! Constructeur
    SymuCoreGraphBuilder(Reseau* pReseau, bool bForSymuMaster);

	//! Destructeur
    virtual ~SymuCoreGraphBuilder();

    virtual bool Build(SymuCore::MultiLayersGraph * pGraph, bool bIsPrimaryGraph);

    const SymuCoreDrivingGraphBuilder & GetDrivingGraphBuilder() const;
    const SymuCorePublicTransportGraphBuilder & GetPublicTransportGraphBuilder() const;

    SymuCore::MultiLayersGraph * GetGraph() const;

protected:

    Reseau*                                 m_pNetwork;

    SymuCore::MultiLayersGraph              *m_pGraph;

    // pour gestion des différences entre la génération du graphe pour SymuMaster et celui pour un usage interne par SymuVia
    bool                                    m_bForSymuMaster;

    SymuCoreDrivingGraphBuilder             m_DrivingGraphBuilder;

    SymuCorePublicTransportGraphBuilder     m_PublicTransportGraphBuilder;
};

#endif // SymuCoreGraphBuilder_H


