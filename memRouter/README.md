# memRouter

## 介绍

随着CPU算力的发展，尤其是ARM核成本的降低，内存成本和内存容量成为约束业务成本和性能的核心痛点，因此如何节省内存成本，如何扩大内存容量成为存储迫切要解决的问题。

memRouter内存分级扩展，根据用户选择内存迁移策略对内存进行分级，分发到不同级别的介质上，降低dram的使用量，来达到内存容量扩展的目的。

## 编译方法

1. 下载memRouter源码
```
git clone https://gitee.com/openeuler/etmem.git
```
2. 编译和运行依赖

    memRouter的编译和运行依赖于libcap-devel、json-c-devel、numactl-devel软件包

3. 编译
```
cd memRouter
mkdir build
cd build
cmake ..
make
```

## 使用说明

### 启动memdcd进程

#### 使用方法

通过运行memdcd二进制运行服务端进程，例如：
```
memdcd -p xx.json
```
策略配置文件的权限需为600，属主需为当前memRouter启动者
#### 命令行参数说明

| 参数            | 参数含义                           | 是否必须 | 是否有参数 | 参数范围              | 示例说明                                                     |
| --------------- | ---------------------------------- | -------- | ---------- | --------------------- | ------------------------------------------------------------ |
| -p或--policy | memRouter日志级别                     | 是       | 是         | 有效地址                   | memdcd所采用的页面分级策略
| -s或--socket    | memRouter监听的名称，用于与客户端交互 | 否       | 是         | 107个字符之内的字符串 | 指定服务端监听的unix socket名称                                         |
| -l或--log      | 帮助信息                           | 否       | 是         | LOG_INFO/LOG_DEBUG             | LOG_INFO: info级别 LOG_DEBUG: debug级别出                                 |
| -t或--timeout      | 帮助信息                           | 否       | 是         | 0-4294967295                    | 收集进程页面信息的时长限制                                 |
| -h或--help      | 帮助信息                           | 否       | 否         | NA                    | 执行时带有此参数会打印后退出                                 |

### 配置文件
目前仅支持阈值策略

#### 阈值策略配置文件
```
{
    "type": "mig_policy_threshold",
    "policy": {
       "unit": "KB",
       "threshold": 20
    }
}
```

| **配置项**    | **配置项含义**                                               | **是否必须** | **是否有参数** | **参数范围**              | **示例说明**                                                 |
| ----------- | ------------------------------------------------------------ | ------------ | -------------- | ------------------------- | ------------------------------------------------------------ |
| type     | 采取的分级策略                                    | 是           | 是             | 暂时支支持mig_policy_threshold                        | "type": "mig_policy_threshold"         |
| policy        | 采取的策略                                           | 是           | 是             | NA                     | NA                                             |
| unit    | 采用的单位                                       | 是           | 是             | KB/MB/GB                    | "unit": "KB"以KB作为单位                              |
| threshold       | 被监控进程的内存阈值                        | 是           | 是             | 0~INT32_MAX                    | "threshold": 20 //限制配监控进程内存上限20KB                             |


#### 所需etmemd配置文件:
各字段含义参见[etmemd](https://gitee.com/openeuler/etmem/blob/master/README.md)
```
options:    
    loop : ${loop}
    interval : ${interval}
    sleep: ${sleep}
	policies:
		type : pid/name
   		value : ${pid}/${name}
    	max_threads: ${max_threads}
    	engine : memdcd
```

## 参与贡献

1.  Fork本仓库
2.  新建个人分支
3.  提交代码
4.  新建Pull Request
