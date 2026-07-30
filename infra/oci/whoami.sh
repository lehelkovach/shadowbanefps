#!/usr/bin/env bash
# Verify OCI CLI credentials without printing secret values.
# Usage: ./infra/oci/whoami.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
# shellcheck disable=SC1091
[[ -f "$ROOT/.env" ]] && set -a && source "$ROOT/.env" && set +a

export PATH="${HOME}/bin:${PATH}"

if ! command -v oci >/dev/null 2>&1; then
  echo "ERROR: oci CLI not found. Install: https://docs.oracle.com/en-us/iaas/Content/API/SDKDocs/cliinstall.htm" >&2
  exit 1
fi

missing=()
for v in OCI_CLI_USER OCI_CLI_FINGERPRINT OCI_CLI_TENANCY OCI_CLI_REGION; do
  if [[ -z "${!v:-}" ]] && [[ ! -f "${HOME}/.oci/config" ]]; then
    missing+=("$v")
  fi
done

if ((${#missing[@]})) && [[ ! -f "${HOME}/.oci/config" ]]; then
  echo "ERROR: No ~/.oci/config and missing env vars: ${missing[*]}" >&2
  echo "See .env.example and docs/OCI_DEPLOY.md" >&2
  exit 1
fi

# Prefer env-based auth when key file path is provided.
if [[ -n "${OCI_CLI_KEY_FILE:-}" ]]; then
  export OCI_CLI_AUTH="${OCI_CLI_AUTH:-api_key}"
fi

echo "== OCI region list (auth check) =="
oci iam region list --output table --query "data[].{name:name}" 2>&1 | head -40

echo
echo "== Tenancy / user (redacted IDs) =="
TENANCY="${OCI_CLI_TENANCY:-}"
USER="${OCI_CLI_USER:-}"
if [[ -z "$TENANCY" || -z "$USER" ]]; then
  # Pull from config without dumping key material
  if [[ -f "${HOME}/.oci/config" ]]; then
    PROFILE="${OCI_CLI_PROFILE:-DEFAULT}"
    TENANCY="$(awk -v p="[$PROFILE]" '
      $0==p {f=1; next} /^\[/{f=0} f && $1=="tenancy" {print $3; exit}
    ' "${HOME}/.oci/config" || true)"
    USER="$(awk -v p="[$PROFILE]" '
      $0==p {f=1; next} /^\[/{f=0} f && $1=="user" {print $3; exit}
    ' "${HOME}/.oci/config" || true)"
  fi
fi

redact() {
  local s="$1"
  if [[ ${#s} -lt 12 ]]; then echo "(unset)"; else echo "${s:0:12}…${s: -6}"; fi
}

echo "tenancy: $(redact "$TENANCY")"
echo "user:    $(redact "$USER")"
echo "region:  ${OCI_CLI_REGION:-$(awk -v p="[${OCI_CLI_PROFILE:-DEFAULT}]" '
  $0==p {f=1; next} /^\[/{f=0} f && $1=="region" {print $3; exit}
' "${HOME}/.oci/config" 2>/dev/null || echo unset)}"

if [[ -n "${OCI_COMPARTMENT_OCID:-${OCI_COMPARTMENT_ID:-}}" ]]; then
  COMP="${OCI_COMPARTMENT_OCID:-$OCI_COMPARTMENT_ID}"
  echo "compartment: $(redact "$COMP")"
  echo
  echo "== Compartment name =="
  oci iam compartment get --compartment-id "$COMP" \
    --query 'data.{"name":name,"lifecycle-state":"lifecycle-state"}' \
    --output table 2>&1 || true
else
  echo "compartment: (set OCI_COMPARTMENT_OCID to inspect)"
fi

echo
echo "OK: OCI credentials appear usable."
