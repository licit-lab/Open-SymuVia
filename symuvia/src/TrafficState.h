#pragma once
#ifndef TRAFFIC_STATE_H
#define TRAFFIC_STATE_H

// author: Julien SOULA <julien.soula@cstb.fr>

#include "DLL_TrafficState.h"

#pragma warning(disable : 4251) // [...] needs to have dll-interface   //  to be used by clients of [...]

#include "TimeUtil.h"

#include <list>
#include <map>
#include <string>
#include <vector>

// Namespaces
namespace eveShared
{
    /*!
    * \short Class to describe a timespan
    */
    class TRAFFICSTATE_DLL_DEF Timespan
    {
    public:
        std::string id;
		STime begin, end;
    };

    /*!
    * \short Class to describe global simulation info
    */
    class TRAFFICSTATE_DLL_DEF SimulationInfo
    {
    public:
		std::string title, typesimulation, loipoursuite;
		STime begin, end;
		SDate date;
        double version, step;
        bool changeLane;
        int duration;
        std::vector<Timespan> timespans;
    };

	 /*!
    * \short Class to describe a level crossing
    */
    class TRAFFICSTATE_DLL_DEF LevelCrossing
    {
    public:		
        double start;
        double end;		        
		int zlevel;				
    };

    /*!
    * \short Class to describe a symuvia troncon
    */
    class TRAFFICSTATE_DLL_DEF Troncon
    {
    public:
		std::string id,id_eltamont,id_eltaval,resolution,revetement;
        unsigned nb_cell_acoustique;
        unsigned nb_voie;
        std::vector<double> largeurs_voies;
        double extremite_amont[3];
        double extremite_aval[3];		
        double vit_reg;

		 //! List of level crossings
		std::list<LevelCrossing*> LevelCrossings;

		// Coordinates of internal points that enable to build troncon as a polyline
		std::vector<double *> point_internes;
    };

    /*!
    * \short Class Symubruit Network with symuvia troncons from xml project
    */
    class TRAFFICSTATE_DLL_DEF SymuviaNetwork
    {
    public:
	    ~SymuviaNetwork();
	    static void Empty(SymuviaNetwork * pSN);

        //! List of all troncons by id
		std::map<std::string,Troncon*> troncons;
      
    };

    /*!
    * \short Class to describe an Eve Troncon
    */
    class TRAFFICSTATE_DLL_DEF EveTroncon
    {
    public:
		std::string id;

        //! Relative to Symuvia troncon
		std::string sym_troncon;
        //! True if part of crossroads/roundabout
        bool interne;

        //! Symuvia lane
        unsigned sym_voie;

        double largeur;
        double longueur;
		std::vector<double*> polyline;

		 //! List of level crossings
		std::list<LevelCrossing*> LevelCrossings;
    };

    /*!
    * \short Class Eve Network with Eve troncons from xml specific file
    */
    class TRAFFICSTATE_DLL_DEF EveNetwork
    {
    public:

        //! List of all troncons by id
		std::map<std::string,EveTroncon*> eveTroncons;

    };

    /*!
    * \short Class to describe a mobile trajectory at instant t
    */
    class TRAFFICSTATE_DLL_DEF Trajectory
    {
    public:
          //! Mobile Id
          int id;

          //! Coordinates of the vehicle
          double abs,ord,z;
          //! Section where is the mobile (for EVE)
          std::string tron;
          //! where will be the mobile after the crossroads
          std::string nextTron;
          //! Distance of mobile since the beginning of the section
          double dst;
          //! Mobile speed
          double vit;
          //! Mobile acceleration
          double acc;
          //! Mobile Type
          std::string type;
          //! Mobile Length
          double lon;
          //! Mobile max speed
          double vitMax;
    };

      /*!
    * \short Class to describe a stream at instant t
    */
    class TRAFFICSTATE_DLL_DEF Stream
    {
    public:
          //! Mobile Id
          int id;
          //! Section where is the mobile (for EVE)
          std::string tron;
   
    };

    /*!
    * \short Class to describe the state of a discretization segment
    */
    class TRAFFICSTATE_DLL_DEF Segment
    {
    public:
        //! Id
        int id;
        //! Concentration
        double conc;
        //! Flow
        double deb;
        //! Upper and Lower speeds
        double vitAm, vitAv;
        //! Upper and Lower accelerations
        double accAm, accAv;
        //! Upper and Lower quantities
        int Nam, Nav;
        //! Segment name
        std::string lib;
        //! Section
        std::string tron;
        //! Upper and Lower for x,y, and z axis
        double Xam, Xav, Yam, Yav, Zam, Zav;
    };

    /*!
    * \short Class to describe traffic lights
    */
    class TRAFFICSTATE_DLL_DEF TrafficLight
    {
    public:
        //! Traffic light controller identifier
        std::string ctrl_feux;
        //! Entrance section id
        std::string troncon_entree;
        //! Exit section id
        std::string troncon_sortie;
        //! LightState
        bool etat;
    };

    /*!
    * \short Class to describe acoustic cells
    */
    class TRAFFICSTATE_DLL_DEF Cell
    {
    public:
        //! Id
        int id;
        //! Section
        std::string tron;
        //! Vector of emission values for 8 octaves (63Hz, 125Hz, 250Hz, 500Hz, 1000Hz, 2000Hz, 4000Hz, 8000Hz) and global level
        double values[9];
        //! Upper and Lower for x,y, and z axis
        double Xam, Xav, Yam, Yav, Zam, Zav;
    };

    /*!
    * \short Class to describe acoustic sources
    */
    class TRAFFICSTATE_DLL_DEF Source
    {
    public:
        //! Id
        int id;
        //! x-axis at begin and end of time step
        double ad, af;
        //! y-axis at begin and end of time step
        double od, of;
        //! z-axis at begin and end of time step
        double zd, zf;
        //! Vector of emission values for 8 octaves (63Hz, 125Hz, 250Hz, 500Hz, 1000Hz, 2000Hz, 4000Hz, 8000Hz) and global level
        double values[9];
    };

    /*!
    * \short Class to describe optional network sensors
    */
    class TRAFFICSTATE_DLL_DEF Sensor
    {
    public:
        //! Type
		std::string type;
        //! Speed for all lanes where is the sensor
        double vit_g;
        //! Speed lane by lane // TODO : CHECK THIS
        double vit_v;
        //! Cumulated amount of mobiles for all lanes
        double nbcum_g;
        //! Cumulated amount of mobiles lane by lane // TODO : CHECK THIS
        double nbcum_v;
    };

    /*!
    * \short Class TrafficState for easy network data exchange
    */
   class TRAFFICSTATE_DLL_DEF TrafficState
   {
   public:
       TrafficState() {}
	   ~TrafficState();
	   static void Empty(TrafficState * pTS);
	   static void Copy(const TrafficState * pTSSrc, TrafficState * pTSDest);
	   TrafficState * Clone();

      //! Traffic State Instant
      double inst;

      //! List of all trajectories
      std::map<int,Trajectory*> Trajectories;
      //! List of all stream
      std::map<int,Stream*> Streams;
      //! List of all discretization segments
      std::list<Segment*> Segments;
      //! List of all traffic lights
      std::list<TrafficLight*> TrafficLights;
      //! List of all acoustic cells
      std::list<Cell*> Cells;
      //! List of all acoustic sources
      std::list<Source*> Sources;
      //! List of all network sensors (optional)
      std::list<Sensor*> Sensors;

   };
}

#endif // TRAFFIC_STATE_H
