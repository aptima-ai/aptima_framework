//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::{
    env, fs,
    path::{Path, PathBuf},
    process::id,
    thread,
    time::Duration,
};

fn auto_gen_schema_bindings_from_c() {
    let mut base_dir = env::current_dir()
        .unwrap_or("Failed to get path of //ten_rust/src".into());
    base_dir.pop();
    base_dir.pop();

    let mut schema_header = base_dir.clone();
    schema_header
        .push("include_internal/ten_utils/schema/bindings/rust/schema.h");
    if !schema_header.exists() {
        println!("Path of schema.h: {}", schema_header.to_str().unwrap());
        panic!("The //include_internal/ten_utils/schema/bindings/rust/schema.h does not exist.");
    }

    println!("cargo:rerun-if-changed={}", schema_header.to_str().unwrap());

    base_dir.push("include");

    let binding_gen = bindgen::Builder::default()
        .clang_arg(format!("-I{}", base_dir.to_str().unwrap()))
        .no_copy("rte_schema_t")
        .header(schema_header.to_str().unwrap())
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    // Generate a unique temporary file based on the current process ID.
    //
    // When `ten_rust` is built, it writes to the
    // `core/src/ten_rust/src/schema/bindings.rs` file, and multiple GN build
    // paths can trigger the `cargo build` of `ten_rust`. For example, one is
    // the `ten_rust_static_lib` GN target, another is the `ten_rust_test` GN
    // target, and another case could be when building `tman`, where the
    // `Cargo.toml` of `tman` includes a dependency on `ten_rust`, thus
    // triggering the build of `ten_rust`. Therefore, if these targets trigger
    // the `cargo build` at the same time, it may result in corrupted content in
    // `core/src/ten_rust/src/schema/bindings.rs`. To prevent this, an
    // atomic operation is needed to ensure that the content of
    // `core/src/ten_rust/src/schema/bindings.rs` is not corrupted due to
    // multiple parallel generation actions overwriting each other's file
    // content. On Windows, Linux, and macOS, file system `rename` operations
    // within the same mount point are atomic. Therefore, taking advantage of
    // this feature, the content of `bindings.rs` is first generated into a
    // temporary file, which is then atomically renamed to the final
    // `bindings.rs`, thereby avoiding content corruption caused by parallel
    // `cargo build` processes.
    //
    // TODO(Wei): Another possible solution is to differentiate the
    // `core/src/ten_rust/src/schema/bindings.rs` files under these GN build
    // paths, with each build path using its own `schema/bindings.rs`.
    let schema_dir = Path::new("src/schema/");
    let generated_bindings = schema_dir.join("bindings.rs");
    let temp_bindings = schema_dir.join(format!("bindings_{}.rs.tmp", id()));

    binding_gen
        .write_to_file(&temp_bindings)
        .expect("Unable to write bindings into file.");

    // Add some rules to the bindings file to disable clippy lints.
    let bindings_content = fs::read_to_string(&temp_bindings)
        .expect("Unable to read generated bindings");
    let disabled_clippy_lints = [
        "#![allow(non_upper_case_globals)]",
        "#![allow(non_camel_case_types)]",
        "#![allow(dead_code)]",
    ];
    let new_bindings_content =
        disabled_clippy_lints.join("\n") + "\n\n" + &bindings_content;
    fs::write(&temp_bindings, new_bindings_content)
        .expect("Unable to add clippy lint rules to the generated bindings.");

    let max_retries = 5;
    // 500 milliseconds delay between retries.
    let retry_delay = Duration::from_millis(500);

    for attempt in 1..=max_retries {
        // Atomically move the temporary file to the target file.
        match fs::rename(&temp_bindings, &generated_bindings) {
            Ok(_) => {
                println!("File renamed successfully.");
                break;
            }
            Err(e) if attempt < max_retries => {
                println!(
                    "Attempt {}/{} failed: {}. Retrying...",
                    attempt, max_retries, e
                );
                thread::sleep(retry_delay);
            }
            Err(e) => {
                panic!(
                    "Unable to move temporary bindings to final destination after {} attempts: {}",
                    max_retries, e
                );
            }
        }
    }
}

// The current auto-detection only supports limited environment combinations;
// for example, cross-compilation is not supported.
fn auto_detect_utils_library_path() -> PathBuf {
    let mut ten_rust_dir =
        env::current_dir().unwrap_or("Failed to get path of //ten_rust".into());
    ten_rust_dir.pop();
    ten_rust_dir.pop();
    ten_rust_dir.pop();
    ten_rust_dir.push("out");

    match std::env::consts::OS {
        "macos" => {
            ten_rust_dir.push("mac");
        }
        "linux" => {
            ten_rust_dir.push("linux");
        }
        "windows" => {
            ten_rust_dir.push("win");
        }
        _ => {
            panic!("Unsupported OS.");
        }
    }

    match std::env::consts::ARCH {
        "x86" => {
            ten_rust_dir.push("x86");
        }
        "x86_64" => {
            ten_rust_dir.push("x64");
        }
        "aarch64" => {
            ten_rust_dir.push("arm64");
        }
        "arm" => {
            ten_rust_dir.push("arm");
        }
        _ => {
            panic!("Unsupported architecture.");
        }
    }

    ten_rust_dir.push("gen/core/src/ten_utils");
    ten_rust_dir
}

fn main() {
    auto_gen_schema_bindings_from_c();

    // If the auto-detected utils library path is incorrect, we can specify it
    // using the environment variable.
    let utils_search_path: String = match env::var("TEN_UTILS_LIBRARY_PATH") {
        Ok(utils_lib_path) => utils_lib_path,
        Err(_) => {
            let utils_lib_path = auto_detect_utils_library_path();
            utils_lib_path.to_str().unwrap().to_string()
        }
    };

    println!("cargo:rustc-link-search={}", utils_search_path);
    println!("cargo:rustc-link-lib=ten_utils_static");

    // #[cfg(target_os = "linux")]
    // println!("cargo:rustc-link-lib=asan");
}
