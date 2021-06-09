#include <static/dataModel/ServiceRoot_v1.h>


void json_serialize_serviceroot(const std::shared_ptr<bmcweb::AsyncResp> asyncResp, ServiceRoot_v1_ServiceRoot* serviceRoot){
    //TODO the order is different between the struct and the old implementation, I assume it matters
    // Resource_v1_Resource oem;
    asyncResp->res.jsonValue["@odata.id"] = serviceRoot->id;
    //std::string description;
    asyncResp->res.jsonValue["Name"] = serviceRoot->name;
    asyncResp->res.jsonValue["RedfishVersion"] = serviceRoot->redfishVersion;
    //std::string UUID;
    asyncResp->res.jsonValue["Links"]["Sessions"] = serviceRoot->links.sessions;
    //asyncResp->res.jsonValue["Links"]["oem"] = links.oem
    asyncResp->res.jsonValue["AccountService"] = serviceRoot->accountService;
    asyncResp->res.jsonValue["Chassis"] = serviceRoot->chassis;
    asyncResp->res.jsonValue["JsonSchemas"] = serviceRoot->jsonSchemas;
    asyncResp->res.jsonValue["Managers"] = serviceRoot->managers;
    asyncResp->res.jsonValue["SessionService"] = serviceRoot->sessionService;
    asyncResp->res.jsonValue["Systems"] = serviceRoot->systems;
    asyncResp->res.jsonValue["Registries"] = serviceRoot->registries;
    asyncResp->res.jsonValue["UpdateService"] = serviceRoot->updateService;
    asyncResp->res.jsonValue["UUID"] = serviceRoot->UUID;
    asyncResp->res.jsonValue["CertificateService"] = serviceRoot->certificateService;
    asyncResp->res.jsonValue["Tasks"] = serviceRoot->tasks;
    asyncResp->res.jsonValue["EventService"] = serviceRoot->eventService;
    asyncResp->res.jsonValue["TelemetryService"] = serviceRoot->telemetryService;

    //fields not used
    //asyncResp->res.jsonValue["certificateServic"] = certificateService;
    //asyncResp->res.jsonValue["resourceBlock"] = resourceBlocks;
    //asyncResp->res.jsonValue["powerEquipmen"] = powerEquipment;
    //asyncResp->res.jsonValue["facilitie"] = facilities;
    //asyncResp->res.jsonValue["aggregationServic"] = aggregationService;
    //asyncResp->res.jsonValue["storag"] = storage;
    //asyncResp->res.jsonValue["nVMeDomain"] = nVMeDomains;
    return ;
}

