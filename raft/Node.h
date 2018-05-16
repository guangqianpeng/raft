//
// Created by frank on 18-5-15.
//

#ifndef RAFT_NODE_H
#define RAFT_NODE_H

#include <atomic>
#include <memory>

#include <tinyev/noncopyable.h>
#include <tinyev/EventLoop.h>
#include <tinyev/EventLoopThread.h>

#include <raft/Callback.h>
#include <raft/Config.h>
#include <raft/Struct.h>
#include <raft/Raft.h>
#include <raft/RaftPeer.h>
#include <raft/RaftService.h>


namespace raft
{

class Node : ev::noncopyable
{
public:
    Node(const Config& config, ev::EventLoop* serverLoop);

    //
    // start the node instance, thread safe
    //
    void Start();

    //
    // wrapper of Raft::GetState(), thread safe
    //
    RaftState GetState();

    //
    // wrapper of Raft::Propose(), thread safe
    //
    ProposeResult Propose(const json::Value& command);

private:

    void StartInLoop();

    //
    // wrapper of Raft::RequestVote(), thread safe
    //
    void RequestVote(const RequestVoteArgs& args,
                     const RequestVoteDoneCallback& done);

    //
    // wrapper of Raft::OnRequestVoteReply(), thread safe
    //
    void OnRequestVoteReply(int peer,
                            const RequestVoteArgs& args,
                            const RequestVoteReply& reply);

    //
    // wrapper of Raft::AppendEntries(), thread safe
    //
    void AppendEntries(const AppendEntriesArgs& args,
                       const AppendEntriesDoneCallback& done);

    //
    // Wrapper of Raft::OnAppendEntriesReply(), thread safe
    //
    void OnAppendEntriesReply(int peer,
                              const AppendEntriesArgs& args,
                              const AppendEntriesReply& reply);

private:
    //
    // three kinds of eventloop schedulers
    //
    template<typename Task>
    void RunTaskInLoop(Task&& task);

    template<typename Task>
    void QueueTaskInLoop(Task&& task);

    template<typename Task>
    void RunTaskInLoopAndWait(Task&& task);

    void AssertInLoop() const
    { loop_->assertInLoopThread(); }

    void AssertStarted() const
    { assert(started_); }

    void AssertNotStarted() const
    { assert(!started_); }

private:
    typedef std::unique_ptr<Raft> RaftPtr;
    typedef std::unique_ptr<RaftPeer> RaftPeerPtr;
    typedef std::vector<RaftPeerPtr> RaftPeerList;

    std::atomic_bool started_ = false;
    RaftPtr raft_;
    RaftPeerList peers_;

    const int id_;
    const int peerNum_;

    std::chrono::milliseconds tickInterval_;

    jrpc::RpcServer rpcServer_;
    RaftService raftService_;

    ev::EventLoopThread loopThread_;
    ev::EventLoop* loop_;
};

}

#endif //RAFT_NODE_H
