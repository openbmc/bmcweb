

#include "logging.hpp"
#include "stacktrace.hpp"
#include "webserver_run.hpp"

#include <exception>

int main(int /*argc*/, char** /*argv*/)
{
    // std::set_terminate(print_stacktrace);
    return run();
}
