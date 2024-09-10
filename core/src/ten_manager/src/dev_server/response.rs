//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
use anyhow::Result;
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug, Clone)]
pub enum Status {
    #[serde(rename = "ok")]
    Ok,

    #[serde(rename = "fail")]
    Fail,
}

impl From<Status> for String {
    fn from(status: Status) -> Self {
        match status {
            Status::Ok => "ok".to_string(),
            Status::Fail => "fail".to_string(),
        }
    }
}

fn status_to_string<S>(
    status: &Status,
    serializer: S,
) -> Result<S::Ok, S::Error>
where
    S: serde::Serializer,
{
    let status_str: String = status.clone().into();
    serializer.serialize_str(&status_str)
}

#[derive(Serialize, Deserialize, Debug)]
pub struct ApiResponse<T> {
    #[serde(serialize_with = "status_to_string")]
    pub status: Status,

    pub data: T,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub meta: Option<Meta>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct Meta {}

#[derive(Serialize, Deserialize, Debug)]
pub struct ErrorResponse {
    #[serde(serialize_with = "status_to_string")]
    pub status: Status,

    pub message: String,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub error: Option<InnerError>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct InnerError {
    #[serde(rename = "type")]
    pub error_type: String,
    pub code: Option<String>,
    pub message: String,
}
