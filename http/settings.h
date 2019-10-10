#pragma once
// settings for crow
// TODO - replace with runtime config. libucl?

/* #define - specifies log level */
/*
    Debug       = 0
    Info        = 1
    Warning     = 2
    Error       = 3
    Critical    = 4

    default to INFO
*/
#define BMCWEB_LOG_LEVEL 1

#if defined(_MSC_VER)
#error "MSVC is not supported"
#endif
