#pragma once

#include "ibm/utils.hpp"
#include "logging.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/endian/conversion.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <variant>

namespace crow
{
namespace ibm_mc_lock
{

/*----------------------------------------
|Segment flags : LockFlag | SegmentLength|
------------------------------------------*/

struct SegmentFlag
{
    std::string first;
    uint32_t second;
};

using SegmentFlags = std::vector<SegmentFlag>;

struct LockRequest
{
    std::string sessionId;
    std::string hmcId;
    std::string locktype;
    uint64_t resourceId;
    SegmentFlags segmentinfo;
};

using LockRequests = std::vector<LockRequest>;

struct RcValue
{
    uint32_t id;
    LockRequest request;
};

struct Rc
{
    using RcTypes = std::variant<uint32_t, RcValue>;
    bool lock;
    RcTypes value;
};

struct RcRelaseLock
{
    bool released;
    RcValue value;
};

using RcGetLockList =
    std::variant<std::string, std::vector<std::pair<uint32_t, LockRequests>>>;
using ListOfTransactionIds = std::vector<uint32_t>;
using RcAcquireLock = std::pair<bool, std::variant<Rc, std::pair<bool, int>>>;
using RcReleaseLockApi = std::pair<bool, std::variant<bool, RcRelaseLock>>;
struct SessionFlags
{
    std::string first;
    std::string second;
};

using ListOfSessionIds = std::vector<std::string>;

class Lock
{
    uint32_t transactionId = 0;
    boost::container::flat_map<uint32_t, LockRequests> lockTable;

  protected:
    /*
     * This function implements the logic for validating an incoming
     * lock request/requests.
     *
     * Returns : True (if Valid)
     * Returns : False (if not a Valid lock request)
     */

    virtual bool isValidLockRequest(const LockRequest& refLockRecord);

    /*
     * This function implements the logic of checking if the incoming
     * multi-lock request is not having conflicting requirements.
     *
     * Returns : True (if conflicting)
     * Returns : False (if not conflicting)
     */

    virtual bool isConflictRequest(const LockRequests& refLockRequestStructure);
    /*
     * Implements the core algorithm to find the conflicting
     * lock requests.
     *
     * This functions takes two lock requests and check if both
     * are conflicting to each other.
     *
     * Returns : True (if conflicting)
     * Returns : False (if not conflicting)
     */
    virtual bool isConflictRecord(const LockRequest& refLockRecord1,
                                  const LockRequest& refLockRecord2);

    /*
     * This function implements the logic of checking the conflicting
     * locks from a incoming single/multi lock requests with the already
     * existing lock request in the lock table.
     *
     */

    virtual Rc isConflictWithTable(const LockRequests& refLockRequestStructure);
    /*
     * This function implements the logic of checking the ownership of the
     * lock from the releaselock request.
     *
     * Returns : True (if the requesting HMC & Session owns the lock(s))
     * Returns : False (if the request HMC or Session does not own the lock(s))
     */

    virtual RcRelaseLock isItMyLock(const ListOfTransactionIds& refRids,
                                    const SessionFlags& ids);

    /*
     * This function validates the the list of transactionID's and returns false
     * if the transaction ID is not valid & not present in the lock table
     */

    virtual bool validateRids(const ListOfTransactionIds& refRids);

    /*
     * This function releases the locks that are already obtained by the
     * requesting Management console.
     */

    void releaseLock(const ListOfTransactionIds& refRids);

    Lock()
    {
        transactionId = lockTable.empty() ? 0 : prev(lockTable.end())->first;
    }

    /*
     * This function implements the algorithm for checking the respective
     * bytes of the resource id based on the lock management algorithm.
     */

    static bool checkByte(uint64_t resourceId1, uint64_t resourceId2,
                          uint32_t position);

    /*
     * This functions implements a counter that generates a unique 32 bit
     * number for every successful transaction. This number will be used by
     * the Management Console for debug.
     */
    virtual uint32_t generateTransactionId();

  public:
    /*
     * Explicitly deleted copy and move constructors
     */
    Lock(const Lock&) = delete;
    Lock(Lock&&) = delete;
    Lock& operator=(const Lock&) = delete;
    Lock& operator=(Lock&&) = delete;

    /*
     * This function implements the logic for acquiring a lock on a
     * resource if the incoming request is legitimate without any
     * conflicting requirements & without any conflicting requirement
     * with the existing locks in the lock table.
     *
     */

    RcAcquireLock acquireLock(const LockRequests& lockRequestStructure);

    /*
     * This function implements the logic for releasing the lock that are
     * owned by a management console session.
     *
     * The locks can be released by two ways
     *  - Using list of transaction ID's
     *  - Using a Session ID
     *
     * Client can choose either of the ways by using `Type` JSON key.
     *
     */
    RcReleaseLockApi releaseLock(const ListOfTransactionIds& p,
                                 const SessionFlags& ids);

    /*
     * This function implements the logic for getting the list of locks obtained
     * by a particular management console.
     */
    RcGetLockList getLockList(const ListOfSessionIds& listSessionId);

    /*
     * This function is releases all the locks obtained by a particular
     * session.
     */

    void releaseLock(const std::string& sessionId);

    static Lock& getInstance()
    {
        static Lock lockObject;
        return lockObject;
    }

    virtual ~Lock() = default;
};

inline RcGetLockList Lock::getLockList(const ListOfSessionIds& listSessionId)
{
    std::vector<std::pair<uint32_t, LockRequests>> lockList{};

    if (!lockTable.empty())
    {
        for (const auto& i : listSessionId)
        {
            auto it = lockTable.begin();
            while (it != lockTable.end())
            {
                // Check if session id of this entry matches with session id
                // given
                if (it->second[0].sessionId == i)
                {
                    BMCWEB_LOG_DEBUG("Session id is found in the locktable");

                    // Push the whole lock record into a vector for returning
                    // the json
                    lockList.emplace_back(it->first, it->second);
                }
                // Go to next entry in map
                it++;
            }
        }
    }
    // we may have found at least one entry with the given session id
    // return the json list of lock records pertaining to the given
    // session id, or send an empty list if lock table is empty
    return {lockList};
}

inline RcReleaseLockApi Lock::releaseLock(const ListOfTransactionIds& p,
                                          const SessionFlags& ids)
{
    bool status = validateRids(p);

    if (!status)
    {
        // Validation of rids failed
        BMCWEB_LOG_ERROR("releaseLock: Contains invalid request id");
        return std::make_pair(false, status);
    }
    // Validation passed, check if all the locks are owned by the
    // requesting HMC
    auto status2 = isItMyLock(p, ids);
    if (!status2.released)
    {
        return std::make_pair(false, status2);
    }
    return std::make_pair(true, status2);
}

inline RcAcquireLock Lock::acquireLock(const LockRequests& lockRequestStructure)
{
    // validate the lock request

    for (const auto& lockRecord : lockRequestStructure)
    {
        bool status = isValidLockRequest(lockRecord);
        if (!status)
        {
            BMCWEB_LOG_DEBUG("Not a Valid record");
            BMCWEB_LOG_DEBUG("Bad json in request");
            return std::make_pair(true, std::make_pair(status, 0));
        }
    }
    // check for conflict record

    const LockRequests& multiRequest = lockRequestStructure;
    bool status = isConflictRequest(multiRequest);

    if (status)
    {
        BMCWEB_LOG_DEBUG("There is a conflict within itself");
        return std::make_pair(true, std::make_pair(status, 1));
    }
    BMCWEB_LOG_DEBUG("The request is not conflicting within itself");

    // Need to check for conflict with the locktable entries.

    auto conflict = isConflictWithTable(multiRequest);

    BMCWEB_LOG_DEBUG("Done with checking conflict with the locktable");
    return std::make_pair(false, conflict);
}

inline void Lock::releaseLock(const std::string& sessionId)
{
    if (!lockTable.empty())
    {
        auto it = lockTable.begin();
        while (it != lockTable.end())
        {
            if (!it->second.empty())
            {
                // Check if session id of this entry matches with session id
                // given
                if (it->second[0].sessionId == sessionId)
                {
                    BMCWEB_LOG_DEBUG("Remove the lock from the locktable "
                                     "having sessionID={}",
                                     sessionId);
                    BMCWEB_LOG_DEBUG("TransactionID ={}", it->first);
                    it = lockTable.erase(it);
                }
                else
                {
                    it++;
                }
            }
        }
    }
}
inline RcRelaseLock Lock::isItMyLock(const ListOfTransactionIds& refRids,
                                     const SessionFlags& ids)
{
    for (const auto& id : refRids)
    {
        // Just need to compare the client id of the first lock records in the
        // complete lock row(in the map), because the rest of the lock records
        // would have the same client id

        std::string expectedClientId = lockTable[id][0].hmcId;
        std::string expectedSessionId = lockTable[id][0].sessionId;

        if ((expectedClientId == ids.first) &&
            (expectedSessionId == ids.second))
        {
            // It is owned by the currently request hmc
            BMCWEB_LOG_DEBUG("Lock is owned  by the current hmc");
            // remove the lock
            if (lockTable.erase(id) != 0U)
            {
                BMCWEB_LOG_DEBUG("Removing the locks with transaction ID : {}",
                                 id);
            }
            else
            {
                BMCWEB_LOG_ERROR("Removing the locks from the lock table "
                                 "failed, transaction ID: {}",
                                 id);
            }
        }
        else
        {
            BMCWEB_LOG_DEBUG("Lock is not owned by the current hmc");
            RcValue foo{id, lockTable[id][0]};
            return RcRelaseLock{false, std::move(foo)};
        }
    }
    return RcRelaseLock{true, {}};
}

inline bool Lock::validateRids(const ListOfTransactionIds& refRids)
{
    for (const auto& id : refRids)
    {
        auto search = lockTable.find(id);

        if (search != lockTable.end())
        {
            BMCWEB_LOG_DEBUG("Valid Transaction id");
            //  continue for the next rid
        }
        else
        {
            BMCWEB_LOG_ERROR("validateRids: At least 1 inValid Request id: {}",
                             id);
            return false;
        }
    }
    return true;
}

inline bool Lock::isValidLockRequest(const LockRequest& refLockRecord)
{
    // validate the locktype

    if (!((refLockRecord.locktype == "Read") ||
           (refLockRecord.locktype == "Write")))
    {
        BMCWEB_LOG_DEBUG("Validation of LockType Failed");
        BMCWEB_LOG_DEBUG("Locktype : {}", refLockRecord.locktype);
        return false;
    }

    BMCWEB_LOG_DEBUG("{}", static_cast<int>(refLockRecord.segmentinfo.size()));

    // validate the number of segments
    // Allowed No of segments are between 2 and 6
    if ((static_cast<int>(refLockRecord.segmentinfo.size()) > 6) ||
        (static_cast<int>(refLockRecord.segmentinfo.size()) < 2))
    {
        BMCWEB_LOG_DEBUG("Validation of Number of Segments Failed");
        BMCWEB_LOG_DEBUG("Number of Segments provided : {}",
                         refLockRecord.segmentinfo.size());
        return false;
    }

    int lockFlag = 0;
    // validate the lockflags & segment length

    for (const auto& p : refLockRecord.segmentinfo)
    {
        // validate the lock flags
        // Allowed lockflags are locksame,lockall & dontlock

        if (!((p.first == "LockSame" || (p.first == "LockAll") ||
               (p.first == "DontLock"))))
        {
            BMCWEB_LOG_DEBUG("Validation of lock flags failed");
            BMCWEB_LOG_DEBUG("{}", p.first);
            return false;
        }

        // validate the segment length
        // Allowed values of segment length are between 1 and 4

        if (p.second < 1 || p.second > 4)
        {
            BMCWEB_LOG_DEBUG("Validation of Segment Length Failed");
            BMCWEB_LOG_DEBUG("{}", p.second);
            return false;
        }

        if ((p.first == "LockSame" || (p.first == "LockAll")))
        {
            ++lockFlag;
            if (lockFlag >= 2)
            {
                return false;
            }
        }
    }

    return true;
}

inline Rc Lock::isConflictWithTable(const LockRequests& refLockRequestStructure)
{
    if (lockTable.empty())
    {
        uint32_t thisTransactionId = generateTransactionId();
        BMCWEB_LOG_DEBUG("{}", thisTransactionId);
        // Lock table is empty, so we are safe to add the lockrecords
        // as there will be no conflict
        BMCWEB_LOG_DEBUG("Lock table is empty, so adding the lockrecords");
        lockTable.emplace(thisTransactionId, refLockRequestStructure);
        Rc::RcTypes foo(std::in_place_index<0>, thisTransactionId);
        return Rc{false, std::move(foo)};
    }
    BMCWEB_LOG_DEBUG(
        "Lock table is not empty, check for conflict with lock table");
    // Lock table is not empty, compare the lockrequest entries with
    // the entries in the lock table

    for (const auto& lockRecord1 : refLockRequestStructure)
    {
        for (const auto& map : lockTable)
        {
            for (const auto& lockRecord2 : map.second)
            {
                bool status = isConflictRecord(lockRecord1, lockRecord2);
                if (status)
                {
                    RcValue foo{map.first, lockRecord2};

                    Rc::RcTypes foo1 = std::move(foo);
                    return Rc{true, std::move(foo1)};
                }
            }
        }
    }

    // Reached here, so no conflict with the locktable, so we are safe to
    // add the request records into the lock table

    // Lock table is empty, so we are safe to add the lockrecords
    // as there will be no conflict
    BMCWEB_LOG_DEBUG(" Adding elements into lock table");
    transactionId = generateTransactionId();
    lockTable.emplace(std::make_pair(transactionId, refLockRequestStructure));

    return {false, transactionId};
}

inline bool Lock::isConflictRequest(const LockRequests& refLockRequestStructure)
{
    // check for all the locks coming in as a part of single request
    // return conflict if any two lock requests are conflicting

    if (refLockRequestStructure.size() == 1)
    {
        BMCWEB_LOG_DEBUG("Only single lock request, so there is no conflict");
        // This means , we have only one lock request in the current
        // request , so no conflict within the request
        return false;
    }

    BMCWEB_LOG_DEBUG(
        "There are multiple lock requests coming in a single request");

    // There are multiple requests a part of one request

    for (uint32_t i = 0; i < refLockRequestStructure.size(); i++)
    {
        for (uint32_t j = i + 1; j < refLockRequestStructure.size(); j++)
        {
            const LockRequest& p = refLockRequestStructure[i];
            const LockRequest& q = refLockRequestStructure[j];
            bool status = isConflictRecord(p, q);

            if (status)
            {
                return true;
            }
        }
    }
    return false;
}

// This function converts the provided uint64_t resource id's from the two
// lock requests subjected for comparison, and this function also compares
// the content by bytes mentioned by a uint32_t number.

// If all the elements in the lock requests which are subjected for comparison
// are same, then the last comparison would be to check for the respective
// bytes in the resourceid based on the segment length.
inline bool Lock::checkByte(uint64_t resourceId1, uint64_t resourceId2,
                            uint32_t position)
{
    if (position >= sizeof(resourceId1))
    {
        return false;
    }

    uint8_t pPosition = 0;
    uint8_t qPosition = 0;
    pPosition = 0xFF & (resourceId1 >> (8 * position));
    qPosition = 0xFF & (resourceId2 >> (8 * position));

    return pPosition == qPosition;
}

inline bool Lock::isConflictRecord(const LockRequest& refLockRecord1,
                                   const LockRequest& refLockRecord2)
{
    // No conflict if both are read locks

    if (refLockRecord1.locktype == "Read") &&
        refLockRecord2.locktype == "Read"))
    {
        BMCWEB_LOG_DEBUG("Both are read locks, no conflict");
        return false;
    }

    uint32_t i = 0;
    for (const auto& p : refLockRecord1.segmentinfo)
    {
        // return conflict when any of them is try to lock all resources
        // under the current resource level.
        if (p.first == "LockAll" ||
            refLockRecord2.segmentinfo[i].first == "LockAll"
        {
            BMCWEB_LOG_DEBUG(
                "Either of the Comparing locks are trying to lock all "
                "resources under the current resource level");
            return true;
        }

        // determine if there is a lock-all-with-same-segment-size.
        // If the current segment sizes are the same,then we should fail.
        if ((p.first == "LockSame" ||
             refLockRecord2.segmentinfo[i].first == "LockSame") &&
            (p.second == refLockRecord2.segmentinfo[i].second))
        {
            return true;
        }

        // if segment lengths are not the same, it means two different locks
        // So no conflict
        if (p.second != refLockRecord2.segmentinfo[i].second)
        {
            BMCWEB_LOG_DEBUG("Segment lengths are not same");
            BMCWEB_LOG_DEBUG("Segment 1 length : {}", p.second);
            BMCWEB_LOG_DEBUG("Segment 2 length : {}",
                             refLockRecord2.segmentinfo[i].second);
            return false;
        }

        // compare segment data

        for (uint32_t j = 0; j < p.second; j++)
        {
            // if the segment data is different, then the locks is on a
            // different resource so no conflict between the lock
            // records.
            // BMC is little endian, but the resourceID is formed by
            // the Management Console in such a way that, the first byte
            // from the MSB Position corresponds to the First Segment
            // data. Therefore we need to convert the incoming
            // resourceID into Big Endian before processing further.
            if (!(checkByte(
                    boost::endian::endian_reverse(refLockRecord1.resourceId),
                    boost::endian::endian_reverse(refLockRecord2.resourceId),
                    j)))
            {
                return false;
            }
        }

        ++i;
    }

    return true;
}

inline uint32_t Lock::generateTransactionId()
{
    ++transactionId;
    return transactionId;
}

} // namespace ibm_mc_lock
} // namespace crow
