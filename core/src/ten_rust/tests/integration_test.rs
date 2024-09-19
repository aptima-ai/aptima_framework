//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//

// Each file in the tests directory is a separate crate, and will be compiled as
// an executable. We need a all-in-one executable to run all the tests, and we
// will copy the all-in-one executable to the out directory. We use this main
// file to achieve this purpose.

fn main() {
    println!("Running integration tests of ten_rust...");
}

// Those following mods will be compiled in one executable.
mod graph_check;
mod interface;
