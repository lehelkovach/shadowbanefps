resource "oci_core_vcn" "game" {
  compartment_id = var.compartment_ocid
  cidr_blocks    = [var.vcn_cidr]
  display_name   = "${var.project_tag}-vcn"
  dns_label      = "sbfps"

  freeform_tags = merge(local.freeform_base, {
    name = "${var.project_tag}-vcn"
  })
}

resource "oci_core_internet_gateway" "igw" {
  compartment_id = var.compartment_ocid
  vcn_id         = oci_core_vcn.game.id
  display_name   = "${var.project_tag}-igw"
  enabled        = true

  freeform_tags = merge(local.freeform_base, {
    name = "${var.project_tag}-igw"
  })
}

resource "oci_core_route_table" "public" {
  compartment_id = var.compartment_ocid
  vcn_id         = oci_core_vcn.game.id
  display_name   = "${var.project_tag}-public-rt"

  route_rules {
    network_entity_id = oci_core_internet_gateway.igw.id
    destination       = "0.0.0.0/0"
    destination_type  = "CIDR_BLOCK"
  }

  freeform_tags = merge(local.freeform_base, {
    name = "${var.project_tag}-public-rt"
  })
}

# Least privilege NSG: SSH from allowlist; Unreal game UDP 7777 from game_udp_cidrs.
resource "oci_core_network_security_group" "game" {
  compartment_id = var.compartment_ocid
  vcn_id         = oci_core_vcn.game.id
  display_name   = "${var.project_tag}-nsg"

  freeform_tags = merge(local.freeform_base, {
    name = "${var.project_tag}-nsg"
  })
}

resource "oci_core_network_security_group_security_rule" "ssh_ingress" {
  for_each = toset(var.ssh_allowed_cidrs)

  network_security_group_id = oci_core_network_security_group.game.id
  direction                 = "INGRESS"
  protocol                  = "6" # TCP
  source                    = each.value
  source_type               = "CIDR_BLOCK"
  stateless                 = false
  description               = "SSH from ${each.value}"

  tcp_options {
    destination_port_range {
      min = 22
      max = 22
    }
  }
}

resource "oci_core_network_security_group_security_rule" "game_udp_ingress" {
  for_each = toset(var.game_udp_cidrs)

  network_security_group_id = oci_core_network_security_group.game.id
  direction                 = "INGRESS"
  protocol                  = "17" # UDP
  source                    = each.value
  source_type               = "CIDR_BLOCK"
  stateless                 = false
  description               = "UE dedicated server UDP 7777 from ${each.value}"

  udp_options {
    destination_port_range {
      min = 7777
      max = 7777
    }
  }
}

resource "oci_core_network_security_group_security_rule" "egress_all" {
  network_security_group_id = oci_core_network_security_group.game.id
  direction                 = "EGRESS"
  protocol                  = "all"
  destination               = "0.0.0.0/0"
  destination_type          = "CIDR_BLOCK"
  stateless                 = false
  description               = "Allow all egress (package updates, Epic CDN if needed)"
}

# Restrictive security list (egress only). Real ingress is NSG-only so we do not
# accidentally open ports via the VCN default security list.
resource "oci_core_security_list" "deny_ingress" {
  compartment_id = var.compartment_ocid
  vcn_id         = oci_core_vcn.game.id
  display_name   = "${var.project_tag}-sl-deny-ingress"

  egress_security_rules {
    protocol    = "all"
    destination = "0.0.0.0/0"
    description = "Allow all egress"
  }

  freeform_tags = merge(local.freeform_base, {
    name = "${var.project_tag}-sl-deny-ingress"
  })
}

resource "oci_core_subnet" "public" {
  compartment_id             = var.compartment_ocid
  vcn_id                     = oci_core_vcn.game.id
  cidr_block                 = var.subnet_cidr
  display_name               = "${var.project_tag}-public"
  dns_label                  = "games"
  prohibit_public_ip_on_vnic = false
  route_table_id             = oci_core_route_table.public.id
  security_list_ids          = [oci_core_security_list.deny_ingress.id]

  freeform_tags = merge(local.freeform_base, {
    name = "${var.project_tag}-public"
  })
}
