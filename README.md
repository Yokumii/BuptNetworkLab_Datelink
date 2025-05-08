# BuptNetworkLab_Datelink

这个仓库是北京邮电大学2024-2025学年《计算机网络》课程实验一：数据链路层滑动窗口协议的设计与实现 的作业仓库。成员包括 Yokumi、BIMU 和 Shinku。

## 项目结构

```
.
├── Examples/           # 教师示例实现
├── Protocols/          # 协议实现文件
│   ├── stopwait.c      # 停等协议实现
│   ├── selectiverepeat.c  # 选择重传协议实现
│   └── gobackn.c       # 回退N帧协议实现
├── Results/           # 测试结果目录
│   ├── selectiverepeat/  # 选择重传协议测试结果
│   │   └── TEST_5BITS_2500DATA_300ACK_30S/  # 测试结果示例
│   ├── stopwait/      # 停等协议测试结果
│   └── gobackn/       # 回退N帧协议测试结果
├── Dockerfile         # 开发环境配置
├── datalink.c         # 数据链路层协议实现
├── datalink.h         # 协议参数规定
├── lprintf.c          # 日志输出
├── lprintf.h          # 日志函数声明
├── protocol.c         # 协议通用功能实现
├── protocol.h         # 协议通用功能声明
├── crc32.c            # CRC32校验实现
├── crc32_cal.c        # CRC32查表计算实现
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

- `SEQ_BITS`: 序列号位数（1-6位，默认：5）
- `DATA_TIMER`: 数据帧超时时间（毫秒，默认：2500）
- `ACK_TIMER`: 确认帧超时时间（毫秒，默认：300）
- `TEST_TIME`: 测试持续时间（秒，默认：30）

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

1. 测试所有协议：
```bash
make test
```

2. 测试特定协议：
```bash
# 测试选择重传协议
make test_selectiverepeat

# 测试停等协议
make test_stopwait

# 测试回退N帧协议
make test_gobackn
```

3. 自定义参数测试：
```bash
# 自定义序列号位数、超时时间和测试时长
make test_selectiverepeat SEQ_BITS=4 DATA_TIMER=1500 ACK_TIMER=500 TEST_TIME=60
```

4. 查看测试结果：
```bash
ls Results/
```

5. 清理测试文件：
```bash
make clean
```

### 测试用例说明

测试用例包含5种不同的网络环境：

1. 理想环境（无丢包、无错误）：`--port=10001 --utopia`
2. 普通环境（无特殊参数）：`--port=10002`
3. 理想环境下的洪泛测试：`--port=10003 --flood --utopia`
4. 洪泛测试：`--port=10004 --flood`
5. 高误码率环境下的洪泛测试：`--port=10005 --flood --ber=1e-4`

### 测试结果目录命名规则

测试结果目录按照以下格式命名：
```
TEST_<SEQ_BITS>BITS_<DATA_TIMER>DATA_<ACK_TIMER>ACK_<TEST_TIME>S
```

例如：
- `TEST_5BITS_2500DATA_300ACK_30S`：使用5位序列号，数据帧超时2500ms，确认帧超时300ms，测试30秒
- `TEST_4BITS_1500DATA_500ACK_60S`：使用4位序列号，数据帧超时1500ms，确认帧超时500ms，测试60秒

## 其他材料

关于实验报告和提交的源代码，可以从 [Release](https://github.com/Yokumii/BuptNetworkLab_Datelink/releases/) 中获取。

## 特别说明

代码不保证绝对的正确性，请谨慎借鉴或使用，并自行遵循有关学术规范，如产生任何后果与作者无关。有问题可以发起 [ISSUE](https://github.com/Yokumii/BuptNetworkLab_Datelink/issues) ，但作者后续不一定继续维护该作业仓库。