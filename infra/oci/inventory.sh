#!/usr/bin/env bash
# Inventory existing shadowbanefps-* OCI resources (read-only).
# Tags: freeform name=shadowbanefps-dev|shadowbanefps-release|shadowbanefps-vcn|...
# Usage: ./infra/oci/inventory.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
# shellcheck disable=SC1091
[[ -f "$ROOT/.env" ]] && set -a && source "$ROOT/.env" && set +a
export PATH="${HOME}/bin:${PATH}"

COMP="${OCI_COMPARTMENT_OCID:-${OCI_COMPARTMENT_ID:-}}"
REGION="${OCI_CLI_REGION:-${OCI_REGION:-}}"

if [[ -z "$COMP" ]]; then
  echo "ERROR: Set OCI_COMPARTMENT_OCID (see .env.example)" >&2
  exit 1
fi
if ! command -v oci >/dev/null 2>&1; then
  echo "ERROR: oci CLI not found" >&2
  exit 1
fi

echo "== Compute instances with freeform tag project=shadowbanefps (or name~shadowbanefps) =="
oci compute instance list \
  --compartment-id "$COMP" \
  ${REGION:+--region "$REGION"} \
  --lifecycle-state RUNNING \
  --all \
  --query "data[?\"freeform-tags\".project=='shadowbanefps' || starts_with(\"display-name\", 'shadowbanefps')].{
    name:\"display-name\",
    id:id,
    shape:shape,
    state:\"lifecycle-state\",
    tags:\"freeform-tags\"
  }" \
  --output table 2>&1 || true

echo
echo "== All shadowbanefps* display-name instances (any state) =="
oci compute instance list \
  --compartment-id "$COMP" \
  ${REGION:+--region "$REGION"} \
  --all \
  --query "data[?starts_with(\"display-name\", 'shadowbanefps')].{
    name:\"display-name\",
    id:id,
    state:\"lifecycle-state\",
    shape:shape
  }" \
  --output table 2>&1 || true

echo
echo "== VCNs named shadowbanefps* =="
oci network vcn list \
  --compartment-id "$COMP" \
  ${REGION:+--region "$REGION"} \
  --all \
  --query "data[?starts_with(\"display-name\", 'shadowbanefps')].{
    name:\"display-name\",
    id:id,
    cidr:\"cidr-blocks\"
  }" \
  --output table 2>&1 || true

echo
echo "== NSGs named shadowbanefps* =="
oci network nsg list \
  --compartment-id "$COMP" \
  ${REGION:+--region "$REGION"} \
  --all \
  --query "data[?starts_with(\"display-name\", 'shadowbanefps')].{
    name:\"display-name\",
    id:id
  }" \
  --output table 2>&1 || true

echo
echo "== Public IPs for tagged VMs (best-effort) =="
# Resolve public IP via VNIC for each matching instance
mapfile -t IDS < <(oci compute instance list \
  --compartment-id "$COMP" \
  ${REGION:+--region "$REGION"} \
  --lifecycle-state RUNNING \
  --all \
  --query "data[?starts_with(\"display-name\", 'shadowbanefps')].id" \
  --raw-output 2>/dev/null | tr -d '[]",' | xargs -n1 echo 2>/dev/null || true)

for id in "${IDS[@]:-}"; do
  [[ -z "$id" ]] && continue
  name="$(oci compute instance get --instance-id "$id" --query 'data."display-name"' --raw-output 2>/dev/null || echo unknown)"
  vnic="$(oci compute instance list-vnics --instance-id "$id" --query 'data[0].id' --raw-output 2>/dev/null || true)"
  pip="$(oci compute instance list-vnics --instance-id "$id" --query 'data[0]."public-ip"' --raw-output 2>/dev/null || true)"
  echo "  $name  public_ip=${pip:-none}  connect=${pip:-?}:7777"
done

echo
echo "Done. Prefer Terraform (infra/oci) for create/update; this script is inventory-only."
