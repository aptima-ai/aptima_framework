//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::ffi::{c_char, CStr, CString};

#[no_mangle]
pub extern "C" fn axis_rust_check_graph_for_app(
    app_base_dir: *const c_char,
    graph_json: *const c_char,
    app_uri: *const c_char,
    out_err_msg: *mut *const c_char,
) -> bool {
    assert!(!app_base_dir.is_null(), "Invalid argument.");
    assert!(!graph_json.is_null(), "Invalid argument.");
    assert!(!app_uri.is_null(), "Invalid argument.");

    let c_app_base_dir = unsafe { CStr::from_ptr(app_base_dir) };
    let c_graph_json = unsafe { CStr::from_ptr(graph_json) };
    let c_app_uri = unsafe { CStr::from_ptr(app_uri) };

    let rust_app_base_dir = c_app_base_dir.to_str().unwrap();
    let rust_graph_json = c_graph_json.to_str().unwrap();
    let rust_app_uri = c_app_uri.to_str().unwrap();

    let ret = crate::pkg_info::axis_rust_check_graph_for_app(
        rust_app_base_dir,
        rust_graph_json,
        rust_app_uri,
    );
    if ret.is_err() {
        let err_msg = format!("{:?}", ret.err().unwrap());
        let c_err_msg =
            CString::new(err_msg).expect("Failed to allocate memory.");
        unsafe {
            *out_err_msg = c_err_msg.into_raw();
        }
        return false;
    }
    true
}
