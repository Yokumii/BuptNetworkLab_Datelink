# BuptNetworkLab_Datelink

这个仓库是北京邮电大学2024-2025学年《计算机网络》课程实验一：数据链路层滑动窗口协议的设计与实现 的作业仓库。成员包括 Yokumi、BIMU 和 Shinku；

## 项目结构

```
.
├── Examples/    # 教师示例实现
├── Docerfile    # 开发环境配置
├── datalink.c   # 数据链路层协议实现
├── datalink.h   # 协议参数规定
├── lprintf.c    # 日志输出
├── lprintf.h    # 日志函数声明
├── Makefile
└── README.md
```

## 开发说明

本实验提供了两个环境的源代码（Windows 和 Linux）。区别主要是 Windows 版本使用 Visual Studio 工程文件(.vcxproj, .sln)进行构建，并包含了 getopt.c 和 getopt.h 文件（Linux系统标准库文件）；Linux 版本直接使用了 gcc 进行编译。核心代码区别不大，但为便于小组成员间协作开发，我们通过容器化开发环境 Docker，见 [Dockerfile](Dockerfile) 进行协作开发；

Visual Studio Code 进行远程开发步骤如下：

1. 安装 Docker Engine（Linux） 或 Docker Desktop（Windows && MacOS）；
2. 安装 VS Code 的 "Remote Development" 扩展；
3. 在 VS Code 项目中选择 Reopen in Container；
4. 容器将自动化构建，完成后 VS Code 会自动连接到容器；

## 测试说明

1. 编辑完成后，如需测试，请先在项目目录下执行 `make` 命令，生成可执行文件  `datalink`；
2. 打开终端窗口，启动站点 A 的进程，输入命令：`./datalink  a`；
3. 打开另一终端窗口，启动站点 B 的进程，输入命令：`./datalink  b`；

为更方便的运行测试，后续需要对 [Makefile](Makefile) 文件进行进一步调整；
