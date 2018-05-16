//
// Created by frank on 18-4-19.
//

#ifndef RAFT_RAFTPEER_H
#define RAFT_RAFTPEER_H

#include <raft/RaftClientStub.h>
#include <raft/Callback.h>
#include <raft/Struct.h>

namespace raft
{

class RaftPeer: ev::noncopyable
{
public:
    RaftPeer(int peer, ev::EventLoop* loop, const ev::InetAddress& serverAddress);

    ~RaftPeer();

    void Start();

    void RequestVote(const RequestVoteArgs& args);

    void AppendEntries(const AppendEntriesArgs& args);

    void SetRequestVoteReplyCallback(const RequestVoteReplyCallback& cb)
    { requestVoteReply_ = cb; }

    void SetAppendEntriesReplyCallback(const AppendEntriesReplyCallback& cb)
    { appendEntriesReply_ = cb; }

private:
    void AssertInLoop()
    { loop_->assertInLoopThread(); }

    void SetConnectionCallback();

    void OnConnection(bool connected);

private:
    const int peer_;
    ev::EventLoop* loop_;
    ev::InetAddress serverAddress_;
    bool connected_ = false;

    RequestVoteReplyCallback requestVoteReply_;
    AppendEntriesReplyCallback appendEntriesReply_;

    typedef std::unique_ptr<jrpc::RaftClientStub> ClientPtr;
    ClientPtr rpcClient;
};

}

#endif //RAFT_RAFTPEER_H
