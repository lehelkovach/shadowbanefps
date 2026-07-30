terraform {
  required_version = ">= 1.5.0"

  required_providers {
    oci = {
      source  = "oracle/oci"
      version = ">= 5.0.0"
    }
  }
}

provider "oci" {
  # Prefer env vars / ~/.oci/config. See .env.example.
  # Never commit private keys or tenancy secrets into this repo.
  region = var.oci_region
}
