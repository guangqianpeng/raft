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
    int id;
    std::string storagePath;
    int heartbeatTimeout;
    int electionTimeout;
    std::chrono::milliseconds timeUnit;
    ev::InetAddress serverAddress;
    std::vector<ev::InetAddress> peerAddresses;
    ApplyCallback applyCallback;
    SnapshotCallback snapshotCallback;
};

}

#endif //RAFT_CONFIG_H
