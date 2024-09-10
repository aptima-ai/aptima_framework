# Version System

The TEN framework follows the semver specification.

## Version Information of TEN Package

The version information of a TEN package is recorded in the `version` field of its own `manifest.json` file. For every TEN package, this `version` field is mandatory.

```json
{
  "version": "1.0.0"
}
```

## Version Requirement of Dependency

> ⚠️ **Note:**
> If no version requirement is specified for a dependency, the latest version is implied.

```text
└── ten_packages/
    ├── extension/
    ├── extension_group/
    ├── protocol/
    └── system/
```

These packages are distinguished by the value of the `type` field in their `manifest.json`.

```json
{
  "type": "system", // "extension", "extension_group", "protocol"
}
```

In a TEN package, the version requirements for the dependent TEN packages are specified as follows.

```json
{
  "dependencies": [
    {
      "type": "system",
      "name": "ten_runtime",
      "version": "1.0.1"
    },
    {
      "type": "system",
      "name": "ten_runtime_go",
      "version": "1.0.3"
    },
    {
      "type": "extension",
      "name": "default_http_extension_cpp",
      "version": "1.0.0"
    },
    {
      "type": "extension_group",
      "name": "default_extension_group",
      "version": "1.0.0"
    }
  ]
}
```

## Version Requirements Specification

A version requirements string can contain multiple version requirement strings, separated by commas. Each version requirement string supports the following operators:

- `>`, `>=`, `=`

  Manually specify a version range or an exact dependency version.

- `~`

  Used to specify a minimum version with some flexibility for updates. If the major, minor, and patch versions are specified, or only the major and minor versions are specified, only patch-level changes are allowed. If only the major version is specified, both minor and patch-level changes are allowed.

- `^`

  The `^` operator can be omitted, meaning that if a version requirement string does not include any operator, it defaults to `^`. For example, the string `0.1.12` represents `^0.1.12`. Although `0.1.12` looks like a specific version, it actually specifies a version requirement that allows SemVer-compatible updates.

  Updates are allowed as long as they do not modify the leftmost non-zero number in the major, minor, or patch combination. In this case, if you upgrade the TEN package, it will update to version `0.1.13` (if it's the latest `0.1.z` version), but not to `0.2.0`. If the version string specified is `1.0`, it will update to `1.1` (if it's the latest `1.y` version), but not to `2.0`.

  > ⚠️ **Note:**
  > Versions `0.0.x` are considered incompatible with any other versions.
