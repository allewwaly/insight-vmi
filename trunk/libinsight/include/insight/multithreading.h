#ifndef MULTITHREADING_H
#define MULTITHREADING_H

#include <QThread>

/**
 * This static class allows to specify the max. number of threads to use for
 * multi-threaded jobs. The default maximum is 8.
 */
class MultiThreading
{
public:
    /**
     * Returns the current maximum thread count. This will never be larger
     * than QThread::idealThreadCount().
     */
    inline static int maxThreads()
    {
        return qMin(_maxThreads, QThread::idealThreadCount());
    }

    /**
     * Sets the maximum no. of threads to use.
     * @param maxThreads
     */
    inline static void setMaxThreads(int maxThreads)
    {
        Q_ASSERT(maxThreads > 0);
        _maxThreads = maxThreads;
    }

private:
    MultiThreading() {}
    static int _maxThreads;
};

#endif // MULTITHREADING_H
