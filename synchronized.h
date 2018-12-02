#ifndef SYNCHRONIZED_H
#define SYNCHRONIZED_H

#include <mutex>

template <typename T>
class Synchronized
{
public:
    explicit Synchronized(T initial = T()) :
        m_value(initial)
    {}

    class Access
    {
    public:
        T& ref_to_value;
        Access(T& val, std::mutex& m) :
            ref_to_value(val),
            m_guard(m)
        {}
    private:
        std::lock_guard<std::mutex> m_guard;
    };

    Access GetAccess()
    {
        return Access(m_value, m_mutex);
    }
private:
    std::mutex m_mutex;
    T m_value;
};

#endif // SYNCHRONIZED_H
