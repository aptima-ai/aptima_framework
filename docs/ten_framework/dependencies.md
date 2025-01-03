# Dependencies

The TEN framework utilizes several third-party libraries. Some are used specifically for testing, while others are integrated into the TEN runtime. Below is a description of these libraries, along with any necessary modifications required for their use within the TEN framework.

## Google gn

Version: 2112 (1de45d1a11cc)

Download directly from [Google GN webpage](https://gn.googlesource.com/gn/).

## Google ninja

Version: 1.11.1

Download directly from [Ninja release page](https://github.com/ninja-build/ninja/releases).

## Jansson

Version: 2.14

[MIT license](https://github.com/akheron/jansson/blob/master/LICENSE)

This is used in the TEN framework core to parse and generate JSON data. Please refer to `third_party/jansson` for details.

## libuv

Version: 1.49.2

[MIT license](https://github.com/libuv/libuv#licensing)

This is one of the event loop libraries used in the TEN runtime. Please refer to `third_party/libuv` for details.

## msgpack-c

Version: 6.1.0

[Boost Software License, Version 1.0](https://github.com/msgpack/msgpack-c#license)

A MessagePack library for C. Please refer to `third_party/msgpack` for details.

## libwebsockets

Version: 4.3.2

[MIT license](https://github.com/warmcat/libwebsockets/blob/main/LICENSE)

Canonical libwebsockets.org networking library. Please refer to `third_party/libwebsockets` for details.

```diff
--- /home/wei/MyData/Temp/libwebsockets-4.3.2/CMakeLists.txt
+++ libwebsockets/CMakeLists.txt
@@ -851,6 +851,7 @@
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
  # Fail the build if any warnings
  add_compile_options(/W3 /WX)
+ add_compile_options(/Zc:preprocessor /wd4819)
  # Unbreak MSVC broken preprocessor __VA_ARGS__ behaviour
  if (MSVC_VERSION GREATER 1925)
   add_compile_options(/Zc:preprocessor /wd5105)
```

Apply the following patch.

```diff
diff --git a/third_party/libwebsockets/cmake/lws_config.h.in b/third_party/libwebsockets/cmake/lws_config.h.in
index f3f4a9d79..e8d36c3ae 100644
--- a/third_party/libwebsockets/cmake/lws_config.h.in
+++ b/third_party/libwebsockets/cmake/lws_config.h.in
@@ -238,3 +238,9 @@
 #cmakedefine LWS_WITH_PLUGINS_API
 #cmakedefine LWS_HAVE_RTA_PREF

+// NOTE (Liu): The libwebsockets library will use external variables from mbedtls
+// if 'LWS_WITH_MBEDTLS' is enabled. On Windows, variable declarations in the header
+// file must start with '__declspec(dllimport)', otherwise, the
+// symbols cannot be accessed.
+// Refer to third_party/mbedtls/include/mbedtls/export.h.
+#define USING_SHARED_MBEDTLS
diff --git a/third_party/libwebsockets/lib/tls/mbedtls/wrapper/platform/ssl_pm.c b/third_party/libwebsockets/lib/tls/mbedtls/wrapper/platform/ssl_pm.c
index e8a0cb2d4..84a164e90 100755
--- a/third_party/libwebsockets/lib/tls/mbedtls/wrapper/platform/ssl_pm.c
+++ b/third_party/libwebsockets/lib/tls/mbedtls/wrapper/platform/ssl_pm.c
@@ -183,7 +183,12 @@ int ssl_pm_new(SSL *ssl)
         mbedtls_ssl_conf_min_version(&ssl_pm->conf, MBEDTLS_SSL_MAJOR_VERSION_3, version);
     } else {
         mbedtls_ssl_conf_max_version(&ssl_pm->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
-    mbedtls_ssl_conf_min_version(&ssl_pm->conf, MBEDTLS_SSL_MAJOR_VERSION_3, 1);
+
+    // NOTE: mbedtls added 'ssl_conf_version_check()' since v3.1.0, and
+    // mbedtls only enables TLS 1.2 by default. So the 'min_tls_version' and
+    // 'max_tls_version' must be '0x303', or 'mbedtls_ssl_setup' will
+    // fail.
+    mbedtls_ssl_conf_min_version(&ssl_pm->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
     }

     mbedtls_ssl_conf_rng(&ssl_pm->conf, mbedtls_ctr_drbg_random, &ssl_pm->ctr_drbg);
```

### Fix for linking mbedtls on Windows

Apply the following patch if the CMake version is higher than 3.24, as `find_package` supports the `GLOBAL` keyword since 3.24.

```diff
diff --git a/third_party/libwebsockets/lib/tls/mbedtls/CMakeLists.txt b/third_party/libwebsockets/lib/tls/mbedtls/CMakeLists.txt
index e34151724..79b15089d 100644
--- a/third_party/libwebsockets/lib/tls/mbedtls/CMakeLists.txt
+++ b/third_party/libwebsockets/lib/tls/mbedtls/CMakeLists.txt
-        find_library(MBEDTLS_LIBRARY mbedtls)
-        find_library(MBEDX509_LIBRARY mbedx509)
-        find_library(MBEDCRYPTO_LIBRARY mbedcrypto)
+        # find_library(MBEDTLS_LIBRARY mbedtls)
+        # find_library(MBEDX509_LIBRARY mbedx509)
+        # find_library(MBEDCRYPTO_LIBRARY mbedcrypto)
+
+        # set(LWS_MBEDTLS_LIBRARIES "${MBEDTLS_LIBRARY}" "${MBEDX509_LIBRARY}" "${MBEDCRYPTO_LIBRARY}")
+
+        # Set the custom search path.
+        set(MBEDTLS_DIR "${MBEDTLS_BUILD_PATH}/cmake")
+
+        // NOTE (Liu): We should use 'find_package' rather than 'find_library'
+        // to search for the mbedtls libraries. 'find_library' only finds a
+        // library, shared or static, without any settings from the external
+        // library.

-        set(LWS_MBEDTLS_LIBRARIES "${MBEDTLS_LIBRARY}" "${MBEDX509_LIBRARY}" "${MBEDCRYPTO_LIBRARY}")
+        // The 'lib/tls/CMakeLists.txt' depends on the mbedtls libraries, so we set the scope to 'GLOBAL' here.
+        find_package(MbedTLS GLOBAL)
+        set(LWS_MBEDTLS_LIBRARIES MbedTLS::mbedcrypto MbedTLS::mbedx509 MbedTLS::mbedtls)
```

```diff
diff --git a/third_party/libwebsockets/BUILD.gn b/third_party/libwebsockets/BUILD.gn
index 4c89c2e2e..e6d641b9c 100644
--- a/third_party/libwebsockets/BUILD.gn
+++ b/third_party/libwebsockets/BUILD.gn
@@ -84,6 +84,10 @@ cmake_project("websockets") {
   }

   options += [
+  // The libwebsockets will use the 'MBEDTLS_BUILD_PATH' variable to set the
+  // search path of 'find_package'.
+  "MBEDTLS_BUILD_PATH=" + rebase_path("$root_gen_dir/cmake/mbedtls"),
+
```

And remove `#define USING_SHARED_MBEDTLS` in `third_party/libwebsockets/cmake/lws_config.h.in`.

## curl

Version: 8.1.2

[MIT-like license](https://github.com/curl/curl/blob/master/COPYING)

A command-line tool and library for transferring data with URL syntax, supporting DICT, FILE, FTP, FTPS, GOPHER, GOPHERS, HTTP, HTTPS, IMAP, IMAPS, LDAP, LDAPS, MQTT, POP3, POP3S, RTMP, RTMPS, RTSP, SCP, SFTP, SMB, SMBS, SMTP, SMTPS, TELNET, and TFTP. libcurl offers a myriad of powerful features.

Please refer to `third_party/curl` for details.

### Patches for curl

Patch `lib/CMakeLists.txt`, to change the shared library name from `_imp.lib` to `.lib`.

```cmake
if(WIN32)
  if(BUILD_SHARED_LIBS)
    if(MSVC)
      // Add "_imp" as a suffix before the extension to avoid conflicting with
      // the statically linked "libcurl.lib"
      //set_target_properties(${LIB_NAME} PROPERTIES IMPORT_SUFFIX "_imp.lib")
      set_target_properties(${LIB_NAME} PROPERTIES IMPORT_SUFFIX ".lib")
    endif()
  endif()
endif()
```

Export `Curl_ws_done` in `lib/ws.h`, because this function needs to be called to prevent memory leaks.

```c
CURL_EXTERN void Curl_ws_done(struct Curl_easy *data);
// ^^^^^^^^
```

## clingo

Version: 5.6.2

[MIT license](https://github.com/potassco/clingo/blob/master/LICENSE.md)

A grounder and solver for logic programs.

Please refer to `third_party/clingo` for details.

## FFmpeg

Version: 6.0

[GPL or LGPL license](https://github.com/FFmpeg/FFmpeg/blob/master/LICENSE.md)

The FFmpeg codebase is mainly LGPL-licensed with optional components licensed under GPL. Please refer to the LICENSE file for detailed information.

Used in ffmpeg extensions, primarily for testing purposes. Please refer to `third_party/ffmpeg` for details.

## libbacktrace

Version: 1.0

[BSD license](https://github.com/ianlancetaylor/libbacktrace/blob/master/LICENSE)

A C library that may be linked into a C/C++ program to produce symbolic backtraces.

> ⚠️ **Note:**
> We have significantly modified `libbacktrace` to conform to the naming conventions and folder structure of the TEN framework. Please refer to `core/src/ten_utils/backtrace` for details.

## uthash

Version: 2.3.0

[BSD license](https://github.com/troydhanson/uthash/blob/master/LICENSE)

C macros for hash tables and more.

> ⚠️ **Note:**
> We have significantly modified `uthash` to conform to the naming conventions and folder structure of the TEN framework. Please refer to the files under `core/include/ten_utils/container` that have `uthash` mentioned in the file headers.

## uuid4

[MIT license](https://github.com/gpakosz/uuid4/blob/master/LICENSE.MIT)
[WTFPLv2 license](<https://github.com/gpakosz/uuid4/blob/master/LICENSE.WTFPLv2>)

UUID v4 generation in C.

> ⚠️ **Note:**
> We have significantly modified `uuid4` to conform to the naming conventions and folder structure of the TEN framework. Please refer to `core/src/ten_utils/lib/sys/general/uuid.c` for details.

## zf_log

[MIT license](https://github.com/wonder-mice/zf_log/blob/master/LICENSE)

Core logging library for C/ObjC/C++.

> ⚠️ **Note:**
> We have significantly modified `zf_log` to conform to the naming conventions and folder structure of the TEN framework. Please refer to `core/src/ten_utils/log` for details.
