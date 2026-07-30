variable "oci_region" {
  description = "OCI region (e.g. us-ashburn-1, us-phoenix-1)."
  type        = string
}

variable "compartment_ocid" {
  description = "Compartment OCID where shadowbanefps VCN/VMs live."
  type        = string
}

variable "ssh_public_key" {
  description = "SSH public key installed on VMs (contents of *.pub). Never the private key."
  type        = string
  sensitive   = true
}

variable "ssh_allowed_cidrs" {
  description = "CIDRs allowed to SSH (TCP 22). Prefer your home/office IPs, not 0.0.0.0/0."
  type        = list(string)
}

variable "game_udp_cidrs" {
  description = "CIDRs allowed to reach Unreal game UDP 7777. Default open for playtests."
  type        = list(string)
  default     = ["0.0.0.0/0"]
}

variable "vcn_cidr" {
  type    = string
  default = "10.55.0.0/16"
}

variable "subnet_cidr" {
  type    = string
  default = "10.55.1.0/24"
}

variable "availability_domain_index" {
  description = "Index into compartment ADs (0 = first)."
  type        = number
  default     = 0
}

variable "vm_shape" {
  description = "x86_64 flex shape. Prefer E4/E5 Flex; avoid ARM unless cooking LinuxArm64."
  type        = string
  default     = "VM.Standard.E4.Flex"
}

variable "vm_ocpus" {
  type    = number
  default = 2
}

variable "vm_memory_gb" {
  type    = number
  default = 8
}

variable "boot_volume_gb" {
  type    = number
  default = 50
}

variable "ubuntu_image_ocid" {
  description = "Optional pin to a specific Ubuntu 22.04 x86_64 image OCID. Empty = auto-discover latest."
  type        = string
  default     = ""
}

variable "create_dev" {
  description = "Create/manage the DEV VM (tag name=shadowbanefps-dev)."
  type        = bool
  default     = true
}

variable "create_release" {
  description = "Create/manage the RELEASE VM (tag name=shadowbanefps-release)."
  type        = bool
  default     = true
}

variable "project_tag" {
  type    = string
  default = "shadowbanefps"
}

variable "assign_public_ip" {
  type    = bool
  default = true
}
