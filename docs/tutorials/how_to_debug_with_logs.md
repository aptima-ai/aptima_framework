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

# How to debug with logs

In this chapter, we’ll cover how to view the logs and understand what they mean.

For instance, if the Astra AI agent is running at localhost:3000, you might see output similar to the following in the logs:

<pre class="language-bash" data-title=">_ Bash" data-overflow="wrap"><code class="lang-bash">...
2024/08/13 05:18:38 INFO handlerPing start channelName=agora_aa1aou requestId=435851e4-b5ff-437a-9930-14ae94b1dee7 service=HTTP_SERVER
<strong>2024/08/13 05:18:38 INFO handlerPing end worker="&#x26;{ChannelName:agora_aa1aou LogFile:/tmp/astra/app-5ea31f2d5d7a5e48a9dbc583959c1b17-1723525636779876885.log PropertyJsonFile:/tmp/property-5ea31f2d5d7a5e48a9dbc583959c1b17-1723525636779876885.json Pid:245 QuitTimeoutSeconds:60 CreateTs:1723525636 UpdateTs:1723526318}" requestId=435851e4-b5ff-437a-9930-14ae94b1dee7 service=HTTP_SERVER
</strong>[GIN] 2024/08/13 - 05:18:38 | 200 |     2.65725ms |    192.168.65.1 | POST     "/ping"
...
</code></pre>

The line starting with LogFile: is what we’re interested in. To view the log file, use the following command:

<pre class="language-bash" data-title=">_Bash"><code class="lang-bash"><strong>cat /tmp/astra/app-5ea31f2d5d7a5e48a9dbc583959c1b17-1723525636779876885.log
</strong></code></pre>

You should then see entries like these:

{% code title=">_Bash" overflow="wrap" %}
```bash
[SttMs] OnSpeechRecognized
2024/08/13 04:40:43.365841 INFO GetChatCompletionsStream recv for input text: [What's going on? ] sent sentence [Not much,] extension=OPENAI_CHATGPT_EXTENSION
2024/08/13 04:40:43.366019 INFO GetChatCompletionsStream recv for input text: [What's going on? ] first sentence sent, first_sentency_latency 876ms extension=OPENAI_CHATGPT_EXTENSION
2024/08/13 04:40:43.489795 INFO GetChatCompletionsStream recv for input text: [What's going on? ] sent sentence [ just here and ready to chat!] extension=OPENAI_CHATGPT_EXTENSION
2024/08/13 04:40:43.493444 INFO GetChatCompletionsStream recv for input text: [What's going on? ] sent sentence [ How about you?] extension=OPENAI_CHATGPT_EXTENSION
2024/08/13 04:40:43.495756 INFO GetChatCompletionsStream recv for input text: [What's going on? ] sent sentence [ What's on your mind?] extension=OPENAI_CHATGPT_EXTENSION
2024/08/13 04:40:43.496120 INFO GetChatCompletionsStream for input text: [What's going on? ] end of segment with sentence [] sent extension=OPENAI_CHATGPT_EXTENSION
```
{% endcode %}

When you see logs like this, it means the system is working correctly and logging each sentence you say.
