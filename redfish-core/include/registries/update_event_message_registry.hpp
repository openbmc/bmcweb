/****************************************************************
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 ***************************************************************/
#pragma once
#include <registries.hpp>

namespace redfish::message_registries::update_event
{
const Header header = {
    "Copyright 2014-2020 DMTF. All rights reserved.",
    "#MessageRegistry.v1_4_1.MessageRegistry",
    "Update.1.0.0",
    "Update Message Registry",
    "en",
    "This registry defines the update status and error messages.",
    "Update",
    "1.0.0",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/Update.1.0.0.json";

constexpr std::array<MessageEntry, 15> registry = {
    MessageEntry{
        "ActivateFailed",
        {
            "Indicates that the component failed to activate the image.",
            "Activation of image '%1' on '%2' failed.",
            "Critical",
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
            "Indicates that all target resources or devices for an update "
            "operation have been determined by the service.",
            "All the target device to be updated have been determined.",
            "OK",
            "OK",
            0,
            {},
            "None.",
        }},
    MessageEntry{"ApplyFailed",
                 {
                     "Indicates that the component failed to apply an image.",
                     "Installation of image '%1' to '%2' failed.",
                     "Critical",
                     "Critical",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"ApplyingOnComponent",
                 {
                     "Indicates that a component is applying an image.",
                     "Image '%1' is being applied on '%2'.",
                     "OK",
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
            "Indicates that the resource or device is awaiting for an action "
            "to proceed with activating an image.",
            "Awaiting for an action to proceed with activating image '%1' on "
            "'%2'.",
            "OK",
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
            "Indicates that the resource or device is awaiting for an action "
            "to proceed with installing an image.",
            "Awaiting for an action to proceed with installing image '%1' on "
            "'%2'.",
            "OK",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "Perform the requested action to advance the update operation.",
        }},
    MessageEntry{"InstallingOnComponent",
                 {
                     "Indicates that a component is installing an image.",
                     "Image '%1' is being installed on '%2'.",
                     "OK",
                     "OK",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{
        "OperationTransitionedToJob",
        {
            "Indicates that the update operation transitioned to a job for "
            "managing the progress of the operation.",
            "The update operation has transitioned to the job at URI '%1'.",
            "OK",
            "OK",
            1,
            {
                "string",
            },
            "Follow the referenced job and monitor the job for further "
            "updates.",
        }},
    MessageEntry{"TargetDetermined",
                 {
                     "Indicates that a target resource or device for a image "
                     "has been determined for update.",
                     "The target device '%1' will be updated with image '%2'.",
                     "OK",
                     "OK",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TransferFailed",
                 {
                     "Indicates that the service failed to transfer an image "
                     "to a component.",
                     "Transfer of image '%1' to '%2' failed.",
                     "Critical",
                     "Critical",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TransferringToComponent",
                 {
                     "Indicates that the service is transferring an image to a "
                     "component.",
                     "Image '%1' is being transferred to '%2'.",
                     "OK",
                     "OK",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"UpdateInProgress",
                 {
                     "Indicates that an update is in progress.",
                     "An update is in progress.",
                     "OK",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"UpdateSuccessful",
                 {
                     "Indicates that a resource or device was updated.",
                     "Device '%1' successfully updated with image '%2'.",
                     "OK",
                     "OK",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"VerificationFailed",
                 {
                     "Indicates that the component failed to verify an image.",
                     "Verification of image '%1' at '%2' failed.",
                     "Critical",
                     "Critical",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"VerifyingAtComponent",
                 {
                     "Indicates that a component is verifying an image.",
                     "Image '%1' is being verified at '%2'.",
                     "OK",
                     "OK",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "None.",
                 }},
};
} // namespace redfish::message_registries::update_event
