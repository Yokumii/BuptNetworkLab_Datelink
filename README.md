# BuptNetworkLab_Datelink

这个仓库是北京邮电大学2024-2025学年《计算机网络》课程实验一：数据链路层滑动窗口协议的设计与实现 的作业仓库。成员包括 Yokumi、BIMU 和 Shinku。

## 项目结构

```
.
├── Examples/           # 教师示例实现
├── Protocols/          # 协议实现文件
│   ├── stop_wait.c     # 停等协议实现
│   └── selective_repeat.c  # 选择重传协议实现
├── Dockerfile         # 开发环境配置
├── datalink.c         # 数据链路层协议实现
├── datalink.h         # 协议参数规定
├── lprintf.c          # 日志输出
├── lprintf.h          # 日志函数声明
├── protocol.c         # 协议通用功能实现
├── protocol.h         # 协议通用功能声明
├── crc32.c            # CRC32校验实现
├── Makefile           # 构建脚本
└── README.md          # 项目说明文档
```

## 开发环境

本实验提供了两个环境的源代码（Windows 和 Linux）。区别主要是 Windows 版本使用 Visual Studio 工程文件(.vcxproj, .sln)进行构建，并包含了 getopt.c 和 getopt.h 文件（Linux系统标准库文件）；Linux 版本直接使用了 gcc 进行编译。核心代码区别不大，但为便于小组成员间协作开发，我们通过容器化开发环境 Docker，见 [Dockerfile](Dockerfile) 进行协作开发。

### 开发环境配置

1. 安装 Docker Engine（Linux） 或 Docker Desktop（Windows && MacOS）；
2. 安装 VS Code 的 "Remote Development" 扩展；
3. 在 VS Code 项目中选择 Reopen in Container；
4. 容器将自动化构建，完成后 VS Code 会自动连接到容器。

## 协议参数说明

- `SEQ_BITS`: 序列号位数（1-6位）
- `DATA_TIMER`: 数据帧超时时间（毫秒）
- `ACK_TIMER`: 确认帧超时时间（毫秒）

## 测试说明

### 手动测试

1. 编译项目：
```bash
make
```

2. 启动站点A：
```bash
./datalink a
```

3. 启动站点B：
```bash
./datalink b
```

### 自动测试

> 说明：现在自动测试存在一些小问题，不知道是不是因为容器的原因

1. 使用默认参数测试：
```bash
make test
```

2. 自定义参数测试：
```bash
make PROTOCOL=selective_repeat SEQ_BITS=4 DATA_TIMER=1500 ACK_TIMER=500 test
```

3. 查看测试结果：
```bash
ls test_results/
```

4. 清理测试文件：
```bash
make clean_test
```

### 测试用例说明

测试用例包含5种不同的网络环境：

1. 理想环境（无丢包、无错误）
2. 普通环境（无特殊参数）
3. 理想环境下的洪泛测试
4. 洪泛测试
5. 高误码率环境下的洪泛测试

## 构建选项

```bash
make [PROTOCOL=protocol_name] [SEQ_BITS=bits] [DATA_TIMER=ms] [ACK_TIMER=ms] [TEST_TIME=seconds]
```

- `PROTOCOL`: 协议类型（stop_wait/selective_repeat）
- `SEQ_BITS`: 序列号位数（默认：6）
- `DATA_TIMER`: 数据帧超时时间（默认：2000ms）
- `ACK_TIMER`: 确认帧超时时间（默认：666ms）
- `TEST_TIME`: 测试持续时间（默认：30秒）

## 帮助信息

查看帮助信息：
```bash
make help
```
