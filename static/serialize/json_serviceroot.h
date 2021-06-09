#include <static/dataModel/ServiceRoot_v1.h>

void jsonSerializeServiceroot(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    ServiceRootV1ServiceRoot* serviceRoot)
{
    // TODO the order is different between the struct and the old
    // implementation, I assume it matters
    // Resource_v1_Resource oem;
    asyncResp->res.jsonValue["@odata.id"] = serviceRoot->id;
    // std::string description;
    asyncResp->res.jsonValue["Name"] = serviceRoot->name;
    asyncResp->res.jsonValue["RedfishVersion"] = serviceRoot->redfishVersion;
    // std::string UUID;
    asyncResp->res.jsonValue["Links"]["Sessions"] = {
        serviceRoot->links.sessions.type, serviceRoot->links.sessions.uri};
    // asyncResp->res.jsonValue["Links"]["oem"] = links.oem
    asyncResp->res.jsonValue["AccountService"] = {
        serviceRoot->accountService.type, serviceRoot->accountService.uri};
    asyncResp->res.jsonValue["Chassis"] = {serviceRoot->chassis.type,
                                           serviceRoot->chassis.uri};
    asyncResp->res.jsonValue["JsonSchemas"] = {
        serviceRoot->jsonSchemas.type, serviceRoot->certificateService.uri};
    asyncResp->res.jsonValue["Managers"] = {serviceRoot->managers.type,
                                            serviceRoot->managers.uri};
    asyncResp->res.jsonValue["SessionService"] = {
        serviceRoot->sessionService.type, serviceRoot->sessionService.uri};
    asyncResp->res.jsonValue["Systems"] = {serviceRoot->systems.type,
                                           serviceRoot->systems.uri};
    asyncResp->res.jsonValue["Registries"] = {serviceRoot->registries.type,
                                              serviceRoot->registries.uri};
    asyncResp->res.jsonValue["UpdateService"] = {
        serviceRoot->updateService.type, serviceRoot->updateService.uri};
    asyncResp->res.jsonValue["UUID"] = serviceRoot->UUID;
    asyncResp->res.jsonValue["CertificateService"] = {
        serviceRoot->certificateService.type,
        serviceRoot->certificateService.uri};
    asyncResp->res.jsonValue["Tasks"] = {serviceRoot->tasks.type,
                                         serviceRoot->tasks.uri};
    asyncResp->res.jsonValue["EventService"] = {serviceRoot->eventService.type,
                                                serviceRoot->eventService.uri};
    asyncResp->res.jsonValue["TelemetryService"] = {
        serviceRoot->telemetryService.type, serviceRoot->telemetryService.uri};

    // fields not used
    // asyncResp->res.jsonValue["certificateServic"] = certificateService;
    // asyncResp->res.jsonValue["resourceBlock"] = resourceBlocks;
    // asyncResp->res.jsonValue["powerEquipmen"] = powerEquipment;
    // asyncResp->res.jsonValue["facilitie"] = facilities;
    // asyncResp->res.jsonValue["aggregationServic"] = aggregationService;
    // asyncResp->res.jsonValue["storag"] = storage;
    // asyncResp->res.jsonValue["nVMeDomain"] = nVMeDomains;
    return;
}
