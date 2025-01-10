//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::str::FromStr;

use anyhow::Result;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub enum MsgConversionType {
    #[serde(rename = "per_property")]
    PerProperty,
}

impl FromStr for MsgConversionType {
    type Err = anyhow::Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "per_property" => Ok(MsgConversionType::PerProperty),
            _ => Err(anyhow::Error::msg(format!(
                "Unsupported message conversion type: {}.",
                s
            ))),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub enum MsgConversionMode {
    #[serde(rename = "fixed_value")]
    FixedValue,

    #[serde(rename = "from_original")]
    FromOriginal,
}

impl FromStr for MsgConversionMode {
    type Err = anyhow::Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "fixed_value" => Ok(MsgConversionMode::FixedValue),
            "from_original" => Ok(MsgConversionMode::FromOriginal),
            _ => Err(anyhow::Error::msg(format!(
                "Unsupported message conversion mode: {}.",
                s
            ))),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct MsgConversionRule {
    pub path: String,

    pub conversion_mode: MsgConversionMode,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub original_path: Option<String>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub value: Option<serde_json::Value>,
}

impl MsgConversionRule {
    pub fn validate(&self) -> Result<()> {
        if self.path.is_empty() {
            return Err(anyhow::anyhow!("property path is empty"));
        }

        match self.conversion_mode {
            MsgConversionMode::FixedValue => {
                if self.value.is_none() {
                    return Err(anyhow::anyhow!(
                        "'value' field is required for the fixed_value conversion mode",
                    ));
                }
            }
            MsgConversionMode::FromOriginal => {
                if self.original_path.is_none() {
                    return Err(anyhow::anyhow!(
                        "'original_path' field is required for the from_original conversion mode",
                    ));
                }
            }
        }

        Ok(())
    }
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct MsgConversionRules {
    pub rules: Vec<MsgConversionRule>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub keep_original: Option<bool>,
}

impl MsgConversionRules {
    pub fn validate(&self) -> Result<()> {
        if self.rules.is_empty() {
            return Err(anyhow::anyhow!("conversion rules are empty"));
        }

        for (idx, rule) in self.rules.iter().enumerate() {
            rule.validate()
                .map_err(|e| anyhow::anyhow!("- rule[{}]: {}", idx, e))?;
        }

        Ok(())
    }
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct MsgConversion {
    #[serde(rename = "type")]
    pub conversion_type: MsgConversionType,

    #[serde(flatten)]
    pub rules: MsgConversionRules,
}

impl MsgConversion {
    pub fn validate(&self) -> Result<()> {
        self.rules.validate()
    }
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct MsgAndResultConversion {
    #[serde(flatten)]
    pub msg: MsgConversion,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub result: Option<MsgConversion>,
}

impl MsgAndResultConversion {
    pub fn validate(&self) -> Result<()> {
        self.msg.validate()?;

        if let Some(result) = &self.result {
            result.validate().map_err(|e| {
                anyhow::anyhow!("invalid result conversion: {}", e)
            })?;
        }

        Ok(())
    }
}
