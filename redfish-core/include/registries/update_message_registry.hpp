// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
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

namespace redfish::registries
{
struct Update
{
static constexpr Header header = {
    "Copyright 2014-2025 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    1,
    2,
    0,
    "Update Message Registry",
    "en",
    "This registry defines the update status and error messages.",
    "Update",
    "DMTF",
};

static constexpr const char* url =
    "https://redfish.dmtf.org/registries/Update.1.2.0.json";

static constexpr std::array registry =
{
    MessageEntry{
        "ActivateFailed",
        {
            "Indicates that the component failed to activate the image.",
            "Activation of image '%1' on '%2' failed.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "AllTargetsDetermined",
        {
            "Indicates that all target resources or devices for an update operation were determined by the service.",
            "All the target devices to be updated were determined.",
            "OK",
            0,
            {},
            "None.",
        }},
    MessageEntry{
        "ApplyFailed",
        {
            "Indicates that the component failed to apply an image.",
            "Installation of image '%1' to '%2' failed.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ApplyingOnComponent",
        {
            "Indicates that a component is applying an image.",
            "Image '%1' is being applied on '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "AwaitToActivate",
        {
            "Indicates that the resource or device is waiting for an action to proceed with activating an image.",
            "Awaiting for an action to proceed with activating image '%1' on '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "Perform the requested action to advance the update operation.",
        }},
    MessageEntry{
        "AwaitToUpdate",
        {
            "Indicates that the resource or device is waiting for an action to proceed with installing an image.",
            "Awaiting for an action to proceed with installing image '%1' on '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "Perform the requested action to advance the update operation.",
        }},
    MessageEntry{
        "InstallingOnComponent",
        {
            "Indicates that a component is installing an image.",
            "Image '%1' is being installed on '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "NoTargetsDetermined",
        {
            "Indicates that no target resource or device for an image was determined for update.",
            "No target device will be updated with image '%1'.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "OperationTransitionedToJob",
        {
            "Indicates that the update operation transitioned to a job for managing the progress of the operation.",
            "The update operation has transitioned to the job at URI '%1'.",
            "OK",
            1,
            {
                "string",
            },
            "Follow the referenced job and monitor the job for further updates.",
        }},
    MessageEntry{
        "TargetDetermined",
        {
            "Indicates that a target resource or device for an image was determined for update.",
            "The target device '%1' will be updated with image '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "TransferFailed",
        {
            "Indicates that the service failed to transfer an image to a component.",
            "Transfer of image '%1' to '%2' failed.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "TransferringToComponent",
        {
            "Indicates that the service is transferring an image to a component.",
            "Image '%1' is being transferred to '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "UpdateInProgress",
        {
            "Indicates that an update is in progress.",
            "An update is in progress.",
            "OK",
            0,
            {},
            "None.",
        }},
    MessageEntry{
        "UpdateNotApplicable",
        {
            "Indicates that the update was not applicable to the resource or device.",
            "Image '%1' was not applicable to device '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "UpdateSkipped",
        {
            "Indicates that an image was applicable to a resource or device, but the update was skipped due to policies in the service.",
            "Device '%1' skipped the update with image '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "UpdateSkippedSameVersion",
        {
            "Indicates that a resource or device update was skipped because the same version is already installed.",
            "Device '%1' skipped the update with image '%2' because the same version is already installed.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "UpdateSuccessful",
        {
            "Indicates that a resource or device was updated.",
            "Device '%1' successfully updated with image '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "VerificationFailed",
        {
            "Indicates that the service or component failed to verify an image.",
            "Verification of image '%1' at '%2' failed.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "VerifyingAtComponent",
        {
            "Indicates that a component is verifying an image.",
            "Image '%1' is being verified at '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},

};

enum class Index
{
    activateFailed = 0,
    allTargetsDetermined = 1,
    applyFailed = 2,
    applyingOnComponent = 3,
    awaitToActivate = 4,
    awaitToUpdate = 5,
    installingOnComponent = 6,
    noTargetsDetermined = 7,
    operationTransitionedToJob = 8,
    targetDetermined = 9,
    transferFailed = 10,
    transferringToComponent = 11,
    updateInProgress = 12,
    updateNotApplicable = 13,
    updateSkipped = 14,
    updateSkippedSameVersion = 15,
    updateSuccessful = 16,
    verificationFailed = 17,
    verifyingAtComponent = 18,
};
}; // struct update

[[gnu::constructor]] inline void registerUpdate()
{ registerRegistry<Update>(); }

} // namespace redfish::registries
