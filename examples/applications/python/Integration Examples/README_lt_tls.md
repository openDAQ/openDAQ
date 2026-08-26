# LT streaming over TLS — `lt_tls_server.py` / `lt_tls_client.py`

A pair of examples demonstrating the secure (TLS) channel of the LT streaming module.

* **`lt_tls_server.py`** — publishes the signals of a reference device over `daq.lts://` only. The
  plaintext `daq.lt://` channel and the control port stay closed.
* **`lt_tls_client.py`** — discovers the server over mDNS, prints its server capabilities
  (protocol group ID, security level, prefix, port), connects to the most secure one and reads a
  signal.
* **`generate_certificates.sh`** — creates the throwaway certificates both of them need.

## 1. Certificates

```bash
./generate_certificates.sh
```

Writes `secrets/` next to the script: `ca`, `server`, `client` and `other-ca` (an unrelated
authority used by the negative demo). Existing files are kept; use `--force` to regenerate and
`--out DIR` to write elsewhere. These are development certificates — never use them anywhere else.

## 2. Server (terminal 1)

```bash
python3 lt_tls_server.py --cert secrets/server.crt --key secrets/server.key --ca secrets/ca.crt
```

| Option | Meaning |
| --- | --- |
| `--cert`, `--key` | server certificate and private key (required) |
| `--ca` | CA verifying the client certificates; required unless `--no-mutual-tls` |
| `--no-mutual-tls` | do not request a certificate from the client |

The channel always listens on the default TLS port, 7415.

Stop it with `Ctrl+C`. The device is always published as `LT TLS streaming server` with local ID
`LtTlsServer` and serial number `lt-tls-01`, so only one instance may run at a time — a second one
clashes during discovery.

## 3. Client (terminal 2)

```bash
python3 lt_tls_client.py --ca secrets/ca.crt --cert secrets/client.crt --key secrets/client.key
```

Reads the first signal the device publishes for 10 seconds and exits. On the reference device that
is the analog input of channel `RefCh0` — a 10 Hz sine sampled at 1 kHz.

| Option | Meaning |
| --- | --- |
| `--ca` | CA the client trusts when verifying the server |
| `--cert`, `--key` | client certificate and key, sent when mutual TLS is on |
| `--no-mutual-tls` | do not send a client certificate |
| `--no-verify-server-cert` | encrypt without authenticating the server (no client certificate is sent either) |
| `--host HOST[:PORT]` | connect directly, skipping discovery |
| `--duration SECONDS` | how long to read (default 10) |
| `--demo-negative` | run the failing-configuration matrix instead of reading data |
| `--other-ca` | the untrusted CA for that matrix (default: `other-ca.crt` next to `--ca`) |

### Negative demo

```bash
python3 lt_tls_client.py --ca secrets/ca.crt --cert secrets/client.crt --key secrets/client.key --demo-negative
```

Tries six client configurations against a running server and prints why each one is accepted or
rejected: valid secrets, an untrusted CA, mutual TLS turned off, server verification turned off, no
CA configured, and a plaintext `daq.lt://` connection to the TLS port. Exits non-zero if any of them
behaves unexpectedly. The expectations assume a server with mutual TLS enabled (the default).

## Running against a build tree

Both scripts follow the same convention as the GUI example: if `import opendaq` resolves to the
installed Python package they use its module directory, and otherwise they load modules from the
current directory. So either use an installed `opendaq` package, or copy the two scripts (plus
`generate_certificates.sh` and `secrets/`) into the `bin` directory of a build and run them from
there.