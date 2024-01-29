#!/usr/bin/env python3

import argparse
import os

import requests

try:
    import redfish
except ModuleNotFoundError:
    raise Exception("Please run pip install redfish to run this script.")
try:
    from OpenSSL import crypto
except ImportError:
    raise Exception("Please run pip install pyOpenSSL to run this script.")

# Script to generate a certificates for a CA, server, and client
# allowing for client authentication using mTLS certificates.
# This can then be used to test mTLS client authentication for Redfish
# and webUI. Note that this requires the pyOpenSSL library to function.
# TODO: Use EC keys rather than RSA keys.


def generateCACert(serial):
    # CA key
    key = crypto.PKey()
    key.generate_key(crypto.TYPE_RSA, 2048)

    # CA cert
    cert = crypto.X509()
    cert.set_serial_number(serial)
    cert.set_version(2)
    cert.set_pubkey(key)
    cert.gmtime_adj_notBefore(0)
    cert.gmtime_adj_notAfter(10 * 365 * 24 * 60 * 60)

    caCertSubject = cert.get_subject()
    caCertSubject.countryName = "US"
    caCertSubject.stateOrProvinceName = "California"
    caCertSubject.localityName = "San Francisco"
    caCertSubject.organizationName = "OpenBMC"
    caCertSubject.organizationalUnitName = "bmcweb"
    caCertSubject.commonName = "Test CA"
    cert.set_issuer(caCertSubject)

    cert.add_extensions(
        [
            crypto.X509Extension(
                b"basicConstraints", True, b"CA:TRUE, pathlen:0"
            ),
            crypto.X509Extension(b"keyUsage", True, b"keyCertSign, cRLSign"),
            crypto.X509Extension(
                b"subjectKeyIdentifier", False, b"hash", subject=cert
            ),
        ]
    )
    cert.add_extensions(
        [
            crypto.X509Extension(
                b"authorityKeyIdentifier", False, b"keyid:always", issuer=cert
            )
        ]
    )

    # sign CA cert with CA key
    cert.sign(key, "sha256")
    return key, cert


def generateCert(commonName, extensions, caKey, caCert, serial):
    # key
    key = crypto.PKey()
    key.generate_key(crypto.TYPE_RSA, 2048)

    # cert
    cert = crypto.X509()
    serial
    cert.set_serial_number(serial)
    cert.set_version(2)
    cert.set_pubkey(key)
    cert.gmtime_adj_notBefore(0)
    cert.gmtime_adj_notAfter(365 * 24 * 60 * 60)

    certSubject = cert.get_subject()
    certSubject.countryName = "US"
    certSubject.stateOrProvinceName = "California"
    certSubject.localityName = "San Francisco"
    certSubject.organizationName = "OpenBMC"
    certSubject.organizationalUnitName = "bmcweb"
    certSubject.commonName = commonName
    cert.set_issuer(caCert.get_issuer())

    cert.add_extensions(extensions)
    cert.add_extensions(
        [
            crypto.X509Extension(
                b"authorityKeyIdentifier", False, b"keyid", issuer=caCert
            )
        ]
    )

    cert.sign(caKey, "sha256")
    return key, cert


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", help="Host to connect to", required=True)
    parser.add_argument(
        "--username", help="Username to connect with", default="root"
    )
    parser.add_argument(
        "--password",
        help="Password for user in order to install certs over Redfish.",
        default="0penBmc",
    )
    args = parser.parse_args()
    host = args.host
    username = args.username
    password = args.password
    if username == "root" and password == "0penBMC":
        print(
            """Note: Using default username 'root' and default password
            '0penBmc'. Use --username and --password flags to change these,
            respectively."""
        )
    serial = 1000

    try:
        print("Making certs directory.")
        os.mkdir("certs")
    except OSError as error:
        if error.errno == 17:
            print("certs directory already exists. Skipping...")
        else:
            print(error)
    caKey, caCert = generateCACert(serial)
    serial += 1
    caKeyDump = crypto.dump_privatekey(crypto.FILETYPE_PEM, caKey)
    caCertDump = crypto.dump_certificate(crypto.FILETYPE_PEM, caCert)
    with open("certs/CA-cert.pem", "wb") as f:
        f.write(caCertDump)
        print("CA cert generated.")
    with open("certs/CA-key.pem", "wb") as f:
        f.write(caKeyDump)
        print("CA key generated.")

    clientExtensions = [
        crypto.X509Extension(
            b"keyUsage",
            True,
            b"""digitalSignature,
                             keyAgreement""",
        ),
        crypto.X509Extension(b"extendedKeyUsage", True, b"clientAuth"),
    ]
    clientKey, clientCert = generateCert(
        username, clientExtensions, caKey, caCert, serial
    )
    serial += 1
    clientKeyDump = crypto.dump_privatekey(crypto.FILETYPE_PEM, clientKey)
    clientCertDump = crypto.dump_certificate(crypto.FILETYPE_PEM, clientCert)
    with open("certs/client-key.pem", "wb") as f:
        f.write(clientKeyDump)
        print("Client key generated.")
    with open("certs/client-cert.pem", "wb") as f:
        f.write(clientCertDump)
        print("Client cert generated.")

    serverExtensions = [
        crypto.X509Extension(
            b"keyUsage",
            True,
            b"""digitalSignature,
                             keyAgreement""",
        ),
        crypto.X509Extension(b"extendedKeyUsage", True, b"serverAuth"),
    ]
    serverKey, serverCert = generateCert(
        host, serverExtensions, caKey, caCert, serial
    )
    serial += 1
    serverKeyDump = crypto.dump_privatekey(crypto.FILETYPE_PEM, serverKey)
    serverCertDump = crypto.dump_certificate(crypto.FILETYPE_PEM, serverCert)
    with open("certs/server-key.pem", "wb") as f:
        f.write(serverKeyDump)
        print("Server key generated.")
    with open("certs/server-cert.pem", "wb") as f:
        f.write(serverCertDump)
        print("Server cert generated.")

    caCertJSON = {}
    caCertJSON["CertificateString"] = caCertDump.decode()
    caCertJSON["CertificateType"] = "PEM"
    caCertPath = "/redfish/v1/Managers/bmc/Truststore/Certificates"
    replaceCertPath = "/redfish/v1/CertificateService/Actions/"
    replaceCertPath += "CertificateService.ReplaceCertificate"
    print("Attempting to install CA certificate to BMC.")
    redfishObject = redfish.redfish_client(
        base_url="https://" + host,
        username=username,
        password=password,
        default_prefix="/redfish/v1",
    )
    redfishObject.login(auth="session")
    response = redfishObject.post(caCertPath, body=caCertJSON)
    if response.status == 500:
        print(
            "An existing CA certificate is likely already installed."
            " Replacing..."
        )
        caCertificateUri = {}
        caCertificateUri["@odata.id"] = caCertPath + "/1"
        caCertJSON["CertificateUri"] = caCertificateUri
        response = redfishObject.post(replaceCertPath, body=caCertJSON)
        if response.status == 200:
            print("Successfully replaced existing CA certificate.")
        else:
            raise Exception(
                "Could not install or replace CA certificate."
                "Please check if a certificate is already installed. If a"
                "certificate is already installed, try performing a factory"
                "restore to clear such settings."
            )
    elif response.status == 200:
        print("Successfully installed CA certificate.")
    else:
        raise Exception("Could not install certificate: " + response.read)
    serverCertJSON = {}
    serverCertJSON["CertificateString"] = (
        serverKeyDump.decode() + serverCertDump.decode()
    )
    serverCertificateUri = {}
    serverCertificateUri["@odata.id"] = (
        "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/1"
    )
    serverCertJSON["CertificateUri"] = serverCertificateUri
    serverCertJSON["CertificateType"] = "PEM"
    print("Replacing server certificate...")
    response = redfishObject.post(replaceCertPath, body=serverCertJSON)
    if response.status == 200:
        print("Successfully replaced server certificate.")
    else:
        raise Exception("Could not replace certificate: " + response.read)
    tlsPatchJSON = {"Oem": {"OpenBMC": {"AuthMethods": {"TLS": True}}}}
    print("Ensuring TLS authentication is enabled.")
    response = redfishObject.patch(
        "/redfish/v1/AccountService", body=tlsPatchJSON
    )
    if response.status == 200:
        print("Successfully enabled TLS authentication.")
    else:
        raise Exception("Could not enable TLS auth: " + response.read)
    redfishObject.logout()
    print("Testing redfish TLS authentication with generated certs.")
    response = requests.get(
        "https://" + host + "/redfish/v1/SessionService/Sessions",
        verify=False,
        cert=("certs/client-cert.pem", "certs/client-key.pem"),
    )
    response.raise_for_status()
    print("Redfish TLS authentication success!")
    print("Generating p12 cert file for browser authentication.")
    pkcs12Cert = crypto.PKCS12()
    pkcs12Cert.set_certificate(clientCert)
    pkcs12Cert.set_privatekey(clientKey)
    pkcs12Cert.set_ca_certificates([caCert])
    pkcs12Cert.set_friendlyname(bytes(username, encoding="utf-8"))
    with open("certs/client.p12", "wb") as f:
        f.write(pkcs12Cert.export())
        print(
            "Client p12 cert file generated and stored in"
            "./certs/client.p12."
        )
        print(
            "Copy this file to a system with a browser and install the"
            "cert into the browser."
        )
        print(
            "You will then be able to test redfish and webui"
            "authentication using this certificate."
        )
        print(
            "Note: this p12 file was generated without a password, so it"
            "can be imported easily."
        )


main()
