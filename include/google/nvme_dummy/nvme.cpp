#include <fmt/format.h>

#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/server/manager.hpp>

#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

using dbus_interface = std::shared_ptr<sdbusplus::asio::dbus_interface>;

void addNVMe(sdbusplus::asio::object_server& nvmeServer, uint8_t index,
             std::vector<dbus_interface>& nvmes)
{
    std::string storagePath =
        fmt::format("/xyz/openbmc_project/inventory/test_storage_{}", index);
    std::string nvmePath =
        fmt::format("/xyz/openbmc_project/inventory/nvme_{}", index);

    std::vector<std::array<int, 2>> fds;

    for (uint8_t i = 0; i < 4; ++i)
    {
        std::string path = fmt::format(
            "/xyz/openbmc_project/inventory/nvme_{}/controller_{}", index, i);
        auto nvme = nvmeServer.add_interface(
            path, "xyz.openbmc_project.Nvme.NVMeAdmin");

        nvme->register_method("GetLogPage", [&fds](const std::string& lid,
                                                   const std::string& lsp,
                                                   const std::string& lsi) {
            // Test out to see if fd gets close.

            // Maybe dbus / kernel closes it showhow.

            // Test if file can be opened by multiple process.

            // check what happens if the client open it late.

            // The fd will not have anything to begin with. Need to write later.
            // Need to pipe to read/write aync
            fds.emplace_back(std::array<int, 2>());
            auto& fd = fds.back();
            int res = pipe(fd.data());
            if (res)
            {
                throw std::runtime_error("Failed to open pipe");
            }

            std::thread{[wfd{fd[1]}, lid, lsp, lsi]() {
                sleep(5);
                std::string bigString(500, '0');
                for (uint8_t i = 0; i < 30; ++i)
                {
                    std::string hello =
                        fmt::format("hello world\n {}, {}, {}-{}", lid, lsp,
                                    lsi, bigString);
                    sleep(2);
                    if (write(wfd, hello.data(), hello.size()))
                    {
                        throw std::runtime_error("Failed to write data");
                    }

                    std::string emptyString(50000, '1');
                    if (write(wfd, emptyString.data(), emptyString.size()) <= 0)
                    {
                        throw std::runtime_error("Failed to write data");
                    }
                }

                std::string finished = "finished get log";
                if (write(wfd, finished.data(), finished.size()) <= 0)
                {
                    throw std::runtime_error("Failed to write data");
                }
                fmt::print(stderr, "Finish sending LogPage\n");
                close(wfd);
            }}.detach();

            return sdbusplus::message::unix_fd{fd[0]};
        });

        /*
        busctl call com.google.gbmc.wltu.nvme
        /xyz/openbmc_project/inventory/nvme_0/controller_0 \
           xyz.openbmc_project.Nvme.NVMeAdmin Identify ss hello world
        */
        nvme->register_method("Identify", [&fds](const std::string& cns,
                                                 const std::string& nsid,
                                                 const std::string& cntid) {
            fds.emplace_back(std::array<int, 2>());
            auto& fd = fds.back();
            int res = pipe(fd.data());
            if (res)
            {
                throw std::runtime_error("Failed to open pipe");
            }

            std::thread{[wfd{fd[1]}, cns, nsid, cntid]() {
                sleep(5);
                std::string bigString(500, '0');
                for (uint8_t i = 0; i < 30; ++i)
                {
                    std::string hello =
                        fmt::format("yes, hello world\n {}, {}, {}-{}", cns,
                                    nsid, cntid, bigString);
                    sleep(2);
                    if (write(wfd, hello.data(), hello.size()) <= 0)
                    {
                        throw std::runtime_error("Failed to write data");
                    }

                    std::string emptyString(50000, '1');
                    if (write(wfd, emptyString.data(), emptyString.size()) <= 0)
                    {
                        throw std::runtime_error("Failed to write data");
                    }
                }

                std::string finished = "finished id";
                if (write(wfd, finished.data(), finished.size()) <= 0)
                {
                    throw std::runtime_error("Failed to write data");
                }
                fmt::print(stderr, "Finish sending Identity\n");
                close(wfd);
            }}.detach();

            return sdbusplus::message::unix_fd(fd[0]);
        });

        nvmes.emplace_back(std::move(nvme));

        auto association = nvmeServer.add_interface(
            path.data(), "xyz.openbmc_project.Association.Definitions");
        std::vector<std::tuple<std::string, std::string, std::string>>
            definitions = {{"nvme", "controllers", nvmePath}};
        association->register_property("Associations", definitions);
        nvmes.emplace_back(std::move(association));
    }

    auto storage = nvmeServer.add_interface(
        storagePath, "xyz.openbmc_project.Inventory.Item.Storage");
    nvmes.emplace_back(std::move(storage));
    auto nvme = nvmeServer.add_interface(
        nvmePath, "xyz.openbmc_project.Inventory.Item.Drive");
    nvme->register_property(
        "Protocol",
        std::string(
            "xyz.openbmc_project.Inventory.Item.Drive.DriveProtocol.NVMe"));

    auto association = nvmeServer.add_interface(
        nvmePath.data(), "xyz.openbmc_project.Association.Definitions");
    std::vector<std::tuple<std::string, std::string, std::string>> definitions =
        {{"storage", "nvme", storagePath}};
    association->register_property("Associations", definitions);
    nvmes.emplace_back(std::move(nvme));
    nvmes.emplace_back(std::move(association));
}

int main()
{
    fmt::print(stderr, "Starting NVMe Daemon\n");
    try
    {
        boost::asio::io_context io;
        std::shared_ptr<sdbusplus::asio::connection> conn =
            std::make_shared<sdbusplus::asio::connection>(io);

        sdbusplus::server::manager::manager objManager(*conn, "/");
        sdbusplus::server::manager::manager inventoryObjManager(
            *conn, "/xyz/openbmc_project/inventory");

        sdbusplus::asio::object_server nvmeServer =
            sdbusplus::asio::object_server(conn);
        std::vector<dbus_interface> nvmes;

        addNVMe(nvmeServer, 0, nvmes);
        addNVMe(nvmeServer, 1, nvmes);
        for (const auto& nvme : nvmes)
        {
            nvme->initialize();
        }

        conn->request_name("com.google.gbmc.wltu.nvme");

        io.run();
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        fmt::print(stderr, "Nvme caught a sdbusplus exception: {}",
                   e.description());
        return 1;
    }
    catch (const std::exception& e)
    {
        fmt::print(stderr, "Nvme caught a standard exception: {}", e.what());
        return 2;
    }

    return 0;
}
