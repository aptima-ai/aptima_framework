//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
mod bindings;
pub mod store;

use std::{
    ffi::c_char,
    sync::{Arc, Mutex},
};

use anyhow::{Ok, Result};

use bindings::axis_schema_t;

/// `TenSchemaRawPointer` acts as a proxy for the raw pointer to `axis_schema_t`
/// in C. When the Rust object bound to this pointer is dropped, we need to
/// ensure that the raw pointer is properly destroyed. However, the `Drop` trait
/// cannot be directly implemented on a raw pointer.
#[derive(Debug)]
struct TenSchemaRawPointer {
    raw_ptr: Mutex<*const axis_schema_t>,
}

unsafe impl Send for TenSchemaRawPointer {}
unsafe impl Sync for TenSchemaRawPointer {}

impl Drop for TenSchemaRawPointer {
    fn drop(&mut self) {
        let ptr = self.raw_ptr.lock().unwrap();
        if !ptr.is_null() {
            unsafe {
                bindings::axis_schema_destroy_proxy(*ptr);
            }
        }
    }
}

#[derive(Debug, Clone)]
pub struct TenSchema {
    // The TenSchema needs to implement Clone, and it will be used in tman
    // designer which will be shared between threads. So we use Arc to wrap the
    // raw_ptr.
    inner: Arc<TenSchemaRawPointer>,
}

// unsafe impl Send for TenSchema {}
// unsafe impl Sync for TenSchema {}

impl TenSchema {
    pub fn new(raw_ptr: *const axis_schema_t) -> Self {
        Self {
            // axis_NOLINTNEXTLINE(clippy::arc_with_non_send_sync)
            // The raw_ptr is not Send or Sync, but it is readonly once created,
            // so it is safe to wrap it in an Arc.
            // #[allow(clippy::arc_with_non_send_sync)]
            inner: Arc::new(TenSchemaRawPointer {
                raw_ptr: Mutex::new(raw_ptr),
            }),
        }
    }

    pub fn as_ptr(&self) -> *const axis_schema_t {
        *self.inner.raw_ptr.lock().unwrap()
    }

    pub fn validate_json(&self, value: &serde_json::Value) -> Result<()> {
        let value_str = serde_json::to_string(value)?;
        let mut err_msg: *mut c_char = std::ptr::null_mut();

        unsafe {
            let c_value_str = std::ffi::CString::new(value_str)?;
            let success =
                bindings::axis_schema_adjust_and_validate_json_str_proxy(
                    self.as_ptr(),
                    c_value_str.as_ptr(),
                    &mut err_msg as *mut _ as *mut *const c_char,
                );

            if !success {
                let rust_err_msg =
                    std::ffi::CStr::from_ptr(err_msg).to_str()?.to_owned();
                // The err_msg is allocated by the C code, so we need to free
                // it.
                libc::free(err_msg as *mut libc::c_void);

                return Err(anyhow::anyhow!(rust_err_msg));
            }
        }

        Ok(())
    }

    pub fn is_compatible_with(&self, target: &TenSchema) -> Result<()> {
        let mut err_msg: *mut c_char = std::ptr::null_mut();
        unsafe {
            let success = bindings::axis_schema_is_compatible_proxy(
                self.as_ptr(),
                target.as_ptr(),
                &mut err_msg as *mut _ as *mut *const c_char,
            );

            if !success {
                let rust_err_msg =
                    std::ffi::CStr::from_ptr(err_msg).to_str()?.to_owned();
                // The err_msg is allocated by the C code, so we need to free
                // it.
                libc::free(err_msg as *mut libc::c_void);

                return Err(anyhow::anyhow!(rust_err_msg));
            }
        }

        Ok(())
    }
}

fn create_schema_from_json(
    schema_json: &serde_json::Value,
) -> Result<TenSchema> {
    let schema_str = serde_json::to_string(schema_json)?;
    create_schema_from_json_str(&schema_str)
}

fn create_schema_from_json_str(schema_json_str: &str) -> Result<TenSchema> {
    let mut err_msg: *mut c_char = std::ptr::null_mut();
    let schema_ptr: *mut axis_schema_t;
    unsafe {
        let c_schema_str = std::ffi::CString::new(schema_json_str)?;
        schema_ptr = bindings::axis_schema_create_from_json_str_proxy(
            c_schema_str.as_ptr(),
            &mut err_msg as *mut _ as *mut *const c_char,
        );
    }

    if schema_ptr.is_null() {
        // Failed to create axis_schema_t in C world.
        unsafe {
            let rust_err_msg =
                std::ffi::CStr::from_ptr(err_msg).to_str()?.to_owned();
            // The err_msg is allocated by the C code, so we need to free it.
            libc::free(err_msg as *mut libc::c_void);

            return Err(anyhow::anyhow!(rust_err_msg));
        }
    }

    Ok(TenSchema::new(schema_ptr))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_create_schema() {
        let schema_json = serde_json::json!({
            "type": "object",
            "properties": {
                "name": {
                    "type": "string"
                },
                "age": {
                    "type": "int8"
                }
            }
        });

        let schema = create_schema_from_json(&schema_json).unwrap();
        assert!(!schema.as_ptr().is_null());
    }

    #[test]
    fn test_create_schema_invalid_json() {
        let schema_str = include_str!("test_data_embed/invalid_schema.json");
        let schema_result = create_schema_from_json_str(schema_str);
        println!("{:?}", schema_result);
        assert!(schema_result.is_err());
    }

    #[test]
    fn test_schema_compatible() {
        let schema1_json = serde_json::json!({
            "type": "object",
            "properties": {
                "name": {
                    "type": "string"
                },
                "age": {
                    "type": "int8"
                }
            },
            "required": ["name"]
        });
        let schema1 = create_schema_from_json(&schema1_json).unwrap();

        let schema2_json = serde_json::json!({
            "type": "object",
            "properties": {
                "name": {
                    "type": "string"
                },
                "age": {
                    "type": "int16"
                }
            }
        });
        let schema2 = create_schema_from_json(&schema2_json).unwrap();

        let result = schema1.is_compatible_with(&schema2);
        assert!(result.is_ok());
    }

    #[test]
    fn test_schema_clone() {
        let schema_json = serde_json::json!({
            "type": "object",
            "properties": {
                "name": {
                    "type": "string"
                },
                "age": {
                    "type": "int8"
                }
            }
        });

        let schema = create_schema_from_json(&schema_json).unwrap();
        assert!(!schema.as_ptr().is_null());

        let cloned_schema = schema.clone();
        assert!(!cloned_schema.as_ptr().is_null());
    }
}
