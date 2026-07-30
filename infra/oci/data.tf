data "oci_identity_availability_domains" "ads" {
  compartment_id = var.compartment_ocid
}

locals {
  ad_name = data.oci_identity_availability_domains.ads.availability_domains[var.availability_domain_index].name

  freeform_base = {
    project = var.project_tag
    managed = "terraform"
  }

  # Ubuntu 22.04 Minimal/Standard x86_64 — used only when ubuntu_image_ocid is empty.
  image_search_shape = var.vm_shape
}

data "oci_core_images" "ubuntu_2204_x86" {
  count = var.ubuntu_image_ocid == "" ? 1 : 0

  compartment_id           = var.compartment_ocid
  operating_system         = "Canonical Ubuntu"
  operating_system_version = "22.04"
  shape                    = local.image_search_shape
  sort_by                  = "TIMECREATED"
  sort_order               = "DESC"
}

locals {
  ubuntu_image_id = var.ubuntu_image_ocid != "" ? var.ubuntu_image_ocid : (
    length(data.oci_core_images.ubuntu_2204_x86) > 0
    ? data.oci_core_images.ubuntu_2204_x86[0].images[0].id
    : ""
  )
}
