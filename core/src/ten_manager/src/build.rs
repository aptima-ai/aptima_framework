//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::fs;
use std::process::Command;

fn main() {
    let output = Command::new("git")
        .args(["describe", "--tags", "--abbrev=0"])
        .output()
        .expect("Failed to execute git command");

    let version =
        String::from_utf8(output.stdout).expect("Invalid UTF-8 sequence");
    let version_trimmed = version.trim();

    let version_without_v =
        version_trimmed.strip_prefix('v').unwrap_or(version_trimmed);

    fs::write(
        "src/version.rs",
        format!("pub const VERSION: &str = \"{}\";", version_without_v),
    )
    .expect("Unable to write version to file");
}
