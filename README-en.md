# etmem

## Introduction

The development of CPU computing power - particularly lower costs of ARM cores - makes memory cost and capacity become the core frustration that restricts business costs and performance. Therefore, the most pressing issue is how to save memory cost and how to expand memory capacity.

etmem is a tiered memory extension technology that uses DRAM+memory compression/high-performance storage media to form tiered memory storage. Memory data is tiered, and cold data is migrated from memory media to high-performance storage media to release memory space and reduce memory costs.

## Compilation Tutorial

1. Download the etmem source code.

    ```
    $ git clone https://gitee.com/openeuler/etmem.git
    ```

2. Compile and run the dependencies.

    The compilation and running of etmem depend on the libboundscheck component.

3. Compile.

    ```
    $ cd etmem
    
    $ mkdir build
    
    $ cd build
    
    $ cmake ..
    
    $ make
    ```


## Instructions

### Starting Etmemd

#### How to Use

Run the etmemd process in binary mode. For example:

```
$ etmemd -l 0 -s etmemd_socket
```

#### Help Information

Options:

```
-l|\-\-log-level <log-level>  Log level

-s|\-\-socket <sockect name>  Name of the socket to be listened to

-h|\-\-help  Show this message

-m|\-\-mode-systemctl Mode used to start (systemctl)
```

#### Command-line Options

| Option            | Description                           | Mandatory | With Parameter or Not | Value Range              | Example Description|
| --------------- | ---------------------------------- | -------- | ---------- | --------------------- | ------------------------------------------------------------ |
| -l or \-\-log-level | etmemd log level                     | No       | Yes         | 0 to 3                   | `0`: debug level. `1`: info level. `2`: warning level. `3`: error level. Only logs of the level that is higher than or equal to the configured level are recorded in the `/var/log/message` file.|
| -s or \-\-socket    | Name of socket to be listened to by etmemd, which is used to interact with the client. | Yes| Yes| A string of fewer than 107 characters| Specify the name of socket to be listened to. |
| -h or \-\-help      | Help information| No| No| N/A| If this option is specified, the command execution exits after the command output is printed.|
| -m or \-\-mode-systemctl|	When etmemd is started as a service, this option can be used in the command to support startup in fork mode.|	No|	No|	N/A|	N/A|
|### etmem configuration file||||||

Before running the etmem process, the administrator needs to plan the processes that require memory extension, configure the process information in the etmem configuration file, and configure the memory scan cycles and times, and cold and hot memory thresholds.

The example configuration file is stored in `conf/example_conf.yaml` of the root directory of the source code in the source code package. You are advised to store the example configuration file in the `/etc/etmem/` directory. The following is an example:

```
[project]
name=test
loop=1
interval=1
sleep=1

#Example of the slide engine
[engine]
name=slide
project=test

[task]
project=test
engine=slide
name=background_slide
type=name
value=mysql
T=1
max_threads=1

#Example of the cslide engine
[engine]
name=cslide
project=test
node_pair=2,0;3,1
hot_threshold=1
node_mig_quota=1024
node_hot_reserve=1024

[task]
project=test
engine=cslide
name=background_cslide
type=pid
name=23456
vm_flags=ht
anon_only=no
ign_host=no

#Example of the thirdparty engine
[engine]
name=thirdparty
project=test
eng_name=my_engine
libname=/usr/lib/etmem_fetch/my_engine.so
ops_name=my_engine_ops
engine_private_key=engine_private_value

[task]
project=test
engine=my_engine
name=backgroud_third
type=pid
value=12345
task_private_key=task_private_value
```

Fields in the configuration file are described as follows.

| Configuration Item| Description| Mandatory | With Parameter or Not | Value Range| Example Description|
|-----------|---------------------|------|-------|------------|-----------------------------------------------------------------|
| [project] | Start flag of the common configuration section of a project| No| No| N/A| Start flag of the `project` configuration item, indicating that the following configuration items, before another *[xxx]* or to the end of the file, belong to the project section|
| name      | Name of a project| Yes| Yes| A string of fewer than 64 characters| Specify the project to‧which‧the‧engine‧and‧task‧are‧to‧be‧mounted. |
| loop      | Number of memory scan cycles| Yes| Yes| 1 to 10       | loop=3 // Scan for three times.|
| interval  | Interval for scanning the memory| Yes| Yes| 1 to 1200     | interval=5 // The scanning interval is 5s.|
| sleep     | Interval between large cycles of each memory scan and operation| Yes| Yes| 1 to 1200     | sleep=10 // The interval between two large cycles is 10s.|
| [engine]      | Start flag of the common configuration section of an engine| No| No| N/A| Start flag of the `engine` configuration item, indicating that the following configuration items, before another *[xxx]* or to the end of the file, belong to the engine section|
| project       | Project to which the engine belongs| Yes| Yes| A string of fewer than 64 characters| If a project named `test` already exists, you can enter `project=test`.|
| engine        | Name of the engine| Yes| Yes| slide/cslide/thirdparty                          | Specify the `slide`, `cslide`, or `thirdparty` policy that is used.|
| node_pair     | Configuration item of the `cslide` engine, which specifies the node pair of the AEP and DRAM in the system | Mandatory when `engine` is set to `cslide`| Yes| Node IDs of the AEP and DRAM are configured in pairs and separated by commas (,). Node pairs are separated by semicolons (;).| node_pair=2,0;3,1                                            |
| hot_threshold | Configuration item of the `cslide` engine, which specifies the threshold of the hot and cold memory| Mandatory when `engine` is set to `cslide`| Yes| Integer (≥ 0)| hot_threshold=3 // Memory that is accessed fewer than 3 times is identified as cold memory.|
|node_mig_quota|Configuration item of the `cslide` engine, which specifies the maximum unidirectional traffic during each migration between the DRAM and AEP|Mandatory when `engine` is set to `cslide`|Yes|Integer (≥ 0)|node_mig_quota=1024 //T he unit is MB. A maximum of 1,024 MB data can be migrated from the AEP to the DRAM or from the DRAM to the AEP at a time.|
|node_hot_reserve|Configuration item of the `cslide` engine, which specifies the size of the reserved space for the hot memory in the DRAM|Mandatory when `engine` is set to `cslide`|Yes|Integer (≥ 0)|node_hot_reserve=1024 // The unit is MB. When the hot memory of all VMs is greater than the value of this configuration item, the hot memory is migrated to the AEP.|
|eng_name|Configuration item of the `thirdparty` engine, which specifies the engine name and is used for task mounting|Mandatory when `engine` is set to `thirdparty`|Yes|A string of fewer than 64 characters|eng_name=my_engine // When a task is mounted to the thirdparty engine, you can enter `engine=my_engine` in the task.|
|libname|Configuration item of the `thirdparty` engine, which specifies the address of the dynamic library of the third-party policy. The address is an absolute address.|Mandatory when `engine` is set to `thirdparty`|Yes|A string of fewer than 64 characters|libname=/user/lib/etmem_fetch/code_test/my_engine.so|
|ops_name|Configuration item of the `thirdparty` engine, which specifies the name of the operator in the dynamic library of the third-party policy|Mandatory when `engine` is set to `thirdparty`|Yes|A string of fewer than 64 characters|ops_name=my_engine_ops // Name of the structure of the third-party policy implementation interface|
|engine_private_key|(Optional) Configuration item of the `thirdparty` engine. This configuration item is reserved for the third-party policy to parse private parameters.|No|No|Configured based on the private parameters of the third-party policy|Set this configuration item based on the private engine parameters of the third-party policy.|
| [task]  | Start flag of the common configuration section of a task| No| No| N/A| Start flag of the `task configuration item`, indicating that the following configuration items, before another *[xxx]* or to the end of the file, belong to the task section|
| project | Project to which the task is mounted| Yes| Yes| A string of fewer than 64 characters| If a project named `test` already exists, you can enter `project=test`.|
| engine  | Engine to which the task is mounted| Yes| Yes| A string of fewer than 64 characters| Specify the name of the engine to which the task is mounted. |
| name    | Name of the task| Yes| Yes| A string of fewer than 64 characters| name=background1 // The task name is `background1`.|
| type    | Method of identifying the target process| Yes| Yes| pid/name    | `pid` indicates that the process is identified based on the process ID, and `name` indicates that the process is identified based on the process name.|
| value   | Specific fields identified by the target process| Yes| Yes| Actual process ID/name| This configuration item is used together with the `type` configuration item to specify the ID or name of the target process. Ensure that the configuration is correct and unique.|
| T                | Configuration item of `task` when `engine` is set `slide`. It specifies the threshold of the hot and cold memory.| Mandatory when `engine` is set to `slide`| Yes| 0 to `loop` x 3         | T=3 // The memory that is accessed fewer than three times is identified as cold memory.|
| max_threads      | Configuration item of `task` when `engine` is set `slide`. It specifies the maximum number of threads in the internal thread pool of etmemd. Each thread processes a memory scan+operation task of a process or subprocess.| No| Yes| 1 to 2 x Number of cores + 1. The default value is `1`.| This configuration item controls the number of internal processing threads of etmemd. When the target process has multiple subprocesses, the larger the value of this configuration item, the more the concurrent executions, but the more the occupied resources.|
| vm_flags         | Configuration item of `task` when `engine` is set `cslide`. It specifies the flag of the VMA to be scanned. If this configuration item is not configured, the scan is not distinguished.| Mandatory when `engine` is set to `cslide`| Yes| Currently, only `ht` is supported.| vm_flags=ht // Scan the VMA memory whose flag is `ht` (huge page).|
| anon_only        | Configuration item of `task` when `engine` is set `cslide`. It specifies whether to scan only anonymous pages.| No| Yes| yes/no               | anon_only=no // If this configuration item is set to `yes`, only anonymous pages are scanned. If this configuration item is set to `no`, non-anonymous pages are also scanned.|
| ign_host         | Configuration item of `task` when `engine` is set `cslide`. It specifies whether to ignore the page table scan information on the host.| No| Yes| yes/no               | ign_host=no // `yes`: Ignore. `no`: Do not ignore.|
| task_private_key | (Optional) Configuration item of `task` when `engine` is set `thirdparty`. This configuration item is reserved for the task of the third-party policy to parse private parameters.| No| No| Configured based on the private parameters of the third-party policy | Set this configuration item based on the private task parameters of the third-party policy.|



### Creating and Deleting Etmem Project/Engine/Task Objects

#### Scenario

1. The administrator creates an etmem project, engine, or task. (A project can contain multiple etmem engines, and an engine can contain multiple tasks.)

2. The administrator deletes an existing etmem project, engine, or task. (Before a project is deleted, all tasks in the project automatically stop.)


#### How to Use

Run etmem in binary mode. Specify the second parameter `obj` to create or delete a project, engine, or task. The project, engine, or task is identified and distinguished based on the content configured in the configuration file. The prerequisite is that the etmem configuration file is correctly configured and the etmemd process is started.

Add an object.

```
etmem obj add -f /etc/example_config.yaml -s etmemd_socket
```

Delete an object.

```
etmem obj del -f /etc/example_config.yaml -s etmemd_socket
```

Print help information.

```
etmem obj help
```

#### Help Information

Usage:

```
etmem obj add [options]

etmem obj del [options]

etmem obj help
```

Options:

```
-f|\-\-file <conf_file> Add configuration file

-s|\-\-socket <socket_name> Socket name to connect
```

Notes: The configuration file is required.

#### Command-line Options


| Option| Description| Mandatory | With Parameter or Not| Example Description|
| ------------ | ------------------------------------------------------------ | -------- | ---------- | -------------------------------------------------------- |
| -f or \-\-file   | Configuration file of an object| Mandatory for the `add` and `del` subcommands| Yes| Specify the path of the file.|
| -s or \-\-socket | Name of the socket for communicating with the etmemd server. The value must be the same as that specified when the etmemd process is started.| Mandatory for the `add` and `del` subcommands| Yes| This option is mandatory. When there are multiple etmemd processes, the administrator selects an etmemd process to communicate with.|

### Starting, Stopping, and Querying an Etmem Task

#### Scenario

After adding a project by running the `etmem obj add` command, you can start or stop the etmem project before running the `etmem obj del` command to delete the project.

1. The administrator starts the added project.

2. The administrator stops a project that has been started.


When the administrator runs the `obj del` command to delete a project, the project automatically stops if it has been started.

#### How to Use

For a project that has been successfully added, you can run the `etmem project` command to start or stop the project. Example commands are as follows:

Start a project.

```
etmem project start -n test -s etmemd_socket
```

Stop a project.

```
etmem project stop -n test -s etmemd_socket
```

Query a project.

```
etmem project show -n test -s etmemd_socket
```

Print help information.

```
etmem project help
```

#### Help Information

Usage:

```
etmem project start [options]

etmem project stop [options]

etmem project show [options]

etmem project help
```

Options:

```
-n|\-\-name <proj_name> Add project name

-s|\-\-socket <socket_name> Socket name to connect
```

Notes:

1. The project name and socket name are required when you execute the `add` or `del` option.

2. The socket name are required when you execute the `show` option.

#### Command-line Options

| Option| Description| Mandatory | With Parameter or Not| Example Description|
| ------------ | ------------------------------------------------------------ | -------- | ---------- | -------------------------------------------------------- |
| -n or \-\-name   | Project name| Mandatory for the `start`, `stop`, and `show` subcommands| Yes| The project name corresponds to the configuration file name.|
| -s or \-\-socket | Name of the socket for communicating with the etmemd server. The value must be the same as that specified when the etmemd process is started.| Mandatory for the `start`, `stop`, and `show` subcommands| Yes| This option is mandatory. When there are multiple etmemd processes, the administrator selects an etmemd process to communicate with.|

### Automatically Starting Etmem with System

#### Scenario

etmemd allows you to configure the systemd configuration file and start the systemd service in fork mode.

#### How to Use

Compile the service configuration file to start etmemd. Use the `-m` option to specify the mode. For example:

```
etmemd -l 0 -s etmemd_socket -m
```

#### Help Information

Options:

```
-l|\-\-log-level <log-level> Log level

-s|\-\-socket <sockect name> Name of the socket to be listened to

-m|\-\-mode-systemctl Mode used to start (systemctl)

-h|\-\-help Show this message
```

#### Command-line Options
| Option| Description | Mandatory | With Parameter or Not| Value Range| Example Description|
|----------------|------------|------|-------|------|-----------|
| -l or \-\-log-level | etmemd log level| No| Yes| 0 to 3| `0`: debug level. `1`: info level. `2`: warning level. `3`: error level. Only logs of the level that is higher than or equal to the configured level are recorded in the `/var/log/message` file.|
| -s or \-\-socket |Name of socket to be listened to by etmemd, which is used to interact with the client.|	Yes| Yes|	A string of fewer than 107 characters| Specify the name of socket to be listened to. |
|-m or \-\-mode-systemctl	| When etmemd is started as a service, this option must be specified in the command.|	No|	No|	N/A|	N/A|
| -h or \-\-help |	Help information|	No|No|N/A|If this option is specified, the command execution exits after the command output is printed.|




### Supporting Third-party Memory Extension Policies

#### Scenario

etmem allows you to register third-party memory extension policies and provides the dynamic library of the scanning module. When etmem is running, the third-party policy elimination algorithm is used to eliminate the memory.

You can use the dynamic library of the scanning module provided by etmem and implement the interfaces in the structure required for connecting to etmem.

#### How to Use

To use a third-party extended elimination policy, perform the following steps:

1. Invoke the scanning interface provided by the scanning module as required.

2. Implement each interface based on the function template provided in the etmem header file and encapsulate the interfaces into structures.

3. Compile the dynamic library of the third-party extended elimination policy.

4. Specify engines of the `thirdparty` type in the configuration file as required.

5. Enter the dynamic library name and interface structure name in the `task` field in the configuration file as required.

Other operations are similar to those of other etmem engines.

Interface structure templates:

```
struct engine_ops {

/* Parse the private parameters of the engine. If there are private parameters, implement this interface; otherwise, set it to NULL. */

int (*fill_eng_params)(GKeyFile *config, struct engine *eng);

/* Clear the private parameters of the engine. If there are private parameters, implement this interface; otherwise, set it to NULL. */

void (*clear_eng_params)(struct engine *eng);

/* Parse the private parameters of the task. If there are private parameters, implement this interface; otherwise, set it to NULL. */

int (*fill_task_params)(GKeyFile *config, struct task *task);

/* Parse the private parameters of the task. If there are private parameters, implement this interface; otherwise, set it to NULL. */

void (*clear_task_params)(struct task *tk);

/* Interface for starting a task */

int (*start_task)(struct engine *eng, struct task *tk);

/* Interface for stopping a task */

void (*stop_task)(struct engine *eng, struct task *tk);

/* Fill in the private parameters related to the PID. */

int (*alloc_pid_params)(struct engine *eng, struct task_pid **tk_pid);

/* Destroy the private parameters related to the PID. */

void (*free_pid_params)(struct engine *eng, struct task_pid **tk_pid);

/* Private commands required by third-party policies. If no private command is required, set it to NULL. */

int (*eng_mgt_func)(struct engine *eng, struct task *tk, char *cmd, int fd);

};
```

The following is an example of the configuration file. For details, see the configuration file description.

```
#thirdparty

[engine]

name=thirdparty

project=test

eng_name=my_engine

libname=/user/lib/etmem_fetch/code_test/my_engine.so

ops_name=my_engine_ops

engine_private_key=engine_private_value

[task]

project=test

engine=my_engine

name=background1

type=pid

value=1798245

task_private_key=task_private_value
```

 **Notes**:

You must use the dynamic library of the scanning module provided by etmem and implement the interfaces in the structure required for connecting to etmem.

The `fd` field in the `eng_mgt_func` interface cannot be set to `0xff` or `0xfe`.

Multiple third-party policy dynamic libraries can be added to a project. They are differentiated by `eng_name` in the configuration file.

### Memory Extension Using the AEP

#### Scenario

Use the etmem component package to enable the channel for tiered memory extension to the AEP.

The huge page of the VM is scanned on the node, and the cslide engine is used to eliminate the policy and migrate the cold memory to the AEP.

#### How to Use

Use the cslide engine to extend the memory. The following is an example. For details about the parameters, see the configuration file description.

```
#cslide

[engine]

name=cslide

project=test

node_pair=2,0;3,1

hot_threshold=1

node_mig_quota=1024

node_hot_reserve=1024

[task]

project=test

engine=cslide

name=background1

type=pid

value=1823197

vm_flags=ht

anon_only=no

ign_host=no
```

 **Note**: Do not scan the same process concurrently.

In addition, the cslide policy supports the following private commands:


- showtaskpages
- showhostpages

You can run the commands to view the page access information related to the task and the system huge page usage on the host of the VM.

The following are example commands:

```
etmem engine showtaskpages <-t task_name> -n proj_name -e cslide -s etmemd_socket

etmem engine showhostpages -n proj_name -e cslide -s etmemd_socket
```

 **Note**: `showtaskpages` and `showhostpages` support only the `cslide` engine.

#### Command-line Options
| Option| Description | Mandatory | With Parameter or Not| Example Description|
|----|------|------|-------|------|
|-n or \-\-proj_name|	Project name|	Yes|	Yes|	Specify the name of the existing project to be executed.|
|-s or \-\-socket|	Name of the socket for communicating with the etmemd server. The value must be the same as that specified when the etmemd process is started.|	Yes|	Yes|	This option is mandatory. When there are multiple etmemd processes, the administrator selects an etmemd process to communicate with.|
|-e or \-\-engine|	Name of the engine to be executed|	Yes|	Yes|	Specify the name of an existing engine to be executed.|
|-t or \-\-task_name|	Name of the task to be executed|	No|	Yes|	Specify the name of an existing task to be executed.|

## Contribution

1.  Fork this repository.
2.  Create the Feat_xxx branch.
3.  Commit code.
4.  Create a pull request (PR).
