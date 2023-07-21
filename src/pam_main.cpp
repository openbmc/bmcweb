#include <security/pam_appl.h>

#include <boost/asio/io_context.hpp>
#include <boost/utility/string_view.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <cstring>
#include <memory>
#include <span>

// function used to get user input
static int pamFunctionConversation(int numMsg, const struct pam_message** msg,
                                   struct pam_response** resp, void* appdataPtr)
{
    if ((appdataPtr == nullptr) || (msg == nullptr) || (resp == nullptr))
    {
        return PAM_CONV_ERR;
    }

    if (numMsg <= 0 || numMsg >= PAM_MAX_NUM_MSG)
    {
        return PAM_CONV_ERR;
    }

    auto msgCount = static_cast<size_t>(numMsg);
    auto messages = std::span(msg, msgCount);
    auto responses = std::span(resp, msgCount);

    for (size_t i = 0; i < msgCount; ++i)
    {
        /* Ignore all PAM messages except prompting for hidden input */
        if (messages[i]->msg_style != PAM_PROMPT_ECHO_OFF)
        {
            continue;
        }

        /* Assume PAM is only prompting for the password as hidden input */
        /* Allocate memory only when PAM_PROMPT_ECHO_OFF is encounterred */

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        char* appPass = reinterpret_cast<char*>(appdataPtr);
        size_t appPassSize = std::strlen(appPass);

        if ((appPassSize + 1) > PAM_MAX_RESP_SIZE)
        {
            return PAM_CONV_ERR;
        }
        // IDeally we'd like to avoid using malloc here, but because we're
        // passing off ownership of this to a C application, there aren't a lot
        // of sane ways to avoid it.

        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        void* passPtr = malloc(appPassSize + 1);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        char* pass = reinterpret_cast<char*>(passPtr);
        if (pass == nullptr)
        {
            return PAM_BUF_ERR;
        }

        std::strncpy(pass, appPass, appPassSize + 1);

        size_t numMsgSize = static_cast<size_t>(numMsg);
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        void* ptr = calloc(numMsgSize, sizeof(struct pam_response));
        if (ptr == nullptr)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
            free(pass);
            return PAM_BUF_ERR;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *resp = reinterpret_cast<pam_response*>(ptr);

        responses[i]->resp = pass;

        return PAM_SUCCESS;
    }

    return PAM_CONV_ERR;
}

/**
 * @brief Attempt username/password authentication via PAM.
 * @param username The provided username aka account name.
 * @param password The provided password.
 * @returns PAM error code or PAM_SUCCESS for success. */
static int pamAuthenticateUser(const std::string& username,
                               const std::string& password)
{
    std::string passStr(password);
    char* passStrNoConst = passStr.data();
    const struct pam_conv localConversation = {pamFunctionConversation,
                                               passStrNoConst};
    pam_handle_t* localAuthHandle = nullptr; // this gets set by pam_start

    int retval = pam_start("webserver", username.c_str(), &localConversation,
                           &localAuthHandle);
    if (retval != PAM_SUCCESS)
    {
        return retval;
    }

    retval = pam_authenticate(localAuthHandle,
                              PAM_SILENT | PAM_DISALLOW_NULL_AUTHTOK);
    if (retval != PAM_SUCCESS)
    {
        pam_end(localAuthHandle, PAM_SUCCESS); // ignore retval
        return retval;
    }

    /* check that the account is healthy */
    retval = pam_acct_mgmt(localAuthHandle, PAM_DISALLOW_NULL_AUTHTOK);
    if (retval != PAM_SUCCESS)
    {
        pam_end(localAuthHandle, PAM_SUCCESS); // ignore retval
        return retval;
    }

    return pam_end(localAuthHandle, PAM_SUCCESS);
}

/*
static int pamUpdatePassword(const std::string& username,
                             const std::string& password)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    char* passStrNoConst = const_cast<char*>(password.c_str());
    const struct pam_conv localConversation = {pamFunctionConversation,
                                               passStrNoConst};
    pam_handle_t* localAuthHandle = nullptr; // this gets set by pam_start

    int retval = pam_start("webserver", username.c_str(), &localConversation,
                           &localAuthHandle);

    if (retval != PAM_SUCCESS)
    {
        return retval;
    }

    retval = pam_chauthtok(localAuthHandle, PAM_SILENT);
    if (retval != PAM_SUCCESS)
    {
        pam_end(localAuthHandle, PAM_SUCCESS);
        return retval;
    }

    return pam_end(localAuthHandle, PAM_SUCCESS);
}
*/

int main()
{
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
    conn->set_address("@bmcweb_auth");
    auto server = sdbusplus::asio::object_server(conn);
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
        server.add_interface("/xyz/openbmc_project/authentication",
                             "xyz.openbmc_project.Authentication");

    iface->register_method(
        "Authenticate",
        [](const std::string& user, const std::string& password) -> int32_t {
            return pamAuthenticateUser(user, password);
        });
    iface->initialize();
    conn->request_name("xyz.openbmc_project.Authentication");
    io.run();
}
