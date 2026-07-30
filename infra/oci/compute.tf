locals {
  cloud_init = templatefile("${path.module}/cloud-init/server.yaml.tftpl", {
    project = var.project_tag
  })

  instances = merge(
    var.create_dev ? {
      dev = {
        display_name = "${var.project_tag}-dev"
        name_tag     = "${var.project_tag}-dev"
        env          = "dev"
      }
    } : {},
    var.create_release ? {
      release = {
        display_name = "${var.project_tag}-release"
        name_tag     = "${var.project_tag}-release"
        env          = "release"
      }
    } : {}
  )
}

resource "oci_core_instance" "server" {
  for_each = local.instances

  availability_domain = local.ad_name
  compartment_id      = var.compartment_ocid
  display_name        = each.value.display_name
  shape               = var.vm_shape

  shape_config {
    ocpus         = var.vm_ocpus
    memory_in_gbs = var.vm_memory_gb
  }

  create_vnic_details {
    subnet_id        = oci_core_subnet.public.id
    assign_public_ip = var.assign_public_ip
    display_name     = "${each.value.display_name}-vnic"
    hostname_label   = replace(each.value.display_name, "_", "-")
    nsg_ids          = [oci_core_network_security_group.game.id]
  }

  source_details {
    source_type             = "image"
    source_id               = local.ubuntu_image_id
    boot_volume_size_in_gbs = var.boot_volume_gb
  }

  metadata = {
    ssh_authorized_keys = var.ssh_public_key
    user_data           = base64encode(local.cloud_init)
  }

  freeform_tags = merge(local.freeform_base, {
    name = each.value.name_tag
    env  = each.value.env
  })

  # Idempotent re-apply: do not replace healthy VMs when cloud-init template tweaks.
  lifecycle {
    ignore_changes = [
      metadata["user_data"],
      source_details[0].source_id,
    ]
  }
}
