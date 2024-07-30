# Generating Json schema for OEM schemas

The config in this directory is for utilizing the DMTF json generation script
located at [1]

To update the config files, run the following command, updating the paths to be
correct on your machine.

```bash
python3 ~/Redfish-Tools/csdl-to-json-convertor/csdl-to-json.py --input ~/bmcweb/redfish-core/schema/oem/openbmc/csdl --output ~/bmcweb/redfish-core/schema/oem/openbmc/json-schema --config ~/bmcweb/scripts/csdl-to-json-converter/csdl-to-json-convertor/openbmc-config.json
```

[1]
https://github.com/DMTF/Redfish-Tools/blob/main/csdl-to-json-convertor/csdl-to-json.py
