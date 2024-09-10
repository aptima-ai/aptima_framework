//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use anyhow::Result;
use semver::Version;
use sha2::{Digest, Sha256};

use super::{
    dependencies::PkgDependency, pkg_type::PkgType, supports::PkgSupport,
    PkgInfo,
};

impl PkgInfo {
    pub fn gen_hash_hex(&self) -> Result<String> {
        gen_hash_hex(
            &self.pkg_identity.pkg_type,
            &self.pkg_identity.name,
            &self.version,
            &self.dependencies,
            &self.supports,
        )
    }
}

pub fn gen_hash_hex(
    pkg_type: &PkgType,
    name: &String,
    version: &Version,
    dependencies: &Vec<PkgDependency>,
    supports: &Vec<PkgSupport>,
) -> Result<String> {
    let mut hasher = Sha256::new();

    // Hash type.
    let type_string = format!("{}", pkg_type);
    hasher.update(type_string);

    // Hash name.
    hasher.update(name);

    // Hash version.
    let version_string = version.to_string();
    hasher.update(version_string);

    // Hash dependencies.
    for dep in dependencies {
        let dep_string = format!(
            "{}:{}@{}",
            dep.pkg_identity.pkg_type, dep.pkg_identity.name, dep.version_req
        );
        hasher.update(dep_string);
    }

    // Hash supports.
    for support in supports {
        let support_string = format!("{}", support);
        hasher.update(support_string);
    }

    let hash_result = hasher.finalize();
    let hash_hex = format!("{:x}", hash_result);

    Ok(hash_hex)
}
