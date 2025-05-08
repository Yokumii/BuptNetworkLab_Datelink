FROM ubuntu:22.04

# 设置时区
ENV TZ=Asia/Shanghai
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# 更新系统并安装必要的软件包
RUN apt-get clean && \
    rm -rf /var/lib/apt/lists/* && \
    apt-get update --fix-missing && \
    apt-get upgrade -y

RUN apt-get install -y \
    build-essential \
    gcc \
    g++ \
    gdb \
    cmake \
    make \
    git \
    vim \
    screen \
    procps \
    net-tools \
    iputils-ping \
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /project

# 配置screen
RUN echo "defscrollback 10000" >> /etc/screenrc && \
    echo "startup_message off" >> /etc/screenrc && \
    echo "hardstatus alwayslastline" >> /etc/screenrc && \
    echo "hardstatus string '%{= kG}[ %{G}%H %{g}][%= %{= kw}%?%-Lw%?%{r}(%{W}%n*%f%t%?(%u)%?%{r})%{w}%?%+Lw%?%?%= %{g}][%{B} %m-%d %{W}%c %{g}]'" >> /etc/screenrc

# 设置默认命令
CMD ["/bin/bash"]