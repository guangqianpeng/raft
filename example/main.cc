

#include <chrono>
#include <tinyev/Logger.h>
#include <raft/Node.h>

using namespace std::chrono_literals;

void usage()
{
    printf("usage: ./raft id address1 address2...");
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    if (argc < 3)
        usage();

    setLogLevel(LOG_LEVEL_DEBUG);

    int id = std::stoi(argv[1]);

    std::vector<ev::InetAddress>
            peerAddresses;

    if (id + 2 >= argc) {
        usage();
    }

    for (int i = 2; i < argc; i++) {
        peerAddresses.emplace_back(std::stoi(argv[i]));
    }

    raft::Config config;
    config.id = id;
    config.storagePath = "./raft." + std::to_string(id);
    config.heartbeatTimeout = 1;
    config.electionTimeout = 5;
    config.timeUnit = 100ms;
    config.serverAddress = peerAddresses[id];
    config.peerAddresses = peerAddresses;
    config.applyCallback = [](const raft::ApplyMsg& msg) {
        assert(msg.command.getStringView() == "raft example");
    };
    config.snapshotCallback = [](const json::Value& snapshot) {
        FATAL("not implemented yet");
    };

    ev::EventLoopThread loopThread;
    ev::EventLoop* raftServerLoop = loopThread.startLoop();
    raft::Node raftNode(config, raftServerLoop);

    ev::EventLoop loop;
    loop.runEvery(1s, [&](){
        auto ret = raftNode.GetState();
        if (ret.isLeader) {
            raftNode.Propose(json::Value("raft example"));
        }
    });

    raftNode.Start();
    loop.loop();
}