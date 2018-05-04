//
// Created by frank on 18-4-19.
//

#ifndef RAFT_RAFTPEER_H
#define RAFT_RAFTPEER_H

#include <raft/RaftClientStub.h>

class Raft;
struct RequestVoteArgs;
struct AppendEntriesArgs;

class RaftPeer: ev::noncopyable
{
public:
    RaftPeer(Raft* raft, int peer, const ev::InetAddress& serverAddress);
    ~RaftPeer();

    void Start();
    void RequestVote(const RequestVoteArgs& args);
    void AppendEntries(const AppendEntriesArgs& args);

private:
    void AssertInLoop()
    { loop_->assertInLoopThread(); }

    void SetConnectionCallback();
    void OnConnection(bool connected);

private:
    Raft* raft_;
    const int peer_;
    ev::EventLoop* loop_;
    bool connected_;
    ev::InetAddress serverAddress_;

    typedef std::unique_ptr<jrpc::RaftClientStub> ClientPtr;
    ClientPtr rpcClient;
};


#endif //RAFT_RAFTPEER_H
