//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
use actix_web::{HttpRequest, HttpResponse, Responder};
use mime_guess::from_path;
use rust_embed::RustEmbed;

// Points to the frontend build output directory.
#[derive(RustEmbed)]
#[folder = "designer_frontend/dist/"]
struct Asset;

pub async fn get_frontend_asset(req: HttpRequest) -> impl Responder {
    let path = req.path().trim_start_matches('/').to_owned();

    if path.is_empty() {
        // If the root path is requested, return `index.html`.
        match Asset::get("index.html") {
            Some(content) => HttpResponse::Ok()
                .content_type("text/html")
                .body(content.data.into_owned()),
            None => HttpResponse::NotFound().body("404 Not Found"),
        }
    } else {
        match Asset::get(&path) {
            Some(content) => {
                let mime = from_path(&path).first_or_octet_stream();
                HttpResponse::Ok()
                    .content_type(mime.as_ref())
                    .body(content.data.into_owned())
            }
            // If the file is not found, return `index.html` to support React
            // Router.
            None => match Asset::get("index.html") {
                Some(content) => HttpResponse::Ok()
                    .content_type("text/html")
                    .body(content.data.into_owned()),
                None => HttpResponse::NotFound().body("404 Not Found"),
            },
        }
    }
}
