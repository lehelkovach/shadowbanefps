# OCI Dedicated Server Deploy — ShadowbaneFPS

Provision Linux **x86_64** VMs on Oracle Cloud for the UE 5.5 headless dedicated
server (`ShadowbaneFPSServer`), deploy cooked `LinuxServer/` packages, and
connect clients to `IP:7777`.

This is **independent of KnowShowGo**. Stay in the `shadowbanefps` compartment
and only touch resources tagged `project=shadowbanefps` /
`name=shadowbanefps-dev|shadowbanefps-release`.

---

## 1. What you get

| Env | Tag / display name | Role |
| --- | --- | --- |
| **dev** | `shadowbanefps-dev` | Unstable / PR / nightly playtests |
| **release** | `shadowbanefps-release` | Stable playtest |

Defaults (Terraform):
- Ubuntu **22.04** x86_64
- Shape **VM.Standard.E4.Flex** (or E5.Flex), **2 OCPU / 8 GB**
- Boot volume **50 GB**
- NSG + UFW: **UDP 7777** (game), **TCP 22** (SSH from allowlisted CIDRs only)
- systemd: `shadowbanefps-server.service`
- Deploy root: `/opt/shadowbanefps/{current,releases/}`

Avoid ARM (`VM.Standard.A1.Flex`) unless you explicitly cook **LinuxArm64**.

---

## 2. Secrets (never commit)

Required names are listed in [`.env.example`](../.env.example). Put values in:

1. **Cursor Dashboard → Cloud Agents → Secrets** (for agents), and/or
2. A local **`.env`** (gitignored), and/or
3. `~/.oci/config` + key file on disk (mode `600`)

**Do not** commit: PEM API keys, private SSH keys, `terraform.tfvars` with real
OCIDs/keys, `.env`, or `*.tfstate`.

Minimum to create VMs:
- `OCI_CLI_USER`, `OCI_CLI_TENANCY`, `OCI_CLI_FINGERPRINT`, `OCI_CLI_KEY_FILE`,
  `OCI_CLI_REGION`
- `OCI_COMPARTMENT_OCID`
- `SSH_PUBLIC_KEY` or `SSH_PUBLIC_KEY_FILE` (**.pub only**)
- `SSH_ALLOWED_CIDRS` (your `/32`s, comma-separated)

Minimum to deploy a build:
- SSH private key available to the operator (Cursor secret or local file)
- `SB_DEV_IP` / `SB_RELEASE_IP` or SSH Host aliases `shadowbanefps-dev` /
  `shadowbanefps-release`

---

## 3. Verify credentials + inventory

```bash
# From repo root (after .env or ~/.oci/config is set)
./infra/oci/whoami.sh
./infra/oci/inventory.sh
```

`whoami` prints **redacted** tenancy/user OCIDs and confirms the CLI can list
regions. `inventory` lists existing `shadowbanefps*` instances/VCNs/NSGs and
best-effort public IPs.

If either fails with missing config: add Cursor secrets / `.env` and retry.
**Do not recreate VMs** if healthy tagged ones already exist — reuse them.

---

## 4. Create / update / destroy (Terraform)

```bash
cp .env.example .env          # fill locally — never commit
cp infra/oci/terraform.tfvars.example infra/oci/terraform.tfvars
# edit terraform.tfvars: compartment, region, ssh_public_key, ssh_allowed_cidrs

./infra/oci/apply.sh plan     # review
./infra/oci/apply.sh          # apply (idempotent)
```

Outputs include public IPs and client connect hints:

```text
dev.connect     = "x.x.x.x:7777"
release.connect = "y.y.y.y:7777"
```

### Destroy

```bash
CONFIRM=yes ./infra/oci/apply.sh destroy
```

Only destroys Terraform-managed resources in this stack. Least privilege: do not
point `compartment_ocid` at unrelated compartments.

### Re-run safety

- Re-running `apply` **updates** network rules / tags; instance `lifecycle`
  ignores cloud-init / image churn so healthy VMs are not replaced casually.
- Prefer `inventory.sh` before apply; if `shadowbanefps-dev` is already RUNNING
  with the right tags, import it into state or skip `create_dev` rather than
  creating a duplicate.

---

## 5. Cook Linux dedicated server (Windows / UE 5.5)

On the MSI (or friend’s) Windows machine with UE 5.5 + **Linux cross-compile
toolchain** (`LINUX_MULTIARCH_ROOT` set per Epic’s Linux requirements):

```powershell
$UE = "C:\Program Files\Epic Games\UE_5.5"
$PROJ = "$PWD\ShadowbaneFPS.uproject"

& "$UE\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
  -project="$PROJ" -noP4 `
  -platform=Linux -serverconfig=Development -server -noclient `
  -cook -stage -pak -archive -archivedirectory="$PWD\Dist\Server"
```

Result (typical):

```text
Dist/Server/LinuxServer/
  ShadowbaneFPSServer.sh
  ShadowbaneFPS/Binaries/Linux/ShadowbaneFPSServer
  ...
```

This agent environment has **no Unreal install** — packaging stays on Windows.
The OCI path only accepts an already-cooked `LinuxServer/` folder.

---

## 6. Deploy to DEV vs RELEASE

```bash
# DEV (unstable)
./scripts/deploy-server.sh --target dev --src Dist/Server/LinuxServer

# RELEASE (stable playtest)
./scripts/deploy-server.sh --target release --src Dist/Server/LinuxServer

# Dry run
./scripts/deploy-server.sh --target dev --src Dist/Server/LinuxServer --dry-run
```

What the script does:
1. `rsync` into `/opt/shadowbanefps/releases/<UTC-timestamp>/`
2. Points `/opt/shadowbanefps/current` at that release
3. Installs `shadowbanefps-server.service` + `/usr/local/bin/shadowbanefps-server-start`
4. `systemctl restart shadowbanefps-server`
5. Prunes old releases (keep `SB_KEEP_RELEASES`, default 5)

Suggested `~/.ssh/config`:

```sshconfig
Host shadowbanefps-dev
  HostName <DEV_PUBLIC_IP>
  User ubuntu
  IdentityFile ~/.ssh/shadowbanefps_oci

Host shadowbanefps-release
  HostName <RELEASE_PUBLIC_IP>
  User ubuntu
  IdentityFile ~/.ssh/shadowbanefps_oci
```

### Placeholder until first cook

Cloud-init leaves `/opt/shadowbanefps/PLACEHOLDER_WAITING_FOR_LINUXSERVER`.
Until a build is uploaded, systemd will fail to start (expected). After the
first successful deploy, the placeholder is removed.

You can also upload a stub tree for path validation:

```bash
mkdir -p /tmp/LinuxServer-placeholder
echo '#!/bin/bash' > /tmp/LinuxServer-placeholder/ShadowbaneFPSServer.sh
echo 'echo placeholder; sleep infinity' >> /tmp/LinuxServer-placeholder/ShadowbaneFPSServer.sh
chmod +x /tmp/LinuxServer-placeholder/ShadowbaneFPSServer.sh
./scripts/deploy-server.sh --target dev --src /tmp/LinuxServer-placeholder
```

---

## 7. systemd

Unit file in repo: [`infra/oci/systemd/shadowbanefps-server.service`](../infra/oci/systemd/shadowbanefps-server.service)

```bash
ssh shadowbanefps-dev 'sudo systemctl status shadowbanefps-server'
ssh shadowbanefps-dev 'sudo journalctl -u shadowbanefps-server -f'
ssh shadowbanefps-dev 'sudo systemctl restart shadowbanefps-server'
```

Manual start (same args the unit uses):

```bash
cd /opt/shadowbanefps/current
./ShadowbaneFPSServer.sh BrokenCitadel -log -port=7777
# or
./ShadowbaneFPS/Binaries/Linux/ShadowbaneFPSServer BrokenCitadel -log -port=7777
```

---

## 8. Ports + how to verify UDP 7777

| Port | Proto | Purpose | Who |
| --- | --- | --- | --- |
| 7777 | UDP | Unreal `GameNetDriver` | Clients / playtesters |
| 22 | TCP | SSH | Operator IPs only (`ssh_allowed_cidrs`) |

**Do not** open extra ports unless you add an explicit NSG rule.

### Verify from your laptop

```bash
# SSH works
ssh shadowbanefps-dev 'echo ok; sudo ss -ulnp | grep 7777 || true'

# UDP reachability (best-effort; UE must be listening)
# Install nmap/nc locally, then:
nc -u -v -z <PUBLIC_IP> 7777
# or
nmap -sU -p 7777 <PUBLIC_IP>
```

From a second shell on the VM while a client connects:

```bash
sudo tcpdump -ni any udp port 7777
```

If SSH works but UDP does not: check NSG rules, UFW (`sudo ufw status`), and
that systemd actually has the server process listening.

---

## 9. Client connect strings

Replace with the public IP from `terraform output` / `inventory.sh`:

```powershell
$UE = "C:\Program Files\Epic Games\UE_5.5"
$PROJ = "$PWD\ShadowbaneFPS.uproject"

# DEV
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PROJ" DEV.IP.HERE:7777 -game -log

# RELEASE
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PROJ" RELEASE.IP.HERE:7777 -game -log
```

Short form: **`open <IP>:7777`** in the Unreal console, or pass `IP:7777` as the
map/URL argument as above.

---

## 10. Rollback

Each deploy keeps prior trees under `/opt/shadowbanefps/releases/`.

```bash
# Point current at the previous release and restart
./scripts/deploy-server.sh --target dev --rollback
./scripts/deploy-server.sh --target release --rollback
```

Manual:

```bash
ssh shadowbanefps-dev
ls -lt /opt/shadowbanefps/releases
sudo ln -sfn /opt/shadowbanefps/releases/<OLDER_TS> /opt/shadowbanefps/current
sudo systemctl restart shadowbanefps-server
```

---

## 11. Layout reference

```text
infra/oci/
  versions.tf variables.tf data.tf network.tf compute.tf outputs.tf
  terraform.tfvars.example
  apply.sh whoami.sh inventory.sh
  cloud-init/server.yaml.tftpl
  systemd/shadowbanefps-server.service
  systemd/shadowbanefps-server-start.sh
scripts/deploy-server.sh
docs/OCI_DEPLOY.md
.env.example
```

GitHub Actions cook/deploy workflow: **out of scope** until requested.
