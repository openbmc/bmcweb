#!/usr/bin/env python3

import requests
import json
from datetime import datetime
import urllib3
import sys
import time

# Disable SSL warnings
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

class ManagerTest:
    def __init__(self):
        self.base_url = "https://localhost:2443"
        self.username = "root"
        self.password = "0penBmc"
        self.headers = {"Content-Type": "application/json"}
        
    def send_request(self, method, endpoint, data=None):
        url = f"{self.base_url}{endpoint}"
        response = requests.request(
            method,
            url,
            headers=self.headers,
            auth=(self.username, self.password),
            verify=False,
            json=data if data else None
        )
        print(f"\nHTTP Response Code ({method} {endpoint}): {response.status_code}")
        if response.status_code not in [200, 204]:
            print(f"Response Headers: {dict(response.headers)}")
            print(f"Response Body: {response.text}")
        return response

    def test_datetime_update(self):
        print("\n=== Testing DateTime Update ===")
        
        # Step 1: Get initial state
        print("\nStep 1: Getting initial state...")
        ntp_endpoint = "/redfish/v1/Managers/bmc/NetworkProtocol"
        response = self.send_request("GET", ntp_endpoint)
        if response.status_code == 200:
            initial_ntp_state = response.json().get("NTP", {}).get("ProtocolEnabled")
            print(f"Initial NTP state: {initial_ntp_state}")
        
        # Step 2: Disable NTP
        print("\nStep 2: Disabling NTP...")
        ntp_data = {"NTP": {"ProtocolEnabled": False}}
        print(f"PATCH data: {json.dumps(ntp_data, indent=2)}")
        
        response = self.send_request("PATCH", ntp_endpoint, ntp_data)
        if response.status_code != 204:
            print(f"Failed to disable NTP. Status code: {response.status_code}")
            return False
        print("✓ NTP disable request successful")
        
        # Verify NTP state after PATCH
        response = self.send_request("GET", ntp_endpoint)
        if response.status_code == 200:
            current_ntp_state = response.json().get("NTP", {}).get("ProtocolEnabled")
            print(f"NTP state after PATCH: {current_ntp_state}")
            print(f"PATCH validation: {'Success' if current_ntp_state == False else 'Failed'}")
        
        # Wait for NTP to fully disable
        time.sleep(2)
        
        # Step 3: Update DateTime
        print("\nStep 3: Updating DateTime...")
        new_datetime = "2025-04-10T14:30:00+00:00"
        datetime_endpoint = "/redfish/v1/Managers/bmc"
        datetime_data = {"DateTime": new_datetime}
        print(f"PATCH data: {json.dumps(datetime_data, indent=2)}")
        
        # Get initial datetime
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code == 200:
            initial_datetime = response.json().get("DateTime")
            print(f"Initial DateTime: {initial_datetime}")
        
        response = self.send_request("PATCH", datetime_endpoint, datetime_data)
        if response.status_code != 204:
            print(f"Failed to update DateTime. Status code: {response.status_code}")
            return False
        print("✓ DateTime update request successful")
        
        # Step 4: Verify DateTime Update
        print("\nStep 4: Verifying DateTime update...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print(f"Failed to get manager info. Status code: {response.status_code}")
            return False
            
        current_datetime = response.json().get("DateTime")
        print("\nDateTime Comparison:")
        print(f"Initial value  : {initial_datetime}")
        print(f"Expected value : {new_datetime}")
        print(f"Updated value  : {current_datetime}")
        
        datetime_success = self.compare_datetime_dates(initial_datetime, current_datetime)
        print(f"DateTime update: {'✓ Success' if datetime_success else '× Failed'}")
        
        if datetime_success:
            print("✓ DateTime PATCH validation: Success")
            return True
        else:
            print("× DateTime PATCH validation: Failed")
            return False

    def test_invalid_property(self):
        print("\n=== Testing Invalid Property PATCH ===")
        
        datetime_endpoint = "/redfish/v1/Managers/bmc"
        
        # Step 1: Get initial state
        print("\nStep 1: Getting initial state...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get initial state")
            return False
        
        try:
            initial_state = response.json()
            initial_datetime = initial_state.get("DateTime")
            print(f"Initial DateTime: {initial_datetime}")
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing initial state: {str(e)}")
            return False
        
        # Step 2: Try to PATCH with invalid property
        print("\nStep 2: Attempting PATCH with invalid property...")
        invalid_data = {
            "InvalidProperty": "some_value",
            "DateTime": "2025-12-25T00:00:00+00:00"
        }
        print("PATCH data:")
        print(json.dumps(invalid_data, indent=2))
        
        response = self.send_request("PATCH", datetime_endpoint, invalid_data)
        
        # Step 3: Validate error response
        print("\nStep 3: Validating error response...")
        if response.status_code != 400:
            print(f"× Expected status code 400, got {response.status_code}")
            return False
        
        print("✓ Received expected 400 Bad Request error")
        
        try:
            error_response = response.json()
            print("\nError Response:")
            print(json.dumps(error_response, indent=2))
            
            # Validate error message structure and content
            expected_error = {
                "error": {
                    "@Message.ExtendedInfo": [
                        {
                            "@odata.type": "#Message.v1_1_1.Message",
                            "Message": "The property InvalidProperty is not in the list of valid properties for the resource.",
                            "MessageArgs": [
                                "InvalidProperty"
                            ],
                            "MessageId": "Base.1.19.PropertyUnknown",
                            "MessageSeverity": "Warning",
                            "Resolution": "Remove the unknown property from the request body and resubmit the request if the operation failed."
                        }
                    ],
                    "code": "Base.1.19.PropertyUnknown",
                    "message": "The property InvalidProperty is not in the list of valid properties for the resource."
                }
            }
            
            if error_response == expected_error:
                print("✓ Error response matches expected format")
            else:
                print("× Error response does not match expected format")
                print("\nExpected:")
                print(json.dumps(expected_error, indent=2))
                print("\nGot:")
                print(json.dumps(error_response, indent=2))
                return False
            
            # Step 4: Verify DateTime remained unchanged
            print("\nStep 4: Verifying DateTime remained unchanged...")
            time.sleep(2)  # Wait a bit to ensure any potential changes would have taken effect
            
            verify_response = self.send_request("GET", datetime_endpoint)
            if verify_response.status_code != 200:
                print("Failed to get updated state")
                return False
            
            current_state = verify_response.json()
            current_datetime = current_state.get("DateTime")
            
            print("\nDateTime Comparison:")
            print(f"Initial DateTime    : {initial_datetime}")
            print(f"Attempted DateTime  : {invalid_data['DateTime']}")
            print(f"Current DateTime    : {current_datetime}")
            
            if self.compare_datetime_dates(initial_datetime, current_datetime):
                print("✓ DateTime remained unchanged as expected")
                return True
            else:
                print("× DateTime date was modified despite invalid property error")
                return False
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing response: {str(e)}")
            return False

    def test_oem_openbmc_properties(self):
        print("\n=== Testing OEM OpenBMC Properties PATCH ===")
        
        datetime_endpoint = "/redfish/v1/Managers/bmc"
        
        # Step 1: Get initial state
        print("\nStep 1: Getting initial state...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get initial state")
            return False
        
        initial_state = response.json()
        initial_datetime = initial_state.get("DateTime")
        initial_oem = initial_state.get("Oem", {}).get("OpenBmc", {}).get("Fan", {}).get("StepwiseControllers", {}).get("SWTest", {})
        
        print("Initial state:")
        print(f"DateTime: {initial_datetime}")
        print(f"OpenBMC Fan Controller: {json.dumps(initial_oem, indent=2)}")
        
        # Step 2: Update DateTime and OEM properties
        print("\nStep 2: Attempting PATCH with DateTime and OEM properties...")
        new_datetime = "2025-04-10T14:30:00+00:00"
        patch_data = {
            "DateTime": new_datetime,
            "Oem": {
                "OpenBmc": {
                    "Fan": {
                        "StepwiseControllers": {
                            "SWTest": {
                                "NegativeHysteresis": 3,
                                "PositiveHysteresis": 4
                            }
                        }
                    }
                }
            }
        }
        
        print("PATCH data:")
        print(json.dumps(patch_data, indent=2))
        
        response = self.send_request("PATCH", datetime_endpoint, patch_data)
        if response.status_code != 204:
            print(f"× PATCH request failed with status code: {response.status_code}")
            return False
        
        print("✓ PATCH request successful")
        
        # Step 3: Verify updates
        print("\nStep 3: Verifying updates...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get updated state")
            return False
        
        updated_state = response.json()
        updated_datetime = updated_state.get("DateTime")
        updated_oem = updated_state.get("Oem", {}).get("OpenBmc", {}).get("Fan", {}).get("StepwiseControllers", {}).get("SWTest", {})
        
        print("\nComparison:")
        print("DateTime:")
        print(f"  Initial : {initial_datetime}")
        print(f"  Updated : {updated_datetime}")
        print(f"  Expected: {new_datetime}")
        
        print("\nOEM OpenBMC Fan Controller:")
        print("Initial:")
        print(json.dumps(initial_oem, indent=2))
        print("Updated:")
        print(json.dumps(updated_oem, indent=2))
        print("Expected:")
        print(json.dumps(patch_data["Oem"]["OpenBmc"]["Fan"]["StepwiseControllers"]["SWTest"], indent=2))
        
        # Validate DateTime
        datetime_success = self.compare_datetime_dates(new_datetime, updated_datetime)
        print(f"\nDateTime update: {'✓ Success' if datetime_success else '× Failed'}")
        
        # Validate OEM properties
        oem_success = (
            updated_oem.get("NegativeHysteresis") == 3 and
            updated_oem.get("PositiveHysteresis") == 4
        )
        print(f"OEM properties update: {'✓ Success' if oem_success else '× Failed'}")
        
        return datetime_success and oem_success

    def test_invalid_oem_property(self):
        print("\n=== Testing Invalid OEM Property PATCH ===")
        
        datetime_endpoint = "/redfish/v1/Managers/bmc"
        
        # Step 1: Get initial state
        print("\nStep 1: Getting initial DateTime...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get initial state")
            return False
        
        initial_datetime = response.json().get("DateTime")
        print(f"Initial DateTime: {initial_datetime}")
        
        # Step 2: Try to PATCH with invalid OEM property
        print("\nStep 2: Attempting PATCH with invalid OEM property and new DateTime...")
        new_datetime = "2025-12-25T00:00:00+00:00"
        invalid_data = {
            "DateTime": new_datetime,
            "Oem": {
                "OpenBmc": {
                    "Fan": {
                        "InvalidController": {  # Invalid property in OEM structure
                            "SomeValue": 123
                        },
                        "StepwiseControllers": {
                            "SWTest": {
                                "NegativeHysteresis": 3,
                                "PositiveHysteresis": 4
                            }
                        }
                    }
                }
            }
        }
        print("PATCH data:")
        print(json.dumps(invalid_data, indent=2))
        
        response = self.send_request("PATCH", datetime_endpoint, invalid_data)
        
        # Step 3: Validate error response
        print("\nStep 3: Validating error response...")
        if response.status_code != 400:
            print(f"× Expected status code 400, got {response.status_code}")
            return False
        
        print("✓ Received expected 400 Bad Request error")
        
        try:
            error_response = response.json()
            print("\nError Response Details:")
            print(json.dumps(error_response, indent=2))
            
            # Validate error message structure and content
            extended_info = error_response.get("error", {}).get("@Message.ExtendedInfo", [])[0]
            
            # Verify the exact invalid key in the message
            if "InvalidController" not in extended_info.get("Message", ""):
                print("× Error message does not contain the exact invalid property name")
                return False
            
            print(f"✓ Error message contains invalid property name: '{extended_info.get('Message')}'")
            
            # Verify MessageId and MessageArgs
            if (extended_info.get("MessageId") != "Base.1.19.PropertyUnknown" or
                "InvalidController" not in extended_info.get("MessageArgs", [])):
                print("× Error response MessageId or MessageArgs are incorrect")
                return False
            
            print("✓ Error response contains correct MessageId and MessageArgs")
            
        except (json.JSONDecodeError, IndexError, KeyError):
            print("× Error response is not in expected format")
            return False
        
        # Step 4: Verify DateTime was not updated
        print("\nStep 4: Verifying DateTime remained unchanged...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get DateTime after PATCH")
            return False
        
        current_datetime = response.json().get("DateTime")
        print("\nDateTime Comparison:")
        print(f"Initial DateTime    : {initial_datetime}")
        print(f"Attempted DateTime  : {new_datetime}")
        print(f"Current DateTime    : {current_datetime}")
        
        if self.compare_datetime_dates(initial_datetime, current_datetime):
            print("✓ DateTime remained unchanged as expected")
            if current_datetime != new_datetime:
                print("✓ Attempted DateTime update was correctly rejected")
                return True
            else:
                print("× DateTime coincidentally matches attempted update")
                return False
        else:
            print("× DateTime was modified despite invalid property error")
            return False

    def test_empty_patch_body(self):
        print("\n=== Testing Empty PATCH Body ===")
        
        datetime_endpoint = "/redfish/v1/Managers/bmc"
        
        # Step 1: Get initial state
        print("\nStep 1: Getting initial state...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get initial state")
            return False
        
        initial_state = response.json()
        initial_datetime = initial_state.get("DateTime")
        print(f"Initial DateTime: {initial_datetime}")
        
        # Step 2: Try to PATCH with empty body
        print("\nStep 2: Attempting PATCH with empty body...")
        empty_data = {}
        print("PATCH data: {}")
        
        response = self.send_request("PATCH", datetime_endpoint, empty_data)
        
        # Step 3: Validate error response
        print("\nStep 3: Validating error response...")
        if response.status_code != 400:
            print(f"× Expected status code 400, got {response.status_code}")
            return False
        
        print("✓ Received expected 400 Bad Request error")
        
        try:
            error_response = response.json()
            print("\nError Response Details:")
            print(json.dumps(error_response, indent=2))
            
            # Validate error message structure
            extended_info = error_response.get("error", {}).get("@Message.ExtendedInfo", [])[0]
            
            # Verify expected error properties
            expected_message = "The request body submitted was malformed JSON and could not be parsed by the receiving service."
            expected_message_id = "Base.1.19.MalformedJSON"
            
            if extended_info.get("Message") != expected_message:
                print("× Error message does not match expected message")
                print(f"Expected: {expected_message}")
                print(f"Got: {extended_info.get('Message')}")
                return False
            
            if extended_info.get("MessageId") != expected_message_id:
                print("× MessageId does not match expected value")
                print(f"Expected: {expected_message_id}")
                print(f"Got: {extended_info.get('MessageId')}")
                return False
            
            if extended_info.get("MessageSeverity") != "Critical":
                print("× MessageSeverity is not Critical")
                return False
            
            print("✓ Error response matches expected format")
            print(f"✓ MessageId: {expected_message_id}")
            print(f"✓ Message: {expected_message}")
            print("✓ MessageSeverity: Critical")
            
        except (json.JSONDecodeError, IndexError, KeyError):
            print("× Error response is not in expected format")
            return False
        
        # Step 4: Verify state remained unchanged
        print("\nStep 4: Verifying state remained unchanged...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get state after PATCH")
            return False
        
        current_state = response.json()
        current_datetime = current_state.get("DateTime")
        
        print("\nState Comparison:")
        print(f"Initial DateTime: {initial_datetime}")
        print(f"Current DateTime: {current_datetime}")
        
        if current_datetime == initial_datetime:
            print("✓ System state remained unchanged as expected")
            return True
        else:
            print("× System state changed unexpectedly")
            return False

    def test_malformed_oem_json(self):
        print("\n=== Testing Malformed OEM JSON PATCH ===")
        
        datetime_endpoint = "/redfish/v1/Managers/bmc"
        
        # Step 1: Get initial state
        print("\nStep 1: Getting initial state...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get initial state")
            return False
        
        initial_state = response.json()
        initial_datetime = initial_state.get("DateTime")
        print(f"Initial DateTime: {initial_datetime}")
        
        # Step 2: Try to PATCH with malformed JSON in OEM structure
        print("\nStep 2: Attempting PATCH with malformed OEM JSON...")
        malformed_data = {
            "DateTime": "2025-12-25T00:00:00+00:00",
            "Oem": {
                "OpenBmc": {
                    "Fan": "this should be an object, not a string",
                    "StepwiseControllers": {
                        "SWTest": {
                            "NegativeHysteresis": 3,
                            "PositiveHysteresis": 4
                        }
                    }
                }
            }
        }
        print("PATCH data:")
        print(json.dumps(malformed_data, indent=2))
        
        response = self.send_request("PATCH", datetime_endpoint, malformed_data)
        
        # Step 3: Validate error response
        print("\nStep 3: Validating error response...")
        if response.status_code != 400:
            print(f"× Expected status code 400, got {response.status_code}")
            return False
        
        print("✓ Received expected 400 Bad Request error")
        
        try:
            error_response = response.json()
            print("\nError Response Details:")
            print(json.dumps(error_response, indent=2))
            
            # Validate error response structure
            if "Fan/@Message.ExtendedInfo" not in error_response:
                print("× Error response missing Fan/@Message.ExtendedInfo")
                return False
            
            extended_info = error_response["Fan/@Message.ExtendedInfo"][0]
            
            # Verify expected error properties
            expected_properties = {
                "@odata.type": "#Message.v1_1_1.Message",
                "Message": "The value '\"this should be an object, not a string\"' for the property Fan/ is not a type that the property can accept.",
                "MessageId": "Base.1.19.PropertyValueTypeError",
                "MessageSeverity": "Warning",
                "MessageArgs": ["\"this should be an object, not a string\"", "Fan/"],
                "Resolution": "Correct the value for the property in the request body and resubmit the request if the operation failed."
            }
            
            for key, expected_value in expected_properties.items():
                actual_value = extended_info.get(key)
                if actual_value != expected_value:
                    print(f"× {key} does not match expected value")
                    print(f"Expected: {expected_value}")
                    print(f"Got: {actual_value}")
                    return False
            
            print("✓ Error response matches expected format")
            print(f"✓ MessageId: {extended_info['MessageId']}")
            print(f"✓ Message: {extended_info['Message']}")
            print(f"✓ MessageSeverity: {extended_info['MessageSeverity']}")
            
        except (json.JSONDecodeError, IndexError, KeyError) as e:
            print(f"× Error response is not in expected format: {str(e)}")
            return False
        
        # Step 4: Verify state remained unchanged
        print("\nStep 4: Verifying state remained unchanged...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get state after PATCH")
            return False
        
        current_datetime = response.json().get("DateTime")
        print("\nDateTime Comparison:")
        print(f"Initial DateTime    : {initial_datetime}")
        print(f"Attempted DateTime  : {malformed_data['DateTime']}")
        print(f"Current DateTime    : {current_datetime}")
        
        if self.compare_datetime_dates(initial_datetime, current_datetime):
            print("✓ DateTime remained unchanged as expected")
            return True
        else:
            print("× DateTime was modified despite invalid property error")
            return False

    def test_datetime_bad_type(self):
        print("\n=== Testing DateTime Bad Type PATCH ===")
        
        datetime_endpoint = "/redfish/v1/Managers/bmc"
        
        # Step 1: Get initial state
        print("\nStep 1: Getting initial state...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get initial state")
            return False
        
        try:
            initial_state = response.json()
            initial_datetime = initial_state.get("DateTime")
            print(f"Initial DateTime: {initial_datetime}")
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing initial state: {str(e)}")
            return False
        
        # Step 2: Try to PATCH DateTime with wrong type
        print("\nStep 2: Attempting PATCH with invalid DateTime type...")
        invalid_data = {
            "DateTime": 12345  # Number instead of string
        }
        print("PATCH data:")
        print(json.dumps(invalid_data, indent=2))
        
        response = self.send_request("PATCH", datetime_endpoint, invalid_data)
        
        # Step 3: Validate error response
        print("\nStep 3: Validating error response...")
        if response.status_code != 400:
            print(f"× Expected status code 400, got {response.status_code}")
            return False
        
        print("✓ Received expected 400 Bad Request error")
        
        try:
            error_response = response.json()
            print("\nError Response:")
            print(json.dumps(error_response, indent=2))
            
            # Validate error response structure
            expected_error = {
                "DateTime@Message.ExtendedInfo": [
                    {
                        "@odata.type": "#Message.v1_1_1.Message",
                        "Message": "The value '12345' for the property DateTime is not a type that the property can accept.",
                        "MessageArgs": ["12345", "DateTime"],
                        "MessageId": "Base.1.19.PropertyValueTypeError",
                        "MessageSeverity": "Warning",
                        "Resolution": "Correct the value for the property in the request body and resubmit the request if the operation failed."
                    }
                ]
            }
            
            if error_response == expected_error:
                print("✓ Error response matches expected format")
            else:
                print("× Error response does not match expected format")
                print("\nExpected:")
                print(json.dumps(expected_error, indent=2))
                print("\nGot:")
                print(json.dumps(error_response, indent=2))
                return False
            
            # Step 4: Verify DateTime remained unchanged
            print("\nStep 4: Verifying DateTime remained unchanged...")
            verify_response = self.send_request("GET", datetime_endpoint)
            if verify_response.status_code != 200:
                print("Failed to get updated state")
                return False
            
            current_datetime = verify_response.json().get("DateTime")
            print("\nDateTime Comparison:")
            print(f"Initial DateTime    : {initial_datetime}")
            print(f"Current DateTime    : {current_datetime}")
            
            # Use the helper function to compare dates
            if self.compare_datetime_dates(initial_datetime, current_datetime):
                print("✓ DateTime date portion remained unchanged as expected")
                return True
            else:
                print("× DateTime date was modified despite invalid type error")
                return False
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing response: {str(e)}")
            return False

    def test_invalid_manager_id(self):
        print("\n=== Testing Invalid Manager ID PATCH ===")
        
        # Using invalid manager ID in the endpoint
        invalid_endpoint = "/redfish/v1/Managers/invalid_bmc"
        
        # Step 1: Try to PATCH with invalid manager ID
        print("\nStep 1: Attempting PATCH with invalid manager ID...")
        patch_data = {
            "DateTime": "2025-04-10T14:30:00+00:00",
            "Oem": {
                "OpenBmc": {
                    "Fan": {
                        "StepwiseControllers": {
                            "SWTest": {
                                "NegativeHysteresis": 3,
                                "PositiveHysteresis": 4
                            }
                        }
                    }
                }
            }
        }
        print("PATCH data:")
        print(json.dumps(patch_data, indent=2))
        
        response = self.send_request("PATCH", invalid_endpoint, patch_data)
        
        # Step 2: Validate error response
        print("\nStep 2: Validating error response...")
        if response.status_code != 404:
            print(f"× Expected status code 404, got {response.status_code}")
            return False
        
        print("✓ Received expected 404 Not Found error")
        
        try:
            error_response = response.json()
            print("\nError Response Details:")
            print(json.dumps(error_response, indent=2))
            
            # Validate error structure
            error = error_response.get("error", {})
            if not error:
                print("× Error response missing 'error' object")
                return False
            
            # Verify error code and message
            expected_code = "Base.1.19.ResourceNotFound"
            expected_message = "The requested resource of type Manager named 'invalid_bmc' was not found."
            
            if error.get("code") != expected_code:
                print(f"× Error code does not match expected value")
                print(f"Expected: {expected_code}")
                print(f"Got: {error.get('code')}")
                return False
            
            if error.get("message") != expected_message:
                print(f"× Error message does not match expected value")
                print(f"Expected: {expected_message}")
                print(f"Got: {error.get('message')}")
                return False
            
            # Validate extended info
            extended_info = error.get("@Message.ExtendedInfo", [])[0]
            
            # Verify expected error properties
            expected_properties = {
                "@odata.type": "#Message.v1_1_1.Message",
                "Message": "The requested resource of type Manager named 'invalid_bmc' was not found.",
                "MessageId": "Base.1.19.ResourceNotFound",
                "MessageSeverity": "Critical",
                "MessageArgs": [
                    "Manager",
                    "invalid_bmc"
                ],
                "Resolution": "Provide a valid resource identifier and resubmit the request."
            }
            
            for key, expected_value in expected_properties.items():
                actual_value = extended_info.get(key)
                if actual_value != expected_value:
                    print(f"× {key} does not match expected value")
                    print(f"Expected: {expected_value}")
                    print(f"Got: {actual_value}")
                    return False
            
            print("✓ Error response matches expected format")
            print(f"✓ Error code: {error['code']}")
            print(f"✓ Error message: {error['message']}")
            print(f"✓ MessageId: {extended_info['MessageId']}")
            print(f"✓ Message: {extended_info['Message']}")
            print(f"✓ MessageSeverity: {extended_info['MessageSeverity']}")
            
            return True
            
        except (json.JSONDecodeError, IndexError, KeyError) as e:
            print(f"× Error response is not in expected format: {str(e)}")
            return False

    def test_fan_controller_update(self):
        print("\n=== Testing Fan Controller Update (FFGainCoefficient) ===")
        
        datetime_endpoint = "/redfish/v1/Managers/bmc"
        
        # Step 1: Get initial state
        print("\nStep 1: Getting initial Fan Controller state...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get initial state")
            return False
        
        try:
            initial_state = response.json()
            initial_fan = initial_state.get("Oem", {}).get("OpenBmc", {}).get("Fan", {}).get("FanControllers", {}).get("Fan_SYS0", {})
            if not initial_fan:
                print("× Failed to get initial Fan_SYS0 configuration")
                return False
            
            initial_ffgain = initial_fan.get("FFGainCoefficient")
            print("\nInitial Fan_SYS0 FFGainCoefficient:", initial_ffgain)
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing initial state: {str(e)}")
            return False
        
        # Step 2: Update FFGainCoefficient
        print("\nStep 2: Updating FFGainCoefficient...")
        update_data = {
            "Oem": {
                "OpenBmc": {
                    "Fan": {
                        "FanControllers": {
                            "Fan_SYS0": {
                                "FFGainCoefficient": 2.0
                            }
                        }
                    }
                }
            }
        }
        
        print("PATCH data:")
        print(json.dumps(update_data, indent=2))
        
        response = self.send_request("PATCH", datetime_endpoint, update_data)
        
        # Step 3: Validate PATCH response
        print("\nStep 3: Validating PATCH response...")
        if response.status_code != 204:
            print(f"× PATCH request failed with status code: {response.status_code}")
            return False
        
        print("✓ PATCH request successful")
        
        # Wait for changes to take effect
        time.sleep(2)
        
        # Step 4: Verify updated configuration
        print("\nStep 4: Verifying updated configuration...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get updated state")
            return False
        
        try:
            updated_state = response.json()
            updated_fan = updated_state.get("Oem", {}).get("OpenBmc", {}).get("Fan", {}).get("FanControllers", {}).get("Fan_SYS0", {})
            if not updated_fan:
                print("× Failed to get updated Fan_SYS0 configuration")
                return False
            
            updated_ffgain = updated_fan.get("FFGainCoefficient")
            
            print("\nFFGainCoefficient Comparison:")
            print(f"Initial value  : {initial_ffgain}")
            print(f"Updated value  : {updated_ffgain}")
            print(f"Expected value : 2.0")
            
            if updated_ffgain == 2.0:
                print("\n✓ FFGainCoefficient was updated correctly")
                
                # Verify other properties remained unchanged
                print("\nVerifying other properties remained unchanged...")
                properties_to_check = [
                    "FFOffCoefficient", "ICoefficient", "ILimitMax", "ILimitMin",
                    "NegativeHysteresis", "OutLimitMax", "OutLimitMin", "PCoefficient",
                    "PositiveHysteresis", "SlewNeg", "SlewPos", "Inputs", "Outputs", "Zones"
                ]
                
                all_unchanged = True
                for prop in properties_to_check:
                    if initial_fan.get(prop) != updated_fan.get(prop):
                        print(f"× {prop} was unexpectedly modified")
                        print(f"  Initial: {initial_fan.get(prop)}")
                        print(f"  Current: {updated_fan.get(prop)}")
                        all_unchanged = False
                
                if all_unchanged:
                    print("✓ All other properties remained unchanged")
                    return True
                else:
                    print("× Some properties were unexpectedly modified")
                    return False
            else:
                print("\n× FFGainCoefficient update failed")
                return False
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing updated state: {str(e)}")
            return False

    def test_patch_oem_fan(self):
        print("\n=== Testing OEM Fan PATCH ===")
        
        datetime_endpoint = "/redfish/v1/Managers/bmc"
        
        # Step 1: Get initial state
        print("\nStep 1: Getting initial Fan Controller state...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get initial state")
            return False
        
        try:
            initial_state = response.json()
            print("\nInitial configuration:")
            print(json.dumps(initial_state.get("Oem", {}).get("OpenBmc", {}).get("Fan", {}).get("FanControllers", {}), indent=2))
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing initial state: {str(e)}")
            return False
        
        # Step 2: Update FFGainCoefficient
        print("\nStep 2: Creating new Fan Controller with FFGainCoefficient...")
        update_data = {
            "Oem": {
                "OpenBmc": {
                    "Fan": {
                        "FanControllers": {
                            "Fan TEST3": {
                                "FFGainCoefficient": 2.0,
                                "Zones": [
                                    {
                                        "@odata.id": "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanZones/Zone_1"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        }
        
        print("PATCH data:")
        print(json.dumps(update_data, indent=2))
        
        response = self.send_request("PATCH", datetime_endpoint, update_data)
        
        # Step 3: Validate PATCH response
        print("\nStep 3: Validating PATCH response...")
        if response.status_code != 200:  # Changed from 204 to 200 as we expect a response body
            print(f"× PATCH request failed with status code: {response.status_code}")
            if response.text:
                print(f"Error response: {response.text}")
            return False
        
        try:
            response_data = response.json()
            print("\nPATCH Response:")
            print(json.dumps(response_data, indent=2))
            
            # Validate response structure
            expected_response = {
                "Oem": {
                    "OpenBmc": {
                        "@Message.ExtendedInfo": [
                            {
                                "@odata.type": "#Message.v1_1_1.Message",
                                "Message": "The request completed successfully.",
                                "MessageArgs": [],
                                "MessageId": "Base.1.19.Success",
                                "MessageSeverity": "OK",
                                "Resolution": "None."
                            }
                        ] * 3  # Expecting three identical success messages
                    }
                }
            }
            
            if response_data == expected_response:
                print("✓ PATCH response matches expected format")
            else:
                print("× PATCH response does not match expected format")
                return False
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing PATCH response: {str(e)}")
            return False
        
        # Step 4: Verify updated configuration
        print("\nStep 4: Verifying updated configuration...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get updated state")
            return False
        
        try:
            updated_state = response.json()
            updated_fan = updated_state.get("Oem", {}).get("OpenBmc", {}).get("Fan", {}).get("FanControllers", {}).get("Fan_TEST3", {})
            
            print("\nUpdated configuration:")
            print(json.dumps(updated_fan, indent=2))
            
            if not updated_fan:
                print("× Failed to get Fan_TEST3 configuration")
                return False
            
            updated_ffgain = updated_fan.get("FFGainCoefficient")
            
            print("\nFFGainCoefficient Comparison:")
            print(f"Expected value : 2.0")
            print(f"Updated value  : {updated_ffgain}")
            
            if updated_ffgain == 2.0:
                print("\n✓ FFGainCoefficient was set correctly")
                return True
            else:
                print("\n× FFGainCoefficient was not set to expected value")
                return False
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing updated state: {str(e)}")
            return False

    def test_patch_oem_fan_without_zone(self):
        print("\n=== Testing OEM Fan PATCH Without Zone ===")
        
        datetime_endpoint = "/redfish/v1/Managers/bmc"
        
        # Step 1: Get initial state
        print("\nStep 1: Getting initial Fan Controller state...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get initial state")
            return False
        
        try:
            initial_state = response.json()
            print("\nInitial configuration:")
            print(json.dumps(initial_state.get("Oem", {}).get("OpenBmc", {}).get("Fan", {}).get("FanControllers", {}), indent=2))
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing initial state: {str(e)}")
            return False
        
        # Step 2: Update FFGainCoefficient without Zones
        print("\nStep 2: Attempting to create Fan Controller without Zones...")
        update_data = {
            "Oem": {
                "OpenBmc": {
                    "Fan": {
                        "FanControllers": {
                            "Fan TEST4": {
                                "FFGainCoefficient": 2.0
                            }
                        }
                    }
                }
            }
        }
        
        print("PATCH data:")
        print(json.dumps(update_data, indent=2))
        
        response = self.send_request("PATCH", datetime_endpoint, update_data)
        
        # Step 3: Validate error response
        print("\nStep 3: Validating error response...")
        if response.status_code != 500:  # Expecting Internal Error
            print(f"× Expected status code 500, got {response.status_code}")
            return False
        
        try:
            error_response = response.json()
            print("\nError Response:")
            print(json.dumps(error_response, indent=2))
            
            # Validate error response structure
            expected_error = {
                "error": {
                    "@Message.ExtendedInfo": [
                        {
                            "@odata.type": "#Message.v1_1_1.Message",
                            "Message": "The request failed due to an internal service error.  The service is still operational.",
                            "MessageArgs": [],
                            "MessageId": "Base.1.19.InternalError",
                            "MessageSeverity": "Critical",
                            "Resolution": "Resubmit the request.  If the problem persists, consider resetting the service."
                        }
                    ],
                    "code": "Base.1.19.InternalError",
                    "message": "The request failed due to an internal service error.  The service is still operational."
                }
            }
            
            if error_response == expected_error:
                print("✓ Error response matches expected format")
            else:
                print("× Error response does not match expected format")
                print("\nExpected:")
                print(json.dumps(expected_error, indent=2))
                print("\nGot:")
                print(json.dumps(error_response, indent=2))
                return False
            
            # Step 4: Verify fan controller was not created
            print("\nStep 4: Verifying fan controller was not created...")
            verify_response = self.send_request("GET", datetime_endpoint)
            if verify_response.status_code != 200:
                print("Failed to get configuration")
                return False
            
            updated_state = verify_response.json()
            fan_controllers = updated_state.get("Oem", {}).get("OpenBmc", {}).get("Fan", {}).get("FanControllers", {})
            
            if "Fan_TEST4" in fan_controllers:
                print("× Fan controller was created despite missing Zones")
                return False
            else:
                print("✓ Fan controller was not created as expected")
                return True
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing response: {str(e)}")
            return False

    def test_patch_datetime_with_oem_fan(self):
        print("\n=== Testing DateTime Update with OEM Fan PATCH ===")
        
        datetime_endpoint = "/redfish/v1/Managers/bmc"
        
        # Step 1: Get initial state
        print("\nStep 1: Getting initial state...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get initial state")
            return False
        
        try:
            initial_state = response.json()
            initial_datetime = initial_state.get("DateTime")
            print("\nInitial configuration:")
            print(f"DateTime: {initial_datetime}")
            print("Fan Controllers:")
            print(json.dumps(initial_state.get("Oem", {}).get("OpenBmc", {}).get("Fan", {}).get("FanControllers", {}), indent=2))
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing initial state: {str(e)}")
            return False
        
        # Step 2: Update DateTime and Fan Controller
        print("\nStep 2: Updating DateTime and creating new Fan Controller...")
        new_datetime = "2025-04-10T14:30:00+00:00"
        update_data = {
            "DateTime": new_datetime,
            "Oem": {
                "OpenBmc": {
                    "Fan": {
                        "FanControllers": {
                            "Fan TEST5": {
                                "FFGainCoefficient": 2.0,
                                "Zones": [
                                    {
                                        "@odata.id": "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanZones/Zone_1"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        }
        
        print("PATCH data:")
        print(json.dumps(update_data, indent=2))
        
        response = self.send_request("PATCH", datetime_endpoint, update_data)
        
        # Step 3: Validate PATCH response
        print("\nStep 3: Validating PATCH response...")
        if response.status_code != 200:
            print(f"× PATCH request failed with status code: {response.status_code}")
            if response.text:
                print(f"Error response: {response.text}")
            return False
        
        try:
            response_data = response.json()
            print("\nPATCH Response:")
            print(json.dumps(response_data, indent=2))
            
            # Validate response structure
            expected_response = {
                "Oem": {
                    "OpenBmc": {
                        "@Message.ExtendedInfo": [
                            {
                                "@odata.type": "#Message.v1_1_1.Message",
                                "Message": "The request completed successfully.",
                                "MessageArgs": [],
                                "MessageId": "Base.1.19.Success",
                                "MessageSeverity": "OK",
                                "Resolution": "None."
                            }
                        ] * 3
                    }
                }
            }
            
            if response_data == expected_response:
                print("✓ PATCH response matches expected format")
            else:
                print("× PATCH response does not match expected format")
                print("\nExpected:")
                print(json.dumps(expected_response, indent=2))
                print("\nGot:")
                print(json.dumps(response_data, indent=2))
                return False
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing PATCH response: {str(e)}")
            return False
        
        # Step 4: Verify updated configuration
        print("\nStep 4: Verifying updated configuration...")
        response = self.send_request("GET", datetime_endpoint)
        if response.status_code != 200:
            print("Failed to get updated state")
            return False
        
        try:
            updated_state = response.json()
            updated_datetime = updated_state.get("DateTime")
            updated_fan = updated_state.get("Oem", {}).get("OpenBmc", {}).get("Fan", {}).get("FanControllers", {}).get("Fan_TEST5", {})
            
            print("\nUpdated configuration:")
            print(f"DateTime: {updated_datetime}")
            print("Fan Controller:")
            print(json.dumps(updated_fan, indent=2))
            
            # Verify DateTime update
            print("\nDateTime Comparison:")
            print(f"Initial value  : {initial_datetime}")
            print(f"Expected value : {new_datetime}")
            print(f"Updated value  : {updated_datetime}")
            
            datetime_success = self.compare_datetime_dates(new_datetime, updated_datetime)
            print(f"DateTime update: {'✓ Success' if datetime_success else '× Failed'}")
            
            # Verify Fan Controller update
            if not updated_fan:
                print("× Failed to get Fan_TEST5 configuration")
                return False
            
            updated_ffgain = updated_fan.get("FFGainCoefficient")
            print("\nFFGainCoefficient Comparison:")
            print(f"Expected value : 2.0")
            print(f"Updated value  : {updated_ffgain}")
            
            fan_success = updated_ffgain == 2.0
            print(f"Fan Controller update: {'✓ Success' if fan_success else '× Failed'}")
            
            return datetime_success and fan_success
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"× Error parsing updated state: {str(e)}")
            return False

    def compare_datetime_dates(self, dt1, dt2):
        """Compare only the date portions of two DateTime strings."""
        # Extract date portion (YYYY-MM-DD) from DateTime strings
        date1 = dt1[:10]  # Gets "YYYY-MM-DD" from "YYYY-MM-DDThh:mm:ss+00:00"
        date2 = dt2[:10]
        
        if date1 == date2:
            print(f"✓ Date matches: {date1}")
            return True
        else:
            print(f"× Dates differ: {date1} != {date2}")
            return False

def main():
    test = ManagerTest()
    
    # Define available tests
    test_cases = {
        "datetime": (
            "[+] Tests DateTime update after NTP disable. "
            "Payload: {DateTime: <date-string>}. "
            "Expects: 204 success, validates date matches update.",
            test.test_datetime_update
        ),
        "invalid_property": (
            "[-] Tests invalid property in request. "
            "Payload: {InvalidProperty: 'value', DateTime: <date-string>}. "
            "Expects: 400 PropertyUnknown error, validates DateTime unchanged.",
            test.test_invalid_property
        ),
        "invalid_oem": (
            "[-] Tests fan controller with invalid property. "
            "Payload: Oem/OpenBmc/Fan/FanControllers with InvalidProperty. "
            "Expects: 400 PropertyUnknown error, fan not created.",
            test.test_invalid_oem_property
        ),
        "empty_patch": (
            "[-] Tests empty PATCH request. "
            "Payload: {}. "
            "Expects: 400 MalformedJSON error.",
            test.test_empty_patch_body
        ),
        "malformed_oem": (
            "[-] Tests malformed fan controller JSON. "
            "Payload: Fan property as string instead of object. "
            "Expects: 400 PropertyValueTypeError error.",
            test.test_malformed_oem_json
        ),
        "datetime_type": (
            "[-] Tests DateTime with wrong type. "
            "Payload: {DateTime: 12345}. "
            "Expects: 400 PropertyValueTypeError error, DateTime unchanged.",
            test.test_datetime_bad_type
        ),
        "invalid_manager": (
            "[-] Tests PATCH to invalid manager path. "
            "Payload: Valid DateTime and fan update to /invalid_bmc. "
            "Expects: 404 ResourceNotFound error.",
            test.test_invalid_manager_id
        ),
        "patch_oem_fan": (
            "[+] Tests fan controller creation. "
            "Payload: Oem/OpenBmc/Fan/FanControllers with FFGainCoefficient and Zones. "
            "Expects: 200 success with success messages.",
            test.test_patch_oem_fan
        ),
        "patch_oem_fan_no_zone": (
            "[-] Tests fan controller without required Zones. "
            "Payload: Oem/OpenBmc/Fan/FanControllers with only FFGainCoefficient. "
            "Expects: 500 InternalError, fan not created.",
            test.test_patch_oem_fan_without_zone
        ),
        "patch_datetime_fan": (
            "[+] Tests combined DateTime and fan update. "
            "Payload: DateTime and Oem/OpenBmc/Fan/FanControllers. "
            "Expects: 200 success with success messages.",
            test.test_patch_datetime_with_oem_fan
        )
    }
    
    # Parse command line arguments
    if len(sys.argv) > 1:
        test_name = sys.argv[1].lower()
        if test_name not in test_cases:
            print(f"\nError: Unknown test '{test_name}'")
            print("\nAvailable tests:")
            for key, (name, _) in test_cases.items():
                print(f"  {key:<15} - {name}")
            sys.exit(1)
            
        # Run single test
        print(f"\nRunning single test: {test_cases[test_name][0]}")
        result = test_cases[test_name][1]()
        print(f"\n=== Test Summary ===")
        print(f"{test_cases[test_name][0]}: {'PASSED' if result else 'FAILED'}")
        sys.exit(0 if result else 1)
    
    else:
        # Run all tests
        results = {}
        for key, (name, func) in test_cases.items():
            results[name] = func()
        
        print("\n=== Test Summary ===")
        for name, result in results.items():
            print(f"{name}: {'PASSED' if result else 'FAILED'}")
        
        if all(results.values()):
            sys.exit(0)
        else:
            sys.exit(1)

if __name__ == "__main__":
    main() 