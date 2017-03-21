generators_list = [
    [0x20, "BMC"],
    [0x2C, "ME"],
    [0x01, 0x1F, 'BIOS'],
    [0x21, 0x3F, 'SMI Handler'],
    [0x41, 0x5F, 'System Management Software'],
    [0x61, 0x7F, 'OEM'],
    [0x81, 0x8D, 'Remote Console Software 1-7'],
    [0x8f, 0x8f, 'Terminal Mode Remote Console software'],
];

sensor_type_list =
[
    [0x00, "Reserved"],
    [0x01, "Temperature"],
    [0x02, "Voltage"],
    [0x03, "Current"],
    [0x04, "Fan"],
    [0x05, "Physical Security (Chassis Intrusion)"],
    [0x06, "Platform Security Violation Attempt"],
    [0x07, "Processor"],
    [0x08, "Power Supply"], 
    [0x09, "Power Unit"],
    [0x0A, "Cooling Device"],
    [0x0B, "Other Units-based Sensor"],
    [0x0C, "Memory"],
    [0x0D, "Bay"],
    [0x0E, "POST Memory Resize"],
    [0x0F, "POST Error"],
    [0x10, "Event Logging Disabled"],
    [0x11, "Watchdog 1"],
    [0x12, "System Event"],
    [0x13, "Critical Interrupt"],
    [0x14, "Button / Switch"],
    [0x15, "Module / Board"],
    [0x16, "Microcontroller / Coprocessor"],
    [0x17, "Add-in Card"],
    [0x18, "Chassis"],
    [0x19, "Chip Set"],
    [0x1A, "Other FRU"],
    [0x1B, "Cable / Interconnect"],
    [0x1C, "Terminator"],
    [0x1D, "System Boot / Restart Initiated"],
    [0x1E, "Boot Error"],
    [0x1F, "Base OS Boot / Installation Status"],
    [0x20, "OS Stop / Shutdown"],
    [0x21, "Slot / Connector"],
    [0x22, "System ACPI Power State"],
    [0x23, "Watchdog 2"],
    [0x24, "Platform Alert"],
    [0x25, "Entity Presence"],
    [0x26, "Monitor ASIC / IC"],
    [0x27, "LAN"],
    [0x28, "Management Subsystem Health"],
    [0x29, "Battery"],
    [0x2A, "Session Audit"],
    [0x2B, "Version Change"],
    [0x2C, "FRU State"],
    [0xC0, 0xFF, "OEM RESERVED"],
    [0x2d, 0xBF, "Reserved"]
];

event_reading_type_list = [
    [0x01, "Threshold"],
    [0x02, 0x0C, "Generic"],
    [0x6F, "Sensor Specific"],
    [0x70, 0x7F, "OEM"],
];

memory_ras_mode = {
    0x00: "No Mode",
    0x01: "Mirroring Mode",
    0x02: "Lockstep Mode",
    0x03: "Invalid RAS Mode",
    0x04: "Rank Sparing Mode"
};

caterr_error_type = {
    0x00: "Unknown",
    0x01: "CATERR",
    0x02: "CPU Core Error",
    0x03: "CPU Icc max Mismatch",
    0x04: "CATERR due to CPU 3-strike timeout",
};

config_error_values = {
    0x00: " - CFG syntax error",
    0x01: " - Chassis auto-detect error",
    0x02: " - SDR/CFG file mismatch",
    0x03: " - SDR or CFG file corrupted",
    0x04: " - SDR syntax error"
};

ras_select_string = " Prior RAS: %s Seleted RAS: %s";


function decode_bios_xeon_dimm_slot_event(generator_id, sensor_type, event_data_offset, event_data){
    var return_string = "";

    var entity_id = 0x20;  // TODO find a way to get this.  
    // This was previously identified by sensor numbers hardcoded.
    // 0x20 = Mem Conf Sensor;  0x2B = Rdnc Mod Sensor

    //if bios generated event and sensor_type is memory
    if (generator_id == 0x01){
        if ((entity_id == 0x020) || (entity_id == 0x02B)){  
            // if sensor is mem conf sensor or rdnc mod sensor
            if ((event_data[0] & 0xA0) == 0xA0){
                var offset = event_data[0] & 0xF
                var prior;
                var selected;
                if (offset <= 1){
                    if (entity_id == 0x02B){
                        offset = sel_data[1] & 0xF;
                        if (offset in memory_ras_mode){
                            prior = memory_ras_mode[offset];
                        }
                    }
                    
                    offset = sel_data[2] & 0xF;
                    if (offset in memory_ras_mode){
                        selected = memory_ras_mode[offset];
                    }
                    if (prior == null){
                        return_string += selected;                        
                    } else {
                        return_string += ras_select_string.format(prior, selected);
                    }
                }
            }
        }
    } 
};


function cpu_caterr_ext_string(generator_id, sensor_type, event_data_offset, event_data){
    var return_string = "";
    if (event_data[0] == 0xA1)
    {

        return_string += "- CATERR type: ";
        return_string += caterr_error_type[event_data[2]] || "Reserved";

        return_string += ", CPU bitmap that causes the system CATERR:";
        for (var cpu_number = 1; cpu_number <=4; cpu_number++){
            if ((1 << cpu_number) & event_data[2] > 0){
                return_string += " CPU" + cpu_number;
            }
        }
    }
    return return_string;
};

function config_error_ext_string(generator_id, sensor_type, event_data_offset, event_data){
    var return_string = "";
    
    if (event_data[0] == 0x21)
    {
        return_string += config_error_values[event_data[1]] || " Reserved";
    }
    return return_string;
};

function mtm_level_change_ext_string(generator_id, sensor_type, event_data_offset, event_data){
    var return_string = "";
    
    if (event_data[0] == 0x81)
    {
        if (event_data[1] <= 2 && event_data[1] >= 1){
            return_string += "Level " + event_data[1];
        } else {
            return_string += "Reserved";
        }
    }
    return return_string;
};

function vrd_hot_ext_string(generator_id, sensor_type, event_data_offset, event_data){
    var return_string = "";
    
    if (event_data_offset == 0x01)
    {
        if (event_data[1] > 0){

            return_string += " - Processor VRD HOT map:";
            for (var cpu_number = 1; cpu_number <=4; cpu_number++){
                if ((1 << cpu_number) & event_data[1] > 0){
                    return_string += " CPU" + cpu_number;
                }
            }
        }
        if (event_data[2] > 0){
            return_string += ", Memory VRD HOT map:";
            for (var cpu_dimm_number = 1; cpu_dimm_number <=4; cpu_dimm_number++){
                if ((1 << cpu_dimm_number) & event_data[2] > 0){
                    return_string += " CPU" + (cpu_dimm_number-1) >> 1;

                    if ((cpu_dimm_number+1) % 2 == 1){
                        return_string += ", DIMM Channel 1/2";
                    } else {
                        return_string += ", DIMM Channel 3/4";
                    }
                }
            }
        }
    }
    return return_string;
};

function decode_drive_or_dimm_temperature_sel(generator_id, sensor_type, event_data_offset, event_data){
    var return_string = ""
    if (generator_id == 0x20){  // if generated from the BMC
        if (((event_data[0] & 0xF0) == 0xA0) && (event_data[1] == 0x01)){
            // drive temperature sel
            // Sensor Type 0x01 (Temperature)
            // Event Type 0x01 (Threshold)
            // OEM Data 0 [7:4] == 0b1010 (2 bytes OEM data, extended format)
            // OEM Data 1 (SSD Drive format)

            // Get the standard string, since this is technically a standard
            // event. Then, add the OEM data.

            for (var drive_number = 0; drive_number < 8; drive_number++) {
                if (( 1 << drive_number) & event_data[1]) {
                    return_string += " :Drive " + drive_number;
                }
            }
        } else if ((event_data[0] & 0xF0) == 0x30) {
            // Dimm aggregate thermal margin
            //TODO fix this
            for (var cpu_number = 0; cpu_number < 8; cpu_number++) {
                if ((1 <<cpu_number) & event_data[2]){
                    cpu_number += 1;
                    break;
                }
            }

            if (event_data[0]){
                for (var dimm_counter = 0; dimm_counter < 8; dimm_counter++) {
                    if ((1 << dimm_counter) & event_data[0]){
                        dimm_number = dimm_counter % 4;
                        dimm_channel = dimm_counter >> 2;
                        break;
                    }
                }
            }
        }
    }
}

/* This dict defines the states for the SEL reader.  The first level of keys
is used to map the event type to a section.  The rest of the sections are used
to map the individual event data to an entry.
if the key "sensor_type" is present, it will use that entry to map a specific sensor
type to a string and a severity.  the lowest level of dictionaries maps the offset
to a user facing string.

a custom_handler  can be added to the dict and invoked at the lowest level to
manage non-standard sel to string implementationations or ones that require code 
to determine.
*/
event_reading_type_states = {
    0x00: "Unspecified",
    0x01: {  // Threshold
        0x00: ["Lower Non-critical - going low", "Degraded"],
        0x01: ["Lower Non-critical - going high", "Degraded"],
        0x02: ["Lower Critical - going low", "Non-Fatal"],
        0x03: ["Lower Critical - going high", "Non-Fatal"],
        0x04: ["Lower Non-recoverable - going low", "Fatal"],
        0x05: ["Lower Non-recoverable - going high", "Fatal"],
        0x06: ["Upper Non-critical - going low", "Degraded"],
        0x07: ["Upper Non-critical - going high", "Degraded"],
        0x08: ["Upper Critical - going low", "Non-Fatal"],
        0x09: ["Upper Critical - going high", "Non-Fatal"],
        0x0A: ["Upper Non-recoverable - going low", "Fatal"],
        0x0B: ["Upper Non-recoverable - going high", "Fatal"],
        "sensor_type": {
            0x01: decode_drive_or_dimm_temperature_sel // hard drive
        }
    },
    0x02: { //Discrete
        0x00: "Transition to Idle",
        0x01: "Transition to Active",
        0x02: "Transition to Busy"
    },
    0x03: {
        0x00: "State Deasserted",
        0x01: "State Asserted",
        "sensor_type": {
            0x07: cpu_caterr_ext_string,
            0x28: [mtm_level_change_ext_string, config_error_ext_string],
            
        }
    },
    0x04: {
        0x00: "Predictive Failure deasserted",
        0x01: ["Predictive Failure asserted", "Degraded"]
    },
    0x05: {
        0x00: "Limit Not Exceeded",
        0x01: ["Limit Exceeded", "Degraded"],
        "sensor_type": {
            0x01: vrd_hot_ext_string
        }
    },
    0x06: {
        0x00: "Performance Met",
        0x01: ["Performance Lags", "Degraded"]
    },
    0x07: {
        0x00: "Transition to OK",
        0x01: ["Transition to Non-Critical from OK", "Degraded"],
        0x02: ["Transition to Critical from less severe", "Non-Fatal"],
        0x03: ["Transition to Non-recoverable from less severe", "Fatal"],
        0x04: ["Transition to Non-Critical from more severe", "Degraded"],
        0x05: ["Transition to Critical from Non-recoverable", "Non-Fatal"], 
        0x06: ["Transition to Non-recoverable", "Fatal"],
        0x07: ["Monitor", "Degraded"],
        0x08: "Informational"
    },
    0x08: {
        0x00: "Device Removed / Device Absent",
        0x01: "Device Inserted / Device Present"
    },
    0x09: {
        0x00: "Device Disabled",
        0x01: "Device Enabled",
        "sensor_type": {
            0x0C: decode_bios_xeon_dimm_slot_event
        }
    },
    0x0A: {
        0x00: "Transition to Running",
        0x01: "Transition to In Test",
        0x02: "Transition to Power Off",
        0x03: "Transition to On Line",
        0x04: "Transition to Off Line",
        0x05: "Transition to Off Duty",
        0x06: "Transition to Degraded",
        0x07: "Transition to Power Save",
        0x08: ["Install Error", "Degraded"]
    },
    0x0B: {
        0x00: "Redundancy Regained",
        0x01: ["Redundancy Lost", "Degraded"],
        0x02: ["Redundancy Degraded", "Degraded"],
        0x03: "Non-redundant:Sufficient Resources from Redundant",
        0x04: "Non-redundant:Sufficient Resources from Insufficient Resources",
        0x05: ["Non-redundant:Insufficient Resources", "Degraded"],
        0x06: ["Redundancy Degraded from Fully Redundant", "Degraded"],
        0x07: ["Redundancy Degraded from Non-redundant", "Degraded"]
    },
    0x0C: {
        0x00: "D0 Power State",
        0x01: "D1 Power State",
        0x02: "D2 Power State",
        0x03: "D3 Power State"
    },
    0x6F: {  // OEM
        "sensor_type": {
            0x00: "",  // reserved
            0x01: "",  // Temperature
            0x02: "",  // Voltage
            0x03: "",  // Current
            0x04: "N/A",  //Fan
            0x05: {  // General Chassis Intrusion
                0x00: ["General Chassis Intrusion", "Degraded"],
                0x01: ["Drive Bay intrusion", "Degraded"],
                0x02: ["I/O Card area intrusion", "Degraded"],
                0x03: ["Processor area intrusion", "Degraded"],
                0x04: ["LAN Leash Lost", "Degraded"],
                0x05: ["Unauthorized dock", "Degraded"],
                0x06: ["FAN area intrusion", "Degraded"],
            },
            0x06: {  // Physical Security - Violation Attempt
                0x00: ["Secure Mode Violation attempt", "Degraded"],
                0x01: ["Pre-boot Password Violation - user password", "Degraded"],
                0x02: ["Pre-boot Password Violation attempt - setup password", "Degraded"],
                0x03: ["Pre-boot Password Violation - network boot password", "Degraded"],
                0x04: ["Other pre-boot Password Violation", "Degraded"],
                0x05: ["Out-of-band Access Password Violation", "Degraded"]
            },
            0x07: { // Processor
                0x00: ["IERR", "Degraded"],
                0x01: ["Thermal Trip", "Degraded"],
                0x02: ["FRB1/BIST failure", "Degraded"],
                0x03: ["FRB2/Hang in POST failure", "Degraded"],
                0x04: ["FRB3/Processor Startup/Initialization failure", "Degraded"],
                0x05: ["Configuration Error", "Degraded"],
                0x06: ["SM BIOS Uncorrectable CPU-complex Error", "Degraded"],
                0x07: ["Processor Presence detected", "Degraded"],
                0x08: ["Processor disabled", "Degraded"],
                0x09: ["Terminator Presence Detected", "Degraded"],
                0x0A: ["Processor Automatically Throttled", "Degraded"],
                0x0B: ["Machine Check Exception (Uncorrectable)", "Fatal"],
                0x0C: ["Correctable Machine Check Error", "Non-Fatal"]
            },
            0x08: {  // Power Supply
                0x00: "Presence detected",
                0x01: ["Power Supply Failure detected", "Degraded"],
                0x02: ["Predictive Failure", "Degraded"],
                0x03: ["Power Supply input lost (AC/DC)", "Degraded"],
                0x04: ["Power Supply input lost or out-of-range", "Degraded"],
                0x05: ["Power Supply input out-of-range, but present", "Degraded"],
                0x06: ["Configuration error", "Degraded"],
                0x07: ["Power Supply Inactive (in standby state)", "Degraded"]
            },  
            0x09: {  // Power unit
                0x00: "Power Off / Power Down",
                0x01: "Power Cycle",
                0x02: "240VA Power Down",
                0x03: "Interlock Power Down",
                0x04: ["AC lost / Power input lost", "Degraded"],
                0x05: ["Soft Power Control Failure", "Degraded"],
                0x06: ["Power Unit Failure detected", "Degraded"],
                0x07: ["Predictive Failure", "Degraded"]
            },
            0x0C: { // Memory
                0x00: ["Correctable ECC / other correctable memory error", "Degraded"],
                0x01: ["Uncorrectable ECC", "Fatal"],
                0x02: "Parity",
                0x03: ["Memory Scrub Failed", "Fatal"],
                0x04: "Memory Device Disabled",
                0x05: ["Correctable ECC / other correctable memory error logging limit reached", "Degraded"],
                0x06: "Presence detected",
                0x07: ["Configuration error", "Fatal"],
                0x08: "Spare",
                0x09: "Memory Automatically Throttled",
                0x0A: ["Critical Overtemperature", "Fatal"]
            },       
            0x0D: { // Drive Slot
                0x00: "Drive Presence",
                0x01: ["Drive Fault", "Fatal"],
                0x02: ["Predictive Failure", "Fatal"],
                0x03: "Hot Spare",
                0x04: "Consistency Check / Parity Check in progress",
                0x05: ["In Critical Array", "Degraded"],
                0x06: ["In Failed Array", "Degraded"],
                0x07: ["Rebuild/Remap in progress", "Degraded"],
                0x08: ["Rebuild/Remap Aborted", "Degraded"]
            },
            0x0F: { // POST Error
                0x00: "POST Error",
                0x01: "System Firmware Hang",
                0x02: "System Firmware Progress"
            },
            0x10: { // Event Logging Disabled
                0x00: "Correctable Memory Error Logging Disabled",
                0x01: "Event Type Logging Disabled",
                0x02: "All Event Logging Disabled",
                0x03: "All Event Logging Disabled",
                0x04: ["SEL Full", "Degraded"],
                0x05: ["SEL Almost Full", "Degraded"],
                0x06: "Correctable Machine Check Error Logging Disabled"
            },
            0x11: { // Watchdog 1
                0x0: ["BIOS Watchdog Reset", "Fatal"],
                0x1: ["OS Watchdog Reset", "Fatal"],
                0x2: ["OS Watchdog Shut Down", "Fatal"],
                0x3: ["OS Watchdog Power Down", "Fatal"],
                0x4: ["OS Watchdog Power Cycle", "Fatal"],
                0x5: ["OS Watchdog NMI / Diagnostic Interrupt", "Fatal"],
                0x6: ["OS Watchdog Expired, status only", "Fatal"],
                0x7: ["OS Watchdog pre-timeout Interrupt, non-NMI", "Fatal"]
            },
            0x12: {  // System Event
                0x0: "System Reconfigured",
                0x1: "OEM System Boot Event",
                0x2: ["Undetermined system hardware failure", "Fatal"],
                0x3: ["Entry added to Auxiliary Log", "Degraded"],
                0x4: ["PEF Action", "Degraded"],
                0x5: "Timestamp Clock Synch"
            },
            0x13: {  // Critical Interrupt
                0x0: ["Front Panel NMI / Diagnostic Interrupt", "Fatal"],
                0x1: ["Bus Timeout", "Fatal"],
                0x2: ["I/O channel check NMI", "Fatal"],
                0x3: ["Software NMI", "Fatal"],
                0x4: ["PCI PERR", "Fatal"],
                0x5: ["PCI SERRt", "Fatal"],
                0x6: ["EISA Fail Safe Timeout", "Fatal"],
                0x7: ["Bus Correctable Error", "Fatal"],
                0x8: ["Bus Uncorrectable Error", "Fatal"],
                0x9: ["Fatal NMI (port 61h, bit 7)", "Fatal"],
                0xA: ["Bus Fatal Error", "Fatal"],
                0xB: ["Bus Degraded", "Fatal"]
            },
            0x14: {  // Button / Switch
                0x0: "Power Button pressed",
                0x1: "Sleep Button pressed",
                0x2: "Reset Button pressed",
                0x3: "FRU latch open",
                0x4: "FRU service request button"
            },
            0x19: {  // Chip Set
                0x0: ["Soft Power Control Failure", "Fatal"],
                0x1: ["Thermal Trip", "Fatal"]
            },
            0x1B: {  // Cable / Interconnect
                0x0: "Cable/Interconnect is connected",
                0x1: ["Configuration Error - Incorrect cable connected / Incorrect interconnection", "Fatal"],
            },
            0x1D: {  // System Boot / Restart Initiated
                0x0: ["Initiated by power up", "Fatal"],
                0x1: ["Initiated by hard reset", "Fatal"],
                0x2: ["Initiated by warm reset", "Fatal"],
                0x3: ["User requested PXE boot", "Fatal"],
                0x4: "Automatic boot to diagnostic",
                0x5: ["OS / run-time software initiated hard reset", "Fatal"],
                0x6: ["OS / run-time software initiated warm reset", "Fatal"],
                0x7: ["System Restart", "Fatal"],
            },
            0x1E: {  // Boot Error
                0x0: ["No bootable media", "Degraded"],
                0x1: ["Non-bootable diskette left in drive", "Degraded"],
                0x2: ["PXE Server not found", "Fatal"],
                0x3: ["Invalid boot sector", "Fatal"],
                0x4: ["Timeout waiting for user selection of boot source", "Fatal"],
            },
            0x1F: {  // Base OS Boot / Installation Status
                0x0: "A: boot completed",
                0x1: "C: boot completed",
                0x2: "PXE boot completed",
                0x3: "Diagnostic boot completed",
                0x4: "CD-ROM boot completed",
                0x5: "ROM boot completed",
                0x6: "boot completed - boot device not specified",
                0x7: "Base OS/Hypervisor Installation started",
                0x8: "Base OS/Hypervisor Installation completed",
                0x9: ["Base OS/Hypervisor Installation aborted", "Fatal"],
                0xA: ["Base OS/Hypervisor Installation failed", "Fatal"]
            },
            0x20: {  // OS Stop / Shutdown
                0x0: ["Critical stop during OS load / initialization", "Fatal"],
                0x1: ["Run-time Critical Stop", "Fatal"],
                0x2: "OS Graceful Stop",
                0x3: "OS Graceful Shutdown",
                0x4: "Soft Shutdown initiated by PEF",
                0x5: ["Agent Not Responding", "Fatal"]
            },
            0x21: {  // Slot / Connector
                0x0: ["Fault Status asserted", "Fatal"],
                0x1: "Identify Status asserted",
                0x2: "Slot / Connector Device installed/attached",
                0x3: "Slot / Connector Ready for Device Installation",
                0x4: "Slot/Connector Ready for Device Removal",
                0x5: "Slot Power is Off",
                0x6: "Slot / Connector Device Removal Request",
                0x7: ["Interlock asserted", "Degraded"],
                0x8: "Slot is Disabled",
                0x9: ["Slot holds spare device", "Fatal"]
            },
            0x22: {  // System ACPI Power State
                0x0: "S0 / G0 \"working\"",
                0x1: "S1 \"sleeping with system h/w & processor context maintained\"",
                0x2: "S2 \"sleeping, processor context lost\"",
                0x3: "S3 \"sleeping, processor & h/w context lost, memory retained.\"",
                0x4: "S4 \"non-volatile sleep / suspend-to disk\"",
                0x5: "S5 / G2 \"soft-off\"",
                0x6: "S4 / S5 soft-off, particular S4 / S5 state cannot be determined",
                0x7: "G3 / Mechanical Off",
                0x8: "Sleeping in an S1, S2, or S3 states",
                0x9: "G1 sleeping",
                0xA: "S5 entered by override",
                0xB: "Legacy ON state",
                0xC: "Legacy OFF state",
                0xE: ["Unknown", "Fatal"]
            },
            0x23: {  // Watchdog 2
                0x0: ["Timer expired, status only", "Non-Fatal"],
                0x1: ["Hard Reset", "Fatal"],
                0x2: ["Power Down", "Fatal"],
                0x3: ["Power Cycle", "Fatal"],
                0x8: ["Timer interrupt", "Fatal"]
            },
            0x24: {  // Platform Alert
                0x0: ["platform generated page", "Non-Fatal"],
                0x1: ["platform generated LAN alert", "Non-Fatal"],
                0x2: ["platform Event Trap generated", "Non-Fatal"],
                0x3: ["platform generated SNMP trap", "Non-Fatal"]
            },
            0x25: {  // Entity Presence
                0x0: "Entity Present",
                0x1: "Entity Absent",
                0x2: "Entity Disabled"
            },
            0x27: {  // LAN
                0x0: ["LAN Heartbeat Lost", "Fatal"],
                0x1: "LAN Heartbeat"
            },
            0x28: {  // Management Subsystem Health
                0x0: ["sensor access degraded or unavailable", "Fatal"],
                0x1: ["controller access degraded or unavailable", "Fatal"],
                0x2: ["management controller off-line", "Fatal"],
                0x3: ["management controller unavailable", "Fatal"],
                0x4: ["Sensor failure", "Fatal"],
                0x5: ["FRU failure", "Fatal"]
            },
            0x29: {  // Battery
                0x0: ["battery low (predictive failure)", "Fatal"],
                0x1: ["battery failed", "Fatal"],
                0x2: "battery presence detected"
            },
            0x2A: {  // Session Audit
                0x0: "Session Activated",
                0x1: "Session Deactivated",
                0x2: ["Invalid Username or Password", "Non-Fatal"],
                0x3: ["Invalid password disable", "Non-Fatal"],
            },
            0x2B: {  // Version Change
                0x0: "Hardware change detected with associated Entity",
                0x1: "Firmware or software change detected with associated Entity",
                0x6: "Hardware Change detected with associated Entity was successful",
                0x7: "Software or F/W Change detected with associated Entity was successful",
                0x2: ["Hardware incompatibility detected with associated Entity", "Fatal"],
                0x3: ["Firmware or software incompatibility detected with associated Entity", "Fatal"],
                0x4: ["Entity is of an invalid or unsupported hardware version", "Fatal"],
                0x5: ["Entity contains an invalid or unsupported firmware or software version", "Fatal"]
            },
            0x2C: {  // FRU State
                0x0: ["FRU Not Installed", "Fatal"],
                0x1: "FRU Inactive (in standby or \"hot spare\" state)",
                0x2: "FRU Activation Requested",
                0x3: "FRU Activation In Progress",
                0x4: "FRU Active",
                0x5: "FRU Deactivation Requested",
                0x6: "FRU Deactivation In Progress",
                0x7: ["FRU Communication Lost", "Fatal"]
            }
        }
    }
};


function getSelDescription(sel_data){
    var sel_description_string = "";
    var severity = "Healthy";
    try{
        var event_data = [sel_data[13], sel_data[14], sel_data[15]];
        var sensor_type = sel_data[10];

        var this_entry;

        var generator_id = sel_data[7];
        var event_reading_type = sel_data[12] & 0x7f;
        var event_asserted = (sel_data[12] >> 7) == 0;
        var event_reading_type_string = getStringFromIdList(event_reading_type_list, event_reading_type);

        if (event_reading_type in event_reading_type_states){
            this_entry = event_reading_type_states[event_reading_type];

            if ("sensor_type" in this_entry){
                if (sensor_type in this_entry["sensor_type"]){
                    this_entry = this_entry["sensor_type"][sensor_type]
                }
            }

            if (typeof this_entry == "string"){
                sel_description_string += this_entry;
            } else if (typeof this_entry == "object") {
                var event_data_offset = event_data[0] & 0xf;

                if (event_data_offset in this_entry){
                    var sensor_entity = this_entry[event_data_offset];
                    if (typeof sensor_entity == "object"){
                        // only apply the severity string if event was asserted
                        // all deassertion events are considered "healthy"
                        if (event_asserted) {
                            severity = sensor_entity[1];
                        }
                        sel_description_string += sensor_entity[0];
                    } else if (typeof sensor_entity == "string"){
                        sel_description_string += sensor_entity;
                    } else if (typeof sensor_entity == "function"){
                        sel_description_string += this_handler(generator_id, sensor_type, event_data_offset, event_data)
                    }
                }
            }
        }

        if (event_asserted){
            sel_description_string += " - Asserted";
        } else {
            sel_description_string += " - Deasserted";
        }
    } catch(err) {
        console.log("SEL decode died with error " + err)
    }
    return [sel_description_string, severity];
}

function parseHexString(str) { 
    var result = [];
    while (str.length >= 2) { 
        result.push(parseInt(str.substring(0, 2), 16));
        str = str.substring(2, str.length);
    }
    return result;
};

function getStringFromIdList(generator_id, generators_list){
    var generator_string = null;
    for (var generator_index = 0; generator_index < generators_list.length; generator_index++){
        this_generator_entry = generators_list[generator_index];
        var generator_id = generator_id & 0xFF;
        // range based match
        if (this_generator_entry.length === 3){
            if (generator_id >= this_generator_entry[0] && 
                generator_id <= this_generator_entry[1]){
                    generator_string = this_generator_entry[2];
                    break;
            }
        } else {
            // exact match
            if (generator_id == this_generator_entry[0]){
                generator_string = this_generator_entry[1];
                break
            }
        }
    }
    
    if (generator_string == null){
        generator_string = "(0X"
        generator_string += ("00" + this_entry.generator_id.toString(16)).substr(-2).toUpperCase();
        generator_string += ")"
    }
    return generator_string;
};

function get_sensor_names_from_sdr(sdr_list){
    var sdr_names = {};

    for (var sdr_index = 0; sdr_index < sdr_list.length; sdr_index++) {
        var this_sdr = parseHexString(sdr_list[sdr_index].raw_data);

        var sdr_type = this_sdr[3];
        var owner = this_sdr[5];
        var string_offset = null;
        if (sdr_type == 0x01){
            string_offset = 47;
        } else if (sdr_type == 0x02){
            string_offset = 31;
        } else if (sdr_type == 0x02){
            string_offset = 16;
        }
        if (string_offset !== null){
            var type_length_code = this_sdr[string_offset];
            var type_code = type_length_code >> 6;
            var length = type_length_code & 0x3F;
            var string_name = "";
            var sensor_number = 0;
            if (type_code == 0x03){
                sensor_number = this_sdr[7];
                for (var string_index = 0; string_index < length; string_index++){
                    var this_character = this_sdr[string_offset + 1 + string_index];
                    var character = String.fromCharCode(this_character);
                    string_name += character;
                }
            }
            sdr_names[(owner << 8) + sensor_number] = string_name;
        }
    
    }
    return sdr_names;
}


angular.module('bmcApp').controller('selController', ['$scope', function($scope) {

    var sdr_promise = Restangular.all('sdrentries').getList();
    var sel_promise = Restangular.all('selentries').getList();
    
    //wait for all the requests to finish
    Promise.all([sdr_promise, sel_promise])  
    .then(function(values) {
        

        sdr_entries_incoming = values[0];
        sel_entries_incoming = values[1];

        var sensor_names = get_sensor_names_from_sdr(sdr_entries_incoming);

        $scope.sel_entries.length = 0;
        // TODO don't generate so many extra entries
        for (var i = 0; i < 20; i++){
            for (var entry_index = 0; entry_index < sel_entries_incoming.length; entry_index++) {
                this_entry = new Object();
                raw_sel = parseHexString(sel_entries_incoming[entry_index]["raw_data"])
                
                this_entry.id = (raw_sel[1] << 8) + raw_sel[0];
                this_entry.generator_id = raw_sel[7];
                this_entry.channel_number = raw_sel[8] >> 4;
                this_entry.sensor_number = raw_sel[11];
                this_entry.lun = raw_sel[8] & 0x03;
                this_entry.timestamp = (raw_sel[6] << 24) + (raw_sel[5] << 16) + (raw_sel[4] << 8) + raw_sel[3];
                this_entry.sensor_type_id = raw_sel[10];

                this_entry.generator_name = getStringFromIdList(this_entry.generator_id, generators_list);
                this_entry.sensor_type_string = getStringFromIdList(this_entry.sensor_type_id, sensor_type_list);
                sel_description = getSelDescription(raw_sel);
                this_entry.description_string = sel_description[0];
                this_entry.event_severity = sel_description[1];

                var sensor_uid = (this_entry.generator_id << 8) + this_entry.sensor_number;
                if (sensor_uid in sensor_names){
                    this_entry.sensor_name = sensor_names[sensor_uid];
                } else {
                    this_entry.sensor_name = "Unknown";
                }

                $scope.sel_entries.push(this_entry);
            }
        }

        var unique_controllers = {};
        for( var sel_index in $scope.sel_entries ){
            unique_controllers[$scope.sel_entries[sel_index].generator_name] = 0;
        }
        var unique_controller_names = [];
        for(var key in unique_controllers){
            unique_controller_names.push(key);
        }

        $scope.unique_columns["controller"] = unique_controller_names;
    });


    $scope.filtered_sel = [];
    $scope.filtered_and_sliced_sel = [];
    $scope.sel_entries = [];
    $scope.current_page = 1;
    $scope.items_per_page = 10;
    $scope.max_size = 5;
    $scope.unique_columns = {"controller": []};
    $scope.selectedColumns = {"controller": []};

    onPageChange =  function() { 
        var begin = (($scope.current_page - 1) * $scope.items_per_page)
        , end = begin + $scope.items_per_page;
        $scope.filtered_and_sliced_sel = $scope.filtered_sel.slice(begin, end)
    };
    
    onFilterChange =  function() {           
        var filtered_sel = [];
        var sel_entries = $scope.sel_entries;
        if ($scope.selectedColumns["controller"].length == 0){
            filtered_sel = $scope.sel_entries;
        } else {
            for(var sel_entry_index = 0; sel_entry_index < sel_entries.length; sel_entry_index++){
                entry = sel_entries[sel_entry_index];
                for (var controller_index = 0; controller_index < $scope.selectedColumns["controller"].length; controller_index++){
                    controller = $scope.selectedColumns["controller"][controller_index];
                    if (angular.equals(entry.generator_name, controller)) {
                        filtered_sel.push(entry);
                        break;
                    }
                }
            }
        }

        $scope.filtered_sel = filtered_sel;
        onPageChange();
    };

    $scope.$watch('current_page + items_per_page ', onPageChange);
    $scope.$watch('selectedColumns', onFilterChange, true /*deep*/);
    $scope.$watchCollection('sel_entries', onFilterChange);

    $scope.setSelectedController = function () {
        var id = this.company;
        if (_.contains($scope.selectedColumns["controller"], id)) {
            $scope.selectedColumns["controller"] = _.without($scope.selectedColumns["controller"], id);
        } else {
            $scope.selectedColumns["controller"].push(id);
        }
        return false;
    };

    $scope.isChecked = function (id) {
        if (_.contains($scope.selectedColumns["controller"], id)) {
            return 'fa fa-check pull-right';
        }
        return false;
    };

    $scope.checkAll = function () {
        $scope.selectedColumns["controller"] = $scope.unique_columns["controller"];
    };

    $scope.uncheckAll = function () {
        $scope.selectedColumns["controller"] = [];
    };
}]);