//
// Created by frank on 18-5-15.
//

#ifndef RAFT_CONFIG_H
#define RAFT_CONFIG_H

#include <chrono>
#include <vector>

#include <tinyev/InetAddress.h>
#include <raft/Callback.h>

namespace raft
{

struct Config
{
    //
    // unique id of the raft Node, starting from 0
    // e.g. 0, 1, 2...
    //
    int id;

    //
    // leveldb persistence data path for each raftNode
    // e.g. /tmp/raft.0, /tmp/raft.1 ...
    //
    std::string storagePath;

    //
    // leader heartbeat timeout, the unit is config.timeUnit
    //
    int heartbeatTimeout = 1;

    //
    // assert(heartbeat < electionTimeout)
    // default = 5
    //
    int electionTimeout = 5;

    //
    // tick frequency of the Node
    //
    std::chrono::milliseconds timeUnit { 100 };

    //
    // RPC server address of this Node
    //
    ev::InetAddress serverAddress;

    //
    // all peer addresses of this Node
    // assert(peerAddresses[id] == serverAddress)
    //
    std::vector<ev::InetAddress> peerAddresses;

    //
    // user callback of a newly applied log
    //
    ApplyCallback applyCallback;

    //
    // user callback of a newly installed snapshot
    //
    SnapshotCallback snapshotCallback;
};

}

#endif //RAFT_CONFIG_H
