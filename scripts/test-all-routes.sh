#!/usr/bin/env bash

if [[ -z $1 ]]; then
    echo "ERR specify filename to store response"
else

    file=$1

    echo "GET /redfish/v1/Systems" > $file
    echo "" >> $file
    curl -k 'https://127.0.0.1:4343/redfish/v1/Systems' \
        -H 'X-Auth-Token: '"$token"'' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "GET /redfish/v1/Systems/system" >> $file
    echo "" >> $file
    curl -k 'https://127.0.0.1:4343/redfish/v1/Systems/system' \
        -H 'X-Auth-Token: '"$token"'' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "GET /redfish/v1/Systems/system0" >> $file
    echo "" >> $file
    curl -k 'https://127.0.0.1:4343/redfish/v1/Systems/system0' \
        -H 'X-Auth-Token: '"$token"'' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "GET /redfish/v1/Systems/system1" >> $file
    echo "" >> $file
    curl -k 'https://127.0.0.1:4343/redfish/v1/Systems/system1' \
        -H 'X-Auth-Token: '"$token"'' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "GET /redfish/v/fileSystems/sysem0" >> $1
    echo "" >> $file
    curl -k 'https://127.0.0.1:4343/redfish/v1/Systems/sysem0' \
        -H 'X-Auth-Token: '"$token"'' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "GET /redfish/v1/Systems/foobar" >> $file
    echo "" >> $file
    echo "" >> $file
    curl -k 'https://127.0.0.1:4343/redfish/v1/Systems/foobar' \
        -H 'X-Auth-Token: '"$token"'' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "GET /redfish/v1/Systems/system/ResetActionInfo" >> $file
    echo "" >> $file
    curl -k 'https://127.0.0.1:4343/redfish/v1/Systems/system/ResetActionInfo' \
        -H 'X-Auth-Token: '"$token"'' >> $file
    echo "#####################################################################" >> $file

    echo "GET /redfish/v1/Systems/system0/ResetActionInfo" >> $file
    echo "" >> $file
    curl -k 'https://127.0.0.1:4343/redfish/v1/Systems/system0/ResetActionInfo' \
        -H 'X-Auth-Token: '"$token"'' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "GET /redfish/v1/Systems/system1/ResetActionInfo" >> $file
    echo "" >> $file
    curl -k 'https://127.0.0.1:4343/redfish/v1/Systems/system1/ResetActionInfo' \
        -H 'X-Auth-Token: '"$token"'' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "GET /redfish/v1/Systems/sysem0/ResetActionInfo" >> $file
    echo "" >> $file
    curl -k 'https://127.0.0.1:4343/redfish/v1/Systems/sysem0/ResetActionInfo' \
        -H 'X-Auth-Token: '"$token"'' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "GET /redfish/v1/Systems/foobar/ResetActionInfo" >> $file
    echo "" >> $file
    curl -k 'https://127.0.0.1:4343/redfish/v1/Systems/foobar/ResetActionInfo' \
        -H 'X-Auth-Token: '"$token"'' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "POST /redfish/v1/Systems/system/Actions/ComputerSystem.Reset" >> $file
    echo "" >> $file
    curl -k -X POST 'https://127.0.0.1:4343/redfish/v1/Systems/system/Actions/ComputerSystem.Reset' \
    -H "Content-Type: application/json" -H 'X-Auth-Token: '"$token"'' \
    -d '{ "ResetType": "GracefulRestart" }' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "POST /redfish/v1/Systems/system0/Actions/ComputerSystem.Reset" >> $file
    echo "" >> $file
    curl -k -X POST 'https://127.0.0.1:4343/redfish/v1/Systems/system0/Actions/ComputerSystem.Reset' \
    -H "Content-Type: application/json" -H 'X-Auth-Token: '"$token"'' \
    -d '{ "ResetType": "GracefulRestart" }' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "POST /redfish/v1/Systems/system1/Actions/ComputerSystem.Reset" >> $file
    echo "" >> $file
    curl -k -X POST 'https://127.0.0.1:4343/redfish/v1/Systems/system1/Actions/ComputerSystem.Reset' \
    -H "Content-Type: application/json" -H 'X-Auth-Token: '"$token"'' \
    -d '{ "ResetType": "GracefulRestart" }' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file

    echo "POST /redfish/v1/Systems/sysem0/Actions/ComputerSystem.Reset" >> $file
    echo "" >> $file
    curl -k -X POST 'https://127.0.0.1:4343/redfish/v1/Systems/sysem0/Actions/ComputerSystem.Reset' \
    -H "Content-Type: application/json" -H 'X-Auth-Token: '"$token"'' \
    -d '{ "ResetType": "GracefulRestart" }' >> $file
    echo "" >> $file
    echo "#####################################################################" >> $file

    echo "" >> $file
    echo "POST /redfish/v1/Systems/foobar/Actions/ComputerSystem.Reset" >> $file
    echo "" >> $file

    curl -k -X POST 'https://127.0.0.1:4343/redfish/v1/Systems/foobar/Actions/ComputerSystem.Reset' \
    -H "Content-Type: application/json" -H 'X-Auth-Token: '"$token"'' \
    -d '{ "ResetType": "GracefulRestart" }' >> $file

    echo "" >> $file
    echo "#####################################################################" >> $file
    echo "" >> $file
fi
