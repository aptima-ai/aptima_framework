# Profiling

## TEN Framework GO Project

Use `gperftools` to profile your program's CPU usage and heap memory usage. The following steps describe how to prepare the environment, use `gperftools` to profile your program, and analyze the performance data.

### Installing `gperftools`

To install `gperftools` on Ubuntu, run the following commands:

```shell
apt update && apt install -y google-perftools

ln -s /usr/lib/x86_64-linux-gnu/libtcmalloc.so.4 /usr/lib/libtcmalloc.so

ln -s /usr/lib/x86_64-linux-gnu/libprofiler.so.0 /usr/lib/libprofiler.so
```

### Profiling CPU Usage with `gperftools`

To profile the CPU usage of your program, run the following commands:

```shell
export LD_PRELOAD=/usr/lib/libtcmalloc.so
export CPUPROFILE=/.../cpu

exec /.../program
```

When the program exits, a CPU performance data file will be generated. The data will be stored in the `/.../cpu.0001.prof` file.

If you want to generate CPU performance data periodically, you can set the `CPUPROFILE_FREQUENCY` environment variable. For example, to generate CPU performance data every 30 seconds, run the following command:

```shell
LD_PRELOAD=/usr/lib/libtcmalloc.so CPUPROFILE=/.../cpu CPUPROFILE_FREQUENCY=30 exec /.../program
```

#### Analyzing CPU Performance Data

To analyze the CPU performance data, run the following commands:

```shell
google-pprof --text /.../program /.../cpu.0001.prof > /.../cpu.0001.prof.txt

google-pprof --text --base=/.../cpu.0001.prof /.../program /.../cpu.0002.prof > /.../diff.txt
```

### Profiling Heap Memory Usage with `gperftools`

To profile your program's heap memory usage, run the following commands:

```shell
export LD_PRELOAD=/usr/lib/libtcmalloc.so
export HEAPPROFILE=/.../heap
export HEAP_PROFILE_TIME_INTERVAL=30

exec /.../program
```

It is recommended to use the `HEAP_PROFILE_TIME_INTERVAL` environment variable to periodically generate heap memory performance data. All heap memory performance data will be stored in the `HEAPPROFILE` directory.

#### Analyzing Heap Memory Performance Data

Each generated heap memory performance data file will have a `.heap` extension. To analyze the heap memory performance data, you can convert the `.heap` files to a human-readable text format. For example, to convert a `.heap` file in `/.../heap` to a text format, run the following command:

```shell
google-pprof --text /.../program /.../heap.0001.heap > /.../heap.0001.heap.txt
```

The performance data will look like this:

```text
Total: 19.7 MB
16.0  81.3%  81.3%     16.0  81.3% lws_zalloc
2.9  14.6%  95.9%      3.3   16.7% setAgoraStreamChannelParameters
...
```

You can also convert the `.heap` file to other formats like PDF or SVG. For example, to convert a `.heap` file in `/.../heap` to PDF format, run the following command:

```shell
google-pprof --pdf /.../program /.../heap.0001.heap > /.../heap.0001.heap.pdf
```

The performance data in PDF format will be similar to the text output but visualized graphically.

Additionally, you can compare two sets of heap memory performance data. For example, to compare the heap memory performance data in `/.../heap.0001.heap` and `/.../heap.0002.heap`, run the following command:

```shell
google-pprof --pdf --base=/.../heap.0001.heap /.../program /.../heap.0002.heap > /.../diff.pdf
```

The PDF will show the differences in heap memory usage between the two data sets.

If you are generating heap memory performance data periodically, we provide a script to analyze all the heap memory performance data in a directory and generate an Excel file showing the heap memory usage trends. For example, to analyze all heap memory performance data in `/.../heap`, run the following command:

```shell
python3 tools/profiler/gperftools/dump_heap_info_to_excel.py -heap_dir=/.../heap -bin=/.../program -output=/.../heap.xlsx -sample_interval=30
```

The script will generate raw performance data with symbols in the `/.../raw` directory, human-readable text performance data in the `/.../text` directory, and an Excel file in `/.../heap.xlsx`.

You can plot a line chart to display the heap memory usage trends.

```shell
python3 tools/profiler/gperftools/draw_line_chart.py -excel_file=/.../heap.xlsx -output_file=/.../heap_line_chart.png -title=HEAP_MEM -x=time/s -y=total_heap_size/MB
```

### Profiling with `pprof`

We recommend using `pprof` to profile programs written in Go. The easiest way to profile a Go program is by using `pprof_app_go` as your TEN application. When activated, the application reads environment variables to decide whether to activate the `pprof` server and heap profiler.

```shell
export TEN_HEAP_DUMP_DIR=/data/prof
export HEAP_PROFILE_TIME_INTERVAL=30
export TEN_PROFILER_SERVER_PORT=6060
```

### Analyzing `pprof` Performance Data

To convert heap performance data to a text format, use the following command:

```shell
python3 tools/profiler/pprof/dump_heap_files_to_text.py -heap_dir=/data/prof -text_dir=/data/text
```

To analyze all heap performance data and generate an Excel file, run:

```shell
python3 tools/profiler/pprof/dump_heap_info_to_excel.py -heap_dir=/data/prof -output=/data/heap.xlsx
```

You can plot a line chart to display the heap memory usage trends:

```shell
python3 tools/profiler/gperftools/draw_line_chart.py -excel_file=/data/heap.xlsx -output_file=/data/heap_line_chart.png -title=HEAP_MEM -x=time/s -y=total_heap_size/MB
```

This will give you a visual representation of Go heap performance data trends.
