
#include <tinyev/Logger.h>

#include <raft/RaftService.h>

using namespace jrpc;

void usage()
{
    printf("usage: ./raft me address1 address2...");
    exit(EXIT_FAILURE);
}

void launchRaft(ev::EventLoop* loop, Raft* raft)
{
    loop->runEvery(1s, [=](){
        auto ret = raft->GetState();
        if (ret.isLeader) {
            raft->Propose(json::Value("raft example"));
        }
    });

    raft->SetApplyCallback([](const ApplyMsg& msg) {
        assert(msg.command.getStringView() == "raft example");
    });
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        usage();
    }

    ev::EventLoop loop;

    setLogLevel(LOG_LEVEL_DEBUG);

    int me = std::stoi(argv[1]);
    std::vector<ev::InetAddress>
            peerAddresses;

    if (me + 2 >= argc) {
        usage();
    }

    for (int i = 2; i < argc; i++) {
        peerAddresses.emplace_back(std::stoi(argv[i]));
    }


    Raft raft(me, "./raft." + std::to_string(me));
    jrpc::RpcServer rpcServer(&loop, peerAddresses[me]);
    RaftService service(rpcServer, raft);

    for (auto& peer: peerAddresses)
        service.AddRaftPeer(peer);

    launchRaft(&loop, &raft);

    rpcServer.start();
    service.StartRaft();
    loop.loop();
}