//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
macro_rules! tman_verbose_println {
    ($tman_config:expr, $($arg:tt)*) => {
        if $tman_config.verbose {
            println!($($arg)*);
        }
    };
}

macro_rules! tman_verbose_print {
    ($tman_config:expr, $($arg:tt)*) => {
        if $tman_config.verbose {
            print!($($arg)*);
        }
    };
}

pub(crate) use tman_verbose_print;
pub(crate) use tman_verbose_println;
