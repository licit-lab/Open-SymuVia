#pragma once

#ifndef _SYMUMASTER_HEURISTICDUALLOOPASSIGNMENTMODEL_
#define _SYMUMASTER_HEURISTICDUALLOOPASSIGNMENTMODEL_

#include "SymuMasterExports.h"

#include "MSADualLoop.h"

#include <Demand/Trip.h>

#include <random>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuMaster {


    int RandomInteger(int min, int max);

    class SYMUMASTER_EXPORTS_DLL_DEF HeuristicDualLoop : public MSADualLoop {

    public:

        //Enums
        enum MHType { MHT_SimulatedAnnealing, MHT_GeneticAlgorithm };

        HeuristicDualLoop();

        virtual ~HeuristicDualLoop();

        virtual bool NeedsParallelSimulationInstances();
        virtual int  GetMaxSimulationInstancesNumber();
        virtual bool GetNeededSimulationInstancesNumber(const std::vector<SymuCore::Trip*> &listSortedTrip, const boost::posix_time::ptime & startPeriodTime, const boost::posix_time::time_duration &PeriodTimeDuration, const boost::posix_time::time_duration& travelTimesUpdatePeriodDuration, int iCurrentIterationIndex, int iPeriod, int & nbSimulationInstances);
        virtual bool AssignPathsToTrips(const std::vector<SymuCore::Trip*> &listSortedTrip, const boost::posix_time::ptime & startPeriodTime, const boost::posix_time::time_duration &PeriodTimeDuration, const boost::posix_time::time_duration& travelTimesUpdatePeriodDuration, int &iCurrentIterationIndex, int iPeriod, int iSimulationInstance, int iSimulationInstanceForBatch, bool bAssignAllToFinalSolution);

        virtual bool onSimulationInstanceDone(const std::vector<SymuCore::Trip *> &listTrip, const boost::posix_time::ptime & endPeriodTime, int iSimulationInstance, int iSimulationInstanceForBatch, int iCurrentIterationIndex);

        //Y------------------------ Structure for SA -------------------
        enum FitnessFunctionType { CostFitness, GapFitness };
        struct Chromosome {
            std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*> > trips;
            FitnessFunctionType eFitnessFunctionType;

            void SetTrips(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath >& mapVPath)
            {
                // iterator.
                for (std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath >::iterator iterPath = mapVPath.begin(); iterPath != mapVPath.end(); ++iterPath)
                {
                    boost::shared_ptr<SymuCore::ValuetedPath> vP = iterPath->first;
                    std::vector<SymuCore::Trip*> VT = iterPath->second;

                    trips[vP] = VT;
                }
            }

            void SetTrip(boost::shared_ptr<SymuCore::ValuetedPath> vp, SymuCore::Trip * t) //std::map<SymuCore::ValuetedPath, std::vector<SymuCore::Trip>> map_VPath
            {
                trips[vp].push_back((t));
            }

            void SetTrips(boost::shared_ptr<SymuCore::ValuetedPath> vp, const std::vector<SymuCore::Trip*> & T)
            {
                //trips.insert(std::make_pair(vP, T));
                for (int i = 0; i < T.size(); i++)
                {
                    SymuCore::Trip *t = T[i];
                    trips[vp].push_back(t);
                }
            }

            std::vector<int> GetAllTripsIDs()
            {
                std::vector<int> T;

                // for each path !
                for (std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>>::iterator it = trips.begin(); it != trips.end(); ++it)
                {
                    std::vector<SymuCore::Trip*> Trs = it->second;
                    for (SymuCore::Trip * t : Trs)
                    {
                        T.push_back(t->GetID());
                    }
                }

                return T;
            }

            std::map<int, SymuCore::Trip*> GetAllTrips()
            {
                std::map<int, SymuCore::Trip*> T = std::map<int, SymuCore::Trip*>();
                for (std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>>::iterator it = trips.begin(); it != trips.end(); ++it)
                {
                    std::vector<SymuCore::Trip*> Trs = it->second;
                    for (SymuCore::Trip * t : Trs) { T[t->GetID()] = t; }
                }
                return T;
            }

            bool IsAvailable()
            {
                bool available = (trips.size() > 0);
                return available;
            }
        };
        struct DNA {
            std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, Chromosome> > chromosomes;
            FitnessFunctionType eFitnessFunctionType;

            void Reset()
            {
                chromosomes.clear();
            }

            void SetChromosoms(std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, Chromosome> > C)
            {
                // iterator.
                for (std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, Chromosome> >::iterator iterChr = C.begin(); iterChr != C.end(); ++iterChr)
                {
                    chromosomes[iterChr->first] = iterChr->second;
                }
            }

            void AddEntry(SymuCore::Origin* org, std::map<SymuCore::Destination*, Chromosome> entry)
            {
                chromosomes[org] = entry;
            }

            void AddChromosome(SymuCore::Origin* org, SymuCore::Destination* dest, Chromosome chr)
            {
                chromosomes[org][dest] = chr;
            }

            bool IsAvailable()
            {
                bool available = true;

                for (std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, Chromosome> >::iterator iterOrg = chromosomes.begin(); iterOrg != chromosomes.end(); ++iterOrg)
                {
                    SymuCore::Origin* pOrigin = iterOrg->first;	std::map<SymuCore::Destination*, Chromosome> mapDest = iterOrg->second;

                    for (std::map<SymuCore::Destination*, Chromosome> ::iterator iterDest = mapDest.begin(); iterDest != mapDest.end(); ++iterDest)
                    {
                        SymuCore::Destination* pDest = iterDest->first;	Chromosome chrA = iterDest->second;

                        if (!chrA.IsAvailable())
                        {
                            BOOST_LOG_TRIVIAL(info) << " Chromosome for [" + pOrigin->getStrNodeName() + ";" + pDest->getStrNodeName() + "] is not available ";
                            available = false;
                        }
                    }
                }
                return available;
            }
        };
        //Y------------------------END Structure for SA -------------------

        struct DNAForSubpopsAndFitness {
            std::map<SymuCore::SubPopulation*, DNA> m_DNABySubPop;
            double fitness;

            size_t chromosomeSize()
            {
                size_t nbChromosomes = 0;
                for (std::map<SymuCore::SubPopulation*, DNA>::const_iterator iter = m_DNABySubPop.begin(); iter != m_DNABySubPop.end(); ++iter)
                {
                    nbChromosomes += iter->second.chromosomes.size();
                }
                return nbChromosomes;
            }

            bool IsAvailable()
            {
                bool bIsAvailable = false;
                for (std::map<SymuCore::SubPopulation*, DNA>::iterator iter = m_DNABySubPop.begin(); iter != m_DNABySubPop.end(); ++iter)
                {
                    if (iter->second.IsAvailable())
                    {
                        bIsAvailable = true;
                    }
                }
                return bIsAvailable;
            }
        };

        struct PopuDNA
        {
            // in the pair : the DNA per SubPopulation as the first, the fitness value as the second
            std::map<int, DNAForSubpopsAndFitness> Solutions;

            void SetSolution(int key, DNAForSubpopsAndFitness s)
            {
                Solutions[key] = s;
            }

            DNAForSubpopsAndFitness GetSolution(int key)
            {
                return Solutions[key];
            }

            void AddSolution(DNAForSubpopsAndFitness s)
            {
                int key = GetNextKey();
                SetSolution(key, s);
            }

            void Remove(int key)
            {
                Solutions.erase(key);
            }

            int GetNextKey()
            {
                if (Solutions.size() == 0) return 0;
                int i = 1;
                while (Solutions.find(i) != Solutions.end()) { i++; }

                return i;
            }

            int Size()
            {
                return (int)Solutions.size();
            }

            void Clear()
            {
                Solutions.clear();
            }

            std::vector<int> GetIDs_Dnaa_Eq_Cost(double cost)
            {
                std::vector<int> L = std::vector<int>();
                for (std::map<int, DNAForSubpopsAndFitness>::iterator itSol = Solutions.begin(); itSol != Solutions.end(); ++itSol)
                {
                    DNAForSubpopsAndFitness element = itSol->second; if (cost == element.fitness) { L.push_back(itSol->first); }
                }

                return L;
            }

            void RemoveByCost(double cost)
            {
                // get all solution that have the "cost".
                std::vector<int> ids = GetIDs_Dnaa_Eq_Cost(cost);
                for (int id : ids) { Solutions.erase(id); }
            }

            DNAForSubpopsAndFitness GetRandom()
            {
                DNAForSubpopsAndFitness s;
                int v = RandomInteger(0, Size() - 1);
                int i = 0;
                for (std::map<int, DNAForSubpopsAndFitness>::iterator itSol = Solutions.begin(); itSol != Solutions.end(); ++itSol)
                {
                    DNAForSubpopsAndFitness element = itSol->second;
                    if (i == v) { s = element; }

                    i++;
                }
                return s;
            }

            void AddSolutions(const std::vector<DNAForSubpopsAndFitness> & S)
            {
                for (DNAForSubpopsAndFitness s : S) { AddSolution(s); }
            }

            void RemoveNotAvailaleSolutions()
            {
                // if is no available
                // for each DNAA.
                std::vector<int> Keys = std::vector<int>();
                for (std::map<int, DNAForSubpopsAndFitness>::iterator itSol = Solutions.begin(); itSol != Solutions.end(); ++itSol) { int idxSol = itSol->first; Keys.push_back(idxSol); }

                for (int key : Keys)
                {
                    DNAForSubpopsAndFitness s = Solutions[key];
                    if (s.chromosomeSize() == 0 || !s.IsAvailable())
                    {
                        // remove it.
                        BOOST_LOG_TRIVIAL(info) << " Solution [" + std::to_string(key) + "] is not available, it will be removed ";
                        Remove(key);
                    }
                }
            }

            void KeepOnlyKeys(std::vector<int> K)
            {
                std::vector<DNAForSubpopsAndFitness> S;
                for (int key : K){ S.push_back(GetSolution(key)); }

                // remove all
                Clear();
                AddSolutions(S);
            }

            void SetFtiness(int idx, double f)
            {
                Solutions[idx].fitness = f;
            }
        };

        //Y -- Edited by Younes ----------------------------------------------------------------

        void ClassicMSAByYounes(int iSimulationInstanceForBatch, std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath>>& listPathFound, const std::vector<SymuCore::Trip*>& listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);
        void GapBasedMSAByYounes(int iSimulationInstanceForBatch, std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath>>& listPathFound, std::vector<SymuCore::Trip *> &listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);
        void Randomize_Method(int iSimulationInstanceForBatch, std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath>>& listPathFound);

        // FIXME - factorize with MSADualLoop::UniformlySwap
        void UnifSwapByYounes(int iSimulationInstanceForBatch, int nbSwap, std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &userDistribution, boost::shared_ptr<SymuCore::ValuetedPath> actualPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> &listPathFound, const std::vector<SymuCore::Trip*>& listUsersOD, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);

        void InitDNA(mapBySubPopulationOriginDestination< std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath > > m_map, std::map<SymuCore::SubPopulation*, DNA> & DNA_Solution);
        void DNAtoMap(int iSimulationInstanceForBatch, mapBySubPopulationOriginDestination< std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath > >& m_map, std::map<SymuCore::SubPopulation*, DNA> DNA_Solution);
        void GetMapValutedFromChr(int iSimulationInstanceForBatch, Chromosome chr, std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath >& mapValuetedPath, const std::vector<boost::shared_ptr<SymuCore::ValuetedPath> > & listPathsFound);
        void DNAATemptoMapAffected();

        int Counter(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath > m_mapValPath);
        void Decision_LIQUID_Case(double& ffS2);
        void Decision_SOLID_Case(double& ffS2);
        //Y -- End Edited by Younes ----------------------------------------------------------

        void Print(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath > M);
        void Print_(std::map<SymuCore::ValuetedPath, std::vector<SymuCore::Trip>> M);
        void Print_(mapBySubPopulationOriginDestination< std::map<SymuCore::ValuetedPath, std::vector<SymuCore::Trip>> >  SoluValues, std::string st);
        void PrintMapValues(mapBySubPopulationOriginDestination< std::map<SymuCore::ValuetedPath, std::vector<SymuCore::Trip>>> SoluValues, std::string st);
        void PrintMapPointers(mapBySubPopulationOriginDestination< std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath>> m_map);
        void Print(Chromosome chr, int m);
        //void PrintChromosomeA(ChromosomeA chr, int m);
        void Print(DNA dna, char c, char tc);
        void Print(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>> M, std::string s, int p);
        void PrintSetOfPaths_Map(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath> mapTrips);
        //void Print_PathSetOf_Map();

        bool  ExistIn(std::vector<SymuCore::Trip*>, SymuCore::Trip* pTrip);

        // GENETIC ALGORITHM FUNCTIONS
        void GeneticAlgorithm();

        void GenerateRandomPopu(int n);
        void RandomSolution();
        void SetSelectedTripsDependOnMap(Chromosome& f, std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> VPs, std::map<int, std::vector<SymuCore::Trip*>> SelectedTrips);
        void RemoveDuplicateTrips(Chromosome& f1, std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> P, std::vector<int> IDTrips, std::vector<int>& TripsNotAffected, std::map<int, int>& counter);
        void FixDNAA(Chromosome c, Chromosome& f, std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> VPaths, boost::shared_ptr<SymuCore::ValuetedPath> worstPath, std::vector<int>& IDTrips);
        void CrossOver_PairChromosomes(Chromosome c1, Chromosome c2, Chromosome& f1, Chromosome& f2, std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> V, std::vector<bool> B);
        void CrossOver_PairChromosomes(Chromosome c1, Chromosome c2, Chromosome& f1, Chromosome& f2, std::vector<int>& IDTrips);
        void CrossOver_PairDNAA(DNAForSubpopsAndFitness s1, DNAForSubpopsAndFitness s2, std::vector<DNAForSubpopsAndFitness>& Childs);
        void Selection(int max);
        void CrossOver_Inner(int to_Select);
        void CrossOver(PopuDNA popu, double crossRate);
        void Replacement(int max_);

        //std::map<int, DNA> CrossOver(std::map<int, DNA> SelctPopu);
        std::map<int, DNA> Fusion(std::map<int, DNA> Popu, std::map<int, DNA> SelctPopu);
        //std::map<int, DNA> Mutation(std::map<int, DNA> Popu);;
        void GA_Inner();

        std::vector<int> KeysOfPopu(PopuDNA P);

        std::vector<SymuCore::Trip*>  GetSetTrips(std::vector<SymuCore::Trip*> listTripsByOD);

        void PrintChromosomeA(Chromosome chrA, int m);
        void PrintChromosomeAWithDetails(Chromosome chrA);

		void GroupCloseShortestPath(const std::map<SymuCore::SubPopulation*, DNA> & shortestpathCandidates);
        


    private:

        MHType m_MHType;

        // --- GA Process Methods ---- //

        Chromosome	RandomChromosome(std::vector<SymuCore::Trip*>listTripsByOD, std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> P);
        std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> GetSetPaths(std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> V);
        std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> GetValutedPointerPaths(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath> mapValuetedPath);

        /*
        void RandomSolution(bool addTOPopu);
        DNA RandomSolution(DNA org);
        void GenerateRandomPopu(int n);
        void GeneticAlgorithm();
        std::map<int, DNA> InitPopulation();
        void Selection(int max);
        //std::map<int, DNA> CrossOver(std::map<int, DNA> SelctPopu);
        std::map<int, DNA> Fusion(std::map<int, DNA> Popu, std::map<int, DNA> SelctPopu);
        //std::map<int, DNA> Mutation(std::map<int, DNA> Popu);;
        void GA_Inner();

        // Y : check the necessec
        std::vector<SymuCore::Trip>  GetSetTrips(std::vector<SymuCore::Trip*> listTripsByOD);
        std::vector<SymuCore::ValuetedPath> GetSetPaths(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath > mapValuetedPath);
        std::map<SymuCore::ValuetedPath, std::vector<SymuCore::Trip> > GetWithoutPointer(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath > mapValuetedPath);
        void ClearAllPaths(std::map<SymuCore::ValuetedPath, std::vector<SymuCore::Trip>>& map_ValutedPathWithOutPointer);
        */
        //------ END GA Process -------------- //

        //Y : Global variables for SA. ---------------------------- 
        bool m_ThisOD;

        double m_ffS1;
        double m_T0;
        int m_SA_iter;

        // -1 : not updated, 0: updated but still in swapping loop, 1: all swaping is over + updated
        int m_IsUpdated;
        // statu ! // 0 : gas, 1:liquid, 2 solid
        int m_statu_SA;

        // original solution.
        std::map<SymuCore::SubPopulation*, SymuMaster::HeuristicDualLoop::DNA> m_DNA_OriginalSolution;
        // use for Gas
        std::map<SymuCore::SubPopulation*, SymuMaster::HeuristicDualLoop::DNA> m_DNA_RandSolu;

        //DNA
        // use for Liquid
        std::map<SymuCore::SubPopulation*, SymuMaster::HeuristicDualLoop::DNA> m_DNA_Randomize_MSA;
        std::map<SymuCore::SubPopulation*, SymuMaster::HeuristicDualLoop::DNA> m_DNA_Randomize_GB;
        // for Solid
        std::map<SymuCore::SubPopulation*, SymuMaster::HeuristicDualLoop::DNA> m_DNA_MSA_Solution;
        std::map<SymuCore::SubPopulation*, SymuMaster::HeuristicDualLoop::DNA> m_DNA_GB_Solution;
        // set of fitness for differents solutions.
        double m_ffRandSol, m_ffRandMSA, m_ffRandGB, m_ffMSASol, m_ffGBSol;
        //Y :: Begin----------- GA Parameters
        bool m_isGa;
        //Y :: {End Global variables for SA} ----------------------------

        bool m_bMaximumSwappingReached;

        // Genetic algorithm members
        PopuDNA popuDnaa; // Big (Main) Population
        PopuDNA SeletedPopuDnaa; // for selection

        DNAForSubpopsAndFitness bestSol; // for Best element in GA.
        int bestsolKey;


    protected:
        virtual std::string GetAlgorithmTypeNodeName() const;
        virtual bool readAlgorithmNode(XERCES_CPP_NAMESPACE::DOMNode * pChildren);

        virtual void Decision();

        virtual bool IsInnerLoopIterationNeeded(mapBySubPopulationOriginDestination<std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> > &prevMapTripsByPath) { return true; }
        virtual void KeepBestAssignment(const std::vector<SymuCore::Trip *> &listTrip, bool IsInnerLoopConverged);

        virtual bool MaximumSwappingReached(int iCurrentIterationIndex) { return m_bMaximumSwappingReached; }

    };

}

#pragma warning(pop)

#endif // _SYMUMASTER_HEURISTICDUALLOOPASSIGNMENTMODEL_
