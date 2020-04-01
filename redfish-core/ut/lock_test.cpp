#include "IBM/locks.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <utils/json_utils.hpp>

#include "gmock/gmock.h"

namespace crow
{
namespace ibm_mc_lock
{
class locktest : public ::testing::Test
{
  protected:
    lockrequests request;
    lockrequests request1, request2;
    lockrequest record;

  public:
    locktest()
    {
        record = {
            "xxxxx", "hmc-id", "Read", 234, {{"DontLock", 2}, {"DontLock", 4}}};
        // lockrequest with multiple lockrequests
        request = {{"xxxxx",
                    "hmc-id",
                    "Read",
                    234,
                    {{"DontLock", 2}, {"DontLock", 4}}},
                   {"xxxxx",
                    "hmc-id",
                    "Read",
                    234,
                    {{"DontLock", 2}, {"DontLock", 4}}}};
        request1 = {{"xxxxx",
                     "hmc-id",
                     "Read",
                     234,
                     {{"DontLock", 2}, {"DontLock", 4}}}};
        request2 = {{"xxxxx",
                     "hmc-id",
                     "Write",
                     234,
                     {{"LockAll", 2}, {"DontLock", 4}}}};
    }
    ~locktest()
    {
    }
};

class mocklock : public crow::ibm_mc_lock::lock
{
  public:
    bool isvalidlockrequest(lockrequest record1) override
    {
        bool status = lock::isvalidlockrequest(record1);
        return status;
    }
    bool isconflictrequest(lockrequests request) override
    {
        bool status = lock::isconflictrequest(request);
        return status;
    }
    rc isconflictwithtable(lockrequests request) override
    {
        auto conflict = lock::isconflictwithtable(request);
        return conflict;
    }
    uint32_t generateTransactionID() override
    {
        uint32_t tid = lock::generateTransactionID();
        return tid;
    }

    bool validaterids(std::vector<uint32_t> tids) override
    {
        bool status = lock::validaterids(tids);
        return status;
    }
    rcrelaselock isitmylock(std::vector<uint32_t> tids,
                            std::pair<stype, stype> ids) override
    {
        auto status = lock::isitmylock(tids, ids);
        return status;
    }
    rcgetlocklist getlist(std::vector<std::string> listsessionid)
    {
        auto status = lock::getlocklist(listsessionid);
        return status;
    }
    friend class locktest;
};

TEST_F(locktest, ValidationGoodTestCase)
{
    mocklock lockmanager;
    const lockrequest& t = record;
    ASSERT_EQ(1, lockmanager.isvalidlockrequest(t));
}

TEST_F(locktest, ValidationBadTestWithLocktype)
{
    mocklock lockmanager;
    // Corrupt the lock type
    std::get<2>(record) = "rwrite";
    const lockrequest& t = record;
    ASSERT_EQ(0, lockmanager.isvalidlockrequest(t));
}

TEST_F(locktest, ValidationBadTestWithlockFlags)
{
    mocklock lockmanager;
    // Corrupt the lockflag
    std::get<4>(record)[0].first = "lock";
    const lockrequest& t = record;
    ASSERT_EQ(0, lockmanager.isvalidlockrequest(t));
}

TEST_F(locktest, ValidationBadTestWithSegmentlength)
{
    mocklock lockmanager;
    // Corrupt the Segment length
    std::get<4>(record)[0].second = 7;
    const lockrequest& t = record;
    ASSERT_EQ(0, lockmanager.isvalidlockrequest(t));
}

TEST_F(locktest, MultiRequestWithoutConflict)
{
    mocklock lockmanager;
    const lockrequests& t = request;
    ASSERT_EQ(0, lockmanager.isconflictrequest(t));
}

TEST_F(locktest, MultiRequestWithConflictduetoSameSegmentLength)
{
    mocklock lockmanager;
    // Corrupt the locktype
    std::get<2>(request[0]) = "Write";
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "LockAll";
    const lockrequests& t = request;
    ASSERT_EQ(1, lockmanager.isconflictrequest(t));
}

TEST_F(locktest, MultiRequestWithoutConflictduetoDifferentSegmentData)
{
    mocklock lockmanager;
    // Corrupt the locktype
    std::get<2>(request[0]) = "Write";
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "DontLock";
    std::get<4>(request[0])[1].first = "LockAll";

    // Change the resource id(2nd byte) of first record, so the locks are
    // different so no conflict
    std::get<3>(request[0]) = 216173882346831872; // 03 00 01 00 2B 00 00 00
    // std::get<3>(request[1]) = 216736832300253184; // 03 02 01 00 2B 00 00 00
    std::get<3>(request[1]) = 216173882346832384; // 03 00 01 00 2B 00 02 00
    const lockrequests& t = request;
    ASSERT_EQ(0, lockmanager.isconflictrequest(t));
}

TEST_F(locktest, MultiRequestWithConflictduetoSameSegmentData)
{
    mocklock lockmanager;
    // Corrupt the locktype
    std::get<2>(request[0]) = "Write";
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "DontLock";
    std::get<4>(request[0])[1].first = "LockAll";
    // Dont Change the resource id(1st & 2nd byte) at all, so that the conflict
    // occurs from the second segment which is trying to lock all the resources.
    std::get<3>(request[0]) = 216173882346831872; // 03 00 01 00 2B 00 00 00
    std::get<3>(request[1]) = 216736832300253184; // 03 02 01 00 2B 00 00 00
    // std::get<3>(request[1]) = 216173882346832384; // 03 00 01 00 2B 00 02 00
    const lockrequests& t = request;
    ASSERT_EQ(1, lockmanager.isconflictrequest(t));
}

TEST_F(locktest, MultiRequestWithoutConflictduetoDifferentSegmentLength)
{
    mocklock lockmanager;
    // Corrupt the locktype
    std::get<2>(request[0]) = "Write";
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "LockSame";
    // Change the segment length , so that the requests are trying to lock
    // two different kind of resources
    std::get<4>(request[0])[0].second = 3;
    const lockrequests& t = request;
    // Return No Conflict
    ASSERT_EQ(0, lockmanager.isconflictrequest(t));
}

TEST_F(locktest, MultiRequestWithoutConflictduetoReadLocktype)
{
    mocklock lockmanager;
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "LockAll";
    const lockrequests& t = request;
    // Return No Conflict
    ASSERT_EQ(0, lockmanager.isconflictrequest(t));
}

TEST_F(locktest, MultiRequestWithoutConflictduetoReadLocktypeAndLockall)
{
    mocklock lockmanager;
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "LockAll";
    std::get<4>(request[0])[1].first = "LockAll";
    const lockrequests& t = request;
    // Return No Conflict
    ASSERT_EQ(0, lockmanager.isconflictrequest(t));
}

TEST_F(locktest, RequestConflictedWithLockTableEntries)
{
    mocklock lockmanager;
    const lockrequests& t = request1;
    auto rc1 = lockmanager.isconflictwithtable(t);
    // Corrupt the lock type
    std::get<2>(request[0]) = "Write";
    // Corrupt the lockflag
    std::get<4>(request[0])[1].first = "LockAll";
    const lockrequests& p = request;
    auto rc2 = lockmanager.isconflictwithtable(p);
    // Return a Conflict
    ASSERT_EQ(1, rc2.first);
}

TEST_F(locktest, RequestNotConflictedWithLockTableEntries)
{
    mocklock lockmanager;
    const lockrequests& t = request1;
    // Insert the request1 into the lock table
    auto rc1 = lockmanager.isconflictwithtable(t);
    // Corrupt the lock type
    std::get<2>(request[0]) = "Read";
    // Corrupt the lockflag
    std::get<4>(request[0])[1].first = "LockAll";
    const lockrequests& p = request;
    auto rc2 = lockmanager.isconflictwithtable(p);
    // Return No Conflict
    ASSERT_EQ(0, rc2.first);
}

TEST_F(locktest, TestGenerateTransactionIDFunction)
{
    mocklock lockmanager;
    uint32_t transactionid1 = lockmanager.generateTransactionID();
    uint32_t transactionid2 = lockmanager.generateTransactionID();
    EXPECT_TRUE(transactionid2 == ++transactionid1);
}

TEST_F(locktest, ValidateTransactionIDsGoodTestCase)
{
    mocklock lockmanager;
    const lockrequests& t = request1;
    // Insert the request1 into the lock table
    auto rc1 = lockmanager.isconflictwithtable(t);
    std::vector<uint32_t> tids = {1};
    const std::vector<uint32_t>& p = tids;
    ASSERT_EQ(1, lockmanager.validaterids(p));
}

TEST_F(locktest, ValidateTransactionIDsBadTestCase)
{
    mocklock lockmanager;
    // Insert the request1 into the lock table
    const lockrequests& t = request1;
    auto rc1 = lockmanager.isconflictwithtable(t);
    std::vector<uint32_t> tids = {3};
    const std::vector<uint32_t>& p = tids;
    ASSERT_EQ(0, lockmanager.validaterids(p));
}

TEST_F(locktest, ValidateisItMyLockGoodTestCase)
{
    mocklock lockmanager;
    // Insert the request1 into the lock table
    const lockrequests& t = request1;
    auto rc1 = lockmanager.isconflictwithtable(t);
    std::vector<uint32_t> tids = {1};
    const std::vector<uint32_t>& p = tids;
    std::string hmcid = "hmc-id";
    std::string sessionid = "xxxxx";
    std::pair<stype, stype> ids = std::make_pair(hmcid, sessionid);
    auto rc = lockmanager.isitmylock(p, ids);
    ASSERT_EQ(1, rc.first);
}

TEST_F(locktest, ValidateisItMyLockBadTestCase)
{
    mocklock lockmanager;

    // Corrupt the client identifier
    std::get<1>(request1[0]) = "randomid";
    // Insert the request1 into the lock table
    const lockrequests& t = request1;
    auto rc1 = lockmanager.isconflictwithtable(t);
    std::vector<uint32_t> tids = {1};
    const std::vector<uint32_t>& p = tids;
    std::string hmcid = "hmc-id";
    std::string sessionid = "xxxxx";
    std::pair<stype, stype> ids = std::make_pair(hmcid, sessionid);
    auto rc = lockmanager.isitmylock(p, ids);
    ASSERT_EQ(0, rc.first);
}

TEST_F(locktest, ValidateSessionIDForGetlocklistBadTestCase)
{
    mocklock lockmanager;
    // Insert the request1 into the lock table
    const lockrequests& t = request1;
    auto rc1 = lockmanager.isconflictwithtable(t);
    std::vector<std::string> sessionid = {"random"};
    auto status = lockmanager.getlocklist(sessionid);
    auto result =
        std::get<std::vector<std::pair<uint32_t, lockrequests>>>(status.second);
    ASSERT_EQ(0, result.size());
}

TEST_F(locktest, ValidateSessionIDForGetlocklistGoodTestCase)
{
    mocklock lockmanager;
    // Insert the request1 into the lock table
    const lockrequests& t = request1;
    auto rc1 = lockmanager.isconflictwithtable(t);
    std::vector<std::string> sessionid = {"xxxxx"};
    auto status = lockmanager.getlocklist(sessionid);
    auto result =
        std::get<std::vector<std::pair<uint32_t, lockrequests>>>(status.second);
    ASSERT_EQ(1, result.size());
}
} // namespace ibm_mc_lock
} // namespace crow
