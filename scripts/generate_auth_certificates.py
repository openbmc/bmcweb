#!/usr/bin/env python3
"""
Script to generate certificates for a CA, server, and client allowing for
client authentication using mTLS certificates. This can then be used to test
mTLS client authentication for Redfish and WebUI.
"""

import argparse
import datetime
import errno
import ipaddress
import os
import socket
import time

import httpx
from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.serialization import (
    load_pem_private_key,
    pkcs12,
)
from cryptography.x509.oid import NameOID

replaceCertPath = "/redfish/v1/CertificateService/Actions/CertificateService.ReplaceCertificate"


class RedfishSessionContext:
    def __init__(self, client, username="root", password="0penBmc"):
        self.client = client
        self.session_uri = None
        self.x_auth_token = None
        self.username = username
        self.password = password

    def __enter__(self):
        r = self.client.post(
            "/redfish/v1/SessionService/Sessions",
            json={
                "UserName": self.username,
                "Password": self.password,
                "Context": f"pythonscript::{os.path.basename(__file__)}",
            },
            headers={"content-type": "application/json"},
        )
        r.raise_for_status()
        self.x_auth_token = r.headers["x-auth-token"]
        self.session_uri = r.headers["location"]
        return self

    def __exit__(self, type, value, traceback):
        if not self.session_uri:
            return
        r = self.client.delete(self.session_uri)
        r.raise_for_status()


def generateCA():
    private_key = ec.generate_private_key(ec.SECP256R1())
    public_key = private_key.public_key()
    builder = x509.CertificateBuilder()

    name = x509.Name(
        [
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, "OpenBMC"),
            x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, "bmcweb"),
            x509.NameAttribute(NameOID.COMMON_NAME, "Test CA"),
        ]
    )
    builder = builder.subject_name(name)
    builder = builder.issuer_name(name)

    builder = builder.not_valid_before(
        datetime.datetime(1970, 1, 1, 0, 0, tzinfo=datetime.timezone.utc)
    )
    builder = builder.not_valid_after(
        datetime.datetime(2070, 1, 1, 0, 0, tzinfo=datetime.timezone.utc)
    )
    builder = builder.serial_number(x509.random_serial_number())
    builder = builder.public_key(public_key)

    basic_constraints = x509.BasicConstraints(ca=True, path_length=None)
    builder = builder.add_extension(basic_constraints, critical=True)

    usage = x509.KeyUsage(
        content_commitment=False,
        crl_sign=True,
        data_encipherment=False,
        decipher_only=False,
        digital_signature=False,
        encipher_only=False,
        key_agreement=False,
        key_cert_sign=True,
        key_encipherment=False,
    )
    builder = builder.add_extension(usage, critical=False)

    auth_key = x509.AuthorityKeyIdentifier.from_issuer_public_key(public_key)

    builder = builder.add_extension(auth_key, critical=False)

    root_cert = builder.sign(
        private_key=private_key, algorithm=hashes.SHA256()
    )

    return private_key, root_cert


def signCsr(csr, ca_key):
    csr.sign(ca_key, algorithm=hashes.SHA256())
    return


def generate_client_key_and_cert(commonName, ca_cert, ca_key):
    private_key = ec.generate_private_key(ec.SECP256R1())
    public_key = private_key.public_key()
    builder = x509.CertificateBuilder()

    builder = builder.subject_name(
        x509.Name(
            [
                x509.NameAttribute(NameOID.COUNTRY_NAME, "US"),
                x509.NameAttribute(
                    NameOID.STATE_OR_PROVINCE_NAME, "California"
                ),
                x509.NameAttribute(NameOID.LOCALITY_NAME, "San Francisco"),
                x509.NameAttribute(NameOID.ORGANIZATION_NAME, "OpenBMC"),
                x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, "bmcweb"),
                x509.NameAttribute(NameOID.COMMON_NAME, commonName),
            ]
        )
    )

    builder = builder.issuer_name(ca_cert.subject)
    builder = builder.public_key(public_key)
    builder = builder.serial_number(x509.random_serial_number())
    builder = builder.not_valid_before(
        datetime.datetime(1970, 1, 1, 0, 0, tzinfo=datetime.timezone.utc)
    )
    builder = builder.not_valid_after(
        datetime.datetime(2070, 1, 1, 0, 0, tzinfo=datetime.timezone.utc)
    )

    usage = x509.KeyUsage(
        content_commitment=False,
        crl_sign=False,
        data_encipherment=False,
        decipher_only=False,
        digital_signature=True,
        encipher_only=False,
        key_agreement=True,
        key_cert_sign=False,
        key_encipherment=False,
    )
    builder = builder.add_extension(usage, critical=False)

    exusage = x509.ExtendedKeyUsage([x509.oid.ExtendedKeyUsageOID.CLIENT_AUTH])
    builder = builder.add_extension(exusage, critical=True)

    auth_key = x509.AuthorityKeyIdentifier.from_issuer_public_key(public_key)
    builder = builder.add_extension(auth_key, critical=False)

    signed = builder.sign(private_key=ca_key, algorithm=hashes.SHA256())

    return private_key, signed


def generateServerCert(url, ca_key, ca_cert, csr):
    builder = x509.CertificateBuilder()

    builder = builder.subject_name(csr.subject)
    builder = builder.issuer_name(ca_cert.subject)
    builder = builder.public_key(csr.public_key())
    builder = builder.serial_number(x509.random_serial_number())
    builder = builder.not_valid_before(
        datetime.datetime(1970, 1, 1, 0, 0, tzinfo=datetime.timezone.utc)
    )
    builder = builder.not_valid_after(
        datetime.datetime(2070, 1, 1, 0, 0, tzinfo=datetime.timezone.utc)
    )

    usage = x509.KeyUsage(
        content_commitment=False,
        crl_sign=False,
        data_encipherment=False,
        decipher_only=False,
        digital_signature=True,
        encipher_only=False,
        key_agreement=False,
        key_cert_sign=True,
        key_encipherment=True,
    )
    builder = builder.add_extension(usage, critical=True)

    exusage = x509.ExtendedKeyUsage([x509.oid.ExtendedKeyUsageOID.SERVER_AUTH])
    builder = builder.add_extension(exusage, critical=True)

    san_list = [x509.DNSName("localhost")]
    try:
        value = ipaddress.ip_address(url)
        san_list.append(x509.IPAddress(value))
    except ValueError:
        san_list.append(x509.DNSName(url))

    altname = x509.SubjectAlternativeName(san_list)
    builder = builder.add_extension(altname, critical=True)
    basic_constraints = x509.BasicConstraints(ca=False, path_length=None)
    builder = builder.add_extension(basic_constraints, critical=True)

    builder = builder.add_extension(
        x509.SubjectKeyIdentifier.from_public_key(ca_key.public_key()),
        critical=False,
    )
    authkeyident = x509.AuthorityKeyIdentifier.from_issuer_public_key(
        ca_key.public_key()
    )
    builder = builder.add_extension(authkeyident, critical=False)

    signed = builder.sign(private_key=ca_key, algorithm=hashes.SHA256())

    return signed


def generateCsr(
    redfish_session,
    commonName,
    manager_uri,
):
    try:
        socket.inet_aton(commonName)
        commonName = "IP: " + commonName
    except socket.error:
        commonName = "DNS: " + commonName

    CSRRequest = {
        "CommonName": commonName,
        "City": "San Fransisco",
        "Country": "US",
        "Organization": "",
        "OrganizationalUnit": "",
        "State": "CA",
        "CertificateCollection": {
            "@odata.id": f"{manager_uri}/NetworkProtocol/HTTPS/Certificates",
        },
        "AlternativeNames": [
            commonName,
            "DNS: localhost",
            "IP: 127.0.0.1",
        ],
    }

    response = redfish_session.post(
        "/redfish/v1/CertificateService/Actions/CertificateService.GenerateCSR",
        json=CSRRequest,
    )
    response.raise_for_status()

    csrString = response.json()["CSRString"]
    csr = x509.load_pem_x509_csr(csrString.encode())
    if not csr.is_signature_valid:
        raise Exception("CSR was not valid")
    return csr


def install_ca_cert(redfish_session, ca_cert_dump, manager_uri):
    ca_certJSON = {
        "CertificateString": ca_cert_dump.decode(),
        "CertificateType": "PEM",
    }
    ca_certPath = f"{manager_uri}/Truststore/Certificates"
    print("Attempting to install CA certificate to BMC.")

    response = redfish_session.post(ca_certPath, json=ca_certJSON)
    if response.status_code == 500:
        print(
            "An existing CA certificate is likely already installed."
            " Replacing..."
        )
        ca_certJSON["CertificateUri"] = {
            "@odata.id": ca_certPath + "/1",
        }

        response = redfish_session.post(replaceCertPath, json=ca_certJSON)
        if response.status_code == 200:
            print("Successfully replaced existing CA certificate.")
        else:
            raise Exception(
                "Could not install or replace CA certificate."
                "Please check if a certificate is already installed. If a"
                "certificate is already installed, try performing a factory"
                "restore to clear such settings."
            )
    response.raise_for_status()
    print("Successfully installed CA certificate.")


def install_server_cert(redfish_session, manager_uri, server_cert_dump):

    server_cert_json = {
        "CertificateString": server_cert_dump.decode(),
        "CertificateUri": {
            "@odata.id": f"{manager_uri}/NetworkProtocol/HTTPS/Certificates/1",
        },
        "CertificateType": "PEM",
    }

    print("Replacing server certificate...")
    response = redfish_session.post(replaceCertPath, json=server_cert_json)
    if response.status_code == 200:
        print("Successfully replaced server certificate.")
    else:
        raise Exception(f"Could not replace certificate: {response.json()}")

    tls_patch_json = {"Oem": {"OpenBMC": {"AuthMethods": {"TLS": True}}}}
    print("Ensuring TLS authentication is enabled.")
    response = redfish_session.patch(
        "/redfish/v1/AccountService", json=tls_patch_json
    )
    if response.status_code == 200:
        print("Successfully enabled TLS authentication.")
    else:
        raise Exception("Could not enable TLS auth: " + response.read)


def generate_pk12(certs_dir, key, client_cert, username):
    print("Generating p12 cert file for browser authentication.")
    p12 = pkcs12.serialize_key_and_certificates(
        username.encode(),
        key,
        client_cert,
        None,
        serialization.NoEncryption(),
    )
    with open(os.path.join(certs_dir, "client.p12"), "wb") as f:
        f.write(p12)


def test_mtls_auth(url, certs_dir):
    response = httpx.get(
        f"https://{url}/redfish/v1/SessionService/Sessions",
        verify=os.path.join(certs_dir, "CA-cert.cer"),
        cert=(
            os.path.join(certs_dir, "client-cert.pem"),
            os.path.join(certs_dir, "client-key.pem"),
        ),
    )
    response.raise_for_status()


def setup_server_cert(
    redfish_session,
    ca_cert_dump,
    certs_dir,
    client_key,
    client_cert,
    username,
    url,
    ca_key,
    ca_cert,
):
    service_root = redfish_session.get("/redfish/v1/")
    service_root.raise_for_status()

    manager_uri = service_root.json()["Links"]["ManagerProvidingService"][
        "@odata.id"
    ]

    install_ca_cert(redfish_session, ca_cert_dump, manager_uri)
    generate_pk12(certs_dir, client_key, client_cert, username)

    csr = generateCsr(
        redfish_session,
        url,
        manager_uri,
    )
    serverCert = generateServerCert(
        url,
        ca_key,
        ca_cert,
        csr,
    )
    server_cert_dump = serverCert.public_bytes(
        encoding=serialization.Encoding.PEM
    )
    with open(os.path.join(certs_dir, "server-cert.pem"), "wb") as f:
        f.write(server_cert_dump)
        print("Server cert generated.")

    install_server_cert(redfish_session, manager_uri, server_cert_dump)


def generate_and_load_certs(url, username, password):
    certs_dir = os.path.expanduser("~/certs")
    print(f"Writing certs to {certs_dir}")
    try:
        print("Making certs directory.")
        os.mkdir(certs_dir)
    except OSError as error:
        if error.errno != errno.EEXIST:
            raise

    ca_cert_filename = os.path.join(certs_dir, "CA-cert.cer")
    ca_key_filename = os.path.join(certs_dir, "CA-key.pem")
    if not os.path.exists(ca_cert_filename):
        ca_key, ca_cert = generateCA()

        ca_key_dump = ca_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        )
        ca_cert_dump = ca_cert.public_bytes(
            encoding=serialization.Encoding.PEM
        )

        with open(ca_cert_filename, "wb") as f:
            f.write(ca_cert_dump)
            print("CA cert generated.")
        with open(ca_key_filename, "wb") as f:
            f.write(ca_key_dump)
            print("CA key generated.")

    with open(ca_cert_filename, "rb") as ca_cert_file:
        ca_cert_dump = ca_cert_file.read()
    ca_cert = x509.load_pem_x509_certificate(ca_cert_dump)

    with open(ca_key_filename, "rb") as ca_key_file:
        ca_key_dump = ca_key_file.read()
    ca_key = load_pem_private_key(ca_key_dump, None)

    client_key, client_cert = generate_client_key_and_cert(
        username, ca_cert, ca_key
    )
    client_key_dump = client_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.TraditionalOpenSSL,
        encryption_algorithm=serialization.NoEncryption(),
    )

    with open(os.path.join(certs_dir, "client-key.pem"), "wb") as f:
        f.write(client_key_dump)
        print("Client key generated.")
    client_cert_dump = client_cert.public_bytes(
        encoding=serialization.Encoding.PEM
    )

    with open(os.path.join(certs_dir, "client-cert.pem"), "wb") as f:
        f.write(client_cert_dump)
        print("Client cert generated.")

    print(f"Connecting to {url}")
    with httpx.Client(
        base_url=f"https://{url}", verify=False, follow_redirects=False
    ) as redfish_session:
        with RedfishSessionContext(
            redfish_session, username, password
        ) as rf_session:
            redfish_session.headers["X-Auth-Token"] = rf_session.x_auth_token
            setup_server_cert(
                redfish_session,
                ca_cert_dump,
                certs_dir,
                client_key,
                client_cert,
                username,
                url,
                ca_key,
                ca_cert,
            )

    print("Testing redfish TLS authentication with generated certs.")

    time.sleep(2)
    test_mtls_auth(url, certs_dir)
    print("Redfish TLS authentication success!")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--username",
        help="Username to connect with",
        default="root",
    )
    parser.add_argument(
        "--password",
        help="Password for user in order to install certs over Redfish.",
        default="0penBmc",
    )
    parser.add_argument("host", help="Host to connect to")

    args = parser.parse_args()
    generate_and_load_certs(args.host, args.username, args.password)


if __name__ == "__main__":
    main()
