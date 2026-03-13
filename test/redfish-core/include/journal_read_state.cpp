// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "utils/journal_read_state.hpp"

#include "utils/journal_utils.hpp"

#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{

namespace
{

// The journald filesystem format is documented here:
// https://systemd.io/JOURNAL_FILE_FORMAT/

TEST(JournalReadState, TestDataFile)
{
    std::string journalFile(TEST_DATA_DIR "/vacuum.journal");
    std::optional<JournalReadState> state =
        JournalReadState::openFile(journalFile);
    ASSERT_TRUE(state);
    if (!state)
    {
        return;
    }
    ASSERT_EQ(state->next(), 1);

    // Note, this is technically opaque, so if it changes in the future
    EXPECT_EQ(state->getCursor(),
              "s=bf184a6f61fc4619886c6a7c63681ec8;"
              "i=120f;"
              "b=6b5b037894684e1fb93498a9469c8f79;"
              "m=788287f6;t=63649492fb873;"
              "x=d7680905cd14f13b");
    EXPECT_EQ(state->getRealtimeUsec(), 1748538248640627);
    EXPECT_EQ(
        state->getData("MESSAGE"),
        "Runtime Journal (/run/log/journal/95ea7c5055094ceb88078b3efa71d2d0) is 8M, max 64M, 56M free.");
    EXPECT_EQ(getJournalMetadataInt(*state, "PRIORITY"), 6);

    ASSERT_EQ(state->next(), 1);
    EXPECT_EQ(state->getCursor(),
              "s=bf184a6f61fc4619886c6a7c63681ec8;"
              "i=1210;b=6b5b037894684e1fb93498a9469c8f79;"
              "m=7882f714;t=63649493027a3;"
              "x=fbf01ac34e1fade");
    EXPECT_EQ(state->getRealtimeUsec(), 1748538248669091);
    EXPECT_EQ(state->getData("MESSAGE"),
              "Received client request to rotate journal, rotating.");
    EXPECT_EQ(getJournalMetadataInt(*state, "PRIORITY"), 6);
}
} // namespace
} // namespace redfish
