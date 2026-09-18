#!/usr/bin/bash
# Generates opendaq.json — a machine-readable description of the openDAQ C++
# API (see README.md).  rtgen discovers every core header that declares
# interfaces or class factories, parses them (retrying unparseable headers
# from a sanitized copy), and links the result: resolves vtable slots and
# scans the error-code and enum-only headers.
set -eu

BINDINGS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$BINDINGS_DIR/../.." && pwd)"

RTGEN="$REPO_DIR/shared/tools/RTGen/bin/rtgen.exe"
if command -v mono &>/dev/null; then
    RTGEN="mono $RTGEN"
fi

# Run from the repository root so that header provenance recorded in the JSON
# stays repository-relative.
cd "$REPO_DIR"
exec $RTGEN --language=json --config --namespace=daq --source=core \
     --outputDir=bindings/json --output=opendaq --extension=.json
