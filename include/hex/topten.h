// HEX SDK

#ifndef HEX_TOPTEN_H
#define HEX_TOPTEN_H

// Config API requires C++
#ifdef __cplusplus

#include <algorithm>
#include <ctime> // time_t
#include <string>
#include <vector>

namespace hex_sdk {

// Array of top ten most frequently occuring events
class TopTen {
public:
    TopTen(size_t maxEntries = 1024,
           time_t interval = 10 * 60 /* 10 min */,
           time_t maxAge = 7 * 24 * 60 * 60 /* 1 week */);

    void clear();

    void update(const std::string& key, size_t count, time_t eventTime, const char *cachePath = NULL);

    void housekeeping(time_t currTime, const char *cachePath);

    struct Results {
        size_t numEntries;
        std::string keys[10];
        time_t eventTimes[10];
        size_t counts[10];
    };

    void getResults(Results& results) const;

    bool serialize(const char *path);

    bool deserialize(const char *path);

private:
    // Internal data

    // Maximum number of items in the list
    const size_t m_maxEntries;

    // Housekeeping interval
    const time_t m_interval;

    // Maximum age of entries in the list
    const time_t m_maxAge;

    // True if top ten list has been modified since it was last serialized
    bool m_dirty;

    // Time of last housekeeping
    time_t m_timeLastHousekeeping;

    struct Data {
        Data() : key(), eventTime(0), count(0) { }
        Data(const std::string& k, time_t t, size_t c)
            : key(k), eventTime(t), count(c) { }
        std::string key;
        time_t eventTime;
        size_t count;
    };

    // Array of entries in unsorted order
    // Some holes may exist in the array as indicated by the free list
    typedef std::vector<Data> DataArray;
    DataArray m_dataArray;

    // Array of indices into data vector sorted by keys in ascending order
    typedef std::vector<Data *> KeyArray;
    KeyArray m_keyArray;

    // Array of indices into data vector sorted chronologically by event time in
    // ascending order (oldest/smallest-value first)
    typedef std::vector<Data *> EventTimeArray;
    EventTimeArray m_eventTimeArray;

    // Array of indices into data vector for freed locations
    typedef std::vector<Data *> FreePool;
    FreePool m_freePool;

    // Comparison functionals

    // Sort by keys only in ascending order
    struct KeyLess {
        bool operator()(const Data *x, const Data *y) { return x->key.compare(y->key) < 0; }
    };

    // Sort by event time in descending order
    struct EventTimeGreater {
        // Oldest elements will have the lowest time value in seconds since the Epoch
        bool operator()(const Data *x, const Data *y) { return (x->eventTime > y->eventTime); }
    };

    // Sort by count in descending order
    // and then by event times in descending order (most recent first) if counts are equal
    // and then by keys in ascending order if event times are equal
    struct CountGreater {
        bool operator()(const Data& x, const Data& y) {
            return ((x.count == y.count) ?
                    ((x.eventTime == y.eventTime) ?
                     (x.key.compare(y.key) < 0) :
                     (x.eventTime > y.eventTime)) :
                    (x.count > y.count));
        }
    };

    // Internal methods

    // Search for location where key can be inserted to preserve sorted order
    // or return location where key already exists
    KeyArray::iterator findKey(const std::string& key);

    // Search for location where event time can be inserted to preserve sorted order
    EventTimeArray::iterator findEventTime(time_t eventTime);

    // Search for the event time in the event time list
    // Return location of current data element
    EventTimeArray::iterator findEventTime(Data *pData);

    // Remove oldest entry and return iterator to key removed
    KeyArray::iterator removeOldest();

    void check();
};

// Inline methods

inline
TopTen::KeyArray::iterator TopTen::findKey(const std::string& key)
{
    Data data(key, 0, 0);
    return std::lower_bound(m_keyArray.begin(), m_keyArray.end(),
                            &data, KeyLess());
}

inline
TopTen::EventTimeArray::iterator TopTen::findEventTime(time_t eventTime)
{
    Data data(std::string(), eventTime, 0);
    return std::lower_bound(m_eventTimeArray.begin(), m_eventTimeArray.end(),
                            &data, EventTimeGreater());
}

inline
TopTen::EventTimeArray::iterator TopTen::findEventTime(Data *pData)
{
    TopTen::EventTimeArray::iterator it;
    // Binary search for beginning of range of elements that have same event time
    it = std::lower_bound(m_eventTimeArray.begin(), m_eventTimeArray.end(),
                          pData, EventTimeGreater());
    // Linear search for this specific data element
    while (it != m_eventTimeArray.end() && *it != pData && (*it)->eventTime == pData->eventTime)
        ++it;
    return it;
}

} // namespace hex_sdk

#endif // __cplusplus

#endif /* endif HEX_TOPTEN_H */

