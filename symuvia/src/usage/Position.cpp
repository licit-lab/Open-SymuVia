#include "stdafx.h"
#include "Position.h"

#include "voie.h"
#include "Connection.h"
#include "ConnectionPonctuel.h"
#include "BriqueDeConnexion.h"
#include "tuyau.h"
#include "reseau.h"
#include "Logger.h"
#include "SymuViaTripNode.h"

#include "xercesc/dom/DOMNode.hpp"

#include <boost/serialization/vector.hpp>

#define UNDEF_LANE_NUMBER -1

XERCES_CPP_NAMESPACE_USE

Position::Position()
{
    m_pConnection = NULL;
    m_pLink = NULL;
    m_pLstLinks = NULL;
    m_dbPosition = UNDEF_LINK_POSITION;
    m_bOutside = false;
    m_LaneNumber = UNDEF_LANE_NUMBER;
}

Position::Position(Connexion * pConnection)
{
    m_pConnection = pConnection;
    m_pLink = NULL;
    m_pLstLinks = NULL;
    m_dbPosition = UNDEF_LINK_POSITION;
    m_bOutside = false;
    m_LaneNumber = UNDEF_LANE_NUMBER;
}

Position::Position(Tuyau * pLink, double dbPosition)
{
    m_pConnection = NULL;
    m_pLink = pLink;
    m_pLstLinks = NULL;
    m_dbPosition = dbPosition;
    m_bOutside = false;
    m_LaneNumber = UNDEF_LANE_NUMBER;
}

Position::~Position()
{
    if(m_pLstLinks)
    {
        delete m_pLstLinks;

        // dns le cas des zones, zuppression également de la connexion virtuelle équivalente
        if(m_pConnection)
        {
            delete m_pConnection;
        }
    }
}

bool Position::Load(DOMNode * pNode, const std::string & strElement, Reseau * pNetwork, Logger & loadingLogger)
{
    if(pNode)
    {
        std::string strId;
        GetXmlAttributeValue(pNode, "id", strId, &loadingLogger);

        // On essaye de voir s'il s'agit d'un link
        Tuyau * pLink = pNetwork->GetLinkFromLabel(strId);
        if(pLink)
        {
            SetLink(pLink);

            // Récupération de la position sur le lien le cas échéant
            GetXmlAttributeValue(pNode, "position", m_dbPosition, &loadingLogger);

            bool bTmp = false;
            GetXmlAttributeValue(pNode, "position_relative", bTmp, &loadingLogger);
            if(bTmp)
            {
                m_dbPosition *= pLink->GetLength();
            }

            if( pLink->GetLength() < m_dbPosition)
            {
                loadingLogger << Logger::Error << "Error : wrong position for item " << strElement << " ( position > link's length )" << std::endl;                
                loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                return false;
            }

            // Récupération de la caractéristique hors voie ou non
            GetXmlAttributeValue(pNode, "hors_voie", m_bOutside, &loadingLogger);

            // Récupération de la caractéristique du numéro de voie
            int numVoie;
            if(GetXmlAttributeValue(pNode, "num_voie", numVoie, &loadingLogger))
            {
                m_LaneNumber = numVoie - 1;
            }
        }
        else
        {
            // On suppose qu'il s'agit d'une connexion ...
            char cTmp;
            Connexion * pConnexion = pNetwork->GetConnectionFromID(strId, cTmp);
            if(pConnexion)
            {
                SetConnection(pConnexion);
            }
            else
            {
                SymuViaTripNode * pSymuNode = pNetwork->GetOrigineFromID(strId, cTmp);
                if(pSymuNode)
                {
                    SetConnection(pSymuNode->GetOutputConnexion());
                }
                else
                {
                    pSymuNode = pNetwork->GetDestinationFromID(strId, cTmp);
                    if(pSymuNode)
                    {
                        SetConnection(pSymuNode->GetInputConnexion());
                    }
                    else
                    {
                        loadingLogger << Logger::Error << "No item with ID " << strId << " for the position of the item " << strElement << std::endl;
                        loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                        return false;
                    }
                }
            }
        }

        return true;
    }
    else
    {
        loadingLogger << Logger::Error << "No node defined for the position of item " << strElement << std::endl;
        loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

        return false;
    }
}

bool Position::IsNull() const
{
    return m_pConnection == NULL
        && m_pLink == NULL;
    // rmq. pas la peine de regarder m_pLstLinks car s'il est défini, m_pConnection est non nul : 
}

bool Position::IsConnection() const
{
    return m_pConnection != NULL && m_pLstLinks == NULL;
}

bool Position::IsLink() const
{
    return m_pLink != NULL;
}

bool Position::IsZone() const
{
    return m_pLstLinks != NULL;
}

Connexion * Position::GetConnection() const
{
    return m_pConnection;
}

void Position::SetConnection(Connexion * pConnexion)
{
    m_pConnection = pConnexion;
}

void Position::SetLink(Tuyau * pLink)
{
    m_pLink = pLink;
}

Tuyau * Position::GetLink() const
{
    return m_pLink;
}

void Position::SetOutside(bool bOutside)
{
    m_bOutside = bOutside;
}

bool Position::IsOutside() const
{
    return m_bOutside;
}

int Position::GetLaneNumber() const
{
    return m_LaneNumber;
}

void Position::SetLaneNumber(int laneNumber)
{
    m_LaneNumber = laneNumber;
}

bool Position::HasLaneNumber() const
{
    return m_LaneNumber != UNDEF_LANE_NUMBER;
}

Tuyau * Position::GetDownstreamLink() const
{
    Tuyau * pLink = nullptr;
    if(IsLink())
    {
        pLink = m_pLink;
    }
    else if(IsConnection())
    {
        assert(m_pConnection->m_LstTuyAv.size() == 1);

        pLink = m_pConnection->m_LstTuyAv.front();
    }
    else
    {
        // Non géré (s'il y a plusieurs tuyaux aval à une zone ?)
        assert(false);
    }
    return pLink;
}

void Position::SetPosition(double dbPosition)
{
    m_dbPosition = dbPosition;
}

double Position::GetPosition() const
{
    return m_dbPosition;
}

bool Position::HasPosition() const
{
    return m_dbPosition != UNDEF_LINK_POSITION;
}


void Position::SetZone(const std::vector<Tuyau*> & tuyaux, const std::string & zoneID, Reseau * pNetwork)
{
    assert(m_pLstLinks == NULL);
    assert(m_pConnection == NULL);

    // recopie de la liste des tuyaux constituant la zone
    m_pLstLinks = new std::vector<Tuyau*>(tuyaux);

    m_pConnection = new Connexion(zoneID, pNetwork);

    std::set<Tuyau*, LessPtr<Tuyau> > setTuyauxAv, setTuyauxAm;

    // pour chaque connexion formant une sortie de zone
    // (au moins un tronçon aval hors de zone et au moins un tronçon amont en zone),
    // on crée un objet ZoneOrigine, en lequel on doit pouvoir gérer la création des véhicules
    // ayant pour origine la zone.
    for(size_t tuyZoneIdx = 0; tuyZoneIdx < m_pLstLinks->size(); tuyZoneIdx++)
    {
        // la connexion candidate à la sortie de zone est la connexion aval
        Connexion * pConnexionAval;
        if(m_pLstLinks->at(tuyZoneIdx)->GetBriqueAval() != NULL)
        {
            pConnexionAval = m_pLstLinks->at(tuyZoneIdx)->GetBriqueAval();
        }
        else
        {
            pConnexionAval = m_pLstLinks->at(tuyZoneIdx)->getConnectionAval();
        }

        // pour chaque tuyau aval qui n'est pas dans la zone, on a un tuyau aval de notre connexion virtuelle
        for(int tuySortieIdx = 0; tuySortieIdx < pConnexionAval->GetNbElAval(); tuySortieIdx++)
        {
            Tuyau * pTuyauSortie = pConnexionAval->m_LstTuyAv[tuySortieIdx];

            // test de la non appartenance du tuyau à la zone
            if(!IsInZone(pTuyauSortie))
            {
                // il faut aussi qu'il existe un mouvement autorisé qui va du tuyau amont vers le tuyau aval
                if(pConnexionAval->IsMouvementAutorise(m_pLstLinks->at(tuyZoneIdx), pTuyauSortie, NULL, NULL))
                {
                    setTuyauxAv.insert(pTuyauSortie);
                }
            }
        }

        // la connexion candidate à l'entrée de zone est la connexion amont
        Connexion * pConnexionAmont;
        if(m_pLstLinks->at(tuyZoneIdx)->GetBriqueAmont() != NULL)
        {
            pConnexionAmont = m_pLstLinks->at(tuyZoneIdx)->GetBriqueAmont();
        }
        else
        {
            pConnexionAmont = m_pLstLinks->at(tuyZoneIdx)->getConnectionAmont();
        }

        // pour chaque tuyau amont qui n'est pas dans la zone, on a un tuyau amont de notre connexion virtuelle
        for(int tuySortieIdx = 0; tuySortieIdx < pConnexionAmont->GetNbElAmont(); tuySortieIdx++)
        {
            Tuyau * pTuyauEntree = pConnexionAmont->m_LstTuyAm[tuySortieIdx];

            // test de la non appartenance du tuyau à la zone
            if(!IsInZone(pTuyauEntree))
            {
                // il faut aussi qu'il existe un mouvement autorisé qui va du tuyau amont vers le tuyau aval
                if(pConnexionAmont->IsMouvementAutorise(pTuyauEntree, m_pLstLinks->at(tuyZoneIdx), NULL, NULL))
                {
                    setTuyauxAm.insert(pTuyauEntree);
                }
            }
        }
    }

    for (std::set<Tuyau*, LessPtr<Tuyau>>::iterator iter = setTuyauxAv.begin(); iter != setTuyauxAv.end(); iter++)
    {
        m_pConnection->m_LstTuyAv.push_back(*iter);
        m_pConnection->AddEltAvalAss(*iter);
    }

    for (std::set<Tuyau*, LessPtr<Tuyau>>::iterator iter = setTuyauxAm.begin(); iter != setTuyauxAm.end(); iter++)
    {
        m_pConnection->m_LstTuyAm.push_back(*iter);
        m_pConnection->AddEltAmontAss(*iter);
    }
}

const std::vector<Tuyau*> & Position::GetZone() const
{
    assert(IsZone());
    return *m_pLstLinks;
}

bool Position::IsInZone(Tuyau * pTuyau) const
{
    assert(IsZone());

    bool result = false;

    for(size_t tuyZoneIdx = 0; tuyZoneIdx < m_pLstLinks->size() && !result; tuyZoneIdx++)
    {
        if(m_pLstLinks->at(tuyZoneIdx) == pTuyau)
        {
            result = true;
        }
    }

    return result;
}

void Position::ComputeBarycentre(Point & pt) const
{
    assert(IsZone());

    double x = 0, y = 0;
    int nbPts = 0;

    Tuyau * pLink;

    // remarque : pas terrible car on peut avoir beaucoup de petits tronçons avec plein de points internes qui pèseront
    // davantage que les longs tronçons rectilignes, mais bon...
    for (size_t tuyZoneIdx = 0; tuyZoneIdx < m_pLstLinks->size(); tuyZoneIdx++)
    {
        pLink = m_pLstLinks->at(tuyZoneIdx);
        x += pLink->GetExtAmont()->dbX;
        y += pLink->GetExtAmont()->dbY;
        for (size_t iPt = 0; iPt < pLink->GetLstPtsInternes().size(); iPt++)
        {
            x += pLink->GetLstPtsInternes()[iPt]->dbX;
            y += pLink->GetLstPtsInternes()[iPt]->dbY;
        }
        x += pLink->GetExtAval()->dbX;
        y += pLink->GetExtAval()->dbY;

        nbPts = nbPts + 2 + (int)pLink->GetLstPtsInternes().size();
    }

    pt.dbX = x / nbPts;
    pt.dbY = y / nbPts;
    pt.dbZ = 0;
}

bool Position::IsReached(VoieMicro * pLane, double laneLength, double startPositionOnLane, double endPositionOnLane) const
{
    bool bReached = false;
    if(IsNull())
    {
        bReached = false;
    }
    else if(IsConnection())
    {	
        bReached = endPositionOnLane > laneLength && m_pConnection->IsTuyauAmont((Tuyau*)pLane->GetParent());
    }
    else if(IsLink())
    {		
        if(m_dbPosition == UNDEF_LINK_POSITION)
        {
            bReached = pLane->GetParent() == m_pLink;
        }
        else
        {
			bReached = pLane->GetParent() == m_pLink && endPositionOnLane * (m_pLink->GetLength() / pLane->GetLength()) >= m_dbPosition - ZERO_DOUBLE;
        }
    }
    else if(IsZone())
    {
        // Cas particulier de la zone de destination : on dit que le leg est terminé lorsque le véhicule arrive sur un tuyau amont à la zone
        bReached = m_pConnection->IsTuyauAmont((Tuyau*)pLane->GetParent());
        // ajout : on traite aussi le cas du véhicule déjà présent sur la zone (il est créé directement dedans, cas par exemple d'une extremité dont le tronçon
        // aval est dans la zone, ou encore d'un trajet de zone à zone qui se touchent en mode périphérique)
        if (!bReached)
        {
            bReached = IsInZone((Tuyau*)pLane->GetParent());
        }
    }
    else
    {
        assert(false); // pas possible si l'objet Position est valide.
    }
    return bReached;
}

double Position::GetRemainingLinkLength() const
{
    double result = 0;
    if(IsLink())
    {
        result = m_pLink->GetLength();
        if(HasPosition())
        {
            result -= m_dbPosition;
        }
    }
    return result;
}

template void Position::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Position::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Position::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pConnection);
    ar & BOOST_SERIALIZATION_NVP(m_pLink);
    ar & BOOST_SERIALIZATION_NVP(m_pLstLinks);
    ar & BOOST_SERIALIZATION_NVP(m_dbPosition);
    ar & BOOST_SERIALIZATION_NVP(m_bOutside);
    ar & BOOST_SERIALIZATION_NVP(m_LaneNumber);
}
