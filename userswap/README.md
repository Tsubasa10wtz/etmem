# userswap

## 介绍

随着CPU算力的发展，尤其是ARM核成本的降低，内存成本和内存容量成为约束业务成本和性能的核心痛点，因此如何节省内存成本，如何扩大内存容量成为存储迫切要解决的问题。

userswap内存分级扩展，根据用户选择内存迁移策略对内存进行分级，分发到不同级别的介质上，降低dram的使用量，来达到内存容量扩展的目的。

## 编译方法

1.下载userswap源码

```text
git clone https://gitee.com/openeuler/etmem.git
```

2.编译运行依赖

userswap运行依赖于openEuler-1.0 LTS以及之后的版本。 并且需要配置启动参数cmdline

```text
enable_userswap
```

可以协同etmem以及memRouter进行用户态swap，也可以由用户定制程序实现用户态swap。

3.编译

```text
cd userswap
./configure
```

## 使用说明

userswap提供一个api头文件(uswap_api.h)以及uswap静态库(libuswap.a)。
应用程序编译方法:

```text
gcc example.c -o demo -luswap
```

### uswap api说明

| api | 功能描述 | 输入 | 输出 |
| ------------ | ----------- | ---------- | ------ |
| set_uswap_log_level | 设置错误日志打印等级 | 0-3 | 0：成功 |
| register_userfaultfd| 注册uswap地址范围用于换入换出 | 地址/长度 | 0：成功 |
| unregister_userfaultfd | 解注册uswap地址范围 | 地址/长度 | 0：成功 |
| register_uswap | 注册uswap换入换出回调函数 | 名称/长度/回调函数 | 0：成功|
| force_swapout | 强制换出操作，无需etmem/memRouter通知 | 地址/长度 |0：成功 |
| uswap_init | uswap换入换出线程初始化 | 换出线程数 | 0：成功 |

## 参与贡献

1. Fork本仓库
2. 新建个人分支
3. 提交代码
4. 新建Pull Request
