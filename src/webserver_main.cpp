#include <bmcweb_config.h>
#include <systemd/sd-daemon.h>

#include <app_class_decl.hpp>
#include <boost/asio/io_context.hpp>
#include <cors_preflight.hpp>
#include <dbus_monitor.hpp>
#include <dbus_singleton.hpp>
#include <google/google_service_root.hpp>
#include <hostname_monitor.hpp>
#include <ibm/management_console_rest.hpp>
#include <image_upload.hpp>
#include <kvm_websocket.hpp>
#include <login_routes.hpp>
#include <obmc_console.hpp>
#include <redfish.hpp>
#include <redfish_v1.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <security_headers.hpp>
#include <ssl_key_handler.hpp>
#include <vm_websocket.hpp>
#include <webassets.hpp>

// Moved from webassets
namespace crow::webassets
{
void requestRoutes(App& app)
{
    constexpr static std::array<std::pair<const char*, const char*>, 17>
        contentTypes{
            {{".css", "text/css;charset=UTF-8"},
             {".html", "text/html;charset=UTF-8"},
             {".js", "application/javascript;charset=UTF-8"},
             {".png", "image/png;charset=UTF-8"},
             {".woff", "application/x-font-woff"},
             {".woff2", "application/x-font-woff2"},
             {".gif", "image/gif"},
             {".ico", "image/x-icon"},
             {".ttf", "application/x-font-ttf"},
             {".svg", "image/svg+xml"},
             {".eot", "application/vnd.ms-fontobject"},
             {".xml", "application/xml"},
             {".json", "application/json"},
             {".jpg", "image/jpeg"},
             {".jpeg", "image/jpeg"},
             // dev tools don't care about map type, setting to json causes
             // browser to show as text
             // https://stackoverflow.com/questions/19911929/what-mime-type-should-i-use-for-javascript-source-map-files
             {".map", "application/json"}}};

    std::filesystem::path rootpath{"/usr/share/www/"};

    std::error_code ec;

    std::filesystem::recursive_directory_iterator dirIter(rootpath, ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Unable to find or open " << rootpath
                         << " static file hosting disabled";
        return;
    }

    // In certain cases, we might have both a gzipped version of the file AND a
    // non-gzipped version.  To avoid duplicated routes, we need to make sure we
    // get the gzipped version first.  Because the gzipped path should be longer
    // than the non gzipped path, if we sort in descending order, we should be
    // guaranteed to get the gzip version first.
    std::vector<std::filesystem::directory_entry> paths(
        std::filesystem::begin(dirIter), std::filesystem::end(dirIter));
    std::sort(paths.rbegin(), paths.rend());

    for (const std::filesystem::directory_entry& dir : paths)
    {
        const std::filesystem::path& absolutePath = dir.path();
        std::filesystem::path relativePath{
            absolutePath.string().substr(rootpath.string().size() - 1)};
        if (std::filesystem::is_directory(dir))
        {
            // don't recurse into hidden directories or symlinks
            if (boost::starts_with(dir.path().filename().string(), ".") ||
                std::filesystem::is_symlink(dir))
            {
                dirIter.disable_recursion_pending();
            }
        }
        else if (std::filesystem::is_regular_file(dir))
        {
            std::string extension = relativePath.extension();
            std::filesystem::path webpath = relativePath;
            const char* contentEncoding = nullptr;

            if (extension == ".gz")
            {
                webpath = webpath.replace_extension("");
                // Use the non-gzip version for determining content type
                extension = webpath.extension().string();
                contentEncoding = "gzip";
            }

            if (boost::starts_with(webpath.filename().string(), "index."))
            {
                webpath = webpath.parent_path();
                if (webpath.string().size() == 0 ||
                    webpath.string().back() != '/')
                {
                    // insert the non-directory version of this path
                    webroutes::routes.insert(webpath);
                    webpath += "/";
                }
            }

            std::pair<boost::container::flat_set<std::string>::iterator, bool>
                inserted = webroutes::routes.insert(webpath);

            if (!inserted.second)
            {
                // Got a duplicated path.  This is expected in certain
                // situations
                BMCWEB_LOG_DEBUG << "Got duplicated path " << webpath;
                continue;
            }
            const char* contentType = nullptr;

            for (const std::pair<const char*, const char*>& ext : contentTypes)
            {
                if (ext.first == nullptr || ext.second == nullptr)
                {
                    continue;
                }
                if (extension == ext.first)
                {
                    contentType = ext.second;
                }
            }

            if (contentType == nullptr)
            {
                BMCWEB_LOG_ERROR << "Cannot determine content-type for "
                                 << absolutePath << " with extension "
                                 << extension;
            }

            if (webpath == "/")
            {
                forward_unauthorized::hasWebuiRoute = true;
            }

            app.routeDynamic(webpath)(
                [absolutePath, contentType, contentEncoding](
                    const crow::Request&,
                    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                    if (contentType != nullptr)
                    {
                        asyncResp->res.addHeader("Content-Type", contentType);
                    }

                    if (contentEncoding != nullptr)
                    {
                        asyncResp->res.addHeader("Content-Encoding",
                                                 contentEncoding);
                    }

                    // res.set_header("Cache-Control", "public, max-age=86400");
                    std::ifstream inf(absolutePath);
                    if (!inf)
                    {
                        BMCWEB_LOG_DEBUG << "failed to read file";
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
                        return;
                    }

                    asyncResp->res.body() = {
                        std::istreambuf_iterator<char>(inf),
                        std::istreambuf_iterator<char>()};
                });
        }
    }
}

}

#include <memory>
#include <string>

#ifdef BMCWEB_ENABLE_VM_NBDPROXY
#include <nbd_proxy.hpp>
#endif

constexpr int defaultPort = 18080;

inline void setupSocket(crow::App& app)
{
    int listenFd = sd_listen_fds(0);
    if (1 == listenFd)
    {
        BMCWEB_LOG_INFO << "attempting systemd socket activation";
        if (sd_is_socket_inet(SD_LISTEN_FDS_START, AF_UNSPEC, SOCK_STREAM, 1,
                              0))
        {
            BMCWEB_LOG_INFO << "Starting webserver on socket handle "
                            << SD_LISTEN_FDS_START;
            app.socket(SD_LISTEN_FDS_START);
        }
        else
        {
            BMCWEB_LOG_INFO
                << "bad incoming socket, starting webserver on port "
                << defaultPort;
            app.port(defaultPort);
        }
    }
    else
    {
        BMCWEB_LOG_INFO << "Starting webserver on port " << defaultPort;
        app.port(defaultPort);
    }
}

int main(int /*argc*/, char** /*argv*/)
{
    crow::Logger::setLogLevel(crow::LogLevel::Debug);

    auto io = std::make_shared<boost::asio::io_context>();
    App app(io);

    crow::connections::systemBus =
        std::make_shared<sdbusplus::asio::connection>(*io);

    // Static assets need to be initialized before Authorization, because auth
    // needs to build the whitelist from the static routes

#ifdef BMCWEB_ENABLE_STATIC_HOSTING
    crow::webassets::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_KVM
    crow::obmc_kvm::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_REDFISH
    redfish::requestRoutes(app);
    redfish::RedfishService redfish(app);

    // Create EventServiceManager instance and initialize Config
    redfish::EventServiceManager::getInstance();
#endif

#ifdef BMCWEB_ENABLE_DBUS_REST
    crow::dbus_monitor::requestRoutes(app);
    crow::image_upload::requestRoutes(app);
    crow::openbmc_mapper::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET
    crow::obmc_console::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_VM_WEBSOCKET
    crow::obmc_vm::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
    crow::ibm_mc::requestRoutes(app);
    crow::ibm_mc_lock::Lock::getInstance();
#endif

#ifdef BMCWEB_ENABLE_GOOGLE_API
    crow::google_api::requestRoutes(app);
#endif

    if (bmcwebInsecureDisableXssPrevention)
    {
        cors_preflight::requestRoutes(app);
    }

    crow::login_routes::requestRoutes(app);

    setupSocket(app);

#ifdef BMCWEB_ENABLE_VM_NBDPROXY
    crow::nbd_proxy::requestRoutes(app);
#endif

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    int rc = redfish::EventServiceManager::startEventLogMonitor(*io);
    if (rc)
    {
        BMCWEB_LOG_ERROR << "Redfish event handler setup failed...";
        return rc;
    }
#endif

#ifdef BMCWEB_ENABLE_SSL
    BMCWEB_LOG_INFO << "Start Hostname Monitor Service...";
    crow::hostname_monitor::registerHostnameSignal();
#endif

    app.run();
    io->run();

    crow::connections::systemBus.reset();
    return 0;
}
