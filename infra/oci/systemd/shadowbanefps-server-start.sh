#!/usr/bin/env bash
# Installed to /usr/local/bin/shadowbanefps-server-start on the VM.
# Locates the cooked Linux dedicated-server binary under /opt/shadowbanefps/current.
set -euo pipefail

ROOT="${SHADOWBANE_ROOT:-/opt/shadowbanefps/current}"
MAP_NAME="${MAP_NAME:-BrokenCitadel}"
# shellcheck disable=SC2086
SERVER_ARGS=${SERVER_ARGS:--log -port=7777}

cd "$ROOT"

candidates=(
  "ShadowbaneFPS/Binaries/Linux/ShadowbaneFPSServer"
  "Binaries/Linux/ShadowbaneFPSServer"
  "ShadowbaneFPSServer.sh"
  "ShadowbaneFPS/ShadowbaneFPSServer.sh"
)

for rel in "${candidates[@]}"; do
  if [[ -x "$ROOT/$rel" ]]; then
    echo "Starting dedicated server: $ROOT/$rel $MAP_NAME $SERVER_ARGS"
    # Intentionally unquoted SERVER_ARGS so multiple flags expand.
    # shellcheck disable=SC2086
    exec "$ROOT/$rel" "$MAP_NAME" $SERVER_ARGS
  fi
done

cat >&2 <<EOF
No ShadowbaneFPSServer binary found under $ROOT.
Upload a cooked LinuxServer/ build with scripts/deploy-server.sh first.
Expected layout (UAT archive):
  LinuxServer/ShadowbaneFPS/Binaries/Linux/ShadowbaneFPSServer
  or LinuxServer/ShadowbaneFPSServer.sh
EOF
exit 1
