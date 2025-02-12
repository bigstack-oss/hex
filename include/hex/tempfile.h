// HEX SDK

#ifndef HEX_TEMPFILE_H
#define HEX_TEMPFILE_H

#ifdef __cplusplus

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <hex/log.h>

// RAII (Resource Acquisition Is Initialization, exception-safe) class for creating temporary files
class HexTempFile
{
public:
    HexTempFile() : m_fd(-1), m_delete(false)
    {
        char tmpfile[] = "/tmp/tmp.XXXXXX";
        m_fd = mkstemp(tmpfile);
        if (m_fd >= 0) {
            m_file = tmpfile;
            m_delete = true;
            HexLogDebug("Created temporary file %s", m_file.c_str());
        }
   }

    ~HexTempFile()
    {
        if (m_fd >= 0)
            ::close(m_fd);
        if (m_file.length() > 0 && m_delete) {
            HexLogDebug("Removing temporary file %s", m_file.c_str());
            unlink(m_file.c_str());
        }
    }

    // Returned path is not valid after this object goes out of scope.
    const char* path() const
    {
        const char *p = NULL;
        if (m_file.length() > 0)
            p = m_file.c_str();
        return p;
    }

    int fd() const
    {
        return m_fd;
    }

    // Return the temporary file name and release ownership.
    // Caller must make a copy of the returned path before this object goes out of scope.
    // Do not delete the temporary file when this object goes out of scope.
    const char* release()
    {
        const char *p = NULL;
        if (m_file.length() > 0) {
            p = m_file.c_str();
            m_delete = false;
        }
        return p;
    }

    // Close the temporary file
    // File with be deleted when this object goes out of scope
    void close()
    {
        if (m_fd >= 0) {
            ::close(m_fd);
            m_fd = -1;
        }
    }

private:
    std::string m_file;
    int m_fd;
    bool m_delete;

    // Disable copying
    HexTempFile(const HexTempFile&) ;
    HexTempFile& operator=(const HexTempFile&);
};

class HexTempDir
{
public:
    HexTempDir() : m_delete(false)
    {
        char tmpdir[] = "/tmp/tmp.XXXXXX";
        char* tmpPtr = mkdtemp(tmpdir);
        if (tmpPtr == tmpdir) {
            m_dir = tmpPtr;
            m_delete = true;
            HexLogDebug("Created temporary directory %s", m_dir.c_str());
        }
    }

    ~HexTempDir()
    {
        if (m_dir.length() > 0 && m_delete) {
            HexLogDebug("Removing temporary directory %s", m_dir.c_str());
            HexSpawn(0, "/bin/rm", "-rf", m_dir.c_str(), ZEROCHAR_PTR);
        }
    }

    // Returned path is not valid after this object goes out of scope.
    const char* path() const
    {
        const char *p = NULL;
        if (m_dir.length() > 0)
            p = m_dir.c_str();
        return p;
    }

    const char* dir() const { return path(); }

    // Return the temporary directory name and release ownership.
    // Caller must make a copy of the returned path before this object goes out of scope.
    // Do not delete the temporary directory when this object goes out of scope.
    const char* release()
    {
        const char *p = NULL;
        if (m_dir.length() > 0) {
            p = m_dir.c_str();
            m_delete = false;
        }
        return p;
    }

private:
    std::string m_dir;
    bool m_delete;

    // Disable copying
    HexTempDir(const HexTempDir&) ;
    HexTempDir& operator=(const HexTempDir&);
};

#endif /* __cplusplus */

#endif /* endif HEX_TEMPFILE_H */

