// HEX SDK

#include <cassert>
#include <cstdio>
#include <string>
#include <string.h>
#include <unistd.h>

#include <hex/hex_impl.h>
#include <hex/topten.h>

#ifdef HEX_DEV
//#define TOPTEN_DEBUG
#endif

namespace hex_sdk {

#ifdef HEX_PROD

inline
void TopTen::check() { }

#else

void TopTen::check()
{
    assert(m_dataArray.size() == m_maxEntries);
    assert(m_keyArray.size() == m_eventTimeArray.size());
    assert(m_keyArray.size() + m_freePool.size() == m_maxEntries);

#ifdef TOPTEN_DEBUG
    printf("TopTen::check()\n");
    printf("  m_maxEntries = %zu\n", m_maxEntries);
    printf("  m_dataArray (size = %zu) =\n", m_dataArray.size());
    for (size_t i = 0; i < m_dataArray.size(); ++i) {
        printf("  %zu (%p) k=\"%s\" t=%lu c=%zu\n",
            i, &m_dataArray[i], m_dataArray[i].key.c_str(), m_dataArray[i].eventTime, m_dataArray[i].count);
    }
    printf("  m_keyArray (size = %zu) =\n", m_keyArray.size());
    for (size_t i = 0; i < m_keyArray.size(); ++i) {
        printf("  %zu (%p) k=\"%s\" t=%lu c=%zu\n",
            i, m_keyArray[i], m_keyArray[i]->key.c_str(), m_keyArray[i]->eventTime, m_keyArray[i]->count);
    }
    printf("  m_eventTimeArray (size = %zu) =\n", m_eventTimeArray.size());
    for (size_t i = 0; i < m_eventTimeArray.size(); ++i) {
        printf("  %zu (%p) k=\"%s\" t=%lu c=%zu\n",
            i, m_eventTimeArray[i], m_eventTimeArray[i]->key.c_str(), m_eventTimeArray[i]->eventTime, m_eventTimeArray[i]->count);
    }
#endif

    for (size_t i = 0; i < m_keyArray.size(); ++i) {
        // Data pointers must be valid
        assert(m_keyArray[i] >= &m_dataArray[0] && m_keyArray[i] <= &m_dataArray[m_maxEntries - 1]);

        // Keys must be sorted in ascending order and unique
        if (i + 1 < m_keyArray.size())
            assert(m_keyArray[i]->key < m_keyArray[i + 1]->key);

        // Data elements must be unique
        for (size_t j = i + 1; j < m_keyArray.size(); ++j)
            assert(m_keyArray[i] != m_keyArray[j]);
    }

    for (size_t i = 0; i < m_eventTimeArray.size(); ++i) {
        // Data pointers must be valid
        assert(m_eventTimeArray[i] >= &m_dataArray[0] && m_eventTimeArray[i] <= &m_dataArray[m_maxEntries - 1]);

        // Event times must be sorted in descending order
        // (but not necessarily unique)
        if (i + 1 < m_eventTimeArray.size())
            assert(m_eventTimeArray[i]->eventTime >=
                   m_eventTimeArray[i + 1]->eventTime);

        // Data elements must be unique
        for (size_t j = i + 1; j < m_eventTimeArray.size(); ++j)
            assert(m_eventTimeArray[i] != m_eventTimeArray[j]);
    }
}

#endif

TopTen::TopTen(size_t maxEntries, time_t interval, time_t maxAge)
    : m_maxEntries(maxEntries),
      m_interval(interval),
      m_maxAge(maxAge)
{
    m_dataArray.reserve(m_maxEntries);
    m_keyArray.reserve(m_maxEntries);
    m_eventTimeArray.reserve(m_maxEntries);
    m_freePool.reserve(m_maxEntries);

    m_dirty = true;

    m_timeLastHousekeeping = 0;

    clear();
}

void TopTen::clear()
{
    m_dirty = true;

    m_dataArray.clear();
    m_keyArray.clear();
    m_eventTimeArray.clear();
    m_freePool.clear();

    // Populate free list
    m_dataArray.resize(m_maxEntries);
    for (size_t i = m_maxEntries; i > 0; --i)
        m_freePool.push_back(&m_dataArray[i - 1]);

    check();
}

void TopTen::update(const std::string& key, size_t count, time_t eventTime, const char *cachePath)
{
#ifdef TOPTEN_DEBUG
    printf("TopTen::update(): k=\"%s\", c=%zu, t=%lu\n", key.c_str(), count, eventTime);
#endif

    housekeeping(eventTime, cachePath);

    if (key.empty() || count == 0)
        return;

    // Does key already exist?
    KeyArray::iterator keyIter = findKey(key);
    if (keyIter != m_keyArray.end() && key == (*keyIter)->key) {
        // Key exists, update it's count
        Data *pData = *keyIter;
        pData->count += count;

        // If this event is more recent we need to update it's time too
        if (eventTime > pData->eventTime) {
            // Search for corresponding entry in event time array
            // Remove it
            EventTimeArray::iterator eventTimeIter = findEventTime(pData);
             assert(eventTimeIter != m_eventTimeArray.end() && *eventTimeIter == pData);
            m_eventTimeArray.erase(eventTimeIter);

            // Add new entry with updated time
            pData->eventTime = eventTime;
            m_eventTimeArray.insert(findEventTime(eventTime), pData);
        }
    } else {
        // Key not found, we'll need to add it
        // Is there room?
        if (m_keyArray.size() == m_maxEntries) {
            // All full, let's make some room by removing the oldest entry
            KeyArray::iterator removedIter = removeOldest();
            // If removed key was before our insert position, we need to adjust our iterator
            if (removedIter < keyIter)
                --keyIter;
#ifdef HEX_DEV
            // Verify that our iterator is pointing in the correct location
            KeyArray::iterator testIter = findKey(key);
            assert(testIter == keyIter);
#endif
        }

        // Get new data element from free pool
        assert(!m_freePool.empty());
        Data *pData = m_freePool.back();
        m_freePool.pop_back();

        // Initialize our new element
        pData->key = key;
        pData->eventTime = eventTime;
        pData->count = count;

        // Insert into key and event time arrays
        m_keyArray.insert(keyIter, pData);
        m_eventTimeArray.insert(findEventTime(eventTime), pData);
    }

    m_dirty = true;

    check();
}

void TopTen::housekeeping(time_t currTime, const char *cachePath)
{
    if (m_timeLastHousekeeping == 0 ||
        m_timeLastHousekeeping + m_interval < currTime) {
        m_timeLastHousekeeping = currTime;

        time_t minEventTime = currTime - m_maxAge;

        while (!m_eventTimeArray.empty() &&
               m_eventTimeArray.front()->eventTime < minEventTime)
            (void)removeOldest();

        if (cachePath != NULL)
            serialize(cachePath);
    }
}

void TopTen::getResults(Results &results) const
{
    DataArray dataCopy(m_dataArray);
    std::sort(dataCopy.begin(), dataCopy.end(), CountGreater());

    results.numEntries = 0;

    size_t n = (m_dataArray.size() < 10) ? m_dataArray.size() : 10;

    for (size_t i = 0; i < n; ++i) {
        if (dataCopy[i].count == 0)
            break;
        results.keys[i] = dataCopy[i].key;
        results.eventTimes[i] = dataCopy[i].eventTime;
        results.counts[i] = dataCopy[i].count;
        ++results.numEntries;
    }

    // Clear out any unused entries
    for (size_t i = results.numEntries; i < 10; ++i) {
        results.keys[i].erase();
        results.eventTimes[i] = 0;
        results.counts[i] = 0;
    }

#ifdef TOPTEN_DEBUG
    printf("getResults():\n");
    printf("  numEntries = %zu\n", results.numEntries);
    for (size_t i = 0; i < 10; ++i) {
        printf("  %zu k=\"%s\" t=%lu c=%zu\n",
            i, results.keys[i].c_str(), results.eventTimes[i], results.counts[i]);
    }
#endif
}

static const char s_magic[8] = { 'p', 'r', 'o', 'v', 't', 't', 'e', 'n' };

bool TopTen::serialize(const char *path)
{
    if (m_dirty) {
        unlink(path);

        FILE *fout = fopen(path, "wb");
        if (!fout)
            return false;

        if (fwrite(s_magic, 1, sizeof(s_magic), fout) != sizeof(s_magic)) {
            fclose(fout);
            return false;
        }

        // Number of keys
        size_t numKeys = m_keyArray.size();
        if (fwrite((char *)&numKeys, 1, sizeof(numKeys), fout) != sizeof(numKeys)) {
            fclose(fout);
            return false;
        }

        for (size_t i = 0; i < numKeys; ++i) {
            // Length of key name
            size_t len = m_keyArray[i]->key.length();
            if (fwrite((char *)&len, 1, sizeof(len), fout) != sizeof(len)) {
                fclose(fout);
                return false;
            }
            // Key name
            const char *p = m_keyArray[i]->key.data();
            if (fwrite((char *)p, 1, len, fout) != len) {
                fclose(fout);
                return false;
            }
            // Event time
            time_t eventTime = m_keyArray[i]->eventTime;
            if (fwrite((char *)&eventTime, 1, sizeof(eventTime), fout) != sizeof(eventTime)) {
                fclose(fout);
                return false;
            }
            // Event count
            size_t count = m_keyArray[i]->count;
            if (fwrite((char *)&count, 1, sizeof(count), fout) != sizeof(count)) {
                fclose(fout);
                return false;
            }
        }

        fclose(fout);

        m_dirty = false;
    }

    return true;
}

bool TopTen::deserialize(const char *path)
{
    if (access(path, F_OK) == 0) {
        clear();

        // Rename input file in case deserialization crashes current process
        std::string newPath(path);
        newPath += ".tmp";
        unlink(newPath.c_str());
        rename(path, newPath.c_str());

        FILE *fin = fopen(newPath.c_str(), "rb");
        if (!fin)
            return false;

        char magic[sizeof(s_magic)];
        if (fread(magic, 1, sizeof(s_magic), fin) != sizeof(s_magic) ||
            memcmp(magic, s_magic, sizeof(s_magic)) != 0) {
            fclose(fin);
            return false;
        }
        // Number of keys
        size_t numKeys;
        if (fread((char *)&numKeys, 1, sizeof(numKeys), fin) != sizeof(numKeys)) {
            fclose(fin);
            return false;
        }

        std::vector<char> buf;
        for (size_t i = 0; i < numKeys; ++i) {
            // Length of key name
            size_t len;
            if (fread((char *)&len, 1, sizeof(len), fin) != sizeof(len)) {
                fclose(fin);
                return false;
            }
            // Key name
            buf.resize(len+1);
            if (fread((char *)&buf[0], 1, len, fin) != len) {
                fclose(fin);
                return false;
            }
            buf[len] = '\0';
            // Event time
            time_t eventTime;
            if (fread((char *)&eventTime, 1, sizeof(eventTime), fin) != sizeof(eventTime)) {
                fclose(fin);
                return false;
            }
            // Event count
            size_t count;
            if (fread((char *)&count, 1, sizeof(count), fin) != sizeof(count)) {
                fclose(fin);
                return false;
            }

            update(&buf[0], count, eventTime);
        }

        fclose(fin);

        unlink(newPath.c_str());

        check();

        // Rewrite file after deserialization is successful
        serialize(path);
    }

    return true;
}

TopTen::KeyArray::iterator TopTen::removeOldest()
{
    // Remove oldest entry from event time array
    Data *pData = m_eventTimeArray.back();
    m_eventTimeArray.pop_back();

#ifdef TOPTEN_DEBUG
    printf("TopTen::removeOldest(): (%p) k=\"%s\", c=%zu, t=%lu\n",
        pData, pData->key.c_str(), pData->count, pData->eventTime);
#endif

    // Remove corresponding entry from key array
    KeyArray::iterator keyIter = findKey(pData->key);
    assert(keyIter != m_keyArray.end());
    m_keyArray.erase(keyIter);

    // Clear data and put back onto the free pool
    pData->key.erase();
    pData->eventTime = 0;
    pData->count = 0;
    m_freePool.push_back(pData);
    m_dirty = true;

    check();

    return keyIter;
}

} // namespace hex_sdk

