#!/usr/bin/env bash
#
# Creates a self-signed set of TLS secrets for the "lt_tls_server.py" and "lt_tls_client.py"
# examples:
#
#   ca.crt / ca.key            the authority signing both certificates below
#   server.crt / server.key    the server identity, valid for localhost, 127.0.0.1 and ::1
#   client.crt / client.key    the client identity, presented when mutual TLS is enabled
#   other-ca.crt / other-ca.key    an unrelated authority, used by "lt_tls_client.py --demo-negative"
#
# These are throwaway development certificates. Never use them for anything but running the
# examples.

set -euo pipefail

OUT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/secrets"
DAYS=825
FORCE=0

usage() {
    cat <<USAGE
Usage: $(basename "$0") [--out DIR] [--force]

  --out DIR   where to write the secrets (default: ./secrets next to this script)
  --force     overwrite existing files instead of leaving them alone
USAGE
}

while [ $# -gt 0 ]; do
    case "$1" in
        --out) OUT_DIR="$2"; shift 2 ;;
        --force) FORCE=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; usage >&2; exit 1 ;;
    esac
done

command -v openssl >/dev/null 2>&1 || { echo "openssl is required but was not found" >&2; exit 1; }

mkdir -p "$OUT_DIR"

if [ "$FORCE" -eq 0 ] && [ -e "$OUT_DIR/ca.crt" ]; then
    echo "Secrets already exist in $OUT_DIR. Re-run with --force to regenerate them."
    exit 0
fi

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

# The certificate authority signing the server and the client
openssl req -x509 -newkey rsa:2048 -nodes -days "$DAYS" \
    -keyout "$OUT_DIR/ca.key" -out "$OUT_DIR/ca.crt" \
    -subj "/O=openDAQ LT TLS example/CN=openDAQ LT TLS example CA" \
    -addext "basicConstraints=critical,CA:TRUE" \
    -addext "keyUsage=critical,keyCertSign,cRLSign" 2>/dev/null

# An unrelated authority, so that the negative demo has a certificate the server does not trust
openssl req -x509 -newkey rsa:2048 -nodes -days "$DAYS" \
    -keyout "$OUT_DIR/other-ca.key" -out "$OUT_DIR/other-ca.crt" \
    -subj "/O=openDAQ LT TLS example/CN=openDAQ LT TLS example untrusted CA" \
    -addext "basicConstraints=critical,CA:TRUE" \
    -addext "keyUsage=critical,keyCertSign,cRLSign" 2>/dev/null

sign() {
    local name="$1" common_name="$2" extensions="$3"

    openssl req -newkey rsa:2048 -nodes \
        -keyout "$OUT_DIR/$name.key" -out "$WORK_DIR/$name.csr" \
        -subj "/O=openDAQ LT TLS example/CN=$common_name" 2>/dev/null

    printf '%s\n' "$extensions" > "$WORK_DIR/$name.ext"

    openssl x509 -req -days "$DAYS" \
        -in "$WORK_DIR/$name.csr" -out "$OUT_DIR/$name.crt" \
        -CA "$OUT_DIR/ca.crt" -CAkey "$OUT_DIR/ca.key" -CAcreateserial \
        -extfile "$WORK_DIR/$name.ext" 2>/dev/null
}

# The server certificate must cover every address a client may use to reach it, otherwise the
# hostname check fails even though the certificate itself is trusted
sign server localhost "$(cat <<'EXT'
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=DNS:localhost,IP:127.0.0.1,IP:0:0:0:0:0:0:0:1
EXT
)"

sign client "openDAQ LT TLS example client" "$(cat <<'EXT'
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature,keyEncipherment
extendedKeyUsage=clientAuth
EXT
)"

chmod 600 "$OUT_DIR"/*.key
rm -f "$OUT_DIR/ca.srl"

echo "TLS secrets written to $OUT_DIR:"
ls -1 "$OUT_DIR"
