#include "stdafx.h"
#include "repartiteur.h"

#include "ControleurDeFeux.h"
#include "tuyau.h"
#include "voie.h"
#include "reseau.h"
#include "Logger.h"

#include <boost/serialization/vector.hpp>

using namespace std;

//---------------------------------------------------------------------------
// Constructeurs, destructeurs et assimilés
//--------------------------------------------------------------------------

// Destructeur
Repartiteur::~Repartiteur()
{
}

Repartiteur::Repartiteur()
{
    m_cType             = 'H';
}

// Constructeur normal : premier constructeur
Repartiteur::Repartiteur(std::string _Nom, char _Type, Reseau *pR): ConnectionPonctuel(_Nom, pR)
{
    // Affectation des variables
    m_cType             =_Type;

    m_NbEltAmont = 0;
    m_NbEltAval = 0;
}

Repartiteur::Repartiteur(std::string _Nom,char _Type, double _Abscisse,double _Ordonnee, double _Rayon,double _DCycle,double _DOrange, Reseau *pR) : ConnectionPonctuel(_Nom, pR)
{
    // Affectation des variables
    m_cType             =_Type;
    Abscisse        = _Abscisse;
    Ordonne         = _Ordonnee;
    rayon           = _Rayon;

    m_NbEltAmont = 0;
    m_NbEltAval = 0;
}

// constructeur complet du répartiteur
    void Repartiteur::Constructeur_complet(int nb_amont ,int nb_aval, bool bInit)
{  //Construction des listes et tableaux necessitant le nombre d'elements aval/amont
    int i;

    m_NbEltAmont = nb_amont;
    m_NbEltAval = nb_aval;
    debit_entree.resize(nb_amont);
    debit_sortie.resize(nb_aval);
    sortie_j_fluide.resize(nb_aval);
    entree_i_fluide.resize(nb_amont);

    for(i=0; i<nb_amont; i++)
    {
        debit_entree[i].resize(m_nNbMaxVoies);
        entree_i_fluide[i].resize(m_nNbMaxVoies);
        for(int j=0; j<m_nNbMaxVoies;j++)
        {
            debit_entree[i][j] = 0;
            entree_i_fluide[i][j] = false;
        }
    }

    for(i=0; i<nb_aval; i++)
    {
        debit_sortie[i].resize(m_nNbMaxVoies);
        sortie_j_fluide[i].resize(m_nNbMaxVoies);
        for(int j=0; j<m_nNbMaxVoies;j++)
        {
            debit_sortie[i][j] = 0;
            sortie_j_fluide[i][j] = false;
        }
    }

}

// création automatique des mouvements autorisés pour un répartiteur généré automatiquement pour l'insertion
void Repartiteur::CreateMouvementsAutorises(int nbVoiesInsertionDroite, int nbVoiesInsertionGauche, const std::string & strTAvPrincipal)
{
    // EVOLUTION n°70 : cas de l'échangeur : on peut avoir plusieurs tronçons aval
    // assert(m_LstTuyAv.size() == 1);
    assert(m_LstTuyAm.size() == 1);

    Tuyau* pTuyAm = m_LstTuyAm[0];
    Tuyau* pTuyAv = NULL;
    if(m_LstTuyAv.size() == 1)
    {
        pTuyAv = m_LstTuyAv[0];
    }
    else
    {
        // EVOLUTION n°70 : cas de l'échangeur : on peut avoir plusieurs tronçons aval
        for(size_t iTuyAvalSec = 0; iTuyAvalSec < m_LstTuyAv.size(); iTuyAvalSec++)
        {
            if(!m_LstTuyAv[iTuyAvalSec]->GetLabel().compare(strTAvPrincipal))
            {
                pTuyAv = m_LstTuyAv[iTuyAvalSec];
                break;
            }
        }
    }
    
    // pour toutes les voies en aval
    for(int i = 0; i < pTuyAv->getNb_voies(); i++)
    {
        int inVoieAmont = i+nbVoiesInsertionDroite;
	    boost::shared_ptr<MouvementsSortie>  MS(new MouvementsSortie());
        MS->m_pReseau = m_pReseau;
	    MS->m_dbTaux = 1;
	    MS->m_strLibelle = "";
	    MS->m_dbLigneFeu = 0.0;
        MS->m_dbVitMax = 0.0;
        MS->m_bHasVitMax = false;
        std::map<int, boost::shared_ptr<MouvementsSortie> > mapTaux;
	    mapTaux.insert( pair<int, boost::shared_ptr<MouvementsSortie> >(i, MS) );
        std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> > mapTAv;
        mapTAv.insert( pair<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> > >(pTuyAv, mapTaux));
        this->m_mapMvtAutorises.insert( pair<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >>(pTuyAm->GetVoie(inVoieAmont), mapTAv) );
    }

    /// EVOLUTION n°70 - cas de l'échangeur avec plus d'un tuyau aval.
    for(size_t iTuyAvalSec = 0; iTuyAvalSec < m_LstTuyAv.size(); iTuyAvalSec++)
    {
        Tuyau* pTuyAvSec = m_LstTuyAv[iTuyAvalSec];
        if(pTuyAvSec != pTuyAv)
        {
            // pour toutes les voies en aval
            for(int i = 0; i < pTuyAvSec->getNb_voies(); i++)
            {
                int inVoieAmont = i;
	            boost::shared_ptr<MouvementsSortie> MS(new MouvementsSortie());
                MS->m_pReseau = m_pReseau;
	            MS->m_dbTaux = 1;
	            MS->m_strLibelle = "";
	            MS->m_dbLigneFeu = 0.0;
                MS->m_dbVitMax = 0.0;
                MS->m_bHasVitMax = false;
                std::map<int, boost::shared_ptr<MouvementsSortie> > mapTaux;
	            mapTaux.insert( pair<int, boost::shared_ptr<MouvementsSortie> >(i, MS) );
                std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> > mapTAv;
                mapTAv.insert( pair<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >>(pTuyAvSec, mapTaux));
                this->m_mapMvtAutorises.insert( pair<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >>(pTuyAm->GetVoie(inVoieAmont), mapTAv) );
            }
        }
    }
}

//================================================================
    void Repartiteur::FinInit
//----------------------------------------------------------------
// Fonction  : Post traitements en fin de chargement
// Version du: 21/06/2013
// Historique: 21/06/2013 (O.Tonck - Ipsis)
//             Création
//================================================================        
(
    Logger   *pOfLog
)
{
    if(m_pCtrlDeFeux)
    {
        int    nVAv;
        TuyauMicro *pTAv;
        std::map<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >, LessPtr<Voie> >::iterator itMvtAutorises;

        // Vérification de la cohérence de la définition des lignes de feux :
        // une ligne de feux à l'intérieur d'un CAF ne peut être positionnée que sur le premier tronçon interne du mouvement correspondant !
        for(itMvtAutorises = m_mapMvtAutorises.begin(); itMvtAutorises != m_mapMvtAutorises.end(); itMvtAutorises++)
	    {
            Voie* pVoieEntree = itMvtAutorises->first;
            std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> > ::iterator itSorties;
		    for(itSorties = (*itMvtAutorises).second.begin(); itSorties != (*itMvtAutorises).second.end(); itSorties++)            
            {
                // Tronçon de sortie                            
			    pTAv = (TuyauMicro*)((*itSorties).first);

			    std::map<int, boost::shared_ptr<MouvementsSortie> >::iterator itVoiesSortie;
			    for(itVoiesSortie = (*itSorties).second.begin(); itVoiesSortie != (*itSorties).second.end(); itVoiesSortie++)        
			    {
				    nVAv = (*itVoiesSortie).first;

                    LigneDeFeu ligneFeu;
                    ligneFeu.m_pTuyAv = pTAv;
                    ligneFeu.m_pTuyAm = (Tuyau*)pVoieEntree->GetParent();
                    ligneFeu.nVAm = pVoieEntree->GetNum();
                    ligneFeu.nVAv = nVAv;
                    ligneFeu.m_pControleur = m_pCtrlDeFeux;

                    if(itVoiesSortie->second->m_dbLigneFeu > 0)
                    {
                        itVoiesSortie->second->m_dbLigneFeu = 0.0;
                        *pOfLog << Logger::Warning << "WARNING : The traffic light line position of a repartitor cannot be after the repartitor (strictly positive value). The traffic light lines of the repartitor " << GetID() << " have been pulled back to the start of the downstream link." << std::endl;
                        *pOfLog << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                    }

                    double dbLengthAmont = pVoieEntree->GetLength();
                    if(dbLengthAmont < -itVoiesSortie->second->m_dbLigneFeu)
                    {
                        // on ramène la ligne à la longueur du tronçon aval
                        itVoiesSortie->second->m_dbLigneFeu = -dbLengthAmont + 0.0001;
                        *pOfLog << Logger::Warning << "WARNING : The traffic light lines of the repartitor " << GetID() << " have been pulled back to the start of the upstream link." << std::endl;
                        *pOfLog << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

                    }

                    ligneFeu.m_dbPosition = dbLengthAmont+itVoiesSortie->second->m_dbLigneFeu;
                    pVoieEntree->GetLignesFeux().push_back(ligneFeu);
                }
            }
        }
    }

    // Calcul des angles des mouvements, pour détermination des tourne à droite ou gauche
    Tuyau * pUpstream, * pDownstream;
    for(size_t i = 0; i < m_LstTuyAm.size(); i++)
    {
        pUpstream = m_LstTuyAm[i];

        // Récupération du segment aval du tuyau amont
        double dbXAm2 = pUpstream->GetAbsAval();
        double dbYAm2 = pUpstream->GetOrdAval();
        double dbXAm1, dbYAm1;
        if(pUpstream->GetLstPtsInternes().empty())
        {
            dbXAm1 = pUpstream->GetAbsAmont();
            dbYAm1 = pUpstream->GetOrdAmont();
        }
        else
        {
            // protection contre les doublons ...
            for(int k = (int)pUpstream->GetLstPtsInternes().size()-1; k >= -1 ; k--)
            {
                if(k == -1)
                {
                    dbXAm1 = pUpstream->GetAbsAmont();
                    dbYAm1 = pUpstream->GetOrdAmont();
                }
                else
                {
                    dbXAm1 = pUpstream->GetLstPtsInternes()[k]->dbX;
                    dbYAm1 = pUpstream->GetLstPtsInternes()[k]->dbY;
                    if(dbXAm1 != dbXAm2 || dbYAm1 != dbYAm2)
                    {
                        // On continue tant qu'on n'a pas trouvé un point différent du premier
                        break;
                    }
                }
            }
        }

        for(size_t j = 0; j < m_LstTuyAv.size(); j++)
        {
            pDownstream = m_LstTuyAv[j];

            double dbXAv1 = pDownstream->GetAbsAmont();
            double dbYAv1 = pDownstream->GetOrdAmont();
            double dbXAv2, dbYAv2;
            if(pDownstream->GetLstPtsInternes().empty())
            {
                dbXAv2 = pDownstream->GetAbsAval();
                dbYAv2 = pDownstream->GetOrdAval();
            }
            else
            {
                for(size_t k = 0; k < pDownstream->GetLstPtsInternes().size()+1; k++)
                {
                    if(k == pDownstream->GetLstPtsInternes().size())
                    {
                        dbXAv2 = pDownstream->GetAbsAval();
                        dbYAv2 = pDownstream->GetOrdAval();
                    }
                    else
                    {
                        dbXAv2 = pDownstream->GetLstPtsInternes()[k]->dbX;
                        dbYAv2 = pDownstream->GetLstPtsInternes()[k]->dbY;
                        if(dbXAv2 != dbXAv1 || dbYAv2 != dbYAv1)
                        {
                            // On continue tant qu'on n'a pas trouvé un point différent du premier
                            break;
                        }
                    }
                }
            }

            if((dbXAv1 != dbXAv2 || dbYAv1 != dbYAv2) && (dbXAm1 != dbXAm2 || dbYAm1 != dbYAm2))
            {
                // On ramène le segment du tuyau aval sur le point amont du segment amont
                // pour avoir les trois points nécessaires au calcul de l'angle qui nous intéresse
                double dbAngle = AngleOfView(dbXAm1, dbYAm1, dbXAm2, dbYAm2, dbXAv2 + (dbXAm1 - dbXAv1), dbYAv2 + (dbYAm1 - dbYAv1));

                // On passe en degrés car la méthode ComputeCost compare avec la fourchette d'angle définie pour les tout droit qui est exprimé en degrés
                dbAngle *= 180.0/M_PI;
                double dbCost;
                if(dbAngle <= m_pReseau->GetAngleToutDroit())
                {
                    // Cas du tout droit légèrement à gauche
                    dbCost = 0;
                }
                else if(dbAngle <= 180)
                {
                    // Cas du tourne à gauche
                    dbCost = m_pReseau->GetNonPriorityPenalty();
                }
                else if(dbAngle < 360 - m_pReseau->GetAngleToutDroit())
                {
                    // Cas du tourne à droite
                    dbCost = m_pReseau->GetRightTurnPenalty();
                }
                else
                {
                    // Cas du tout droit légèrement à droite
                    dbCost = 0;
                }
                for (size_t iTypeVeh = 0; iTypeVeh < m_pReseau->m_LstTypesVehicule.size(); iTypeVeh++)
                {
                    m_LstCoutsAVide[std::make_pair(pUpstream, pDownstream)][m_pReseau->m_LstTypesVehicule[iTypeVeh]] = dbCost;
                }
            }
            else if(dbXAv1 != dbXAv2 || dbYAv1 != dbYAv2)
            {
                *pOfLog << Logger::Warning << "The link " << pDownstream->GetLabel() << " has punctual geometry, impossible to determine the movement's angle." << std::endl;
                for (size_t iTypeVeh = 0; iTypeVeh < m_pReseau->m_LstTypesVehicule.size(); iTypeVeh++)
                {
                    m_LstCoutsAVide[std::make_pair(pUpstream, pDownstream)][m_pReseau->m_LstTypesVehicule[iTypeVeh]] = 0;
                }
            }
            else if(dbXAm1 != dbXAm2 || dbYAm1 != dbYAm2)
            {
                *pOfLog << Logger::Warning << "The link " << pUpstream->GetLabel() << " has a punctual geometry, impossible to determine the movement's angle." << std::endl;
                for (size_t iTypeVeh = 0; iTypeVeh < m_pReseau->m_LstTypesVehicule.size(); iTypeVeh++)
                {
                    m_LstCoutsAVide[std::make_pair(pUpstream, pDownstream)][m_pReseau->m_LstTypesVehicule[iTypeVeh]] = 0;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------
// Fonctions relatives à la simulation des répartiteurs
//--------------------------------------------------------------------------

// Initialisation de la simulation
void Repartiteur::InitSimuTrafic(void)
{
    Vmax.resize(m_LstTuyAv.size());
    for(size_t i=0; i<m_LstTuyAv.size(); i++)
        Vmax[i].resize(m_nNbMaxVoies);
}

// Terminaison de la simulation
void Repartiteur::FinSimuTrafic(void)
{

}

// ------------------------------------
// ------------------------------------
// Simulation effective des répartiteur
//void Repartiteur::simulation(double t, double pasTmps) $$$
void Repartiteur::CalculEcoulementConnections
(
    double dbt                       // Instant de la simulation
)
{
    if( IsMicro() )
        return;

    int         i, j, k, l;    

    double **     Di; // Tableau des Demandes de chaque entrée
    double **     Dj; // Tableau des Demandes de chaque sortie
    double **     Oj; // Tableau des Offres de chaque sortie
    double **     Qj; // Tableau des Débits de chaque sortie
    double **     Qi; // Tableau des Débits de chaque entrée
    double ****   Dij;  // Tableau à 2 dim demande de i vers j
    double ****   Qij; // Tableau à 2 dim débit de i vers j
    double **     Di_feux; // Tableau des demandes de chaque entrée en tenant compte des feux
    double **     Dj_feux; // Tableau des demandes de chaque sortie en tenant compte des feux

    // Cette procédure doit être appelée uniquement dans le cas d'un répartiteur de flux
    //if( !IsFlux() )
      //  return;

    // Allocation des tableaux temporaires
    Di = new double*[(int) m_LstTuyAm.size()];
    AllocTabDblElt(Di, (int) m_LstTuyAm.size());

    Dj = new double*[(int) m_LstTuyAv.size()];
    AllocTabDblElt(Dj, (int) m_LstTuyAv.size());

    Oj = new double*[(int) m_LstTuyAv.size()];
    AllocTabDblElt(Oj, (int) m_LstTuyAv.size());

    Qi = new double*[(int) m_LstTuyAm.size()];
    AllocTabDblElt(Qi, (int) m_LstTuyAm.size());

    Qj = new double*[(int) m_LstTuyAv.size()];
    AllocTabDblElt(Qj, (int) m_LstTuyAv.size());

    Di_feux = new double*[(int) m_LstTuyAm.size()];
    AllocTabDblElt(Di_feux, (int) m_LstTuyAm.size());

    Dj_feux = new double*[(int) m_LstTuyAv.size()];
    AllocTabDblElt(Dj_feux, (int) m_LstTuyAv.size());

    Dij=new double***[(int) m_LstTuyAm.size()];
    for(i=0;i<(int) m_LstTuyAm.size();i++)
    {
        Dij[i]=new double**[m_nNbMaxVoies];
        for(k=0;k<m_nNbMaxVoies;k++)
        {
            Dij[i][k] = new double*[(int) m_LstTuyAv.size()];
            AllocTabDblElt(Dij[i][k], (int) m_LstTuyAv.size());
        }
    }

    Qij=new double***[(int) m_LstTuyAm.size()];
    for(i=0;i<(int) m_LstTuyAm.size();i++)
    {
        Qij[i]=new double**[m_nNbMaxVoies];
        for(k=0;k<m_nNbMaxVoies;k++)
        {
            Qij[i][k] = new double*[(int) m_LstTuyAv.size()];
            AllocTabDblElt(Qij[i][k], (int) m_LstTuyAv.size());
        }
    }

    // On récupère la demande des tronçons se situant en amont du répartiteur
    // On la stocke dans Di
	std::deque<Tuyau*>::iterator itT;
	i = 0;

	for(itT = m_LstTuyAm.begin(); itT != m_LstTuyAm.end(); itT++)
	{
		for(k=0; k<m_nNbMaxVoies; k++)
		{
			Di[i][k]= (*itT)->GetDemande(k);
		}        
		i++;
	}

    // On récupère l'offre des tronçons se situant en aval du répartiteur
    // On la stocke dans O	
	i = 0;

	for(itT = m_LstTuyAv.begin(); itT != m_LstTuyAv.end(); itT++)    
    {
        for(k=0; k<m_nNbMaxVoies; k++)
            Oj[i][k]=(*itT)->GetOffre(k);

		i++;
    }

    // On calcule la demande par origine destination en tenant compte des coefficients directionnels
    // et de l'état des feux pour répartir la demande de chaque entrée
    // On stocke le tout dans Dij
    // On en profite pour déterminer pour chaque entrée la demande i qui a la potentialité de s'écouler (la demande
    // ne peut s'écouler si le feu de i vers j est au rouge.
    // On stocke la demande pouvant potentiellement s'écouler dans Di_feux

	ControleurDeFeux *pCDF = GetCtrlDeFeux();
	double dbPropVertOrange;

    // rmq. : les coefficients directionnels sont à présent définis par type de véhicule : on utilise la répartition du type de véhicule principal ici (le premier de la liste par convention)
    TypeVehicule * pTypeVeh = m_pReseau->GetLstTypesVehicule().front();

	i = 0;
	for(itT = m_LstTuyAm.begin(); itT != m_LstTuyAm.end(); itT++)    
    {
        for(k=0; k<m_nNbMaxVoies; k++)
        {			
            Di_feux[i][k]=0;
			std::deque<Tuyau*>::iterator itTav;
			j = 0;

			for(itTav = m_LstTuyAv.begin(); itTav != m_LstTuyAv.end(); itTav++)
			{
				if( pCDF )
					GetCtrlDeFeux()->GetStatutCouleurFeux(dbt, (*itT), (*itTav), NULL, &dbPropVertOrange );
				else 
					dbPropVertOrange = 1;

                for(l=0; l<m_nNbMaxVoies; l++)
                {
                    Dij[i][k][j][l]= GetRepartition(pTypeVeh, i, k, j, l, dbt)* Di[i][k]* dbPropVertOrange;
                    Di_feux[i][k]+=Dij[i][k][j][l];
                }
				j++;
			}
		}
		i++;
    }

    // Calcul des débits de sortie. Ce calcul s'effectue en déterminant tout d'abord
    // la demande voulant sortir à chaque sortie (Dj) comme la somme des demandes Dij pour toutes
    // les entrées i. Cette demande est comparée à l'offre de la sortie (Oj). Le débit (Qj) est le
    // minimum des deux. Si la demande prédomine la sortie est fluide (sortie_j_fluide = true) sinon
    // la sortie est congestionnée.

	j = 0;
	for(itT = m_LstTuyAv.begin(); itT != m_LstTuyAv.end(); itT++)    
    {
        for(l=0; l< m_nNbMaxVoies; l++)
        {
            Dj[j][l]=0;

            for(i=0;i<  (int) m_LstTuyAm.size();i++)
            {
                for(k=0; k< m_nNbMaxVoies; k++)
                    Dj[j][l]+=Dij[i][k][j][l];  // Calcul de la demande pour la sortie j
            }

            if(Dj[j][l]<=Oj[j][l]) // Sortie fluide
            {
                Qj[j][l]=Dj[j][l]; // Cas fluide le débit est égal à la demande
                sortie_j_fluide[j][l]=true;
            }
            else // Sortie congestionnée
            {
                Qj[j][l]=Oj[j][l];
                sortie_j_fluide[j][l]=false;
            }
        }
		j++;
    }

    // Calcul des débits par origine/destination. Le débit Qij est calculé à partir du débit
    // sortant de j (Qj) au prorata de la demande de i vers j par rapport à la demande pour la sortie j
	i = 0;
	for(itT = m_LstTuyAm.begin(); itT != m_LstTuyAm.end(); itT++)
	{
        for(k=0; k<m_nNbMaxVoies;k++)
        {
            for(j=0;j< (int) m_LstTuyAv.size();j++)
                for(l=0; l<m_nNbMaxVoies;l++)
                {
                    if(Dj[j][l]==0) // La demande vers la sortie j est nulle
                        Qij[i][k][j][l]=0; // Tous les débits vers j sont donc nuls
                    else
                        Qij[i][k][j][l]=Dij[i][k][j][l]*Qj[j][l]/Dj[j][l]; // Qj au prorata de Dij sur Dj
                }
        }
		i++;
	}

    // Calcul des débits pour chaque entrée (Qi) comme la somme des Qij pour tous les j
    // On détermine ensuite si l'entrée est fluide (Qi=Di) ou si elle est congestionnée (Qi<Di)
    // NB: si la demande pouvant potentiellement s'écouler est nulle (Di_Feux=0) alors que la demande
    // est non nulle (cas d'un feu rouge qui bloque toute la voie) alors l'entrée est considérée comme
    // congestionnée
    // A vérifier : Ce dernier calcul est peut-être redondant
    for(i=0;i<  (int) m_LstTuyAm.size();i++)
        for(k=0; k<m_nNbMaxVoies;k++)
        {
            Qi[i][k]=0;
            for(j=0;j< (int) m_LstTuyAv.size();j++)
                for(l=0; l<m_nNbMaxVoies;l++)
                    Qi[i][k]+=Qij[i][k][j][l];

            if(Qi[i][k]==Di[i][k])
                entree_i_fluide[i][k]=true;
            else
                entree_i_fluide[i][k]=false;

            if(Di_feux[i][k]==0 && Di[i][k] != 0)
                entree_i_fluide[i][k]=false;
           
        }

    // Affectation des des débits entrant Qi sortant Qj dans les tableaux les stockant

    for(i=0;i<  (int) m_LstTuyAm.size();i++)
    {
        for(k=0; k<m_nNbMaxVoies; k++)
            debit_entree[i][k]=Qi[i][k];     // le débit est affecté
    }

    for(j=0;j<  (int) m_LstTuyAv.size();j++)
    {
        for(l=0; l<m_nNbMaxVoies; l++)
            debit_sortie[j][l] = Qj[j][l];      // le débit est affecté
    }

    // Calcul des vitesses de sortie
    CalculVitMaxSortie(dbt);

    DesallocTabDblElt( Di, (int) m_LstTuyAm.size());
    DesallocTabDblElt( Qi, (int) m_LstTuyAm.size());
    DesallocTabDblElt( Dj, (int) m_LstTuyAv.size());
    DesallocTabDblElt( Oj, (int) m_LstTuyAv.size());
    DesallocTabDblElt( Qj, (int) m_LstTuyAv.size());
    DesallocTabDblElt( Di_feux, (int) m_LstTuyAm.size());
    DesallocTabDblElt( Dj_feux, (int) m_LstTuyAv.size());

    for(i=0;i<(int) m_LstTuyAm.size();i++)
    {
        for(k=0;k<m_nNbMaxVoies;k++)
        {
            DesallocTabDblElt(Dij[i][k], (int) m_LstTuyAv.size());
        }
        delete [] Dij[i];
    }
    delete [] Dij;
    Dij = NULL;

    for(i=0;i<(int) m_LstTuyAm.size();i++)
    {
        for(k=0;k<m_nNbMaxVoies;k++)
        {
            DesallocTabDblElt(Qij[i][k], (int) m_LstTuyAv.size());
        }
        delete [] Qij[i];
    }
    delete [] Qij;
    Qij = NULL;

}

//--------------------------------------------------------------------------
// Fonctions liées à la gestion des autobus
//--------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Fonctions de renvoi l'état du répartiteur
//--------------------------------------------------------------------------

// renvoi du débit de sortie vers le troncon n
double Repartiteur::Debit_sortie(std::string n, int nVoie)
{
	int i=0;
	std::deque<Tuyau*>::iterator itT;

	for(itT = m_LstTuyAv.begin(); itT != m_LstTuyAv.end(); itT++)
	{
		if(! (*itT)->GetLabel().compare(n) )
		{
			return debit_sortie[i][nVoie];
		}
		i++;
	}
  
  return 0;
}

// renvoi du débit d'entrée provenant du troncon n
double Repartiteur::Debit_entree(std::string n, int nVoie)
{
	int i=0;
	std::deque<Tuyau*>::iterator itT;

	for(itT = m_LstTuyAm.begin(); itT != m_LstTuyAm.end(); itT++)
	{                                                    // En amont
		if(! (*itT)->GetLabel().compare(n) )
		{
		 return debit_entree[i][nVoie];
		}
	}
	return 0;
}

// renvoi l'état de l'entrée avec le troncon n
bool Repartiteur::get_entree_fluide(std::string n, int nVoie)
{
	int i=0;
	std::deque<Tuyau*>::iterator itT;

	for(itT = m_LstTuyAm.begin(); itT != m_LstTuyAm.end(); itT++)
	{
		if(! (*itT)->GetLabel().compare(n) )
		{
			return entree_i_fluide[i][nVoie];
		}
	}
  return true;
}

// renvoi l'état de la sortie vers le troncon n
bool Repartiteur::get_sortie_fluide(std::string n, int nVoie)
{
	int i=0;
	std::deque<Tuyau*>::iterator itT;

	for(itT = m_LstTuyAv.begin(); itT != m_LstTuyAv.end(); itT++)
	{
		if(! (*itT)->GetLabel().compare(n) )
		{
		 return sortie_j_fluide[i][nVoie];
		}
  }
  return true;
}

// renvoi la vitesse maximal autorisée vers le tronçon n
double Repartiteur::getVitesseMaxEntree(std::string n, int nVoie)
{
	//renvoie la vitesse maximale en entrée concernant le tronçon demandeur
	std::deque<Tuyau*>::iterator itT;
	int i = 0;

	for(itT = m_LstTuyAv.begin(); itT != m_LstTuyAv.end(); itT++)
	{
		if(! (*itT)->GetLabel().compare(n) )
			return Vmax[i][nVoie];

		i++;
    }
  
  return 0;
}

//---------------------------------------------------------------------------
// Fonctions utilitaires
//--------------------------------------------------------------------------


// Calcul le rayon du répartiteur
void Repartiteur:: calculRayon(void)
        {
        double i;int c;

		i=sqrtl((((m_LstTuyAm.front())->GetAbsAval() -Abscisse )* ((m_LstTuyAm.front())->GetAbsAval() -Abscisse )   +  ((m_LstTuyAm.front())->GetOrdAval() -Ordonne )* ((m_LstTuyAm.front())->GetOrdAval() -Ordonne ) ));

        for(c=0;c<(int) m_LstTuyAm.size();c++)
                {
                  if (i>sqrtl((((m_LstTuyAm[c])->GetAbsAval() -Abscisse )* ((m_LstTuyAm[c])->GetAbsAval() -Abscisse )   +  ((m_LstTuyAm[c])->GetOrdAval() -Ordonne )* ((m_LstTuyAm[c])->GetOrdAval() -Ordonne  ) )))
                        {
                            i=sqrtl((((m_LstTuyAm[c])->GetAbsAval() -Abscisse )* ((m_LstTuyAm[c])->GetAbsAval() -Abscisse )   +  ((m_LstTuyAm[c])->GetOrdAval() -Ordonne )* ((m_LstTuyAm[c])->GetOrdAval() -Ordonne ) ));
                        }

                 }
       for(c=0;c<(int) m_LstTuyAv.size();c++)
                {
                  if (i>sqrtl((((m_LstTuyAv[c])->GetAbsAmont() -Abscisse )* ((m_LstTuyAv[c])->GetAbsAmont() -Abscisse  )   +  ((m_LstTuyAv[c])->GetOrdAmont() -Ordonne )* ((m_LstTuyAv[c])->GetOrdAmont() -Ordonne ) )))
                        {
                            i=sqrtl((((m_LstTuyAv[c])->GetAbsAmont() -Abscisse )* ((m_LstTuyAv[c])->GetAbsAmont() -Abscisse )   +  ((m_LstTuyAv[c])->GetOrdAmont() -Ordonne  )* ((m_LstTuyAv[c])->GetOrdAmont() -Ordonne ) ));
                        }

                 }
          rayon=i;

        if(rayon<=0)   // pour qu'on voit le repartitur ds tous les cas
                rayon=2;

        }


//---------------------------------------------------------------------------
// Fonctions de renvoi des paramètres du répartiteur
//--------------------------------------------------------------------------

// renvoi le nombre de tronçons amont
int Repartiteur::getNbAmont(void)
        { return (int) m_LstTuyAm.size(); }

//---------------------------------------------------------------------------
// Fonctions de modifications des paramètres du répartieur
//--------------------------------------------------------------------------

// Affecte le type du répartiteur
void Repartiteur::SetType(char c)
        { m_cType=c; }

double Repartiteur::GetVitesseMaxSortie(int n, int nVoie, double dbTime)
{
    int         i, k;
    double      dbVitMax;
	ControleurDeFeux *pCDF = GetCtrlDeFeux();

    // Initialisation de la vitesse max à la vitesse libre
    dbVitMax=m_LstTuyAv[n]->GetVitesseMax();

	i=0;
	std::deque<Tuyau*>::iterator itT;

    // rmq. : les coefficients directionnels sont à présent définis par type de véhicule : on utilise la répartition du type de véhicule principal ici (le premier de la liste par convention)
    TypeVehicule * pTypeVeh = m_pReseau->GetLstTypesVehicule().front();

	for(itT = m_LstTuyAm.begin(); itT != m_LstTuyAm.end(); itT++)
	{		
		if( !pCDF || ( pCDF->GetStatutCouleurFeux(dbTime, (*itT), m_LstTuyAv[n]) == FEU_VERT )	) // Pas de feux ou le feu est vert
		{
			// La vitesse max est calculée comme le minimum des vitesses max des voies en amont du répartiteur
			for(k=0; k<m_nNbMaxVoies ;k++)
			{
				// La vitesse max de la voie en amont est considérée ssi la répartition vers la voie en aval traitée est non nulle            
                if ((*itT)->GetVitesseMaxSortie(k)<dbVitMax && GetRepartition(pTypeVeh, i, k, n, nVoie, dbTime) >0)
				{
					dbVitMax=(*itT)->GetVitesseMaxSortie(k);
				}
			}
		}
		i++;
    }

    return dbVitMax;
}

//================================================================
    void Repartiteur::AllocTabDblElt
//----------------------------------------------------------------
// Fonction  : 
// Version du: 11/01/2005
// Historique: 11/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double **       pTab,
    int             nNbElem
)
{
    int i;

    for(i=0;i<nNbElem;i++)
    {
        pTab[i]=new double[m_nNbMaxVoies];
    }
}

//================================================================
    void Repartiteur::DesallocTabDblElt
//----------------------------------------------------------------
// Fonction  :
// Version du: 11/01/2005
// Historique: 11/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double **       pTab,
    int             nNbElem
)
{
    int i;
    
    if(!pTab)
        return;

    for(i=0;i<nNbElem;i++)
    {
        delete []pTab[i];
    }
    delete []pTab;

    pTab = NULL;
}

//================================================================
    void Repartiteur::CalculVitMaxSortie
//----------------------------------------------------------------
// Fonction   :  Calcul des vitesses maximales en sortie
// Remarque   :  Cette vitesse est déterminée comme le minimum des
//               vitesses maximales au niveau des différentes entrées
//               pour lesquelles le feu est vert
// Version du :  06/01/2005
// Historique :  06/01/2005 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    double  dbTime
)
{
    int l;

    for(int j=0;j<(int)m_LstTuyAv.size();j++)
    {
        for(l=0; l<m_nNbMaxVoies; l++)
            Vmax[j][l] = GetVitesseMaxSortie(j,l, dbTime);
    }
}


//================================================================
    void Repartiteur:: MiseAJourSAS
//----------------------------------------------------------------
// Fonction   :  Met à jour les SAS entrée des voies aval d'un
//               répartiteur à partir des SAS sortie des voies amont
// Remarque   :  Cette mise à jour peut sembler inutile car les SAS
//               après le calcul de débit doivent toujours être complémentaire...
//               Or dans le cas de la création d'un véhicule sur une voie amont vide,
//               la demande est modifiée et cette modification doit être pris
//               en compte dans les tuyaux en aval. Ceci n'est pas le cas pour l'instant
//               dans l'algorithme d'où cette verrue...
//               (doit aussi résoudre le pb lorsque au cours du mëme pas de temps
//               plusieurs véhicules sont créés sur la même voie...)
// Version du :  01/09/2006
// Historique :  01/09/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    double dbInstant
)
{
    int         i,j,k,l;
    Tuyau*      pTuyauAmont;
    Tuyau*      pTuyauAval;
    VoieMicro*  pVoieAmont;
    VoieMicro*  pVoieAval;
    double      dbRepartition;
    double      dbOld;
	std::deque<Tuyau*>::iterator itTAm, itTAv;

    if( IsMicro() )
        return;

    // Mise à 0	
    for(itTAv=m_LstTuyAv.begin(); itTAv != m_LstTuyAv.end(); itTAv++)
    {
        pTuyauAval = (*itTAv);

        if( pTuyauAval->GetResolution() == RESOTUYAU_MICRO )
        {
            for(l=0;l<pTuyauAval->getNb_voies();l++)
            {
                pVoieAval = (VoieMicro*)pTuyauAval->GetLstLanes()[l];
                pVoieAval->SetNbVehSASEntree(0);
            }
        }		
    }

    // rmq. : les coefficients directionnels sont à présent définis par type de véhicule : on utilise la répartition du type de véhicule principal ici (le premier de la liste par convention)
    TypeVehicule * pTypeVeh = m_pReseau->GetLstTypesVehicule().front();
	
	i = 0;
	for(itTAm=m_LstTuyAm.begin(); itTAm != m_LstTuyAm.end(); itTAm++)
    {
        pTuyauAmont = (*itTAm);

        if( pTuyauAmont->GetResolution() == RESOTUYAU_MICRO )
        {
			 j = 0;
             for(itTAv=m_LstTuyAv.begin(); itTAv != m_LstTuyAv.end(); itTAv++)
             {
                pTuyauAval = (*itTAv);

                if( pTuyauAval->GetResolution() == RESOTUYAU_MICRO )
                {
                    for(k=0;k<pTuyauAmont->getNb_voies();k++)
                    {
                        pVoieAmont = (VoieMicro*)pTuyauAmont->GetLstLanes()[k];

                        for(l=0;l<pTuyauAval->getNb_voies();l++)
                        {
                            pVoieAval = (VoieMicro*)pTuyauAval->GetLstLanes()[l];

                            dbRepartition = GetRepartition(pTypeVeh,i, k, j, l, dbInstant) / 100.;

                            dbOld = pVoieAval->GetNbVehSASEntree();
                            pVoieAval->SetNbVehSASEntree(dbOld + dbRepartition*(1 - pVoieAmont->GetNbVehSASSortie()) );

                        }                            
                    }
                }
				j++;
             }
        }
		i++;
    }

}

//================================================================
   void Repartiteur::IncNbVehRep
//----------------------------------------------------------------
// Fonction   : Incrémente le compteur du nombre de véhicule
//              traversant le répartiteur pour l'origine et la
//              destination passées en paramètre. Chaque cycle de
//              feu possède son propre compteur, ce qui permet de
//              connaître le nombre de véhicule traversant à chaque
//              cycle.
// Version du :  24/11/2006
// Historique :  24/11/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    TypeVehicule* pTypeVeh, // Type de véhicule (Ajouté par cohérence avec l'ajout du type de véhicule pour les coefficients directionnels, mais fonction inutilisée de toute façon.
    int nAmont,             // Numéro du tronçon amont
    int nVoieAmont,         // Numéro de voie du tronçon amont
    int nAval,              // Numéro du tronçon aval
    int nVoieAval,          // Numéro de voie du tronçon aval
    double dbTime           // Instant de traversée du répartiteur
)

{
    RepartitionFlux*    pRF;

    // Vérification
    if( nAmont >= (int) m_LstTuyAm.size() || nAval >= (int) m_LstTuyAv.size() )
        return ;
    if(  nVoieAmont >= m_nNbMaxVoies || nVoieAmont >= m_nNbMaxVoies )
        return ;

    // Recherche de la répartition active
    std::map<TypeVehicule*, std::deque<TimeVariation<RepartitionFlux> >* >::iterator iter = m_mapLstRepartition.find(pTypeVeh);
    if (iter == m_mapLstRepartition.end())
        return;

    pRF = GetVariation(dbTime, iter->second, m_pReseau->GetLag());

    if(pRF)
        pRF->nNbVeh[nAmont*m_nNbMaxVoies + nVoieAmont][nAval*m_nNbMaxVoies + nVoieAval]++;
}

//================================================================
   int Repartiteur::GetNbVehRep
//----------------------------------------------------------------
// Fonction   :  Retourne le nombre de véhicule comptabilisé
// Version du :  27/11/2006
// Historique :  27/11/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    TypeVehicule* pTypeVeh, // Type de véhicule (Ajouté par cohérence avec l'ajout du type de véhicule pour les coefficients directionnels, mais fonction inutilisée de toute façon.
    int nAmont,             // Numéro du tronçon amont
    int nVoieAmont,         // Numéro de voie du tronçon amont
    int nAval,              // Numéro du tronçon aval
    int nVoieAval,          // Numéro de voie du tronçon aval
    int nVar                // Indice de la variation
)

{
   std::deque <RepartitionFlux>::iterator cur, debut, fin;

    // Vérification
    if( nAmont >= (int) m_LstTuyAm.size() || nAval >= (int) m_LstTuyAv.size() )
        return 0;
    if(  nVoieAmont >= m_nNbMaxVoies || nVoieAmont >= m_nNbMaxVoies )
        return 0;

    std::map<TypeVehicule*, std::deque<TimeVariation<RepartitionFlux> >* >::iterator iter = m_mapLstRepartition.find(pTypeVeh);
    if (iter == m_mapLstRepartition.end())
        return 0;

    return iter->second->at(nVar).m_pData->nNbVeh[nAmont*m_nNbMaxVoies + nVoieAmont][nAval*m_nNbMaxVoies + nVoieAval];
}


//! Calcule le cout d'un mouvement autorisé
double Repartiteur::ComputeCost(TypeVehicule* pTypeVeh, Tuyau* pTuyauAmont, Tuyau * pTuyauAval)
{
    double dbResult = 0;
    // Si le tuyau amont a des temps mesurés, pas la peine de pénaliser davantage (ca ferait doublon)
    if(pTuyauAmont->HasRealTravelTime())
    {
        return dbResult;
    }

    std::pair<Tuyau*,Tuyau*> myPair = std::make_pair(pTuyauAmont, pTuyauAval);
    std::map<std::pair<Tuyau*,Tuyau*>, std::map<TypeVehicule*, double> >::const_iterator iter = m_LstCoutsAVide.find(myPair);
    if(iter != m_LstCoutsAVide.end())
    {
        dbResult = iter->second.at(pTypeVeh);
    }
    return dbResult;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void Repartiteur::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Repartiteur::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Repartiteur::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ConnectionPonctuel);

    ar & BOOST_SERIALIZATION_NVP(m_cType);
    ar & BOOST_SERIALIZATION_NVP(rayon);
    ar & BOOST_SERIALIZATION_NVP(debit_entree);
    ar & BOOST_SERIALIZATION_NVP(debit_sortie);
    ar & BOOST_SERIALIZATION_NVP(sortie_j_fluide);
    ar & BOOST_SERIALIZATION_NVP(entree_i_fluide);
    ar & BOOST_SERIALIZATION_NVP(Vmax);
}