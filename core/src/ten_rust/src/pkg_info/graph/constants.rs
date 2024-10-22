//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub const ERR_MSG_APP_LOCALHOST_DISALLOWED_SINGLE: &str =
    "'localhost' is not allowed in graph definition, and the graph seems to be a single-app graph, just remove the 'app' field";

pub const ERR_MSG_APP_LOCALHOST_DISALLOWED_MULTI: &str =
    "'localhost' is not allowed in graph definition, change the content of 'app' field to be consistent with '_ten::uri'";

pub const ERR_MSG_APP_DECLARATION_MISMATCH: &str = "Either all nodes should have 'app' declared, or none should, but not a mix of both.";

pub const ERR_MSG_APP_IS_EMPTY: &str = "the 'app' field can not be empty, remove the field if the graph is a single-app graph";

pub const ERR_MSG_APP_SHOULD_NOT_DECLARED: &str =
    "the 'app' should not be declared, as not any node has declared it";

pub const ERR_MSG_APP_SHOULD_DECLARED: &str =
    "the 'app' can not be none, as it has been declared in nodes";
