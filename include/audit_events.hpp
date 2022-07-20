#pragma once
#include <libaudit.h>

namespace audit
{

void auditAcctEvent(int type, const char* userName, uid_t uid,
                    const char* remoteHostName, const char* remoteIPAddress,
                    const char* tty, bool success)
{
#ifdef BMCWEB_ENABLE_LINUX_AUDIT_EVENTS
    int auditfd;
    int result;
    const char* op = NULL;

    auditfd = audit_open();
    if (auditfd < 0)
    {
        BMCWEB_LOG_ERROR("Error opening audit socket : {}", strerror(errno));
        return;
    }
    if (type == AUDIT_USER_LOGIN)
    {
        op = "login";
    }
    else if (type == AUDIT_USER_LOGOUT)
    {
        op = "logout";
    }
    result = success == true ? 1 : 0;

    if (audit_log_acct_message(auditfd, type, NULL, op, userName, uid,
                               remoteHostName, remoteIPAddress, tty,
                               result) <= 0)
    {
        BMCWEB_LOG_ERROR("Error writing audit message: {}", strerror(errno));
    }
    close(auditfd);
#endif
}

} // namespace audit
