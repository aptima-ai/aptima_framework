//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
pub mod cmd_check;
pub mod cmd_delete;
pub mod cmd_designer;
pub mod cmd_install;
pub mod cmd_package;
pub mod cmd_publish;
pub mod cmd_run;
pub mod cmd_uninstall;

use anyhow::Result;

use super::config::TmanConfig;

pub enum CommandData {
    Install(self::cmd_install::InstallCommand),
    Uninstall(self::cmd_uninstall::UninstallCommand),
    Package(self::cmd_package::PackageCommand),
    Publish(self::cmd_publish::PublishCommand),
    Delete(self::cmd_delete::DeleteCommand),
    Designer(self::cmd_designer::DesignerCommand),
    Check(self::cmd_check::CheckCommandData),
    Run(self::cmd_run::RunCommand),
}

pub async fn execute_cmd(
    tman_config: &TmanConfig,
    command_data: CommandData,
) -> Result<()> {
    match command_data {
        CommandData::Install(cmd) => {
            crate::cmd::cmd_install::execute_cmd(tman_config, cmd).await
        }
        CommandData::Uninstall(cmd) => {
            crate::cmd::cmd_uninstall::execute_cmd(tman_config, cmd).await
        }
        CommandData::Package(cmd) => {
            crate::cmd::cmd_package::execute_cmd(tman_config, cmd).await
        }
        CommandData::Publish(cmd) => {
            crate::cmd::cmd_publish::execute_cmd(tman_config, cmd).await
        }
        CommandData::Delete(cmd) => {
            crate::cmd::cmd_delete::execute_cmd(tman_config, cmd).await
        }
        CommandData::Designer(cmd) => {
            crate::cmd::cmd_designer::execute_cmd(tman_config, cmd).await
        }
        CommandData::Check(cmd) => {
            crate::cmd::cmd_check::execute_cmd(tman_config, cmd).await
        }
        CommandData::Run(cmd) => {
            crate::cmd::cmd_run::execute_cmd(tman_config, cmd).await
        }
    }
}
