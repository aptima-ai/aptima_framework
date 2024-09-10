//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use std::{fmt, str::FromStr};

use anyhow::{Error, Result};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone, Debug)]
pub enum ValueType {
    Null,
    Bool,

    Int8,
    Int16,
    Int32,
    Int64,

    Uint8,
    Uint16,
    Uint32,
    Uint64,

    Float32,
    Float64,

    String,
    Buf,

    Array,
    Object,

    Ptr,
}

impl FromStr for ValueType {
    type Err = anyhow::Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "bool" => Ok(ValueType::Bool),

            "int8" => Ok(ValueType::Int8),
            "int16" => Ok(ValueType::Int16),
            "int32" => Ok(ValueType::Int32),
            "int64" => Ok(ValueType::Int64),

            "uint8" => Ok(ValueType::Uint8),
            "uint16" => Ok(ValueType::Uint16),
            "uint32" => Ok(ValueType::Uint32),
            "uint64" => Ok(ValueType::Uint64),

            "float32" => Ok(ValueType::Float32),
            "float64" => Ok(ValueType::Float64),

            "string" => Ok(ValueType::String),
            "buf" => Ok(ValueType::Buf),

            "array" => Ok(ValueType::Array),
            "object" => Ok(ValueType::Object),

            _ => Err(Error::msg("Failed to parse string to value type")),
        }
    }
}

impl fmt::Display for ValueType {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            ValueType::Bool => write!(f, "bool"),

            ValueType::Int8 => write!(f, "int8"),
            ValueType::Int16 => write!(f, "int16"),
            ValueType::Int32 => write!(f, "int32"),
            ValueType::Int64 => write!(f, "int64"),

            ValueType::Uint8 => write!(f, "Uint8"),
            ValueType::Uint16 => write!(f, "Uint16"),
            ValueType::Uint32 => write!(f, "Uint32"),
            ValueType::Uint64 => write!(f, "Uint64"),

            ValueType::Float32 => write!(f, "float32"),
            ValueType::Float64 => write!(f, "float64"),

            ValueType::String => write!(f, "string"),
            ValueType::Buf => write!(f, "buf"),

            ValueType::Array => write!(f, "array"),
            ValueType::Object => write!(f, "object"),

            _ => panic!("Failed to parse string to value type"),
        }
    }
}

pub fn are_types_compatible(
    from_type: &ValueType,
    to_type: &ValueType,
) -> bool {
    matches!(
        (from_type, to_type),
        (
            ValueType::Uint8
                | ValueType::Int8
                | ValueType::Uint16
                | ValueType::Int16
                | ValueType::Uint32
                | ValueType::Int32
                | ValueType::Uint64
                | ValueType::Int64,
            ValueType::Uint8
                | ValueType::Int8
                | ValueType::Uint16
                | ValueType::Int16
                | ValueType::Uint32
                | ValueType::Int32
                | ValueType::Uint64
                | ValueType::Int64,
        ) | (
            ValueType::Float32 | ValueType::Float64,
            ValueType::Float32 | ValueType::Float64,
        ) | (ValueType::Bool, ValueType::Bool)
            | (ValueType::String, ValueType::String)
            | (ValueType::Buf, ValueType::Buf)
            | (ValueType::Array, ValueType::Array)
            | (ValueType::Object, ValueType::Object)
            | (ValueType::Ptr, ValueType::Ptr)
            | (ValueType::Null, ValueType::Null)
    )
}
