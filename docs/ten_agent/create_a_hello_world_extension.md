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

# Create a hello world extension

In this chapter, we are going to create a `Hello World` extension step by step, available in Python, Go, and C++. Feel free to choose whichever language you prefer. So buckle up.

## Prerequisites

Before diving into this chapter, you’ll need to be familiar with the [basics covered earlier](quickstart.md). Specifically, ensure you understand how to use `docker compose up` and are aware of the services running in the background.

## 1. Compose up the servers

First things first, let’s start by composing the servers. Run the following command:

{% hint style="info" %}
If the caption says `Terminal`, it means you are running the command locally. If the caption says `Bash`, it means you are running the command in the Docker container.
{% endhint %}

{% code title=">_ Terminal" %}

```bash
docker compose up
```

{% endcode %}

Once the command is entered, you should see output similar to this:

<pre class="language-bash" data-title=">_ Terminal"><code class="lang-bash">....
Attaching to astra_agents_dev, astra_playground, ten_graph_designer
astra_agents_dev    | >> run graph designer server
astra_agents_dev    | cd agents && tman dev-server
astra_agents_dev    | :-)  Starting server at http://0.0.0.0:49483
astra_playground    |   ▲ Next.js 14.2.4
astra_playground    |   - Local:        http://localhost:3000
astra_playground    |   - Network:      http://0.0.0.0:3000
astra_playground    |
astra_playground    |  ✓ Starting...
ten_graph_designer  |   ▲ Next.js 14.2.4
ten_graph_designer  |   - Local:        http://localhost:3000
ten_graph_designer  |   - Network:      http://0.0.0.0:3000
ten_graph_designer  |
ten_graph_designer  |  ✓ Starting...
astra_playground    |  ✓ Ready in 394ms
ten_graph_designer  |  ✓ Ready in 387ms
...
</code></pre>

Now, we’ve got the following services running:

• `astra_agents_dev` at `http://0.0.0.0:49483` (the backend server)

• `astra_playground` at `http://localhost:3000` (the frontend of TEN Agent)

• `ten_graph_designer` at `http://localhost:3001` (the frontend of Graph Designer)

## 2. Enter the docker container

To work within the isolated environment, run the following command:

{% code title=">_ Terminal" %}

```bash
docker exec -it astra_agents_dev bash
```

{% endcode %}

## 3. Create the hello world extension

By running the following commands, an extension called `hello_world` will be created in Python, Go, or C++.

{% tabs %}
{% tab title="Python" %}
<pre class="language-bash" data-title=">_ Bash" data-overflow="wrap"><code class="lang-bash">cd agents

<strong>tman install extension default_extension_python --template-mode --template-data package_name=hello_world --template-data class_name_prefix=HelloWorld
</strong>
</code></pre>
{% endtab %}

{% tab title="Go" %}
<pre class="language-bash" data-title=">_ Bash" data-overflow="wrap"><code class="lang-bash">cd agents

<strong>tman install extension default_extension_go --template-mode --template-data package_name=hello_world --template-data class_name_prefix=HelloWorld
</strong>
</code></pre>
{% endtab %}

{% tab title="C++" %}
<pre class="language-bash" data-title=">_ Bash" data-overflow="wrap"><code class="lang-bash">cd agents

<strong>tman install extension default_extension_cpp --template-mode --template-data package_name=hello_world --template-data class_name_prefix=HelloWorld
</strong>
</code></pre>
{% endtab %}
{% endtabs %}

After running the command, the log will display something like this:

<pre class="language-bash" data-title=">_ Bash"><code class="lang-bash">...
Resolving packages...
<strong>:-)  Install successfully in xxx seconds
</strong>...
</code></pre>

## 4. Adding API to the extension

Navigate into the `hello_world` directory and open manifest.json. Add the API objects with `data_in` and `cmd_out`, which we will use shortly within the Graph Designer:

<pre class="language-json" data-title="./hello_world/manifest.json"><code class="lang-json">{
  "type": "extension",
  "name": "hello_world",
  "version": "0.4.1",
  "language": "python",
  "dependencies": [
    {
      "type": "system",
      "name": "rte_runtime_python",
      "version": "0.4.1"
    }
  ],
  "api": {
<strong>    "data_in": [
</strong><strong>      {
</strong><strong>        "name": "text_data",
</strong><strong>        "property": {
</strong><strong>          "text": {
</strong><strong>            "type": "string"
</strong><strong>          },
</strong><strong>          "is_final": {
</strong><strong>            "type": "bool"
</strong><strong>          }
</strong><strong>        }
</strong><strong>      }
</strong><strong>    ],
</strong><strong>    "cmd_out": [
</strong><strong>      {
</strong><strong>        "name": "flush"
</strong><strong>      }
</strong><strong>    ]
</strong>  }
}
</code></pre>

## 5. Build the extension

Let's use `cd /app` command to go back to the root of the project, and run `make build` to build the extension.

{% code title=">_ Bash" %}

```bash
cd /app

make build
```

{% endcode %}

## 6. Restart the server

You don’t need to restart the server when you first build the agent. However, after making minor updates, if refreshing the page doesn’t apply the changes, you’ll need to restart the server in Docker to ensure the updates take effect.

<figure><img src="../assets/gif/docker_restart_server.gif" alt=""><figcaption><p>Restart the server for astra_agents_dev</p></figcaption></figure>

## 7. Verify the extension&#x20;

Open `http://localhost:3001` in your browser. You should see `hello_world` in the left menu. Drag it to the canvas, and connect it to the `text_data` input and `flash` output.

You see the green and red color indicting the possible routes of node connecting.&#x20;

<figure><img src="../assets/gif/hello_world_python.gif" alt=""><figcaption><p>hello_world extension</p></figcaption></figure>

Congratulations! You’ve successfully created your first `hello_world` extension, and it’s working seamlessly within the Graph Designer canvas.