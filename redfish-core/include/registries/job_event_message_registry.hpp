#pragma once
/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 * DO NOT modify this registry outside of running the
 * parse_registries.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/
#include "registries.hpp"

#include <array>

// clang-format off

namespace redfish::registries::job_event
{
const Header header = {
    "Copyright 2014-2023 DMTF in cooperation with the Storage Networking Industry Association (SNIA). All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    "JobEvent.1.0.1",
    "Job Event Message Registry",
    "en",
    "This registry defines the messages for job related events.",
    "JobEvent",
    "1.0.1",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/JobEvent.1.0.1.json";

constexpr std::array registry =
{
    MessageEntry{
        "JobCancelled",
        {
            "A job was cancelled.",
            "The job with Id '%1' was cancelled.",
            "Warning",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "JobCompletedException",
        {
            "A job has completed with warnings or errors.",
            "The job with Id '%1' has completed with warnings or errors.",
            "Warning",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "JobCompletedOK",
        {
            "A job has completed.",
            "The job with Id '%1' has completed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "JobProgressChanged",
        {
            "A job has changed progress.",
            "The job with Id '%1' has changed to progress %2 percent complete.",
            "OK",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "JobRemoved",
        {
            "A job was removed.",
            "The job with Id '%1' was removed.",
            "Warning",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "JobResumed",
        {
            "A job has resumed.",
            "The job with Id '%1' has resumed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "JobStarted",
        {
            "A job has started.",
            "The job with Id '%1' has started.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "JobSuspended",
        {
            "A job was suspended.",
            "The job with Id '%1' was suspended.",
            "Warning",
            1,
            {
                "string",
            },
            "None.",
        }},

};

enum class Index
{
    jobCancelled = 0,
    jobCompletedException = 1,
    jobCompletedOK = 2,
    jobProgressChanged = 3,
    jobRemoved = 4,
    jobResumed = 5,
    jobStarted = 6,
    jobSuspended = 7,
};
} // namespace redfish::registries::job_event
