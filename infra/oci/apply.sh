#!/usr/bin/env bash
# Idempotent Terraform apply for shadowbanefps OCI infra.
# Reuses state; does not recreate healthy tagged VMs (lifecycle ignore_changes).
#
# Usage:
#   ./infra/oci/apply.sh            # plan + apply
#   ./infra/oci/apply.sh plan       # plan only
#   ./infra/oci/apply.sh destroy    # destroy (requires CONFIRM=yes)
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$DIR/../.." && pwd)"
# shellcheck disable=SC1091
[[ -f "$ROOT/.env" ]] && set -a && source "$ROOT/.env" && set +a
export PATH="${HOME}/bin:${PATH}"

cd "$DIR"

if ! command -v terraform >/dev/null 2>&1; then
  echo "ERROR: terraform not on PATH" >&2
  exit 1
fi

ACTION="${1:-apply}"

# Map common env names into TF_VAR_* if not already set.
export TF_VAR_oci_region="${TF_VAR_oci_region:-${OCI_CLI_REGION:-${OCI_REGION:-}}}"
export TF_VAR_compartment_ocid="${TF_VAR_compartment_ocid:-${OCI_COMPARTMENT_OCID:-${OCI_COMPARTMENT_ID:-}}}"

if [[ -n "${SSH_PUBLIC_KEY_FILE:-}" && -f "${SSH_PUBLIC_KEY_FILE}" ]]; then
  export TF_VAR_ssh_public_key="${TF_VAR_ssh_public_key:-$(cat "$SSH_PUBLIC_KEY_FILE")}"
elif [[ -n "${SSH_PUBLIC_KEY:-}" ]]; then
  export TF_VAR_ssh_public_key="${TF_VAR_ssh_public_key:-$SSH_PUBLIC_KEY}"
fi

if [[ -n "${SSH_ALLOWED_CIDRS:-}" && -z "${TF_VAR_ssh_allowed_cidrs:-}" ]]; then
  # Comma-separated -> JSON list
  export TF_VAR_ssh_allowed_cidrs
  TF_VAR_ssh_allowed_cidrs="$(python3 - <<'PY'
import json, os
print(json.dumps([c.strip() for c in os.environ["SSH_ALLOWED_CIDRS"].split(",") if c.strip()]))
PY
)"
fi

need=(TF_VAR_oci_region TF_VAR_compartment_ocid TF_VAR_ssh_public_key)
missing=()
for v in "${need[@]}"; do
  [[ -z "${!v:-}" ]] && missing+=("$v")
done
if ((${#missing[@]})); then
  echo "ERROR: missing required vars: ${missing[*]}" >&2
  echo "Copy .env.example -> .env and infra/oci/terraform.tfvars.example -> terraform.tfvars" >&2
  exit 1
fi

if [[ -z "${TF_VAR_ssh_allowed_cidrs:-}" ]]; then
  if [[ ! -f terraform.tfvars && ! -f terraform.tfvars.json ]]; then
    echo "ERROR: Set TF_VAR_ssh_allowed_cidrs or SSH_ALLOWED_CIDRS (your /32s), or put ssh_allowed_cidrs in terraform.tfvars" >&2
    exit 1
  fi
fi

terraform init -input=false

case "$ACTION" in
  plan)
    terraform plan -input=false
    ;;
  apply)
    terraform plan -input=false -out=tfplan
    terraform apply -input=false tfplan
    rm -f tfplan
    terraform output -json servers 2>/dev/null | python3 -m json.tool || terraform output
    ;;
  destroy)
    if [[ "${CONFIRM:-}" != "yes" ]]; then
      echo "Refusing destroy. Re-run with CONFIRM=yes ./infra/oci/apply.sh destroy" >&2
      exit 1
    fi
    terraform destroy -input=false -auto-approve
    ;;
  *)
    echo "Usage: $0 [plan|apply|destroy]" >&2
    exit 2
    ;;
esac
