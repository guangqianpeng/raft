[![Build Status](https://travis-ci.org/guangqianpeng/raft.svg?branch=master)](https://travis-ci.org/guangqianpeng/raft)

# 简介

该版本的raft实现源自[MIT 6.824](http://nil.csail.mit.edu/6.824/2017/)，做完lab以后我尝试用C++实现一遍。相比于go语言的版本，这个版本的特点有：

- RPC框架使用了我自己写的[jrpc](https://github.com/guangqianpeng/jrpc)，请求是异步的，而lab中是同步的
- 使用多线程 + EventLoop的并发模型，而不是goroutine
- 使用EventLoop, CountdownLatch之类的线程同步设施，拒绝直接使用mutex
- 使用JSON格式的序列化/反序列化，使用LevelDB持久化JSON文本（应该用protobuf ？）

# 功能

- Leader election
- Log replication
- Persistence

# TODO 

- Log compaction
- Test
- Benchmark

# 实现

一个raft节点有两个线程（即两个EventLoop），一个跑rpc server，另一个跑raft算法以及rpc client。若将这两部分放在一个线程里面固然可以简化代码（单线程编程），但是由于rpc框架调度的延迟不确定，可能导致心跳发送不及时。也许应该把rpc client单独放在一个线程，在jrpc的支持下这点不难做到。

核心部分是raft的纯算法实现（[Raft.h](raft/Raft.h)/[Raft.cc](raft/Raft.cc)），它的rpc请求、回复以及时钟都需要外部输入，这些输入具体包括：

- rpc server收到的请求（`Raft::RequestVote()`，`Raft::AppendEntries()`）
- rpc client收到的回复 （`Raft::OnRequestVoteReply()`， `Raft::OnAppendEntriesReply()`）
- 固定频率的时钟激励（`Raft::Tick()`）
- raft用户尝试提交log（`Raft::Propose()`）

我并没有将rpc请求和回复关联起来，而是当成独立的消息输入来处理，这样方便处理 expired/duplicate/out-of-order rpc 消息。用户并不直接使用Raft类，而是使用Node类（[Node.h](raft/Node.h)/[Node.cc](raft/Node.cc)）。Node类封装了rpc通信、时钟、多线程等内容。

# 玩一下

## 安装

**首先，你需要gcc 7.x或者更高的版本，可以手动安装或者直接上Ubuntu 18.04。。。**

```sh
sudo apt install libleveldb-dev
git clone https://github.com/guangqianpeng/raft.git
cd raft
git submodule update --init --recursive
./build.sh
./build.sh install
cd ../raft-build/Release-install/bin
```

## 单节点

开一个单节点的raft，server端口号是`9877`，虽然这个server没什么用：

```sh
./raft_demo 0 9877 | grep 'raft\['
```

你可以看到运行的流程，每隔1s，leader就会propose一条log，最后一行是每隔5s输出一次的统计信息：

```
20180504 07:06:39.926993 raft[0] follower, peerNum = 1 starting... 
20180504 07:06:40.427085 raft[0] follower -> candidate
20180504 07:06:40.427127 raft[0] candidate -> leader
20180504 07:06:40.927118 raft[0] leader, term 1, start log 1
20180504 07:06:40.927164 raft[0] leader, term 1, apply log [1]
20180504 07:06:41.927097 raft[0] leader, term 1, start log 2
20180504 07:06:41.927145 raft[0] leader, term 1, apply log [2]
20180504 07:06:42.927097 raft[0] leader, term 1, start log 3
20180504 07:06:42.927144 raft[0] leader, term 1, apply log [3]
20180504 07:06:43.927027 raft[0] leader, term 1, start log 4
20180504 07:06:43.927082 raft[0] leader, term 1, apply log [4]
20180504 07:06:44.927069 raft[0] leader, term 1, #votes 1, commit 4
```

## 多节点

开3个节点的raft集群，需要起3个进程，server端口号分别是`9877,9878,9879`分别运行：

```shell
./raft_demo 0 9877 9878 9879 | grep 'raft\['
./raft_demo 1 9877 9878 9879 | grep 'raft\['
./raft_demo 2 9877 9878 9879 | grep 'raft\['
```

你可以看到leader election的过程，然后重启一个进程，看看会发生什么？Have fun :-)