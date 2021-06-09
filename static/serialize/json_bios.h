#include <static/dataModel/ServiceRoot_v1.h>

void jsonSerializeBios(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       BiosV1Bios* bios)
{
    asyncResp->res.jsonValue["@odata.id"] = bios->id;
    asyncResp->res.jsonValue["@odata.type"] = bios->type;
    asyncResp->res.jsonValue["Name"] = bios->name;
    asyncResp->res.jsonValue["description"] = bios->description;
    asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"] = {
        {bios->actions.resetBios.target}, {bios->actions.resetBios.uri}};

    return;
}
