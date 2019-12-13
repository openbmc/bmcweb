#include "ibm/locks.hpp"
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
    LockRequests request;
    LockRequests request1, request2;
    LockRequest record;

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

class mocklock : public crow::ibm_mc_lock::Lock
{
  public:
    bool isValidLockRequest(LockRequest record1) override
    {
        bool status = Lock::isValidLockRequest(record1);
        return status;
    }
    bool isConflictRequest(LockRequests request) override
    {
        bool status = Lock::isConflictRequest(request);
        return status;
    }
    Rc isConflictWithTable(LockRequests request) override
    {
        auto conflict = Lock::isConflictWithTable(request);
        return conflict;
    }
    uint32_t generateTransactionId() override
    {
        uint32_t tid = Lock::generateTransactionId();
        return tid;
    }

    bool validateRids(std::vector<uint32_t> tids) override
    {
        bool status = Lock::validateRids(tids);
        return status;
    }
    RcRelaseLock isItMyLock(std::vector<uint32_t> tids,
                            std::pair<SType, SType> ids) override
    {
        auto status = Lock::isItMyLock(tids, ids);
        return status;
    }
    RcGetLockList getList(std::vector<std::string> listSessionid)
    {
        auto status = Lock::getLockList(listSessionid);
        return status;
    }
    friend class locktest;
};

TEST_F(locktest, ValidationGoodTestCase)
{
    mocklock lockmanager;
    const LockRequest& t = record;
    ASSERT_EQ(1, lockmanager.isValidLockRequest(t));
}

TEST_F(locktest, ValidationBadTestWithLocktype)
{
    mocklock lockmanager;
    // Corrupt the lock type
    std::get<2>(record) = "rwrite";
    const LockRequest& t = record;
    ASSERT_EQ(0, lockmanager.isValidLockRequest(t));
}

TEST_F(locktest, ValidationBadTestWithlockFlags)
{
    mocklock lockmanager;
    // Corrupt the lockflag
    std::get<4>(record)[0].first = "lock";
    const LockRequest& t = record;
    ASSERT_EQ(0, lockmanager.isValidLockRequest(t));
}

TEST_F(locktest, ValidationBadTestWithSegmentlength)
{
    mocklock lockmanager;
    // Corrupt the Segment length
    std::get<4>(record)[0].second = 7;
    const LockRequest& t = record;
    ASSERT_EQ(0, lockmanager.isValidLockRequest(t));
}

TEST_F(locktest, MultiRequestWithoutConflict)
{
    mocklock lockmanager;
    const LockRequests& t = request;
    ASSERT_EQ(0, lockmanager.isConflictRequest(t));
}

TEST_F(locktest, MultiRequestWithConflictduetoSameSegmentLength)
{
    mocklock lockmanager;
    // Corrupt the locktype
    std::get<2>(request[0]) = "Write";
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "LockAll";
    const LockRequests& t = request;
    ASSERT_EQ(1, lockmanager.isConflictRequest(t));
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
    const LockRequests& t = request;
    ASSERT_EQ(0, lockmanager.isConflictRequest(t));
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
    const LockRequests& t = request;
    ASSERT_EQ(1, lockmanager.isConflictRequest(t));
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
    const LockRequests& t = request;
    // Return No Conflict
    ASSERT_EQ(0, lockmanager.isConflictRequest(t));
}

TEST_F(locktest, MultiRequestWithoutConflictduetoReadLocktype)
{
    mocklock lockmanager;
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "LockAll";
    const LockRequests& t = request;
    // Return No Conflict
    ASSERT_EQ(0, lockmanager.isConflictRequest(t));
}

TEST_F(locktest, MultiRequestWithoutConflictduetoReadLocktypeAndLockall)
{
    mocklock lockmanager;
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "LockAll";
    std::get<4>(request[0])[1].first = "LockAll";
    const LockRequests& t = request;
    // Return No Conflict
    ASSERT_EQ(0, lockmanager.isConflictRequest(t));
}

TEST_F(locktest, RequestConflictedWithLockTableEntries)
{
    mocklock lockmanager;
    const LockRequests& t = request1;
    auto rc1 = lockmanager.isConflictWithTable(t);
    // Corrupt the lock type
    std::get<2>(request[0]) = "Write";
    // Corrupt the lockflag
    std::get<4>(request[0])[1].first = "LockAll";
    const LockRequests& p = request;
    auto rc2 = lockmanager.isConflictWithTable(p);
    // Return a Conflict
    ASSERT_EQ(1, rc2.first);
}

TEST_F(locktest, RequestNotConflictedWithLockTableEntries)
{
    mocklock lockmanager;
    const LockRequests& t = request1;
    // Insert the request1 into the lock table
    auto rc1 = lockmanager.isConflictWithTable(t);
    // Corrupt the lock type
    std::get<2>(request[0]) = "Read";
    // Corrupt the lockflag
    std::get<4>(request[0])[1].first = "LockAll";
    const LockRequests& p = request;
    auto rc2 = lockmanager.isConflictWithTable(p);
    // Return No Conflict
    ASSERT_EQ(0, rc2.first);
}

TEST_F(locktest, TestGenerateTransactionIDFunction)
{
    mocklock lockmanager;
    uint32_t transactionid1 = lockmanager.generateTransactionId();
    uint32_t transactionid2 = lockmanager.generateTransactionId();
    EXPECT_TRUE(transactionid2 == ++transactionid1);
}

TEST_F(locktest, ValidateTransactionIDsGoodTestCase)
{
    mocklock lockmanager;
    const LockRequests& t = request1;
    // Insert the request1 into the lock table
    auto rc1 = lockmanager.isConflictWithTable(t);
    std::vector<uint32_t> tids = {1};
    const std::vector<uint32_t>& p = tids;
    ASSERT_EQ(1, lockmanager.validateRids(p));
}

TEST_F(locktest, ValidateTransactionIDsBadTestCase)
{
    mocklock lockmanager;
    // Insert the request1 into the lock table
    const LockRequests& t = request1;
    auto rc1 = lockmanager.isConflictWithTable(t);
    std::vector<uint32_t> tids = {3};
    const std::vector<uint32_t>& p = tids;
    ASSERT_EQ(0, lockmanager.validateRids(p));
}

TEST_F(locktest, ValidateisItMyLockGoodTestCase)
{
    mocklock lockmanager;
    // Insert the request1 into the lock table
    const LockRequests& t = request1;
    auto rc1 = lockmanager.isConflictWithTable(t);
    std::vector<uint32_t> tids = {1};
    const std::vector<uint32_t>& p = tids;
    std::string hmcid = "hmc-id";
    std::string sessionid = "xxxxx";
    std::pair<SType, SType> ids = std::make_pair(hmcid, sessionid);
    auto rc = lockmanager.isItMyLock(p, ids);
    ASSERT_EQ(1, rc.first);
}

TEST_F(locktest, ValidateisItMyLockBadTestCase)
{
    mocklock lockmanager;

    // Corrupt the client identifier
    std::get<1>(request1[0]) = "randomid";
    // Insert the request1 into the lock table
    const LockRequests& t = request1;
    auto rc1 = lockmanager.isConflictWithTable(t);
    std::vector<uint32_t> tids = {1};
    const std::vector<uint32_t>& p = tids;
    std::string hmcid = "hmc-id";
    std::string sessionid = "xxxxx";
    std::pair<SType, SType> ids = std::make_pair(hmcid, sessionid);
    auto rc = lockmanager.isItMyLock(p, ids);
    ASSERT_EQ(0, rc.first);
}

TEST_F(locktest, ValidateSessionIDForGetlocklistBadTestCase)
{
    mocklock lockmanager;
    // Insert the request1 into the lock table
    const LockRequests& t = request1;
    auto rc1 = lockmanager.isConflictWithTable(t);
    std::vector<std::string> sessionid = {"random"};
    auto status = lockmanager.getLockList(sessionid);
    auto result =
        std::get<std::vector<std::pair<uint32_t, LockRequests>>>(status.second);
    ASSERT_EQ(0, result.size());
}

TEST_F(locktest, ValidateSessionIDForGetlocklistGoodTestCase)
{
    mocklock lockmanager;
    // Insert the request1 into the lock table
    const LockRequests& t = request1;
    auto rc1 = lockmanager.isConflictWithTable(t);
    std::vector<std::string> sessionid = {"xxxxx"};
    auto status = lockmanager.getLockList(sessionid);
    auto result =
        std::get<std::vector<std::pair<uint32_t, LockRequests>>>(status.second);
    ASSERT_EQ(1, result.size());
}
} // namespace ibm_mc_lock
} // namespace crow
