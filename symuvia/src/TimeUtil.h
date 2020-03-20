#pragma once

#ifndef TimeUtilHeader
#define TimeUtilHeader

#include "DLL_TrafficState.h"

#include <string>
#include <time.h>

namespace boost {
    namespace serialization {
        class access;
    }
}

class STime;

class TRAFFICSTATE_DLL_DEF STimeSpan
{
private:
  int m_hour;
  int m_minute;
  int m_second;
  long m_jour;

public:
  void Copy(const STimeSpan &ts);
  STimeSpan( );
  STimeSpan(const STimeSpan &ts);
  STimeSpan(
    time_t time 
    );
  STimeSpan(
    long lJours,
    int nHeures,
    int nMinutes,
    int nSecondes 
    );
  STimeSpan & operator=(const STimeSpan &ts);
  long GetDays() const;
  long GetTotalHours() const;
  int GetHours() const;
  long GetTotalMinutes() const;
  int GetMinutes() const;
  long GetTotalSeconds() const;
  int GetSeconds() const;


  STimeSpan operator+( const STimeSpan &span );
  STimeSpan operator-( const STimeSpan &span ) const;
  STimeSpan& operator+=( const STimeSpan &span );
  STimeSpan& operator-=( const STimeSpan &span );
  bool operator==( const STimeSpan &span ) const ;
  bool operator!=( const STimeSpan &span ) const ;
  bool operator<( const STimeSpan &span ) const ;
  bool operator>( const STimeSpan &span ) const ;
  bool operator<=( const STimeSpan &span ) const;
  bool operator>=( const STimeSpan &span ) const;

  static void RegulariseSomme(STimeSpan &ts);
  static void RegulariseSomme(STime &ts);
  static void RegulariseDiff(STimeSpan &ts);
  static void RegulariseDiff(STime &ts);
  static STimeSpan Difference(const STime &T1, const STime &T2);
  static STime Difference(const STime &T1, const STimeSpan &T2);
  static STime Somme(const STime &T1, const STimeSpan &T2);
  static int Compare(const STimeSpan &T1, const STimeSpan &T2);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
friend class boost::serialization::access;
template<class Archive>
void serialize(Archive & ar, const unsigned int version);
};

class SDate;

class TRAFFICSTATE_DLL_DEF SDateTime
{
public:
  int m_hour;
  int m_minute;
  int m_second;
  int m_jour;
  int m_mois;
  int m_annee;
public:
  SDateTime();
  SDateTime(int a, int mo, int j, int h, int mi, int s);
  void Copy(const SDateTime &T);
  SDateTime(const SDateTime &T);
  SDateTime & operator=(const SDateTime &T);

  int Compare(const SDateTime * T1, const SDateTime &T2) const;

  bool operator<(const SDateTime &T) const;
  bool operator>(const SDateTime &T) const;
  bool operator<=(const SDateTime &T) const;
  bool operator>=(const SDateTime &T) const;
  bool operator==(const SDateTime &T) const;
  bool operator!=(const SDateTime &T) const;
  int GetDay();
  int GetMonth();
  int GetYear();
  int GetHour();
  int GetMinute();
  int GetSecond();
  std::string ToISOString();
  std::string ToString();

  STime ToTime();

  SDate ToDate();

  static SDateTime Now();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
friend class boost::serialization::access;
template<class Archive>
void serialize(Archive & ar, const unsigned int version);
};

class TRAFFICSTATE_DLL_DEF STime
{
public:
  int m_hour;
  int m_minute;
  int m_second;
public:
  STime();
  STime(int h, int m, int s);
  void Copy(const STime &T);
  STime(const STime &T);
  STime & operator=(const STime &T);
  STime & operator=(const SDateTime &T);
  int GetHour();
  int GetMinute();
  int GetSecond();
  int ToSecond() const;
  bool operator<(const STime &T) const;
  bool operator>(const STime &T) const;
  bool operator<=(const STime &T) const;
  bool operator>=(const STime &T) const;
  bool operator==(const STime &T) const;
  bool operator!=(const STime &T) const;

  STimeSpan operator-(const STime &T) const;
  //STimeSpan operator+(const STime &T) const
  //{
  //	STimeSpan ts = STimeSpan::Somme(*this, T);
  //	return ts;
  //}
  STime operator-(const STimeSpan &T) const;
  STime operator+(const STimeSpan &T) const;
  std::string ToString() const;
  int Compare(const STime * T1, const STime &T2) const;

  static STime FromSecond( double dSecond) ;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
friend class boost::serialization::access;
template<class Archive>
void serialize(Archive & ar, const unsigned int version);
};

class TRAFFICSTATE_DLL_DEF SDate
{
public:
  int m_jour;
  int m_mois;
  int m_annee;
public:
  SDate();
  SDate(int a, int m, int j);
  void Copy(const SDate &T);
  SDate(const SDate &T);
  SDate & operator=(const SDate &T);
  SDate & operator=(const SDateTime &T);
  std::string ToString();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
friend class boost::serialization::access;
template<class Archive>
void serialize(Archive & ar, const unsigned int version);
};

#endif
