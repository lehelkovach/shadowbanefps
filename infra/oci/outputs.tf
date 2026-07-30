output "vcn_id" {
  value = oci_core_vcn.game.id
}

output "nsg_id" {
  value = oci_core_network_security_group.game.id
}

output "subnet_id" {
  value = oci_core_subnet.public.id
}

output "ubuntu_image_id" {
  value = local.ubuntu_image_id
}

output "servers" {
  description = "Public IPs and OCIDs for tagged shadowbanefps VMs."
  value = {
    for key, inst in oci_core_instance.server :
    key => {
      display_name = inst.display_name
      id           = inst.id
      public_ip    = try(inst.public_ip, null)
      private_ip   = try(inst.private_ip, null)
      connect      = try(inst.public_ip, null) != null ? "${inst.public_ip}:7777" : null
      env          = key
    }
  }
}

output "client_connect_hint" {
  value = {
    for key, inst in oci_core_instance.server :
    key => (
      try(inst.public_ip, null) != null
      ? "UnrealEditor.exe ShadowbaneFPS.uproject ${inst.public_ip}:7777 -game -log"
      : "no public IP yet"
    )
  }
}
