# FAQs

## Why do I see `/app/agnets/bin/start: not found`?

Windows users occasionally encounter line-ending issues, which may result in errors like `/app/agnets/bin/start: not found`.

For best practices, we recommend configuring Git to automatically convert line endings to LF on Windows.

To fix this:
1. Configure Git to automatically convert line endings to LF on Windows:

{% code title=">_ Terminal" %}
```bash
git config --global core.autocrlf false
```
{% endcode %}

2. Re-clone the repository

Alternatively, you can download and extract the ZIP file directly from GitHub.

## How to verify my internet connection?

Please make sure that both of you your **HTTPS** and **SSH** are connected to the internet.

Test **HTTPS** connection:

{% code title=">_ Terminal" %}
```bash
ping www.google.com

# You should see the following output:
PING google.com (198.18.1.94): 56 data bytes
64 bytes from 198.18.1.94: icmp_seq=0 ttl=64 time=0.099 ms
64 bytes from 198.18.1.94: icmp_seq=1 ttl=64 time=0.121 ms
```
{% endcode %}

Test **SSH** connection:

{% code title=">_ Terminal" %}
```bash
curl www.google.com

# You should see the following output:
<html>
<head><title>301 Moved Permanently</title></head>
<body>
<h1>301 Moved Permanently</h1>
</body>
</html>
```
{% endcode %}

## How to refresh env file?

To see updated changes during development, follow these steps:

1. Stop the server
2. Save changes to the `.env` file
3. Run `source .env` to refresh the environment variables

## Line-ending Issues on Windows

Windows users occasionally encounter line-ending issues, which may result in errors like "agent/bin/start is not a valid directory".

To fix this:
1. Configure Git to automatically convert line endings to LF on Windows:

{% code title=">_ Terminal" %}
```bash
git config --global core.autocrlf false
```
{% endcode %}

2. Re-clone the repository

Alternatively, you can download and extract the ZIP file directly from GitHub.
