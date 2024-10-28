//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use std::sync::Arc;
use std::{io::Write, path::PathBuf, time::Duration};

use anyhow::{anyhow, Context, Result};
use reqwest::header::{HeaderMap, AUTHORIZATION, CONTENT_TYPE};
use semver::Version;
use serde::{Deserialize, Serialize};
use serde_json::{json, Value};
use tempfile::NamedTempFile;
use tokio::sync::RwLock;
use tokio::time::sleep;

use super::{FoundResult, SearchCriteria};
use crate::{
    config::TmanConfig, error::TmanError, log::tman_verbose_println,
    registry::found_result::RegistryPackageData,
};
use ten_rust::pkg_info::{
    pkg_identity::PkgIdentity, supports::get_manifest_supports_from_pkg,
    PkgInfo,
};

async fn retry_async<'a, F, T>(
    tman_config: &TmanConfig,
    max_retries: u32,
    retry_delay: Duration,
    mut operation: F,
) -> Result<T>
where
    F: FnMut() -> std::pin::Pin<
        Box<dyn std::future::Future<Output = Result<T>> + 'a>,
    >,
{
    for attempt in 0..=max_retries {
        match operation().await {
            Ok(result) => return Ok(result),
            Err(e) => {
                tman_verbose_println!(
                    tman_config,
                    "Attempt {} failed: {:?}",
                    attempt + 1,
                    e
                );

                if attempt == max_retries {
                    return Err(e);
                }

                sleep(retry_delay).await;
            }
        }
    }

    unreachable!("retry logic should either return or error out")
}

#[derive(Debug)]
struct UploadInfo {
    resource_id: String,
    url: String,
}

async fn get_package_upload_info(
    tman_config: &TmanConfig,
    base_url: &str,
    client: &reqwest::Client,
    pkg_info: &PkgInfo,
) -> Result<UploadInfo> {
    // Basically, the principle here is that when tman install is run, all the
    // necessary metadata for a package (such as the package's dependencies,
    // support, etc.) should be uploaded to the registry during tman publish and
    // stored by the registry. This way, when tman install is run and the
    // package's metadata is requested, the registry can return it directly,
    // without tman needing to request the actual zip file from the registry,
    // extract the manifest.json from it, and then retrieve the information.
    // That would be too cumbersome. Therefore, information not needed during
    // tman install, such as api/schema, does not need to be uploaded to the
    // registry during tman publish. Of course, if it is uploaded, the potential
    // benefit is that this information can be directly displayed in the cloud
    // store.

    // TODO: Implement the 'environments' mechanism.
    // Add a new environments field, or perhaps directly upload the content of
    // manifest.json to the cloud store.

    let max_retries = 3;
    let retry_delay = Duration::from_millis(100);

    retry_async(tman_config, max_retries, retry_delay, || {
        let base_url = base_url.to_string();
        let client = client.clone();
        let pkg_info = pkg_info.clone();

        Box::pin(async move {
            let payload = json!(RegistryPackageData {
                pkg_type: pkg_info.pkg_identity.pkg_type.clone(),
                name: pkg_info.pkg_identity.name.clone(),
                version: pkg_info.version.clone(),
                dependencies: pkg_info
                    .dependencies
                    .clone()
                    .into_iter()
                    .map(|d| d.into())
                    .collect(),
                supports: Some(get_manifest_supports_from_pkg(
                    &pkg_info.supports
                )),
                hash: pkg_info.hash.clone(),
            });

            tman_verbose_println!(
                tman_config,
                "Payload of publishing: {}",
                payload.to_string()
            );

            let mut headers = HeaderMap::new();

            if let Some(user_token) = &tman_config.user_token {
                let basic_token = format!("Basic {}", user_token);
                headers.insert(
                    AUTHORIZATION,
                    basic_token.parse().map_err(|e| {
                        eprintln!("Failed to parse authorization token: {}", e);
                        e
                    })?,
                );
            } else {
                tman_verbose_println!(
                    tman_config,
                    "Authorization token is missing"
                );
                return Err(TmanError::Custom(
                    "Authorization token is missing".to_string(),
                )
                .into());
            }

            let response = client
                .post(base_url)
                .headers(headers)
                .json(&payload)
                .send()
                .await
                .map_err(|e| anyhow!("Failed to send request: {}", e))?;

            if !response.status().is_success() {
                return Err(anyhow!(
                    "Received non-success status code: {}",
                    response.status(),
                ));
            }

            let data: Value = response
                .json()
                .await
                .map_err(|e| anyhow!("Failed to parse JSON response: {}", e))?;

            let resource_id = data["data"]["resourceId"]
                .as_str()
                .ok_or_else(|| anyhow!("Missing 'resourceId' in response"))?
                .to_string();
            let url = data["data"]["url"]
                .as_str()
                .ok_or_else(|| anyhow!("Missing 'url' in response"))?
                .to_string();

            Ok(UploadInfo { resource_id, url })
        })
    })
    .await
}

async fn upload_package_to_remote(
    tman_config: &TmanConfig,
    client: &reqwest::Client,
    package_file_path: &str,
    url: &str,
) -> Result<()> {
    let max_retries = 3;
    let retry_delay = Duration::from_millis(100);

    retry_async(tman_config, max_retries, retry_delay, || {
        let client = client.clone();
        let package_file_path = package_file_path.to_string();
        let url = url.to_string();

        Box::pin(async move {
            let body = match std::fs::read(&package_file_path) {
                Ok(file) => file,
                Err(e) => {
                    eprintln!(
                        "Failed to read file at '{}': {}",
                        &package_file_path, e
                    );
                    return Err(e.into());
                }
            };

            let mut headers = HeaderMap::new();

            headers.insert(
                CONTENT_TYPE,
                "application/zip".parse().map_err(|e| {
                    eprintln!("Failed to parse content type: {}", e);
                    e
                })?,
            );

            let response =
                client.put(url).headers(headers).body(body).send().await;
            match response {
                Ok(resp) => {
                    if resp.status().is_success() {
                        Ok(())
                    } else {
                        Err(TmanError::Custom(format!(
                            "Failed to upload package with HTTP status {:?}",
                            resp.text().await
                        ))
                        .into())
                    }
                }
                Err(e) => Err(e.into()),
            }
        })
    })
    .await
}

async fn ack_of_uploading(
    tman_config: &TmanConfig,
    base_url: &str,
    client: &reqwest::Client,
    resource_id: &str,
) -> Result<()> {
    let max_retries = 3;
    let retry_delay = Duration::from_millis(100);

    retry_async(tman_config, max_retries, retry_delay, || {
        let base_url = base_url.to_string();
        let client = client.clone();
        let resource_id = resource_id.to_string();

        Box::pin(async move {
            let url = match reqwest::Url::parse(&base_url) {
                Ok(url) => url,
                Err(e) => {
                    eprintln!("Invalid base URL: {}", e);
                    return Err(e.into());
                }
            };

            let body = json!({ "resourceId": resource_id });

            let response = client.patch(url).json(&body).send().await;

            match response {
                Ok(resp) => {
                    if resp.status().is_success() {
                        Ok(())
                    } else {
                        Err(TmanError::Custom(format!(
                    "Failed to acknowledge uploading with HTTP status {}",
                    resp.status()
                ))
                        .into())
                    }
                }
                Err(e) => Err(e.into()),
            }
        })
    })
    .await
}

pub async fn upload_package(
    tman_config: &TmanConfig,
    base_url: &str,
    package_file_path: &str,
    pkg_info: &PkgInfo,
) -> Result<()> {
    let client = reqwest::Client::new();

    let upload_info =
        get_package_upload_info(tman_config, base_url, &client, pkg_info)
            .await?;

    upload_package_to_remote(
        tman_config,
        &client,
        package_file_path,
        &upload_info.url,
    )
    .await?;

    ack_of_uploading(tman_config, base_url, &client, &upload_info.resource_id)
        .await?;

    Ok(())
}

// Parse the header of content-range.
fn parse_content_range(content_range: &str) -> Option<(u64, u64, u64)> {
    let parts: Vec<&str> = content_range.split('/').collect();
    if parts.len() == 2 {
        let total_size = parts[1].parse().ok()?;
        let range_parts: Vec<&str> = parts[0].split('-').collect();
        if range_parts.len() == 2 {
            let start = range_parts[0].parse().ok()?;
            let end = range_parts[1].parse().ok()?;
            return Some((start, end, total_size));
        }
    }
    None
}

pub async fn get_package<'a>(
    tman_config: &TmanConfig,
    url: &str,
    temp_file: &'a mut NamedTempFile,
) -> Result<()> {
    let client = reqwest::Client::new();

    let temp_file = Arc::new(RwLock::new(temp_file));

    let max_retries = 3;
    let retry_delay = Duration::from_millis(100);

    let download_complete = Arc::new(RwLock::new(false));

    retry_async(tman_config, max_retries, retry_delay, || {
        let client = client.clone();
        let url = url.to_string();
        let temp_file = Arc::clone(&temp_file); // Clone the Rc pointer.
        let download_complete = Arc::clone(&download_complete);

        Box::pin(async move {
            // Check the size of the file that has already been downloaded.
            let temp_file_len = {
                let temp_file_read = temp_file.read().await;
                temp_file_read
                    .as_file()
                    .metadata()
                    .map(|metadata| metadata.len())
                    .unwrap_or(0)
            };

            // Set the Range header to support resumable downloads.
            let mut headers = HeaderMap::new();
            if temp_file_len > 0 {
                headers.insert(
                    reqwest::header::RANGE,
                    format!("bytes={}-", temp_file_len).parse().unwrap(),
                );
            }

            let response = client
                .get(&url)
                .headers(headers)
                .send()
                .await
                .with_context(|| {
                    format!("Failed to send GET request to {}", url)
                })?;

            if !response.status().is_success()
                && response.status() != reqwest::StatusCode::PARTIAL_CONTENT
            {
                return Err(anyhow!(
                    "Failed to download the package: HTTP {}",
                    response.status()
                ));
            }

            // Get the headers of Content-Range or Content-Length.
            let content_range = response
                .headers()
                .get(reqwest::header::CONTENT_RANGE)
                .cloned();
            let content_length = response
                .headers()
                .get(reqwest::header::CONTENT_LENGTH)
                .cloned();

            // Read the content of the response and append the newly downloaded
            // part to the file.
            let content = response
                .bytes()
                .await
                .with_context(|| "Failed to read bytes from response")?;

            if content.is_empty() {
                return Err(anyhow!("No new content downloaded"));
            }

            let mut temp_file_write = temp_file.write().await;
            temp_file_write
                .as_file_mut()
                .write_all(&content)
                .with_context(|| "Failed to write content to temporary file")?;

            // Check if we have downloaded the entire file.
            if let Some(content_range) = content_range {
                // Parse the content-range to check if download is complete.
                let content_range_str = content_range.to_str().unwrap();
                if let Some((_, _, total_size)) =
                    parse_content_range(content_range_str)
                {
                    if temp_file_len + content.len() as u64 >= total_size {
                        *download_complete.write().await = true;
                    }
                }
            } else if content_length.is_some() {
                // If there is no content-range but content-length exists, the
                // download should be complete in one go.
                *download_complete.write().await = true;
            }

            Ok(())
        })
    })
    .await?;

    // Only print when `download_complete` is `true`.
    if *download_complete.read().await {
        let temp_file_borrow = temp_file.read().await;
        tman_verbose_println!(
            tman_config,
            "Package downloaded successfully from {} and written to {}",
            url,
            temp_file_borrow.path().display()
        );
    }

    Ok(())
}

#[derive(Debug, Serialize, Deserialize)]
struct ApiResponse {
    status: String,
    data: RegistryPackagesData,
    meta: Value,
}

#[derive(Debug, Serialize, Deserialize)]
struct RegistryPackagesData {
    #[serde(rename = "totalSize")]
    total_size: u32,
    packages: Vec<RegistryPackageData>,
}

pub async fn get_package_list(
    tman_config: &TmanConfig,
    base_url: &str,
    pkg_identity: &PkgIdentity,
    criteria: &SearchCriteria,
) -> Result<Vec<FoundResult>> {
    let max_retries = 3;
    let retry_delay = Duration::from_millis(100);

    retry_async(tman_config, max_retries, retry_delay, || {
        let base_url = base_url.to_string();
        let pkg_identity = pkg_identity.clone();
        let client = reqwest::Client::new();

        Box::pin(async move {
            let mut results = Vec::new();
            let mut current_page = 1;
            let mut total_size;

            loop {
                let mut url = reqwest::Url::parse(&base_url)?;
                url.query_pairs_mut()
                    .append_pair("type", &pkg_identity.pkg_type.to_string())
                    .append_pair("name", &pkg_identity.name)
                    .append_pair("version", &criteria.version_req.to_string())
                    .append_pair("pageSize", "10")
                    .append_pair("page", &current_page.to_string());

                let response = client.get(url).send().await;

                let response = match response {
                    Ok(response) => response,
                    Err(e) => {
                        return Err(anyhow::anyhow!("Request failed: {}", e))
                    }
                };

                if response.status() != reqwest::StatusCode::OK {
                    return Err(anyhow::anyhow!(
                        "API responded with non-ok status: {}",
                        response.status()
                    ));
                }

                let body = response.text().await?;
                let api_response = serde_json::from_str::<ApiResponse>(&body);
                let api_response = match api_response {
                    Ok(api_response) => api_response,
                    Err(e) => {
                        return Err(anyhow::anyhow!(
                            "Failed to parse JSON response: {}",
                            e
                        ))
                    }
                };

                if api_response.status != "ok" {
                    return Err(anyhow::anyhow!(
                        "API responded with non-ok status: {}",
                        api_response.status
                    ));
                }

                total_size = api_response.data.total_size as usize;

                for package_data in api_response.data.packages {
                    let url = PathBuf::from(format!(
                        "{}/{}/{}/{}/{}",
                        &base_url,
                        package_data.pkg_type,
                        package_data.name,
                        package_data.version,
                        package_data.hash,
                    ));
                    results.push(FoundResult { url, package_data });
                }

                // Check if we've fetched all packages based on totalSize.
                if results.len() >= total_size {
                    break;
                }
                current_page += 1;
            }

            tman_verbose_println!(
                &tman_config,
                "Fetched {} packages for {}:{}",
                results.len(),
                pkg_identity.pkg_type,
                pkg_identity.name,
            );

            Ok(results)
        })
    })
    .await
}

pub async fn delete_package(
    tman_config: &TmanConfig,
    base_url: &str,
    pkg_identity: &PkgIdentity,
    version: &Version,
    hash: &str,
) -> Result<()> {
    let max_retries = 3;
    let retry_delay = Duration::from_millis(100);

    retry_async(tman_config, max_retries, retry_delay, || {
        let base_url = base_url.to_string();
        let pkg_identity = pkg_identity.clone();
        let version = version.clone();
        let client = reqwest::Client::new();

        Box::pin(async move {
            // Ensure the base URL ends with a '/'.
            let base_url = if base_url.ends_with('/') {
                base_url
            } else {
                format!("{}/", base_url)
            };

            let url = reqwest::Url::parse(&base_url).inspect_err(|&e| {
                eprintln!("Failed to parse base URL: {}", e);
            })?;

            let url = url
                .join(&format!(
                    "{}/{}/{}/{}",
                    &pkg_identity.pkg_type.to_string(),
                    &pkg_identity.name,
                    version,
                    hash,
                ))
                .inspect_err(|&e| {
                    eprintln!("Failed to join URL path: {}", e);
                })?;

            let mut headers = HeaderMap::new();

            if let Some(admin_token) = &tman_config.admin_token {
                let basic_token = format!("Basic {}", admin_token);
                headers.insert(
                    AUTHORIZATION,
                    basic_token.parse().map_err(|e| {
                        eprintln!("Failed to parse authorization token: {}", e);
                        e
                    })?,
                );
            } else {
                tman_verbose_println!(
                    &tman_config,
                    "Authorization token is missing"
                );
                return Err(TmanError::Custom(
                    "Authorization token is missing".to_string(),
                )
                .into());
            }

            // Sending the DELETE request.
            let response = client.delete(url).headers(headers).send().await;

            match response {
                Ok(resp) => {
                    if resp.status().is_success() {
                        Ok(())
                    } else {
                        Err(TmanError::Custom(format!(
                            "HTTP error: {}",
                            resp.status()
                        ))
                        .into())
                    }
                }
                Err(e) => Err(e.into()),
            }
        })
    })
    .await
}
