#!/usr/bin/env bash
# Deploy a cooked Unreal LinuxServer package to DEV or RELEASE OCI host.
#
# Safe flow:
#   1) rsync into /opt/shadowbanefps/releases/<timestamp>/
#   2) flip current symlink
#   3) install/refresh systemd unit + start helper
#   4) restart shadowbanefps-server.service
#   5) keep previous release for rollback
#
# Usage:
#   ./scripts/deploy-server.sh --target dev --src Dist/Server/LinuxServer
#   ./scripts/deploy-server.sh --target release --src Dist/Server/LinuxServer
#   ./scripts/deploy-server.sh --target dev --src Dist/Server/LinuxServer --rollback
#   ./scripts/deploy-server.sh --target dev --src Dist/Server/LinuxServer --dry-run
#
# Host selection (first match wins):
#   SB_DEPLOY_HOST_DEV / SB_DEPLOY_HOST_RELEASE
#   SSH config Host aliases: shadowbanefps-dev / shadowbanefps-release
#   SB_DEV_IP / SB_RELEASE_IP (+ SB_SSH_USER, default ubuntu)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
[[ -f "$ROOT/.env" ]] && set -a && source "$ROOT/.env" && set +a

TARGET=""
SRC=""
DRY_RUN=0
DO_ROLLBACK=0
KEEP_RELEASES="${SB_KEEP_RELEASES:-5}"
REMOTE_ROOT="${SB_REMOTE_ROOT:-/opt/shadowbanefps}"
SSH_USER="${SB_SSH_USER:-ubuntu}"
SSH_IDENTITY="${SB_SSH_IDENTITY:-${SSH_IDENTITY_FILE:-}}"

usage() {
  sed -n '2,20p' "$0" | tr -d '#'
  exit 2
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --target) TARGET="$2"; shift 2 ;;
    --src) SRC="$2"; shift 2 ;;
    --dry-run) DRY_RUN=1; shift ;;
    --rollback) DO_ROLLBACK=1; shift ;;
    -h|--help) usage ;;
    *) echo "Unknown arg: $1" >&2; usage ;;
  esac
done

[[ -n "$TARGET" ]] || { echo "--target dev|release required" >&2; exit 2; }
TARGET="$(echo "$TARGET" | tr '[:upper:]' '[:lower:]')"
[[ "$TARGET" == "dev" || "$TARGET" == "release" ]] || { echo "target must be dev or release" >&2; exit 2; }

resolve_host() {
  local t="$1"
  if [[ "$t" == "dev" ]]; then
    if [[ -n "${SB_DEPLOY_HOST_DEV:-}" ]]; then echo "$SB_DEPLOY_HOST_DEV"; return; fi
    if [[ -n "${SB_DEV_IP:-}" ]]; then echo "${SSH_USER}@${SB_DEV_IP}"; return; fi
    echo "shadowbanefps-dev"
  else
    if [[ -n "${SB_DEPLOY_HOST_RELEASE:-}" ]]; then echo "$SB_DEPLOY_HOST_RELEASE"; return; fi
    if [[ -n "${SB_RELEASE_IP:-}" ]]; then echo "${SSH_USER}@${SB_RELEASE_IP}"; return; fi
    echo "shadowbanefps-release"
  fi
}

HOST="$(resolve_host "$TARGET")"

SSH_OPTS=(-o StrictHostKeyChecking=accept-new -o ServerAliveInterval=30)
if [[ -n "$SSH_IDENTITY" ]]; then
  SSH_OPTS+=(-i "$SSH_IDENTITY")
fi

ssh_cmd() { ssh "${SSH_OPTS[@]}" "$HOST" "$@"; }
rsync_ssh() {
  local rsh="ssh"
  for o in "${SSH_OPTS[@]}"; do rsh+=" $o"; done
  echo "$rsh"
}

echo "Target=$TARGET Host=$HOST RemoteRoot=$REMOTE_ROOT"

if (( DO_ROLLBACK )); then
  echo "Rolling back to previous release on $HOST ..."
  ssh_cmd "bash -s" <<'EOS'
set -euo pipefail
ROOT=/opt/shadowbanefps
cd "$ROOT/releases"
mapfile -t rels < <(ls -1dt */ 2>/dev/null | sed 's#/##' || true)
if (( ${#rels[@]} < 2 )); then
  echo "Need at least 2 releases to roll back; found ${#rels[@]}" >&2
  exit 1
fi
prev="${rels[1]}"
echo "Pointing current -> releases/$prev"
ln -sfn "$ROOT/releases/$prev" "$ROOT/current"
sudo systemctl restart shadowbanefps-server.service
sudo systemctl --no-pager --full status shadowbanefps-server.service | head -40
EOS
  exit 0
fi

[[ -n "$SRC" ]] || { echo "--src path/to/LinuxServer required (unless --rollback)" >&2; exit 2; }
[[ -d "$SRC" ]] || { echo "Source dir not found: $SRC" >&2; exit 1; }

# Accept either LinuxServer/ itself or a parent that contains it.
if [[ -d "$SRC/LinuxServer" ]]; then
  SRC="$SRC/LinuxServer"
fi
if [[ ! -e "$SRC/ShadowbaneFPSServer.sh" && ! -d "$SRC/ShadowbaneFPS" && ! -d "$SRC/Binaries" ]]; then
  echo "WARNING: $SRC does not look like a UE LinuxServer tree (no ShadowbaneFPSServer.sh / ShadowbaneFPS / Binaries)." >&2
  echo "Continuing anyway — placeholder deploys are allowed." >&2
fi

TS="$(date -u +%Y%m%dT%H%M%SZ)"
REMOTE_REL="$REMOTE_ROOT/releases/$TS"

echo "Will upload -> $HOST:$REMOTE_REL"

if (( DRY_RUN )); then
  echo "[dry-run] rsync -az --delete $SRC/ $HOST:$REMOTE_REL/"
  echo "[dry-run] flip symlink + restart systemd"
  exit 0
fi

# Ensure remote layout + unit files from this repo.
UNIT_SRC="$ROOT/infra/oci/systemd/shadowbanefps-server.service"
START_SRC="$ROOT/infra/oci/systemd/shadowbanefps-server-start.sh"
[[ -f "$UNIT_SRC" && -f "$START_SRC" ]] || { echo "Missing systemd files under infra/oci/systemd" >&2; exit 1; }

ssh_cmd "sudo mkdir -p $REMOTE_ROOT/releases $REMOTE_ROOT/shared/logs && sudo chown -R \$(whoami):\$(whoami) $REMOTE_ROOT || true"
ssh_cmd "mkdir -p '$REMOTE_REL'"

RSYNC_RSH="$(rsync_ssh)"
rsync -az --delete -e "$RSYNC_RSH" \
  "$SRC/" "$HOST:$REMOTE_REL/"

# Install start helper + unit (idempotent), flip symlink, restart.
scp "${SSH_OPTS[@]}" "$START_SRC" "$HOST:/tmp/shadowbanefps-server-start"
scp "${SSH_OPTS[@]}" "$UNIT_SRC" "$HOST:/tmp/shadowbanefps-server.service"

ssh_cmd "bash -s" <<EOS
set -euo pipefail
ROOT="$REMOTE_ROOT"
TS="$TS"
KEEP="$KEEP_RELEASES"
sudo install -m 0755 /tmp/shadowbanefps-server-start /usr/local/bin/shadowbanefps-server-start
sudo install -m 0644 /tmp/shadowbanefps-server.service /etc/systemd/system/shadowbanefps-server.service
# Ensure sbfps can read the tree; fall back to ubuntu if user missing.
if id sbfps >/dev/null 2>&1; then
  sudo chown -R sbfps:sbfps "\$ROOT/releases/\$TS"
else
  echo "WARNING: user sbfps missing; leaving ownership as \$(whoami)" >&2
fi
ln -sfn "\$ROOT/releases/\$TS" "\$ROOT/current"
sudo systemctl daemon-reload
sudo systemctl enable shadowbanefps-server.service
if sudo systemctl restart shadowbanefps-server.service; then
  sleep 2
  sudo systemctl --no-pager --full status shadowbanefps-server.service | head -50
else
  echo "WARNING: systemd restart failed (expected if binary not yet present)." >&2
  sudo journalctl -u shadowbanefps-server.service -n 40 --no-pager || true
fi
# Prune old releases
cd "\$ROOT/releases"
mapfile -t rels < <(ls -1dt */ 2>/dev/null | sed 's#/##' || true)
if (( \${#rels[@]} > KEEP )); then
  for old in "\${rels[@]:KEEP}"; do
    echo "Pruning old release \$old"
    rm -rf "\$old"
  done
fi
rm -f "\$ROOT/PLACEHOLDER_WAITING_FOR_LINUXSERVER" || true
echo "Active: \$(readlink -f \$ROOT/current)"
EOS

echo
echo "Deploy complete."
echo "  Client connect: <PUBLIC_IP>:7777"
echo "  Rollback: $0 --target $TARGET --rollback"
echo "  Logs: ssh $HOST 'sudo journalctl -u shadowbanefps-server -f'"
