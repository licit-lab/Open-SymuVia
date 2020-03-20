#pragma once
#ifndef ReaderWriterLockH
#define ReaderWriterLockH

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

class ReaderWriterLock
{
public:
    ReaderWriterLock();
    ~ReaderWriterLock();

    void BeginRead();
    void EndRead();

    void BeginWrite();
    void EndWrite();

private:
    boost::mutex m_Mutex;
    boost::condition_variable m_Cond;
    bool m_bHasWriter;
    unsigned int m_nbReaders;
    unsigned int m_nbMaxReaders;
};

#endif // ReaderWriterLockH