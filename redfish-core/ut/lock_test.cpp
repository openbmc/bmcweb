#include "ibm/locks.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <gmock/gmock.h> // IWYU pragma: keep
#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace crow::ibm_mc_lock
{
namespace
{

using SType = std::string;
using LockRequest = std::tuple<SType, SType, SType, uint64_t, SegmentFlags>;
using LockRequests = std::vector<LockRequest>;
using Rc =
    std::pair<bool, std::variant<uint32_t, std::pair<uint32_t, LockRequest>>>;
using RcRelaseLock = std::pair<bool, std::pair<uint32_t, LockRequest>>;
using RcGetLockList =
    std::variant<std::string, std::vector<std::pair<uint32_t, LockRequests>>>;
using ListOfTransactionIds = std::vector<uint32_t>;
using RcAcquireLock = std::pair<bool, std::variant<Rc, std::pair<bool, int>>>;
using RcReleaseLockApi = std::pair<bool, std::variant<bool, RcRelaseLock>>;
using SessionFlags = std::pair<SType, SType>;
using ListOfSessionIds = std::vector<std::string>;
using ::testing::IsEmpty;

class LockTest : public ::testing::Test
{
  protected:
    LockRequests request;
    LockRequests request1, request2;
    LockRequest record;

  public:
    LockTest() :
        // lockrequest with multiple lockrequests
        request{{"xxxxx",
                 "hmc-id",
                 "Read",
                 234,
                 {{"DontLock", 2}, {"DontLock", 4}}},
                {"xxxxx",
                 "hmc-id",
                 "Read",
                 234,
                 {{"DontLock", 2}, {"DontLock", 4}}}},
        request1{{"xxxxx",
                  "hmc-id",
                  "Read",
                  234,
                  {{"DontLock", 2}, {"DontLock", 4}}}},
        request2{{"xxxxx",
                  "hmc-id",
                  "Write",
                  234,
                  {{"LockAll", 2}, {"DontLock", 4}}}},
        record{
            "xxxxx", "hmc-id", "Read", 234, {{"DontLock", 2}, {"DontLock", 4}}}
    {}

    ~LockTest() override = default;

    LockTest(const LockTest&) = delete;
    LockTest(LockTest&&) = delete;
    LockTest& operator=(const LockTest&) = delete;
    LockTest& operator=(const LockTest&&) = delete;
};

class MockLock : public crow::ibm_mc_lock::Lock
{
  public:
    bool isValidLockRequest(const LockRequest& record1) override
    {
        bool status = Lock::isValidLockRequest(record1);
        return status;
    }
    bool isConflictRequest(const LockRequests& request) override
    {
        bool status = Lock::isConflictRequest(request);
        return status;
    }
    Rc isConflictWithTable(const LockRequests& request) override
    {
        auto conflict = Lock::isConflictWithTable(request);
        return conflict;
    }
    uint32_t generateTransactionId() override
    {
        uint32_t tid = Lock::generateTransactionId();
        return tid;
    }

    bool validateRids(const ListOfTransactionIds& tids) override
    {
        bool status = Lock::validateRids(tids);
        return status;
    }
    RcRelaseLock isItMyLock(const ListOfTransactionIds& tids,
                            const SessionFlags& ids) override
    {
        auto status = Lock::isItMyLock(tids, ids);
        return status;
    }
    friend class LockTest;
};

TEST_F(LockTest, ValidationGoodTestCase)
{
    MockLock lockManager;
    const LockRequest& t = record;
    EXPECT_TRUE(lockManager.isValidLockRequest(t));
}

TEST_F(LockTest, ValidationBadTestWithLocktype)
{
    MockLock lockManager;
    // Corrupt the lock type
    std::get<2>(record) = "rwrite";
    const LockRequest& t = record;
    EXPECT_FALSE(lockManager.isValidLockRequest(t));
}

TEST_F(LockTest, ValidationBadTestWithlockFlags)
{
    MockLock lockManager;
    // Corrupt the lockflag
    std::get<4>(record)[0].first = "lock";
    const LockRequest& t = record;
    EXPECT_FALSE(lockManager.isValidLockRequest(t));
}

TEST_F(LockTest, ValidationBadTestWithSegmentlength)
{
    MockLock lockManager;
    // Corrupt the Segment length
    std::get<4>(record)[0].second = 7;
    const LockRequest& t = record;
    EXPECT_FALSE(lockManager.isValidLockRequest(t));
}

TEST_F(LockTest, MultiRequestWithoutConflict)
{
    MockLock lockManager;
    const LockRequests& t = request;
    EXPECT_FALSE(lockManager.isConflictRequest(t));
}

TEST_F(LockTest, MultiRequestWithConflictduetoSameSegmentLength)
{
    MockLock lockManager;
    // Corrupt the locktype
    std::get<2>(request[0]) = "Write";
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "LockAll";
    const LockRequests& t = request;
    EXPECT_TRUE(lockManager.isConflictRequest(t));
}

TEST_F(LockTest, MultiRequestWithoutConflictduetoDifferentSegmentData)
{
    MockLock lockManager;
    // Corrupt the locktype
    std::get<2>(request[0]) = "Write";
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "DontLock";
    std::get<4>(request[0])[1].first = "LockAll";

    // Change the resource id(2nd byte) of first record, so the locks are
    // different so no conflict
    std::get<3>(request[0]) = 216179379183550464; // HEX 03 00 06 00 00 00 00 00
    std::get<3>(request[1]) = 288236973221478400; // HEX 04 00 06 00 00 00 00 00
    const LockRequests& t = request;
    EXPECT_FALSE(lockManager.isConflictRequest(t));
}

TEST_F(LockTest, MultiRequestWithConflictduetoSameSegmentData)
{
    MockLock lockManager;
    // Corrupt the locktype
    std::get<2>(request[0]) = "Write";
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "DontLock";
    std::get<4>(request[0])[1].first = "LockAll";
    // Dont Change the resource id(1st & 2nd byte) at all, so that the
    // conflict occurs from the second segment which is trying to lock all
    // the resources.
    std::get<3>(request[0]) = 216173882346831872; // 03 00 01 00 2B 00 00 00
    std::get<3>(request[1]) = 216173882346831872; // 03 00 01 00 2B 00 00 00
    const LockRequests& t = request;
    EXPECT_TRUE(lockManager.isConflictRequest(t));
}

TEST_F(LockTest, MultiRequestWithoutConflictduetoDifferentSegmentLength)
{
    MockLock lockManager;
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
    EXPECT_FALSE(lockManager.isConflictRequest(t));
}

TEST_F(LockTest, MultiRequestWithoutConflictduetoReadLocktype)
{
    MockLock lockManager;
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "LockAll";
    const LockRequests& t = request;
    // Return No Conflict
    EXPECT_FALSE(lockManager.isConflictRequest(t));
}

TEST_F(LockTest, MultiRequestWithoutConflictduetoReadLocktypeAndLockall)
{
    MockLock lockManager;
    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "LockAll";
    std::get<4>(request[0])[1].first = "LockAll";
    const LockRequests& t = request;
    // Return No Conflict
    EXPECT_FALSE(lockManager.isConflictRequest(t));
}

TEST_F(LockTest, RequestConflictedWithLockTableEntries)
{
    MockLock lockManager;
    const LockRequests& t = request1;
    auto rc1 = lockManager.isConflictWithTable(t);
    // Corrupt the lock type
    std::get<2>(request[0]) = "Write";
    // Corrupt the lockflag
    std::get<4>(request[0])[1].first = "LockAll";
    const LockRequests& p = request;
    auto rc2 = lockManager.isConflictWithTable(p);
    // Return a Conflict
    EXPECT_TRUE(rc2.first);
}

TEST_F(LockTest, RequestNotConflictedWithLockTableEntries)
{
    MockLock lockManager;
    const LockRequests& t = request1;
    // Insert the request1 into the lock table
    auto rc1 = lockManager.isConflictWithTable(t);
    // Corrupt the lock type
    std::get<2>(request[0]) = "Read";
    // Corrupt the lockflag
    std::get<4>(request[0])[1].first = "LockAll";
    const LockRequests& p = request;
    auto rc2 = lockManager.isConflictWithTable(p);
    // Return No Conflict
    EXPECT_FALSE(rc2.first);
}

TEST_F(LockTest, TestGenerateTransactionIDFunction)
{
    MockLock lockManager;
    uint32_t transactionId1 = lockManager.generateTransactionId();
    uint32_t transactionId2 = lockManager.generateTransactionId();
    EXPECT_EQ(transactionId2, ++transactionId1);
}

TEST_F(LockTest, ValidateTransactionIDsGoodTestCase)
{
    MockLock lockManager;
    const LockRequests& t = request1;
    // Insert the request1 into the lock table
    auto rc1 = lockManager.isConflictWithTable(t);
    std::vector<uint32_t> tids = {1};
    const std::vector<uint32_t>& p = tids;
    EXPECT_TRUE(lockManager.validateRids(p));
}

TEST_F(LockTest, ValidateTransactionIDsBadTestCase)
{
    MockLock lockManager;
    // Insert the request1 into the lock table
    const LockRequests& t = request1;
    auto rc1 = lockManager.isConflictWithTable(t);
    std::vector<uint32_t> tids = {10};
    const std::vector<uint32_t>& p = tids;
    EXPECT_FALSE(lockManager.validateRids(p));
}

TEST_F(LockTest, ValidateisItMyLockGoodTestCase)
{
    MockLock lockManager;
    // Insert the request1 into the lock table
    const LockRequests& t = request1;
    auto rc1 = lockManager.isConflictWithTable(t);
    std::vector<uint32_t> tids = {1};
    const std::vector<uint32_t>& p = tids;
    std::string hmcid = "hmc-id";
    std::string sessionid = "xxxxx";
    std::pair<SType, SType> ids = std::make_pair(hmcid, sessionid);
    auto rc = lockManager.isItMyLock(p, ids);
    EXPECT_TRUE(rc.first);
}

TEST_F(LockTest, ValidateisItMyLockBadTestCase)
{
    MockLock lockManager;
    // Corrupt the client identifier
    std::get<1>(request1[0]) = "randomid";
    // Insert the request1 into the lock table
    const LockRequests& t = request1;
    auto rc1 = lockManager.isConflictWithTable(t);
    std::vector<uint32_t> tids = {1};
    const std::vector<uint32_t>& p = tids;
    std::string hmcid = "hmc-id";
    std::string sessionid = "random";
    std::pair<SType, SType> ids = std::make_pair(hmcid, sessionid);
    auto rc = lockManager.isItMyLock(p, ids);
    EXPECT_FALSE(rc.first);
}

TEST_F(LockTest, ValidateSessionIDForGetlocklistBadTestCase)
{
    MockLock lockManager;
    // Insert the request1 into the lock table
    const LockRequests& t = request1;
    auto rc1 = lockManager.isConflictWithTable(t);
    std::vector<std::string> sessionid = {"random"};
    auto status = lockManager.getLockList(sessionid);
    auto result =
        std::get<std::vector<std::pair<uint32_t, LockRequests>>>(status);
    EXPECT_THAT(result, IsEmpty());
}

TEST_F(LockTest, ValidateSessionIDForGetlocklistGoodTestCase)
{
    MockLock lockManager;
    // Insert the request1 into the lock table
    const LockRequests& t = request1;
    auto rc1 = lockManager.isConflictWithTable(t);
    std::vector<std::string> sessionid = {"xxxxx"};
    auto status = lockManager.getLockList(sessionid);
    auto result =
        std::get<std::vector<std::pair<uint32_t, LockRequests>>>(status);
    EXPECT_EQ(result.size(), 1);
}

} // namespace
} // namespace crow::ibm_mc_lock
