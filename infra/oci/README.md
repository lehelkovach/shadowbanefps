# OCI infra for ShadowbaneFPS dedicated servers

Terraform + helper scripts to manage **dev** / **release** Linux x86_64 VMs.

**Operator runbook:** [`docs/OCI_DEPLOY.md`](../../docs/OCI_DEPLOY.md)

```bash
../oci/whoami.sh      # from infra/oci: ./whoami.sh
./inventory.sh
./apply.sh plan
./apply.sh
```

Deploy builds with [`scripts/deploy-server.sh`](../../scripts/deploy-server.sh).
