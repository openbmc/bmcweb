/****************************************************************
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined json messages for event.
 ***************************************************************/
namespace redfish
{
namespace messages
{

inline nlohmann::json targetDetermined(const std::string& arg1,
                                       const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.TargetDetermined"},
        {"Message", "The target device '" + arg1 +
                        "' will be updated with image '" + arg2 + "'."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}
inline nlohmann::json allTargetsDetermined()
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.AllTargetsDetermined"},
        {"Message",
         "All the target device to be updated have been determined."},
        {"MessageArgs", {}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}
inline nlohmann::json updateInProgress()
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.UpdateInProgress"},
        {"Message", "An update is in progress."},
        {"MessageArgs", {}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}
inline nlohmann::json transferringToComponent(const std::string& arg1,
                                              const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.TransferringToComponent"},
        {"Message",
         "Image '" + arg1 + "' is being transferred to '" + arg2 + "'."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}
inline nlohmann::json verifyingAtComponent(const std::string& arg1,
                                           const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.VerifyingAtComponent"},
        {"Message",
         "Image '" + arg1 + "' is being verified at '" + arg2 + "'."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}
inline nlohmann::json installingOnComponent(const std::string& arg1,
                                            const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.InstallingOnComponent"},
        {"Message",
         "Image '" + arg1 + "' is being installed on '" + arg2 + "'."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}
inline nlohmann::json applyingOnComponent(const std::string& arg1,
                                          const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.ApplyingOnComponent"},
        {"Message", "Image '" + arg1 + "' is being applied on '" + arg2 + "'."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}
inline nlohmann::json transferFailed(const std::string& arg1,
                                     const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.TransferFailed"},
        {"Message",
         "Transfer of image '" + arg1 + "' to '" + arg2 + "' failed."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Critical"},
        {"Resolution", "None."}};
}
inline nlohmann::json verificationFailed(const std::string& arg1,
                                         const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.VerificationFailed"},
        {"Message",
         "Verification of image '" + arg1 + "' at '" + arg2 + "' failed."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Critical"},
        {"Resolution", "None."}};
}
inline nlohmann::json applyFailed(const std::string& arg1,
                                  const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.ApplyFailed"},
        {"Message",
         "Installation of image '" + arg1 + "' to '" + arg2 + "' failed."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Critical"},
        {"Resolution", "None."}};
}
inline nlohmann::json activateFailed(const std::string& arg1,
                                     const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.ActivateFailed"},
        {"Message",
         "Activation of image '" + arg1 + "' on '" + arg2 + "' failed."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Critical"},
        {"Resolution", "None."}};
}
inline nlohmann::json awaitToUpdate(const std::string& arg1,
                                    const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.AwaitToUpdate"},
        {"Message",
         "Awaiting for an action to proceed with installing image '" + arg1 +
             "' on '" + arg2 + "'."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "OK"},
        {"Resolution",
         "Perform the requested action to advance the update operation."}};
}
inline nlohmann::json awaitToActivate(const std::string& arg1,
                                      const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.AwaitToActivate"},
        {"Message",
         "Awaiting for an action to proceed with activating image '" + arg1 +
             "' on '" + arg2 + "'."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "OK"},
        {"Resolution",
         "Perform the requested action to advance the update operation."}};
}
inline nlohmann::json updateSuccessful(const std::string& arg1,
                                       const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.UpdateSuccessful"},
        {"Message", "Device '" + arg1 + "' successfully updated with image '" +
                        arg2 + "'."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}
inline nlohmann::json operationTransitionedToJob(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_1.MessageRegistry"},
        {"MessageId", "Update.1.0.0.OperationTransitionedToJob"},
        {"Message",
         "The update operation has transitioned to the job at URI '" + arg1 +
             "'."},
        {"MessageArgs", {arg1}},
        {"Severity", "OK"},
        {"Resolution",
         "Follow the referenced job and monitor the job for further updates."}};
}
} // namespace messages
} // namespace redfish