//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#[derive(Debug)]
pub enum TmanError {
    FileNotFound(String),
    ReadFileContentError(String),
    InvalidPath(String, String),
}

// One of the purposes of an error is to be displayed, so it needs to implement
// the Display trait.
impl std::fmt::Display for TmanError {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            TmanError::FileNotFound(file_path) => {
                write!(f, "'{}' is not found.", file_path)
            }
            TmanError::ReadFileContentError(file_path) => {
                write!(f, "Failed to read '{}'.", file_path)
            }
            TmanError::InvalidPath(path, root_cause) => {
                write!(f, "The path '{}' is not valid: {}.", path, root_cause)
            }
        }
    }
}

impl std::error::Error for TmanError {}
