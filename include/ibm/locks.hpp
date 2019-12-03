#pragma once

#include <app.h>

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace crow
{
namespace ibm_mc_lock
{

namespace fs = std::filesystem;
using stype = std::string;

/*----------------------------------------
|Segment flags : LockFlag | SegmentLength|
------------------------------------------*/

using segmentflags = std::vector<std::pair<stype, uint32_t>>;

// Lockrequest = session-id | hmc-id | locktype | resourceid | segmentinfo
using lockrequest = std::tuple<stype, stype, stype, uint64_t, segmentflags>;

using lockrequests = std::vector<lockrequest>;
using rc =
    std::pair<bool, std::variant<uint32_t, std::pair<uint32_t, lockrequest>>>;
using rcrelaselock = std::pair<bool, lockrequest>;
using rcgetlocklist = std::pair<
    bool,
    std::variant<std::string, std::vector<std::pair<uint32_t, lockrequests>>>>;

using rcacquirelock = std::pair<bool, std::variant<rc, std::pair<bool, int>>>;

class Lock
{
    uint32_t transactionID;
    std::map<uint32_t, lockrequests> lockTable;

    /*
     * This function implements the logic for validating an incomming
     * lock request/requests.
     *
     * Returns : True (if Valid)
     * Returns : False (if not a Valid lock request)
     */

    bool isValidLockRequest(const lockrequest);

    /*
     * This function implements the logic of checking if the incomming
     * multi-lock request is not having conflicting requirements.
     *
     * Returns : True (if conflicting)
     * Returns : False (if not conflicting)
     */

    bool isConflictRequest(const lockrequests);
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
    bool isConflictRecord(const lockrequest, const lockrequest);

    /*
     * This function implements the logic of checking the conflicting
     * locks from a incomming single/multi lock requests with the already
     * existing lock request in the lock table.
     *
     */

    rc isConflictWithTable(const lockrequests);

    /*
     * This function implements the algorithm for checking the respective
     * bytes of the resource id based on the lock management algorithm.
     */

    bool checkByte(uint64_t, uint64_t, uint32_t);

    /*
     * This functions implements a counter that generates a unique 32 bit
     * number for every successful transaction. This number will be used by
     * the Management Console for debug.
     */
    uint32_t generateTransactionID();

  public:
    /*
     * This function implements the logic for acquiring a lock on a
     * resource if the incomming request is legitimate without any
     * conflicting requirements & without any conflicting requirement
     * with the exsiting locks in the lock table.
     *
     */

    rcacquirelock acquireLock(const lockrequests);

  public:
    Lock()
    {
        transactionID = 0;
    }

} lockObject;

rcacquirelock Lock::acquireLock(const lockrequests lockRequestStructure)
{

    // validate the lock request

    for (auto lockRecord : lockRequestStructure)
    {
        const lockrequest &request = lockRecord;
        bool status = isValidLockRequest(request);
        if (!status)
        {
            BMCWEB_LOG_DEBUG << "Not a Valid record";
            BMCWEB_LOG_DEBUG << "Bad json in request";
            return std::make_pair(true, std::make_pair(status, 0));
        }
    }
    // check for conflict record

    const lockrequests &multiRequest = lockRequestStructure;
    bool status = isConflictRequest(multiRequest);

    if (status)
    {
        BMCWEB_LOG_DEBUG << "There is a conflict within itself";
        return std::make_pair(true, std::make_pair(status, 1));
    }
    else
    {
        BMCWEB_LOG_DEBUG << "The request is not conflicting within itself";

        // Need to check for conflict with the locktable entries.

        auto conflict = isConflictWithTable(multiRequest);

        BMCWEB_LOG_DEBUG << "Done with checking conflict with the locktable";
        return std::make_pair(false, conflict);
    }

    return std::make_pair(true, std::make_pair(true, 1));
}

bool Lock::isValidLockRequest(const lockrequest refLockRecord)
{

    // validate the locktype

    if (!((boost::equals(std::get<2>(refLockRecord), "Read") ||
           (boost::equals(std::get<2>(refLockRecord), "Write")))))
    {
        BMCWEB_LOG_DEBUG << "Validation of LockType Failed";
        BMCWEB_LOG_DEBUG << "Locktype : " << std::get<2>(refLockRecord);
        return false;
    }

    BMCWEB_LOG_DEBUG << static_cast<int>(std::get<4>(refLockRecord).size());

    // validate the number of segments
    // Allowed No of segments are between 2 and 6
    if ((static_cast<int>(std::get<4>(refLockRecord).size()) > 6) ||
        (static_cast<int>(std::get<4>(refLockRecord).size()) < 2))
    {
        BMCWEB_LOG_DEBUG << "Validation of Number of Segements Failed";
        BMCWEB_LOG_DEBUG << "Number of Segments provied : "
                         << sizeof(std::get<4>(refLockRecord));
        return false;
    }

    int lockFlag = 0;
    // validate the lockflags & segment length

    for (const auto &p : std::get<4>(refLockRecord))
    {

        // validate the lock flags
        // Allowed lockflags are locksame,lockall & dontlock

        if (!((boost::equals(p.first, "LockSame") ||
               (boost::equals(p.first, "LockAll")) ||
               (boost::equals(p.first, "DontLock")))))
        {
            BMCWEB_LOG_DEBUG << "Validation of lock flags failed";
            BMCWEB_LOG_DEBUG << p.first;
            return false;
        }

        // validate the segment length
        // Allowed values of segment length are between 1 and 4

        if (p.second < 1 || p.second > 4)
        {
            BMCWEB_LOG_DEBUG << "Validation of Segment Length Failed";
            BMCWEB_LOG_DEBUG << p.second;
            return false;
        }

        if ((boost::equals(p.first, "LockSame") ||
             (boost::equals(p.first, "LockAll"))))
        {
            ++lockFlag;
            if (lockFlag >= 2)
            {
                return false;
            }
        }
    }

    // validate the segment length
    return true;
}

rc Lock::isConflictWithTable(const lockrequests refLockRequestStructure)
{

    uint32_t transactionID;

    if (lockTable.empty())
    {
        transactionID = generateTransactionID();
        BMCWEB_LOG_DEBUG << transactionID;
        // Lock table is empty, so we are safe to add the lockrecords
        // as there will be no conflict
        BMCWEB_LOG_DEBUG << "Lock table is empty, so adding the lockrecords";
        lockTable.emplace(std::pair<uint32_t, lockrequests>(
            transactionID, refLockRequestStructure));

        return std::make_pair(false, transactionID);
    }

    else
    {
        BMCWEB_LOG_DEBUG
            << "Lock table is not empty, check for conflict with lock table";
        // Lock table is not empty, compare the lockrequest entries with
        // the entries in the lock table

        for (auto lockRecord1 : refLockRequestStructure)
        {
            for (const auto &map : lockTable)
            {
                for (auto lockRecord2 : map.second)
                {
                    const lockrequest &p = lockRecord1;
                    const lockrequest &q = lockRecord2;
                    bool status = isConflictRecord(p, q);
                    if (status)
                    {
                        return std::make_pair(true,
                                              std::make_pair(map.first, q));
                    }
                }
            }
        }

        // Reached here, so no conflict with the locktable, so we are safe to
        // add the request records into the lock table

        // Lock table is empty, so we are safe to add the lockrecords
        // as there will be no conflict
        BMCWEB_LOG_DEBUG << " Adding elements into lock table";
        transactionID = generateTransactionID();
        lockTable.emplace(
            std::make_pair(transactionID, refLockRequestStructure));
    }
    return std::make_pair(false, transactionID);
}

bool Lock::isConflictRequest(const lockrequests refLockRequestStructure)
{
    // check for all the locks coming in as a part of single request
    // return conflict if any two lock requests are conflicting

    if (refLockRequestStructure.size() == 1)
    {
        BMCWEB_LOG_DEBUG << "Only single lock request, so there is no conflict";
        // This means , we have only one lock request in the current
        // request , so no conflict within the request
        return false;
    }

    else
    {
        BMCWEB_LOG_DEBUG
            << "There are multiple lock requests coming in a single request";

        // There are multiple requests a part of one request

        for (uint32_t i = 0; i < refLockRequestStructure.size(); i++)
        {
            for (uint32_t j = i + 1; j < refLockRequestStructure.size(); j++)
            {
                const lockrequest &p = refLockRequestStructure[i];
                const lockrequest &q = refLockRequestStructure[j];
                bool status = isConflictRecord(p, q);

                if (status)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

// This function converts the provided uint64_t resource id's from the two
// lock requests subjected for comparision, and this function also compares
// the content by bytes mentioned by a uint32_t number.

// If all the elements in the lock requests which are subjected for comparison
// are same, then the last comparision would be to check for the respective
// bytes in the resourceid based on the segment length.

bool Lock::checkByte(uint64_t resourceId1, uint64_t resourceId2,
                     uint32_t position)
{
    uint8_t *p = reinterpret_cast<uint8_t *>(&resourceId1);
    uint8_t *q = reinterpret_cast<uint8_t *>(&resourceId2);

    BMCWEB_LOG_DEBUG << "Comparing bytes " << std::to_string(p[position]) << ","
                     << std::to_string(q[position]);
    if (p[position] != q[position])
    {
        return false;
    }

    else
    {
        return true;
    }
    return true;
}

bool Lock::isConflictRecord(const lockrequest refLockRecord1,
                            const lockrequest refLockRecord2)
{
    // No conflict if both are read locks

    if (boost::equals(std::get<2>(refLockRecord1), "Read") &&
        boost::equals(std::get<2>(refLockRecord2), "Read"))
    {
        BMCWEB_LOG_DEBUG << "Both are read locks, no conflict";
        return false;
    }

    else
    {
        uint32_t i = 0;
        for (const auto &p : std::get<4>(refLockRecord1))
        {

            // return conflict when any of them is try to lock all resources
            // under the current resource level.
            if (boost::equals(p.first, "LockAll") ||
                boost::equals(std::get<4>(refLockRecord2)[i].first, "LockAll"))
            {
                BMCWEB_LOG_DEBUG
                    << "Either of the Comparing locks are trying to lock all "
                       "resources under the current resource level";
                return true;
            }

            // determine if there is a lock-all-with-same-segment-size.
            // If the current segment sizes are the same,then we should fail.

            if ((boost::equals(p.first, "LockSame") ||
                 boost::equals(std::get<4>(refLockRecord2)[i].first,
                               "LockSame")) &&
                (p.second == std::get<4>(refLockRecord2)[i].second))
            {
                return true;
            }

            // if segment lengths are not the same, it means two different locks
            // So no conflict
            if (p.second != std::get<4>(refLockRecord2)[i].second)
            {
                BMCWEB_LOG_DEBUG << "Segment lengths are not same";
                BMCWEB_LOG_DEBUG << "Segment 1 length : " << p.second;
                BMCWEB_LOG_DEBUG << "Segment 2 length : "
                                 << std::get<4>(refLockRecord2)[i].second;
                return false;
            }

            // compare segment data

            for (uint32_t i = 0; i < p.second; i++)
            {
                // if the segment data is different , then the locks is on a
                // different resource So no conflict between the lock records
                if (!(checkByte(std::get<3>(refLockRecord1),
                                std::get<3>(refLockRecord2), i)))
                {
                    return false;
                }
            }

            ++i;
        }
    }

    return false;
}

uint32_t Lock::generateTransactionID()
{
    ++transactionID;
    return transactionID;
}

} // namespace ibm_mc_lock
} // namespace crow
