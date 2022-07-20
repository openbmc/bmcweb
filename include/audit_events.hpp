#pragma once
#include <libaudit.h>

namespace audit
{

void audit_acct_event(int type, const char* username, uid_t uid,
                      const char* remote_host_name,
                      const char* remote_ip_address, const char* tty,
                      bool success)
{
    int auditfd, result;
    const char* op = NULL;

    auditfd = audit_open();
    if (auditfd < 0)
    {
        BMCWEB_LOG_ERROR << "Error opening audit socket : " << strerror(errno);
        return;
    }
    if (type == AUDIT_USER_LOGIN)
        op = "login";
    else if (type == AUDIT_USER_LOGOUT)
        op = "logout";
    result = success == true ? 1 : 0;

    if (audit_log_acct_message(auditfd, type, NULL, op, username, uid,
                               remote_host_name, remote_ip_address, tty,
                               result) <= 0)
        BMCWEB_LOG_ERROR << "Error writing audit message: " << strerror(errno);

    close(auditfd);
}

} // namespace audit
