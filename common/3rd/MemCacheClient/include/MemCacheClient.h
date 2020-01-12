/*! @file       MemCacheClient.h
    @version    2.0
    @brief      Basic memcached client
 */
/*! @mainpage

    <table>
        <tr><th>Library     <td>MemCacheClient
        <tr><th>Author      <td>Brodie Thiesfield [code at jellycan dot com]
        <tr><th>Source      <td>http://code.jellycan.com/memcacheclient/
    </table>

    A basic C++ client for a memcached server.

    This client was designed for use with Windows software but with some tweaking
    there is no reason that it shouldn't also build on posix systems.

    @section features FEATURES

    -   MIT Licence allows free use in all software (including GPL and commercial)
    -   multi-platform (Windows 95/98/ME/NT/2K/XP, Linux, Unix)
    -   multiple servers with consistent server hashing
    -   support for most memcached commands
    -   supports multiple requests in a single batch

    @section usage USAGE

    The following steps are required to use MemCacheClient.

    <ol>

    <li> Include the MemCacheClient.*, ReadWriteBuffer.* and md5.* files in your project.

<pre>@#include "MemCacheClient.h"</pre>

    <li> Create an instance of the MemCacheClient object.

<pre>MemCacheClient * pClient = new MemCacheClient;</pre>

    <li> Add some servers to the client.

<pre>pClient->AddServer("127.0.0.1");</pre>

    <li> Make a request.

<pre>MemCacheClient::MemRequest req;
req.mKey  = "datakey";
req.mData.WriteBytes("foobar", 5);
pClient->Add(req);
if (req.mResult == MCERR_OK) ...</pre>

    </ol>

    @section notes NOTES

    See the protocol document for memcached at:
    http://code.sixapart.com/svn/memcached/trunk/server/doc/protocol.txt

    The included MD5 library is from the LUA project: http://www.keplerproject.org/md5/
    MD5 was designed and implemented by Roberto Ierusalimschy and Marcela Ozorio Suarez.
    The implementation is not derived from licensed software. The DES 56 C library was
    implemented by Stuart Levy and uses a MIT licence too.


    @section licence MIT LICENCE

    The licence text below is the boilerplate "MIT Licence" used from:
    http://www.opensource.org/licenses/mit-license.php

    Copyright (c) 2008, Brodie Thiesfield

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is furnished
    to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef INCLUDED_MemCacheClient
#define INCLUDED_MemCacheClient

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#ifndef WIN32
# include <stdint.h>
#endif

#include <string>
#include <vector>

#ifdef CROSSBASE_API
#include <Core/ReadWriteBuffer.h>
START_CL_NAMESPACE
#else
# include "ReadWriteBuffer.h"
# include "Matilda.h"
#endif

// ----------------------------------------------------------------------------
/*! @brief Result code for requests to a server.

    Success codes are greater or equal to 0.
    Failure codes are less than 0.
 */
enum MCResult {
    // success: n >= 0
    MCERR_OK        = 0,    //!< Success
    MCERR_NOREPLY   = 1,    //!< Success assumed (no reply requested)
    MCERR_NOTSTORED = 2,    //!< Success but item not stored (see memcached docs)
    MCERR_NOTFOUND  = 3,    //!< Success but item not found (see memcached docs)

    // failure: n < 0
    MCERR_NOSERVER  = -1    //!< Failure, connection error with server
};

// ----------------------------------------------------------------------------
/*! @brief Public client interface for memcached.

    Create an instance of this class, add all servers to it and then call
    request methods. A single instance of the client is not threadsafe.
    Use explicit locking or an instance per thread. Each instance will maintain
    a single TCP socket open to each server.
 */
class CROSSBASE_CLASS_API CMemCacheClient
{
    class CServer;                           //!< server connection implementation

public:
    typedef std::string         string_t;   //!< Abstract the string class for internal use
#ifdef WIN32
    typedef unsigned __int64    uint64_t;   //!< 64-bit unsigned type
#else
    typedef unsigned long long  uint64_t;   //!< 64-bit unsigned type
#endif

    /*! @brief Input and output structure for most requests to the server */
    struct MemRequest
    {
        struct  Sort;       //!< sort requests into server order
        CServer *pServer;   //!< server that will be used for this request

    public:
        /*! @brief constructor */
        MemRequest();

        /*! @brief Clear the structure in preparation for a new request. */
        void Clear();

    public:
        /*! @brief required service for this request. Set to a specific service to restrict
            the used server to only a server providing that service. */
        unsigned uService;

        /*! @brief The request key.

            Set this value to the key that should be used for the request.
            It will not be modified by MemCacheClient.
         */
        string_t strKey;

        /*! @brief Flags for the request.

            These flags will be sent to the server when using one of the storage requests
            and returned with a get request. The flags are not used by MemCacheClient.
         */
        unsigned int uFlags;

        /*! @brief Expiry time for the data.

            This is used only by storage requests. The expiry time is in seconds.
            0 = no expiry. 1..2592000 (30 days) = expiry time relative to now.
            All other values are an explicit time_t value.
         */
        time_t tExpiry;

        /*! @brief CAS value returned by Gets and sent by CheckSet */
        uint64_t u64Cas;

        /*! @brief Result of a request.

            Set this to MCERR_NOREPLY prior to calling the function if a reply isn't
            required. Any other value will request a reply from the server and
            return the result.
         */
        MCResult eResult;

        /*! @brief Data to be sent to the server in a storage request, or data returned
            from the server for get requests.
         */
        CReadWriteBuffer data;

        /*! @brief User managed context for this request. */
        void *pContext;
    };

public:
    /*! @brief Maximum number of request objects in a single call */
    const static int MAX_REQUESTS = 50;

    /*! @brief Initialise the memcached client */
    CMemCacheClient();

    /*! @brief Destructor.

        All servers will be disconnected.
     */
    ~CMemCacheClient();

    /*! @brief Set the network timeout for all operations.

        All network operations share the same timeout.

        @param aTimeoutMs     Timeout in milliseconds
     */
    void SetTimeout(size_t aTimeoutMs);

    /*! @brief Set the period to wait before trying to reconnect to a server
        that isn't available.

        @param aRetryMs     Period in milliseconds
     */
    void SetRetryPeriod(size_t aRetryMs);

    /*! @brief turn a result code into a string */
    static const char * ConvertResult(MCResult aResult);

    /*! @brief dump internal tables to trace log */
    void DumpTables();

    /*-----------------------------------------------------------------------*/
    /*! @{ @name Servers */

    /*! @brief Add a server to the client.

        This function will fail only if the address is not a valid IP
        address or PORT. It will not fail if the server cannot be contacted
        but will instead continue to occasionally attempt connections
        to that server.

        @param pServerAddress   The memcached server to be as IP[:PORT].
                                The port will default to 11211 if not supplied.
        @param pServerName      Display name for the server. Default is aServerAddress.
        @param uServiceFlags    The type of services this server is used for.
            When looking up a server to be used, only servers that have
            a matching bit in these flags will be used. See SetRequiredService().

        @return true if server was added
     */
    bool AddServer(
        const char *pServerAddress
        , const char *pServerName = nullptr
        , unsigned uServices = (unsigned)-1);

    /*! @brief Delete a server from the client.

        The server should be specified as documented in AddServer.

        @param aServerAddress   The server to be added specified as IP[:PORT].
                                The port will default to 11211 if not supplied.

        @return true if server was deleted
     */
    bool DelServer(const char *pServerAddress);

    /*! @brief Request the list of current servers

        @param aServers  list of all servers registered to this client
     */
    void GetServers(std::vector<string_t> &vecServers);

    /*! @brief Disconnect from and remove all servers */
    void ClearServers();

    /*-----------------------------------------------------------------------*/
    /*! @} */
    /*! @{ @name Single Requests */

    /*! @brief Add an item to the server.

        This will fail if the item already exists at the server.

        @param  item  Item to be added
        @return Number of items with a success result.
     */
    inline int Add(MemRequest &item) { return Store("add", &item, 1); }

    /*! @brief Set an item to the server.

        This will always set the item at the server regardless of if it
        already exists or not.

        @param  item  Item to be updated
        @return Number of items with a success result.
     */
    inline int Set(MemRequest &item) { return Store("set", &item, 1); }

    /*! @brief Replace an item in the server.

        This will fail if the item does not already exist at the server.

        @param  item  Item to be updated
        @return Number of items with a success result.
     */
    inline int Replace(MemRequest &item) { return Store("replace", &item, 1); }

    /*! @brief Append data to an existing item in the server.

        @param  item  Item to be updated
        @return Number of items with a success result.
     */
    inline int Append(MemRequest &item) { return Store("append", &item, 1); }

    /*! @brief Prepend data to an existing item in the server.

        @param  item  Item to be updated
        @return Number of items with a success result.
     */
    inline int Prepend(MemRequest &item) { return Store("prepend", &item, 1); }

    /*! @brief Set the data to an existing item in the server only if it has not been
        modified since it was last retrieved.

        This requires the mCas member to be set in the request. Use the Gets
        command to retrieve an item with a valid mCas member.

        @param  item  Item to be updated
        @return Number of items with a success result.
     */
    inline int CheckSet(MemRequest &item) { return Store("cas", &item, 1); }

    /*! @brief Get an item from the server.

        @param  item  Item to be retrieved
        @return Number of items with a success result.
     */
    inline int Get(MemRequest &item) { return Combine("get", &item, 1); }

    /*! @brief Get an item from the server including the CAS data.

        @param  item  Item to be retrieved
        @return Number of items with a success result.
     */
    inline int Gets(MemRequest &item) { return Combine("gets", &item, 1); }

    /*! @brief Delete an item from the server.

        @param  item  Item to be removed.
        @return Number of items with a success result.
     */
    inline int Del(MemRequest &item) { return Combine("del", &item, 1); }


    /*! @brief Increment a value at the server.

        See the INCR command in the memcached protocol.

        @param aKey         Key to increment
        @param aNewValue    Receive the new value if desired. If aWantReply is false,
                            this value will not be updated. If aWantReply is true, a
                            reply will stil be requested from the server even if this
                            item is nullptr.
        @param u64Diff      Positive integer to increment the value at the server by.
        @param bWantReply   Should a reply be requested from the server.
        @return MCResult
     */
    inline MCResult Increment(
        const char *pKey
        , uint64_t *pNewValue = nullptr
        , uint64_t u64Diff = 1
        , bool bWantReply = true
        , unsigned uService = 0) {
        return IncDec("incr", uService, pKey, pNewValue, u64Diff, bWantReply);
    }

    /*! @brief Decrement a value at the server.

        See the DECR command in the memcached protocol.

        @param aKey         Key to decrement
        @param aNewValue    Receive the new value if desired. If aWantReply is false,
                            this value will not be updated. If aWantReply is true, a
                            reply will stil be requested from the server even if this
                            item is nullptr.
        @param u64Diff      Positive integer to decrement the value at the server by.
        @param aWantReply   Should a reply be requested from the server.
        @return MCResult
     */
    inline MCResult Decrement(
        const char *pKey
        , uint64_t *pNewValue = nullptr
        , uint64_t u64Diff = 1
        , bool bWantReply = true
        , unsigned uService = 0) {
        return IncDec("decr", uService, pKey, pNewValue, u64Diff, bWantReply);
    }

    /*-----------------------------------------------------------------------*/
    /*! @} */
    /*! @{ @name Multiple Requests */

    /*! @brief Add multiple items to the server.

        This will fail if the item already exists at the server.

        @note   Each request is independent of other requests in the array. Failure
                of one request does not imply failure of any other.

        @param pItem    Array of items to be added
        @param aCount   Number of items in the array
        @return Number of items with a success result
     */
    inline int Add(MemRequest *pItem, int aCount) { return Store("add", pItem, aCount); }

    /*! @brief Set multiple items to the server.

        This will always set the item at the server regardless of if it already exists or not.

        @note   Each request is independent of other requests in the array. Failure
                of one request does not imply failure of any other.

        @param aItem    Array of items to be added
        @param aCount   Number of items in the array
        @return Number of items with a success result
     */
    inline int Set(MemRequest *pItem, int aCount) { return Store("set", pItem, aCount); }

    /*! @brief Replace multiple items in the server.

        This will fail if the item does not already exist at the server.

        @note   Each request is independent of other requests in the array. Failure
                of one request does not imply failure of any other.

        @param aItem    Array of items to be added
        @param aCount   Number of items in the array
        @return Number of items with a success result
     */
    inline int Replace(MemRequest *pItem, int aCount) { return Store("replace", pItem, aCount); }

    /*! @brief Append data to multiple existing items in the server.

        @note   Each request is independent of other requests in the array. Failure
                of one request does not imply failure of any other.

        @param aItem    Array of items to be added
        @param aCount   Number of items in the array
        @return Number of items with a success result
     */
    inline int Append(MemRequest *pItem, int aCount) { return Store("append", pItem, aCount); }

    /*! @brief Prepend data to multiple existing items in the server.

        @note   Each request is independent of other requests in the array. Failure
                of one request does not imply failure of any other.

        @param aItem    Array of items to be added
        @param aCount   Number of items in the array
        @return Number of items with a success result
     */
    inline int Prepend(MemRequest *pItem, int aCount) { return Store("prepend", pItem, aCount); }

    /*! @brief Set the data to multiple existing items in the server only if it has not been
        modified since it was last retrieved.

        This requires the mCas member to be set in the request. Use the Gets command to
        retrieve an item with a valid mCas member.

        @note   Each request is independent of other requests in the array. Failure
                of one request does not imply failure of any other.

        @param pItem    Array of items to be added
        @param nCount   Number of items in the array
        @return Number of items with a success result
     */
    inline int CheckSet(MemRequest *pItem, int nCount) { return Store("cas", pItem, nCount); }

    /*! @brief Get an item from the server.

        @note   Each request is independent of other requests in the array. Failure
                of one request does not imply failure of any other.

        @param pItem    Array of items to be retrieved
        @param nCount   Number of items in the array
        @return Number of items with a success result
     */
    inline int Get(MemRequest *pItem, int nCount) { return Combine("get", pItem, nCount); }

    /*! @brief Get multiple items from the server including the CAS data.

        @note   Each request is independent of other requests in the array. Failure
                of one request does not imply failure of any other.

        @param pItem    Array of items to be retrieved
        @param nCount   Number of items in the array
        @return Number of items with a success result
     */
    inline int Gets(MemRequest *pItem, int nCount) { return Combine("gets", pItem, nCount); }

    /*! @brief Delete multiple items from the server.

        @note   Each request is independent of other requests in the array. Failure
                of one request does not imply failure of any other.

        @param pItem    Array of items to be retrieved
        @param nCount   Number of items in the array
        @return Number of items with a success result
     */
    inline int Del(MemRequest *pItem, int nCount) { return Combine("del", pItem, nCount); }

    /*! @brief Send a flush_all command to a specific server or all servers.

        @note   Each request is independent of other requests in the array. Failure
                of one request does not imply failure of any other.

        @param pszServerAddress Server to flush. If nullptr, all servers will be flushed.
        @param nExpiry          Expiry time for the flush. The server will wait before executing
                                the server flush. See MemRequest::mExpiry for details of possible
                                values.
        @return Number of servers that were flushed.
     */
    int FlushAll(const char *pszServerAddress = nullptr, int nExpiry = 0);

    /*! @} */

private:
    // disabled
    CMemCacheClient(const CMemCacheClient &)             = delete;
    CMemCacheClient & operator=(const CMemCacheClient &) = delete;

    /*-----------------------------------------------------------------------*/
    /*! @{
        @internal
        @name Internal
     */
    ClTrace                 m_trace;
    std::vector<CServer *>  m_vecServer;    //!< current servers
    size_t                  m_siTimeoutMs;  //!< network timeout in millisec for all operations
    size_t                  m_siRetryMs;    //!< retry period in millisec for unavailable servers

    /*! @brief Maintain the N:1 hash key to server relationship used for consistent hashing.

        Consistent hashing for the servers ensures that even with changes to the server list,
        many of the data keys will continue to mapped to the same server. This ensures that
        changes to the server list doesn't invalidate all cached data.
     */
    struct ConsistentHash
    {
        unsigned long   uHash;      //!< hash value
        CServer         *pServer;   //!< server implementation
        unsigned        uServices;  //!< services this server provides
        int             nEntry;     //!< distinguish same hash and server

        /*! constructor */
        ConsistentHash(unsigned long _uHash, CServer *_pServer, unsigned _uServices, int _nEntry);

        /*! copy constructor */
        ConsistentHash(const ConsistentHash &rhs);

        /*! copy */
        ConsistentHash & operator=(const ConsistentHash &rhs);

        /*! does this server handle the required service? */
        inline bool services(unsigned uService) { return (uServices & uService) == uService; }

        /*! comparison on hash value */
        bool operator<(const ConsistentHash &rhs) const;

        /*! match on server pointer */
        struct MatchServer;

    private:
        bool operator==(const ConsistentHash &rhs) const;
    };

    /*! @brief Consistent hash ring for servers.

        This vector is in sorted order on the hash key.
     */
    std::vector<ConsistentHash> mServerHash;

    /*! create a hash value from the key */
    unsigned long CreateKeyHash(const char *pKey);

    /*! find which server will be used to handle the key */
    CServer * FindServer(const string_t &strKey, unsigned aService);

    /*! send storage requests (add, set, cas, replace, append, prepend) */
    int Store(const char *pType, MemRequest *pItem, int nCount);

    /*! handle a single storage response from a server */
    void HandleStoreResponse(CServer *pServer, MemRequest &item);

    /*! send multiple commands to all servers for commands that can be combined (get, gets, del) */
    int Combine(const char *pType, MemRequest *pItem, int aCount);

    /*! handle a single get or gets response from a server */
    int HandleGetResponse(CServer *pServer, MemRequest **pBegin, MemRequest **pEnd);

    /*! handle a single del response from a server */
    int HandleDelResponse(CServer *pServer, MemRequest **pBegin, MemRequest **pEnd);

    /*! send an incr or decr request to a server and handle the response */
    MCResult IncDec(
        const char *pType
        , unsigned aService
        , const char *pKey
        , uint64_t *pNewValue
        , uint64_t u64Diff
        , bool aWantReply);

    /*! @} */
};

#ifdef CROSSBASE_API
END_CL_NAMESPACE
#endif

#endif // INCLUDED_MemCacheClient
