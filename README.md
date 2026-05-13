# TinyFastDFS

TinyFastDFS 是一个基于 C++11 实现的轻量级分布式文件存储系统，参考 FastDFS 的 `tracker-storage-client` 架构进行设计。

项目目标是从零实现一个具备核心分布式文件系统能力的工程项目，用于学习网络编程、自定义协议、节点管理、文件索引、负载均衡和副本同步等机制。

## Features

- Tracker / Storage / Client 三角色架构
- 自定义 `Header + Body` 协议
- Storage 注册与心跳机制
- Tracker 节点存活检测与 offline 标记
- 多 Storage 节点管理
- Round Robin 上传负载均衡
- 文件上传、下载、删除
- 文件元数据查询
- FileIndex 文件位置索引与持久化
- Binlog 操作日志
- 手动副本同步

## Architecture

```text
Client
  |
  | query storage / report file status
  v
Tracker Server
  |
  | storage registry / file index / scheduling
  v
Storage Server 1 / Storage Server 2 / ...
  |
  | file operations / metadata / binlog / sync
  v
Local File System
```

## Project Layout

```text
fastdfs/
├── conf/          # configuration files
├── data/          # runtime data
├── docs/          # design notes
├── scripts/       # helper scripts
├── src/           # source code
├── CMakeLists.txt
└── README.md
```

## Environment

| Platform | Requirement |
|---|---|
| Windows | Windows 10/11, PowerShell, VSCode, CMake |
| Linux | g++ >= 7, CMake >= 3.10 |
| macOS | clang++ / g++, CMake >= 3.10 |

The project uses C++11.

## Build

Windows PowerShell:

```powershell
cmake -S . -B build
cmake --build build -j
```

Linux / macOS:

```bash
cmake -S . -B build
cmake --build build -j
```

## Quick Start

Start tracker:

```powershell
.\build\bin\tracker_server.exe conf\tracker.conf
```

Start storage1:

```powershell
.\build\bin\storage_server.exe conf\storage1.conf
```

Start storage2:

```powershell
.\build\bin\storage_server.exe conf\storage2.conf
```

Test client:

```powershell
.\build\bin\fdfs_cli.exe ping
```

## Client Commands

Upload:

```powershell
.\build\bin\fdfs_cli.exe upload test.txt
```

Download:

```powershell
.\build\bin\fdfs_cli.exe download <file_id> out.txt
```

Delete:

```powershell
.\build\bin\fdfs_cli.exe delete <file_id>
```

Query metadata:

```powershell
.\build\bin\fdfs_cli.exe stat <file_id>
```

Manual sync:

```powershell
.\build\bin\fdfs_cli.exe sync 127.0.0.1:23000 127.0.0.1:23001
```

## Configuration

Main config files:

```text
conf/tracker.conf
conf/storage1.conf
conf/storage2.conf
conf/client.conf
```

See `conf/README.md` for details.

## Runtime Data

Runtime files are stored under `data/`.

Typical files include:

```text
data/tracker/file_index.dat
data/storage1/binlog.dat
data/storage2/binlog.dat
data/storage*/files/
```

Runtime data is generated during testing and should normally not be committed.

## Current Limitations

- The current network model is blocking TCP.
- Protocol body is mainly `key=value` text.
- FileIndex records one primary storage for each file.
- Replica sync is manually triggered.
- Sync reads full binlog instead of incremental offset.
- HTTP access is not implemented yet.

## Roadmap

- epoll + Reactor network model
- Thread pool
- Strict binary protocol body
- Automatic replica synchronization
- Binlog offset management
- Multiple replicas in FileIndex
- HTTP gateway
- File checksum
- Small file merge storage

## License

This project is for learning and educational purposes.