//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::{env, fs, path::PathBuf};

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

    let generated_bindings = "src/schema/bindings.rs";
    binding_gen
        .write_to_file(generated_bindings)
        .expect("Unable to write bindings into file.");

    // Add some rules to the bindings file to disable clippy lints.
    let bindings_content = fs::read_to_string(generated_bindings)
        .expect("Unable to read generated bindings");
    let disabled_clippy_lints = [
        "#![allow(non_upper_case_globals)]",
        "#![allow(non_camel_case_types)]",
        "#![allow(dead_code)]",
    ];
    let new_bindings_content =
        disabled_clippy_lints.join("\n") + "\n\n" + &bindings_content;
    fs::write(generated_bindings, new_bindings_content)
        .expect("Unable to add clippy lint rules to the generated bindings.");
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
