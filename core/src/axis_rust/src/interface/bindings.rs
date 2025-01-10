//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::ffi::{c_char, CStr, CString};

#[no_mangle]
pub extern "C" fn axis_interface_schema_resolve_definition(
    content: *const c_char,
    base_dir: *const c_char,
    out_content: *mut *const c_char,
    out_err_msg: *mut *const c_char,
) -> bool {
    assert!(!content.is_null(), "Invalid argument.");
    assert!(!base_dir.is_null(), "Invalid argument.");

    let c_content = unsafe { CStr::from_ptr(content) };
    let rust_content = match c_content.to_str() {
        Ok(s) => s,
        Err(_) => {
            let err_msg =
                CString::new("Failed to convert interface content to string.")
                    .expect("Failed to allocate memory.");
            unsafe {
                *out_err_msg = err_msg.into_raw();
            }
            return false;
        }
    };

    let c_base_dir = unsafe { CStr::from_ptr(base_dir) };
    let rust_base_dir = match c_base_dir.to_str() {
        Ok(s) => s,
        Err(_) => {
            let err_msg = CString::new("Failed to convert base_dir to string.")
                .expect("Failed to allocate memory.");
            unsafe {
                *out_err_msg = err_msg.into_raw();
            }
            return false;
        }
    };

    let ret = crate::interface::axis_interface_schema_resolve_definition(
        rust_content,
        rust_base_dir,
    );
    match ret {
        Ok(interface_json) => match serde_json::to_string(&interface_json) {
            Ok(interface_json_str) => match CString::new(interface_json_str) {
                Ok(c_interface_content) => {
                    unsafe {
                        *out_content = c_interface_content.into_raw();
                    }
                    true
                }
                Err(_) => {
                    let err_msg = CString::new("Failed to allocate memory.")
                        .expect("Failed to allocate memory.");
                    unsafe {
                        *out_err_msg = err_msg.into_raw();
                    }
                    false
                }
            },
            Err(_) => {
                let err_msg =
                    CString::new("Failed to serialize interface JSON.")
                        .expect("Failed to allocate memory.");
                unsafe {
                    *out_err_msg = err_msg.into_raw();
                }
                false
            }
        },
        Err(err) => {
            let err_msg = CString::new(err.to_string())
                .expect("Failed to allocate memory.");
            unsafe {
                *out_err_msg = err_msg.into_raw();
            }
            false
        }
    }
}
