---
layout:
  title:
    visible: true
  description:
    visible: false
  tableOfContents:
    visible: true
  outline:
    visible: true
  pagination:
    visible: true
---

# Quickstart

In this chapter, let's build the APTIMA Agent playground together.

## Prerequisites

{% tabs %}
{% tab title="API Keys" %}

* Agora [ App ID ](https://docs.agora.io/en/video-calling/get-started/manage-agora-account?platform=web#create-an-agora-project) and [ App Certificate ](https://docs.agora.io/en/video-calling/get-started/manage-agora-account?platform=web#create-an-agora-project)(free minutes every month)
<!-- * [OpenAI](https://openai.com/index/openai-api/) API key -->
<!-- * Azure [speech-to-text](https://azure.microsoft.com/en-us/products/ai-services/speech-to-text) and [text-to-speech](https://azure.microsoft.com/en-us/products/ai-services/text-to-speech) API keys -->
{% endtab %}

{% tab title="Installations" %}

* [Docker](https://www.docker.com/) / [Docker Compose](https://docs.docker.com/compose/)
* [Node.js(LTS) v18](https://nodejs.org/en)
{% endtab %}

{% tab title="Minimum system requirements" %}
:tada: CPU >= 2 Core

:smile: RAM >= 4 GB
{% endtab %}
{% endtabs %}

**Docker setting on Apple Silicon**

{% hint style="info" %}
For Apple Silicon Macs, uncheck "Use Rosetta for x86/amd64 emulation" in Docker settings. Note: This may result in slower build times on ARM, but performance will be normal when deployed to x64 servers.
{% endhint %}

<figure><img src="../assets/gif/docker_setting.gif" alt="" width="563"><figcaption><p>Make sure the box is unchecked</p></figcaption></figure>

## Next step

**1. Clone down the APTIMA Agent repository**

{% code title=">_ Terminal" %}

```sh
git clone https://github.com/APTIMA-framework/APTIMA-Agent.git
```

{% endcode %}

**2. Prepare config files**

Open APTIMA Agent in your code editor. In the root of the project, use `cd` command to create \`.env\` file from example.

{% code title=">_ Terminal" %}

```sh
cp ./.env.example ./.env
```

{% endcode %}

**3. Setup Agora App ID and App Certificate in   .env file**

Open the `.env` file and fill in Agora App ID and App Certificate.These will be used to connect to Agora RTC extension.

{% code title=".env" %}

```bash
AGORA_APP_ID=
AGORA_APP_CERTIFICATE=
```

{% endcode %}

**4. Start agent builder toolkit containers**

In the same directory, run the `docker` command to compose containers:

{% code title=">_ Terminal" %}

```bash
docker compose up -d
```

{% endcode %}

**5. Enter container**

Use the following command to enter the container:

{% code title=">_ Bash" %}

```bash
docker exec -it ten_agent_dev bash
```

{% endcode %}

**6. Build the agent**

Use the following command to build the agent:

{% code title=">_ Bash" %}

```bash
task use
```

{% endcode %}

**7. Start the web server**

Use the following command to start the web server:

{% code title=">_ Bash" %}

```bash
task run
```

{% endcode %}


**8. Edit playground settings**

Open the playground at [localhost:3000](http://localhost:3000) to configure your agent.
 1. Select a graph type (e.g. Voice Agent, Realtime Agent)
 2. Choose a corresponding module
 3. Select an extension and configure its API key settings

![Module Example](https://github.com/APTIMA-framework/docs/blob/main/assets/gif/module-example.gif?raw=true)