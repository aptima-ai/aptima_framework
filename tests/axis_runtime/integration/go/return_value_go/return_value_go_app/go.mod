module app

go 1.20

replace aptima_framework => ./aptima_packages/system/aptima_runtime_go/interface

replace go_common_dep => ./go_common_dep

require aptima_framework v0.0.0-00010101000000-000000000000

require go_common_dep v0.0.0-00010101000000-000000000000
