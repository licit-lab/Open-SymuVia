#include "stdafx.h"
#include "ITSCAF.h"

#include "ControleurDeFeux.h"
#include "ConnectionPonctuel.h"
#include "reseau.h"
#include "tuyau.h"
#include "voie.h"
#include "Xerces/XMLUtil.h"

#include <Communication/CommunicationRunner.h>

#include "boost/date_time/posix_time/posix_time.hpp"

XERCES_CPP_NAMESPACE_USE

ITSCAF::ITSCAF()
{

}

ITSCAF::ITSCAF(StaticElement *pParent, Reseau* pNetwork, const std::string &strID, const std::string &strType, const SymuCom::Point &pPoint, SymuCom::Graph *pGraph, int iSendFrequency,
               bool bCanSend, bool bCanReceive, double dbEntityRange, double dbAlphaDown, double dbBetaDown)
    : ITSStation(pNetwork, strID, strType, pPoint, pGraph, bCanSend, bCanReceive, dbEntityRange, dbAlphaDown, dbBetaDown), m_iSendFrequency(iSendFrequency)
{
    SetStaticParent(pParent);
}

ITSCAF::~ITSCAF()
{

}

eCouleurFeux GetNextPhase(CDFSequence *pCurentSequence, CDFSequence *pApplicableSequence, PlanDeFeux* pPDF, SignalActif* pSignalActif, double& dbTimeInSequence, double& dbTimeTillNextPhase)
{
    if(pCurentSequence != pApplicableSequence)
    {
        dbTimeTillNextPhase += (pCurentSequence->GetTotalLength() - dbTimeInSequence);
        pCurentSequence = pPDF->GetNextSequence(pCurentSequence);
        dbTimeInSequence = 0.0;
		return GetNextPhase(pCurentSequence, pApplicableSequence, pPDF, pSignalActif, dbTimeInSequence, dbTimeTillNextPhase);
    }

    if(dbTimeInSequence < pSignalActif->GetActivationDelay())
    {
        //seconds before the next color phase (vert)
        dbTimeTillNextPhase += pSignalActif->GetActivationDelay() - dbTimeInSequence;
        dbTimeInSequence = pSignalActif->GetActivationDelay();
        return FEU_VERT;
    }else if(dbTimeInSequence < pSignalActif->GetActivationDelay() + pSignalActif->GetGreenDuration())
    {
        //seconds before the next color phase (orange)
        dbTimeTillNextPhase += pSignalActif->GetActivationDelay() + pSignalActif->GetGreenDuration() - dbTimeInSequence;
        dbTimeInSequence = pSignalActif->GetActivationDelay() + pSignalActif->GetGreenDuration();
        return FEU_ORANGE;
    }else if(dbTimeInSequence < pSignalActif->GetActivationDelay() + pSignalActif->GetGreenDuration() + pSignalActif->GetOrangeDuration())
    {
        //seconds before the next color phase (red)
        dbTimeTillNextPhase += pSignalActif->GetActivationDelay() + pSignalActif->GetGreenDuration() + pSignalActif->GetOrangeDuration() - dbTimeInSequence;
        dbTimeInSequence = pSignalActif->GetActivationDelay() + pSignalActif->GetGreenDuration() + pSignalActif->GetOrangeDuration();
        return FEU_ROUGE;
	}
	else // dbTimeInSequence > pSignalActif->GetActivationDelay() + pSignalActif->GetGreenDuration() + pSignalActif->GetOrangeDuration()
	{
		dbTimeTillNextPhase += (pCurentSequence->GetTotalLength());
		dbTimeInSequence -= (pCurentSequence->GetTotalLength());
		pCurentSequence = pPDF->GetNextSequence(pCurentSequence);
		return GetNextPhase(pCurentSequence, pApplicableSequence, pPDF, pSignalActif, dbTimeInSequence, dbTimeTillNextPhase);
	}

}

bool ITSCAF::Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration)
{
    ControleurDeFeux* pControleurDeFeux = dynamic_cast<ControleurDeFeux*>(GetStaticParent());
    if(!pControleurDeFeux)
        return false;

    Reseau* pNetwork = pControleurDeFeux->GetReseau();

    int dbGapSendDuration = (int)(dbTimeDuration / m_iSendFrequency);
    double dbInstant = (double)(dtTimeStart - pNetwork->GetSimulationStartTime()).total_milliseconds();
    boost::posix_time::ptime dtEndTime = dtTimeStart + boost::posix_time::milliseconds((int64_t)dbTimeDuration);
    boost::posix_time::ptime dtSendStart = boost::posix_time::ptime(dtTimeStart);

    //send message for all SendFrequency
    //Data for TraficLightMessage must be the form Action;TraficLightMessage/InTronc;ID_first_TronconIn/OutTronc;ID_first_TronconOut/Pos;12.156/ActualColor;Red/NextPhase;18[/NextPhase;14][/InTronc;ID_second_TronconIn/OutTronc;ID_second_TronconOut/Pos;6.425/ActualColor;Green/NextPhase;12[/NextPhase;17]]
    while(dtSendStart < dtEndTime){
        std::string strData;
        strData = "Action;TraficLightMessage";

        PlanDeFeux*     pPDF;
        STimeSpan       tsTpsEcPlan;
        int             nSec;
        int             nTmp;
        CDFSequence*    pActualSequence, *pApplicableSequence;
        bool            bPriority = false;

        if(!pControleurDeFeux->GetPDFActif())
        {
            pPDF = pControleurDeFeux->GetTrafficLightCycle(dbInstant, tsTpsEcPlan);

            // Le plan de feux calculé est-il remplacé par un plan de feux de VGP prioritaire
            if(pControleurDeFeux->GetPDFEx() && dbInstant >= pControleurDeFeux->GetDebutPDFEx())
            {
                if(dbInstant <= pControleurDeFeux->GetFinPDFEx())
                {
                    pPDF = pControleurDeFeux->GetPDFEx().get();
                    tsTpsEcPlan = (int)(dbInstant - pControleurDeFeux->GetDebutPDFEx());
                    bPriority = true;
                }
                else
                {
                    pControleurDeFeux->GetPDFEx().reset();    // le plan de feux de VGP prioritaire est terminé
                }
            }
        }
        else
        {
            pPDF = pControleurDeFeux->GetPDFActif();
            STimeSpan   tsInst( (int)dbInstant);
            tsTpsEcPlan = pNetwork->GetSimuStartTime() + tsInst - pPDF->GetStartTime();
        }

        if(!pPDF)
            return NULL;

        // find current Sequence
        nSec = (int)tsTpsEcPlan.GetTotalSeconds( );
        nTmp = nSec % (int)pPDF->GetCycleLength();

		double  dbTpsPrevSeqs, dbTpsCurSeqs;
        pActualSequence = pPDF->GetSequence(nTmp, dbTpsPrevSeqs, dbTpsCurSeqs);

        SignalActif* pSignalActif;
        for(size_t i=0; i<pPDF->GetLstSequences().size(); i++)
        {
            pApplicableSequence = pPDF->GetLstSequences()[i];
            for(size_t j=0; j<pApplicableSequence->GetLstActiveSignals().size(); j++)
            {
                pSignalActif = pApplicableSequence->GetLstActiveSignals()[j];
                strData += "/InTronc;" + std::to_string(pSignalActif->GetInputLink()->GetID()) + "/OutTronc;" + std::to_string(pSignalActif->GetOutputLink()->GetID());

                double dbPos;
                bool positionFound = false;
                for(size_t j=0; j<pSignalActif->GetInputLink()->GetLstLanes().size() && !positionFound; j++)
                {
                    Voie* pVoie = pSignalActif->GetInputLink()->GetVoie((int)j);
                    for(size_t k=0; k<pVoie->GetLignesFeux().size() && !positionFound; k++)
                    {
                        LigneDeFeu ligne = pVoie->GetLignesFeux()[k];
                        if(ligne.m_pControleur == pControleurDeFeux && ligne.m_pTuyAm == pSignalActif->GetInputLink() && ligne.m_pTuyAv == pSignalActif->GetOutputLink())
                        {
							dbPos = ligne.m_dbPosition - pVoie->GetLength();
                            positionFound = true;
                        }
                    }
                }
                if(!positionFound)
                    dbPos = 0.0;//End of entrance lane
                strData += "/Pos;" + std::to_string(dbPos);

                //calcul mean time to reach the trafic light
                double dbTimeToReach = (m_dbRange*2) / pSignalActif->GetInputLink()->GetVitesseMax();

                strData += "/ActualColor;";

                if(pActualSequence == pApplicableSequence)
                {
                    switch(pSignalActif->GetCouleurFeux(dbTpsCurSeqs))
                    {
                        case FEU_ROUGE:
                        {
                            strData += "Red";
                            break;
                        }
                        case FEU_ORANGE:
                        {
                            strData += "Orange";
                            break;
                        }
                        case FEU_VERT:
                        {
                            strData += "Green";
                            break;
                        }
                    }
                }else{
                    strData += "Red";
                }

                //write next phases
                CDFSequence *pTempSequence = pActualSequence;
                double dbTimeInSequence = dbTpsCurSeqs;
                double dbTotalTime = 0.0;
                while(dbTimeToReach >= dbTotalTime){
                    //seconds before the next color phase
                    GetNextPhase(pTempSequence, pApplicableSequence, pPDF, pSignalActif, dbTimeInSequence, dbTotalTime);
                    strData += "/NextPhase;" + boost::posix_time::to_simple_string(dtSendStart +  boost::posix_time::milliseconds((int)(dbTotalTime * 1000000.0)));
                }
            }
        }
        SendBroadcastMessage(strData, dtSendStart, m_dbRange);

        dtSendStart = dtSendStart + boost::posix_time::milliseconds((int)dbGapSendDuration);
        dbInstant = dbInstant + dbGapSendDuration;
    }

    return true;
}

bool ITSCAF::TreatMessageData(SymuCom::Message *pMessage)
{
    //for now, ITSCAF doesn't have any message to treat. Message will be ignored
    return true;
}

bool ITSCAF::LoadFromXML(DOMDocument *pDoc, DOMNode *pXMLNode)
{
    //Nothing to do. Range value and penetration rate are already read by Simulator.
    return true;
}

ITSStation *ITSCAF::CreateCopy(const std::string& strID)
{
    ITSCAF* pNewStation = new ITSCAF(NULL, m_pNetwork, strID, m_strStationType, m_ptCoordinates, m_pCommunicationRunner->GetGraph(), m_bCanSend, m_bCanReceive, m_dbRange);
    pNewStation->m_iSendFrequency = m_iSendFrequency;

    return pNewStation;
}

int ITSCAF::GetSendFrequency() const
{
    return m_iSendFrequency;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ITSCAF::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSCAF::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSCAF::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSStation);
    ar & BOOST_SERIALIZATION_NVP(m_iSendFrequency);
}

