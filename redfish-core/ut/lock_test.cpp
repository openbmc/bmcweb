#include "nlohmann/json.hpp"
#include "openbmc_ibm_mc_rest.hpp"

#include <string>

#include "gmock/gmock.h"

namespace crow
{
namespace ibm_mc_lock
{

class locktest : public ::testing::Test
{
    // friend class lock;

  protected:
    lockrequest request;
    lockrequest request1, request2;
    lockrecord record;

    /* bool validlockrecord(lockrecord* record1)
     {
         bool status = lockobj.isvalidlockrecord(record1);
         return status;
     }*/

  public:
    locktest()
    {
        // segmentflags flags = {{"DontLock", 2}, {"DontLock", 4} };
        // record1 = std::make_tuple("xxxxx","hmc-id","read","234",segmentflags
        // {{"DontLock", 2}, {"DontLock", 4} });

        // lockrecord with a single request with multiple segment
        record = {
            "xxxxx", "hmc-id", "read", 234, {{"DontLock", 2}, {"DontLock", 4}}};

        // lockrequest with multiple lockrequests
        request = {{"xxxxx",
                    "hmc-id",
                    "read",
                    234,
                    {{"DontLock", 2}, {"DontLock", 4}}},
                   {"xxxxx",
                    "hmc-id",
                    "read",
                    234,
                    {{"Dontlock", 2}, {"DontLock", 4}}}};
        request1 = {{"xxxxx",
                     "hmc-id",
                     "read",
                     234,
                     {{"DontLock", 2}, {"DontLock", 4}}}};
        request2 = {{"xxxxx",
                     "hmc-id",
                     "write",
                     234,
                     {{"Lockall", 2}, {"DontLock", 4}}}};
        // lock lockobj;
    }

    ~locktest()
    {
    }
};

TEST_F(locktest, ValidationGoodTestCase)
{
    lock testlockobject;

    // check validity lockrecordof single request with multiple segment
    ASSERT_EQ(1, testlockobject.isvalidlockrecord(&record));
}

TEST_F(locktest, ValidationBadTestWithLocktype)
{
    lock testlockobject;

    // Corrupt the lock type
    std::get<2>(record) = "rwrite";
    ASSERT_EQ(0, testlockobject.isvalidlockrecord(&record));
}

TEST_F(locktest, ValidationBadTestWithlockFlags)
{
    lock testlockobject;

    // Corrupt the lockflag
    std::get<4>(record)[0].first = "lock";
    ASSERT_EQ(0, testlockobject.isvalidlockrecord(&record));
}

TEST_F(locktest, ValidationBadTestWithSegmentlength)
{
    lock testlockobject;

    // Corrupt the Segment length
    std::get<4>(record)[0].second = 7;
    ASSERT_EQ(0, testlockobject.isvalidlockrecord(&record));
}

TEST_F(locktest, MultiRequestWithoutConflict)
{
    lock testlockobject;
    ASSERT_EQ(0, testlockobject.isconflictrequest(&request));
}

TEST_F(locktest, MultiRequestWithConflictduetoSameSegmentLength)
{
    lock testlockobject;

    // Corrupt the locktype
    std::get<2>(request[0]) = "write";

    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "Lockall";

    ASSERT_EQ(1, testlockobject.isconflictrequest(&request));
}
TEST_F(locktest, MultiRequestWithoutConflictduetoDifferentSegmentData)
{
    lock testlockobject;

    // Corrupt the locktype
    std::get<2>(request[0]) = "write";

    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "DontLock";
    std::get<4>(request[0])[1].first = "Lockall";

    // Change the resource id(2nd byte) of first record, so the locks are
    // different so no conflict

    std::get<3>(request[0]) = 216173882346831872; // 03 00 01 00 2B 00 00 00
    // std::get<3>(request[1]) = 216736832300253184; // 03 02 01 00 2B 00 00 00
    std::get<3>(request[1]) = 216173882346832384; // 03 00 01 00 2B 00 02 00
    ASSERT_EQ(0, testlockobject.isconflictrequest(&request));
}
TEST_F(locktest, MultiRequestWithConflictduetoSameSegmentData)
{
    lock testlockobject;

    // Corrupt the locktype
    std::get<2>(request[0]) = "write";

    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "DontLock";
    std::get<4>(request[0])[1].first = "Lockall";

    // Dont Change the resource id(1st & 2nd byte) at all, so that the conflict
    // occurs from the second segment which is trying to lock all the resources.

    std::get<3>(request[0]) = 216173882346831872; // 03 00 01 00 2B 00 00 00
    std::get<3>(request[1]) = 216736832300253184; // 03 02 01 00 2B 00 00 00
    // std::get<3>(request[1]) = 216173882346832384; // 03 00 01 00 2B 00 02 00
    ASSERT_EQ(1, testlockobject.isconflictrequest(&request));
}

TEST_F(locktest, MultiRequestWithoutConflictduetoDifferentSegmentLength)
{
    lock testlockobject;

    // Corrupt the locktype
    std::get<2>(request[0]) = "write";

    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "Locksame";

    // Change the segment length , so that the requests are trying to lock
    // two different kind of resources
    std::get<4>(request[0])[0].second = 3;

    // Return No Conflict
    ASSERT_EQ(0, testlockobject.isconflictrequest(&request));
}

TEST_F(locktest, MultiRequestWithoutConflictduetoReadLocktype)
{
    lock testlockobject;

    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "Lockall";

    // Return No Conflict
    ASSERT_EQ(0, testlockobject.isconflictrequest(&request));
}

TEST_F(locktest, MultiRequestWithoutConflictduetoReadLocktypeAndLockall)
{
    lock testlockobject;

    // Match the segment lengths to points them to lock similar kind of
    // resource
    std::get<4>(request[0])[0].first = "Lockall";
    std::get<4>(request[0])[1].first = "Lockall";

    // Return No Conflict
    ASSERT_EQ(0, testlockobject.isconflictrequest(&request));
}

TEST_F(locktest, RequestConflictedWithLockTableEntries)
{
    lock testlockobject;

    auto rc1 = testlockobject.isconflictwithtable(&request1);

    // Corrupt the lock type
    std::get<2>(request[0]) = "write";
    // Corrupt the lockflag
    std::get<4>(request[0])[1].first = "Lockall";

    auto rc2 = testlockobject.isconflictwithtable(&request);
    // Return a Conflict
    ASSERT_EQ(1, rc2.first);
}

TEST_F(locktest, RequestNotConflictedWithLockTableEntries)
{
    lock testlockobject;

    // Insert the request1 into the lock table
    auto rc1 = testlockobject.isconflictwithtable(&request1);

    // Corrupt the lock type
    std::get<2>(request[0]) = "read";
    // Corrupt the lockflag
    std::get<4>(request[0])[1].first = "Lockall";

    auto rc2 = testlockobject.isconflictwithtable(&request);
    // Return No Conflict
    ASSERT_EQ(0, rc2.first);
}

TEST_F(locktest, TestGetMyRequestIDFunction)
{
    lock testlockobject;

    uint32_t rid1 = testlockobject.getmyrequestid();
    uint32_t rid2 = testlockobject.getmyrequestid();

    EXPECT_TRUE(rid2 == ++rid1);
}

TEST_F(locktest, ValidateRidsGoodTestCase)
{
    lock testlockobject;
    // Insert the request1 into the lock table
    auto rc1 = testlockobject.isconflictwithtable(&request1);
    std::vector<uint32_t> rids = {1};
    ASSERT_EQ(1, testlockobject.validaterids(&rids));
}
TEST_F(locktest, ValidateRidsBadTestCase)
{
    lock testlockobject;
    // Insert the request1 into the lock table
    auto rc1 = testlockobject.isconflictwithtable(&request1);
    std::vector<uint32_t> rids = {3};
    ASSERT_EQ(0, testlockobject.validaterids(&rids));
}
TEST_F(locktest, ValidateisItMyLockGoodTestCase)
{
    lock testlockobject;
    // Insert the request1 into the lock table
    auto rc1 = testlockobject.isconflictwithtable(&request1);
    std::vector<uint32_t> rids = {1};
    std::string hmcid = "hmc-id";
    auto rc = testlockobject.isitmylock(&rids, hmcid);
    ASSERT_EQ(1, rc.first);
}

TEST_F(locktest, ValidateisItMyLockBadTestCase)
{
    lock testlockobject;

    // Corrupt the client identifier
    std::get<1>(request1[0]) = "randomid";

    // Insert the request1 into the lock table
    auto rc1 = testlockobject.isconflictwithtable(&request1);
    std::vector<uint32_t> rids = {1};
    std::string hmcid = "hmc-id";
    auto rc = testlockobject.isitmylock(&rids, hmcid);
    ASSERT_EQ(0, rc.first);
}

TEST_F(locktest, ValidateSessionIDForGetlocklistBadTestCase)
{
    lock testlockobject;
    // Insert the request1 into the lock table
    auto rc1 = testlockobject.isconflictwithtable(&request1);
    // Corrupt the lock type
    // std::get<1>(record) = "rwrite";
    std::string sessionid = "random";
    auto status = testlockobject.getlocklist(sessionid);
    ASSERT_EQ(0, status.first);
}

TEST_F(locktest, ValidateSessionIDForGetlocklistGoodTestCase)
{
    lock testlockobject;
    // Insert the request1 into the lock table
    auto rc1 = testlockobject.isconflictwithtable(&request1);
    // Corrupt the lock type
    // std::get<1>(record) = "rwrite";
    std::string sessionid = "xxxxx";
    auto status = testlockobject.getlocklist(sessionid);
    ASSERT_EQ(1, status.first);
}
} // namespace ibm_mc_lock
} // namespace crow
