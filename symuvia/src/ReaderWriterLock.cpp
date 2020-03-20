#include "stdafx.h"
#include "ReaderWriterLock.h"

#include <boost/thread.hpp>

ReaderWriterLock::ReaderWriterLock()
{
    m_bHasWriter = false;
    m_nbReaders = 0;

    // On n'autorise pas plus d'un reader par thread (ne sert à rien de faire plus
    // et permet de se protéger d'une explosion de mémoire consommée si on lance trop de requête le lecture en parallele)
    m_nbMaxReaders = boost::thread::hardware_concurrency();
}

ReaderWriterLock::~ReaderWriterLock()
{
}

void ReaderWriterLock::BeginRead()
{
    boost::unique_lock<boost::mutex> lock(m_Mutex);

    while (m_bHasWriter || m_nbReaders == m_nbMaxReaders)
    {
        m_Cond.wait(lock);
    }

    m_nbReaders++;
}

void ReaderWriterLock::EndRead()
{
    boost::unique_lock<boost::mutex> lock(m_Mutex);

    m_nbReaders--;

    m_Cond.notify_one();
}

void ReaderWriterLock::BeginWrite()
{
    boost::unique_lock<boost::mutex> lock(m_Mutex);

    while (m_bHasWriter || m_nbReaders > 0)
    {
        m_Cond.wait(lock);
    }

    m_bHasWriter = true;
}

void ReaderWriterLock::EndWrite()
{
    boost::unique_lock<boost::mutex> lock(m_Mutex);

    m_bHasWriter = false;

    m_Cond.notify_all();
}
