#include "stdafx.h"
#include "TimeUtil.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

#ifdef MICROSOFT
#pragma warning(disable : 4996)
#endif

using namespace std;

STime STime::FromSecond( double dSecond) 
{
    int iHour = (int) (dSecond /3600.0);
    int iMinute = (int)((dSecond - iHour*3600 ) /60.0); 
    int iSeconde = (int)(dSecond - iHour*3600 -iMinute *60); 

    return STime(iHour, iMinute, iSeconde);
}
STime::STime()
{
    m_hour = 0;
    m_minute = 0;
    m_second = 0;
}
STime::STime(int h, int m, int s)
{
    m_hour = h;
    m_minute = m;
    m_second = s;
}
void STime::Copy(const STime &T)
{
    m_hour = T.m_hour;
    m_minute = T.m_minute;
    m_second = T.m_second;
}
STime::STime(const STime &T)
{
    Copy(T);
}
STime & STime::operator=(const STime &T)
{
    Copy(T);
    return *this;
}
STime & STime::operator=(const SDateTime &T)
{
    m_hour = T.m_hour;
    m_minute = T.m_minute;
    m_second = T.m_second;
    return *this;
}
int STime::GetHour()
{
    return m_hour;
}
int STime::GetMinute()
{
    return m_minute;
}
int STime::GetSecond()
{
    return m_second;
}

int STime::ToSecond() const
{
    return m_second + m_minute *60 + m_hour *3600;
}
bool STime::operator<(const STime &T) const
{
    return Compare(this, T) < 0;
}
bool STime::operator>(const STime &T) const
{
    return Compare(this, T) > 0;
}
bool STime::operator<=(const STime &T) const
{
    return Compare(this, T) <= 0;
}
bool STime::operator>=(const STime &T) const
{
    return Compare(this, T) >= 0;
}
bool STime::operator==(const STime &T) const
{
    return Compare(this, T) == 0;
}
bool STime::operator!=(const STime &T) const
{
    return Compare(this, T) != 0;
}

STimeSpan STime::operator-(const STime &T) const
{
    STimeSpan ts = STimeSpan::Difference(*this, T);
    return ts;
}
//STimeSpan operator+(const STime &T) const
//{
//	STimeSpan ts = STimeSpan::Somme(*this, T);
//	return ts;
//}
STime STime::operator-(const STimeSpan &T) const
{
    STime ts = STimeSpan::Difference(*this, T);
    return ts;
}
STime STime::operator+(const STimeSpan &T) const
{
    STime ts = STimeSpan::Somme(*this, T);
    return ts;
}
std::string STime::ToString() const
{
    char Buf[512];
#ifdef WIN32
    sprintf_s(Buf, 512, "%02d:%02d:%02d", m_hour, m_minute, m_second);
#else
    snprintf(Buf, 512, "%02d:%02d:%02d", m_hour, m_minute, m_second);
#endif

    std::string sres = Buf;
    return sres;
}

int STime::Compare(const STime * T1, const STime &T2) const
{
	const int INF = -1;
	const int EQU = 0;
	const int SUP = 1;

	if (T1->m_hour < T2.m_hour)
	{
		return INF;
	}
	else
	{
		if (T1->m_hour > T2.m_hour)
		{
			return SUP;
		}
		else
		{
////
			if (T1->m_minute < T2.m_minute)
			{
				return INF;
			}
			else
			{
				if (T1->m_minute > T2.m_minute)
				{
					return SUP;
				}
				else
				{
					if (T1->m_second < T2.m_second)
					{
						return INF;
					}
					else
					{
						if (T1->m_second > T2.m_second)
						{
							return SUP;
						}
						else
						{
							return EQU;
						}
					}
				}
			}

////
		}
	}
}

SDateTime::SDateTime()
{
    m_hour = 0;
    m_minute = 0;
    m_second = 0;
    m_jour = 0;
    m_mois = 0;
    m_annee = 0;
}
SDateTime::SDateTime(int a, int mo, int j, int h, int mi, int s)
{
    m_hour = h;
    m_minute = mi;
    m_second = s;
    m_jour = j;
    m_mois = mo;
    m_annee = a;
}
void SDateTime::Copy(const SDateTime &T)
{
    m_hour = T.m_hour;
    m_minute = T.m_minute;
    m_second = T.m_second;
    m_jour = T.m_jour;
    m_mois = T.m_mois;
    m_annee = T.m_annee;
}
SDateTime::SDateTime(const SDateTime &T)
{
    Copy(T);
}
SDateTime & SDateTime::operator=(const SDateTime &T)
{
    Copy(T);
    return *this;
}

int SDateTime::Compare(const SDateTime * T1, const SDateTime &T2) const
{
	const int INF = -1;
	const int EQU = 0;
	const int SUP = 1;
	if (T1->m_annee < T2.m_annee)
	{
		return INF;
	}
	else
	{
		if (T1->m_annee > T2.m_annee)
		{
			return SUP;
		}
		else
		{
			if (T1->m_mois < T2.m_mois)
			{
				return INF;
			}
			else
			{
				if (T1->m_mois > T2.m_mois)
				{
					return SUP;
				}
				else
				{
///
					if (T1->m_jour < T2.m_jour)
					{
						return INF;
					}
					else
					{
						if (T1->m_jour > T2.m_jour)
						{
							return SUP;
						}
						else
						{
							if (T1->m_hour < T2.m_hour)
							{
								return INF;
							}
							else
							{
								if (T1->m_hour > T2.m_hour)
								{
									return SUP;
								}
								else
								{
////
									if (T1->m_minute < T2.m_minute)
									{
										return INF;
									}
									else
									{
										if (T1->m_minute > T2.m_minute)
										{
											return SUP;
										}
										else
										{
											if (T1->m_second < T2.m_second)
											{
												return INF;
											}
											else
											{
												if (T1->m_second > T2.m_second)
												{
													return SUP;
												}
												else
												{
													return EQU;
												}
											}
										}
									}

////
								}
							}
						}
					}

///
				}
			}
		}
	}
}

bool SDateTime::operator<(const SDateTime &T) const
{
    return Compare(this, T) < 0;
}
bool SDateTime::operator>(const SDateTime &T) const
{
    return Compare(this, T) > 0;
}
bool SDateTime::operator<=(const SDateTime &T) const
{
    return Compare(this, T) <= 0;
}
bool SDateTime::operator>=(const SDateTime &T) const
{
    return Compare(this, T) >= 0;
}
bool SDateTime::operator==(const SDateTime &T) const
{
    return Compare(this, T) == 0;
}
bool SDateTime::operator!=(const SDateTime &T) const
{
    return Compare(this, T) != 0;
}
int SDateTime::GetDay()
{
    return m_jour;
}
int SDateTime::GetMonth()
{
    return m_mois;
}
int SDateTime::GetYear()
{
    return m_annee;
}
int SDateTime::GetHour()
{
    return m_hour;
}
int SDateTime::GetMinute()
{
    return m_minute;
}
int SDateTime::GetSecond()
{
    return m_second;
}
std::string SDateTime::ToISOString()
{
    char Buf[512];		
#ifdef WIN32
    sprintf_s(Buf, 512, "%04d-%02d-%02dT%02d:%02d:%02d", m_annee, m_mois, m_jour, m_hour, m_minute, m_second);
#else
    snprintf(Buf, 512, "%04d-%02d-%02dT%02d:%02d:%02d", m_annee, m_mois, m_jour, m_hour, m_minute, m_second);		
#endif

    std::string sres = Buf;
    return sres;
}
std::string SDateTime::ToString()
{
    char Buf[512];
#ifdef WIN32
    sprintf_s(Buf, 512, "%04d/%02d/%02d %02d:%02d:%02d", m_annee, m_mois, m_jour, m_hour, m_minute, m_second);
#else
    snprintf(Buf, 512, "%04d/%02d/%02d %02d:%02d:%02d", m_annee, m_mois, m_jour, m_hour, m_minute, m_second);
#endif
    

    std::string sres = Buf;
    return sres;
}

STime SDateTime::ToTime()
{
	return STime(m_hour, m_minute, m_second);
}

SDate SDateTime::ToDate()
{
	return SDate(m_annee, m_mois, m_jour);
}

SDateTime SDateTime::Now()
{
    time_t t = time(NULL);
    struct tm * timeinfo;

    timeinfo = localtime ( &t );

    SDateTime dt((*timeinfo).tm_year+1900, 
        (*timeinfo).tm_mon+1, 
        (*timeinfo).tm_mday,
        (*timeinfo).tm_hour, 
        (*timeinfo).tm_min, 
        (*timeinfo).tm_sec);
    return dt;
}

void STimeSpan::Copy(const STimeSpan &ts)
{
    m_hour = ts.m_hour;
    m_minute = ts.m_minute;
    m_second = ts.m_second;
    m_jour = ts.m_jour;
}
STimeSpan::STimeSpan( )
{
    m_hour = 0;
    m_minute = 0;
    m_second = 0;
    m_jour = 0;
}
STimeSpan::STimeSpan(const STimeSpan &ts)
{
    Copy(ts);
}
STimeSpan::STimeSpan(
    time_t time 
    )
{
    time_t reste;
    m_second = (int)(time % 60);
    reste = time / 60;
    m_minute = (int)(reste % 60);
    reste = reste / 60;
    m_hour = (int)(reste % 24);
    m_jour = (long)(reste / 24);
}
STimeSpan::STimeSpan(
    long lJours,
    int nHeures,
    int nMinutes,
    int nSecondes 
    )
{
    m_second = nSecondes;
    m_minute = nMinutes;
    m_hour = nHeures;
    m_jour = lJours;
}
STimeSpan & STimeSpan::operator=(const STimeSpan &ts)
{
    Copy(ts);
    return *this;
}
long STimeSpan::GetDays() const
{
    return m_jour;
}
long STimeSpan::GetTotalHours() const 
{
    return 24 * m_jour + m_hour;
}
int STimeSpan::GetHours() const 
{
    return m_hour;
}
long STimeSpan::GetTotalMinutes() const 
{
    return (24 * m_jour + m_hour) * 60 + m_minute;
}
int STimeSpan::GetMinutes() const 
{
    return m_minute;
}
long STimeSpan::GetTotalSeconds() const 
{
    return ((24 * m_jour + m_hour) * 60 + m_minute) * 60 + m_second;
}
int STimeSpan::GetSeconds() const 
{
    return m_second;
}
STimeSpan STimeSpan::operator+( const STimeSpan &span )
{
    STimeSpan res;
    res.m_second = m_second + span.m_second;
    res.m_minute = m_minute + span.m_minute;
    res.m_hour = m_hour + span.m_hour;
    res.m_jour = m_jour + span.m_jour;
    RegulariseSomme(res);
    return res;
}
STimeSpan STimeSpan::operator-( const STimeSpan &span ) const 
{
    STimeSpan res;
    res.m_second = m_second - span.m_second;
    res.m_minute = m_minute - span.m_minute;
    res.m_hour = m_hour - span.m_hour;
    res.m_jour = m_jour - span.m_jour;
    RegulariseDiff(res);
    return res;
}
STimeSpan& STimeSpan::operator+=( const STimeSpan &span )
{
    m_second += span.m_second;
    m_minute += span.m_minute;
    m_hour += span.m_hour;
    m_jour += span.m_jour;
    RegulariseSomme(*this);
    return *this;
}
STimeSpan& STimeSpan::operator-=( const STimeSpan &span )
{
    m_second -= span.m_second;
    m_minute -= span.m_minute;
    m_hour -= span.m_hour;
    m_jour -= span.m_jour;
    RegulariseDiff(*this);
    return *this;
}
bool STimeSpan::operator==( const STimeSpan &span ) const 
{
    return Compare(*this, span) == 0;
}
bool STimeSpan::operator!=( const STimeSpan &span ) const 
{
    return Compare(*this, span) != 0;
}
bool STimeSpan::operator<( const STimeSpan &span ) const 
{
    return Compare(*this, span) < 0;
}
bool STimeSpan::operator>( const STimeSpan &span ) const 
{
    return Compare(*this, span) > 0;
}
bool STimeSpan::operator<=( const STimeSpan &span ) const
{
    return Compare(*this, span) <= 0;
}
bool STimeSpan::operator>=( const STimeSpan &span ) const
{
    return Compare(*this, span) >= 0;
}

void STimeSpan::RegulariseSomme(STimeSpan &ts)
{
	while (ts.m_second >= 60)
	{
		ts.m_second -= 60;
		ts.m_minute ++;
	}
	while (ts.m_minute >= 60)
	{
		ts.m_minute -= 60;
		ts.m_hour ++;
	}
	while (ts.m_hour >= 24)
	{
		ts.m_hour -= 24;
		ts.m_jour ++;
	}
}

void STimeSpan::RegulariseSomme(STime &ts)
{
	while (ts.m_second >= 60)
	{
		ts.m_second -= 60;
		ts.m_minute ++;
	}
	while (ts.m_minute >= 60)
	{
		ts.m_minute -= 60;
		ts.m_hour ++;
	}
	//if (ts.m_hour >= 24)
	//{
	//	ts.m_hour -= 24;
	//	ts.m_jour = 1;
	//}
}

void STimeSpan::RegulariseDiff(STimeSpan &ts)
{
	if (ts.m_second < 0)
	{
		ts.m_second += 60;
		ts.m_minute -= 1;
	}
	if (ts.m_minute < 0)
	{
		ts.m_minute += 60;
		ts.m_hour -= 1;
	}
	if (ts.m_hour < 0)
	{
		ts.m_hour += 24;
		ts.m_jour --;//Erreur
	}
}

void STimeSpan::RegulariseDiff(STime &ts)
{
	if (ts.m_second < 0)
	{
		ts.m_second += 60;
		ts.m_minute -= 1;
	}
	if (ts.m_minute < 0)
	{
		ts.m_minute += 60;
		ts.m_hour --;
	}
}

STimeSpan STimeSpan::Difference(const STime &T1, const STime &T2)
{
	STimeSpan ts;

	ts.m_second = T1.m_second - T2.m_second;
	ts.m_minute = T1.m_minute - T2.m_minute;
	ts.m_hour = T1.m_hour - T2.m_hour;
	RegulariseDiff(ts);
	return ts;
}

STime STimeSpan::Difference(const STime &T1, const STimeSpan &T2)
{
	STime ts;

	ts.m_second = T1.m_second - T2.m_second;
	ts.m_minute = T1.m_minute - T2.m_minute;
	ts.m_hour = T1.m_hour - T2.m_hour - 24 * T2.m_jour;
	RegulariseDiff(ts);
	return ts;
}

STime STimeSpan::Somme(const STime &T1, const STimeSpan &T2)
{
	STime ts;

	ts.m_second = T1.m_second + T2.m_second;
	ts.m_minute = T1.m_minute + T2.m_minute;
	ts.m_hour = T1.m_hour + T2.m_hour + T2.m_jour*24;
	RegulariseSomme(ts);
	return ts;
}

int STimeSpan::Compare(const STimeSpan &T1, const STimeSpan &T2)
{
	const int INF = -1;
	const int EQU = 0;
	const int SUP = 1;

	if (T1.m_jour < T2.m_jour)
	{
		return INF;
	}
	else
	{
		if (T1.m_jour > T2.m_jour)
		{
			return SUP;
		}
		else
		{
//
			if (T1.m_hour < T2.m_hour)
			{
				return INF;
			}
			else
			{
				if (T1.m_hour > T2.m_hour)
				{
					return SUP;
				}
				else
				{
///
					if (T1.m_minute < T2.m_minute)
					{
						return INF;
					}
					else
					{
						if (T1.m_minute > T2.m_minute)
						{
							return SUP;
						}
						else
						{
////
							if (T1.m_second < T2.m_second)
							{
								return INF;
							}
							else
							{
								if (T1.m_second > T2.m_second)
								{
									return SUP;
								}
								else
								{
									return EQU;
								}
							}
////
						}
					}
///
				}
			}
//
		}
	}
}

SDate::SDate()
{
    m_jour = 0;
    m_mois = 0;
    m_annee = 0;
}
SDate::SDate(int a, int m, int j)
{
    m_jour = j;
    m_mois = m;
    m_annee = a;
}
void SDate::Copy(const SDate &T)
{
    m_jour = T.m_jour;
    m_mois = T.m_mois;
    m_annee = T.m_annee;
}
SDate::SDate(const SDate &T)
{
    Copy(T);
}
SDate & SDate::operator=(const SDate &T)
{
    Copy(T);
    return *this;
}
SDate & SDate::operator=(const SDateTime &T)
{
    m_jour = T.m_jour;
    m_mois = T.m_mois;
    m_annee = T.m_annee;
    return *this;
}
std::string SDate::ToString()
{
    char Buf[512];		
#ifdef WIN32
    sprintf_s(Buf, 512, "%04d-%02d-%02d", m_annee, m_mois, m_jour);
#else
    snprintf(Buf, 512, "%04d-%02d-%02d", m_annee, m_mois, m_jour);
#endif

    std::string sres = Buf;
    return sres;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void STimeSpan::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void STimeSpan::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void STimeSpan::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_hour);
    ar & BOOST_SERIALIZATION_NVP(m_minute);
    ar & BOOST_SERIALIZATION_NVP(m_second);
    ar & BOOST_SERIALIZATION_NVP(m_jour);
}

template void SDateTime::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void SDateTime::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void SDateTime::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_hour);
    ar & BOOST_SERIALIZATION_NVP(m_minute);
    ar & BOOST_SERIALIZATION_NVP(m_second);
    ar & BOOST_SERIALIZATION_NVP(m_jour);
    ar & BOOST_SERIALIZATION_NVP(m_mois);
    ar & BOOST_SERIALIZATION_NVP(m_annee);
}

template void STime::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void STime::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void STime::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_hour);
    ar & BOOST_SERIALIZATION_NVP(m_minute);
    ar & BOOST_SERIALIZATION_NVP(m_second);
}

template void SDate::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void SDate::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void SDate::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_jour);
    ar & BOOST_SERIALIZATION_NVP(m_mois);
    ar & BOOST_SERIALIZATION_NVP(m_annee);
}