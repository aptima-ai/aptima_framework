//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
pub mod cmd_delete;
pub mod cmd_dev_server;
pub mod cmd_install;
pub mod cmd_package;
pub mod cmd_publish;
pub mod cmd_uninstall;

use anyhow::Result;

use super::config::TmanConfig;

pub enum CommandData {
    Install(self::cmd_install::InstallCommand),
    Uninstall(self::cmd_uninstall::UninstallCommand),
    Package(self::cmd_package::PackageCommand),
    Publish(self::cmd_publish::PublishCommand),
    Delete(self::cmd_delete::DeleteCommand),
    DevServer(self::cmd_dev_server::DevServerCommand),
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
        CommandData::DevServer(cmd) => {
            crate::cmd::cmd_dev_server::execute_cmd(tman_config, cmd).await
        }
    }
}
