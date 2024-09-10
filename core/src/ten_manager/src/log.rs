//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
macro_rules! tman_verbose_println {
    ($tman_config:expr, $($arg:tt)*) => {
        if $tman_config.verbose {
            println!($($arg)*);
        }
    };
}

pub(crate) use tman_verbose_println;
