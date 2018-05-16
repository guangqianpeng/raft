//
// Created by frank on 18-5-15.
//

#ifndef RAFT_NODE_H
#define RAFT_NODE_H

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
    explicit
    Node(const Config& config, ev::EventLoop* serverLoop);

    //
    // start the raft instance, thread safe
    //
    void Start();

    //
    // thread safe, return:
    //   1. current term
    //   2. whether this serverAddress believes it is the leader
    //
    RaftState GetState();

    //
    // the service using Raft (e.g. a k/v serverAddress) wants to start
    // agreement on the next command to be appended to Raft's log. if this
    // serverAddress isn't the leader, returns false. Otherwise start the
    // agreement and return immediately. there is no guarantee that this
    // command will ever be committed to the Raft log, since the leader
    // may fail or lose an election. Thread safe.
    //
    // return:
    //   1. the index that the command will appear if it's ever committed.
    //   2. current term.
    //   3. true if this serverAddress believes it is the leader.
    //
    ProposeResult Propose(const json::Value& command);

private:

    void StartInLoop();

    //
    // RequestVote RPC handler, thread safe
    //
    void RequestVote(const RequestVoteArgs& args,
                     const RequestVoteDoneCallback& done);

    //
    // RequestVote done callback, thread safe.
    // In current implementation, it is only called in Raft thread
    //
    void OnRequestVoteReply(int peer,
                            const RequestVoteArgs& args,
                            const RequestVoteReply& reply);

    //
    // AppendEntries RPC handler, thread safe
    //
    void AppendEntries(const AppendEntriesArgs& args,
                       const AppendEntriesDoneCallback& done);

    //
    // AppendEntries RPC handler, thread safe
    // In current implementation, it is only called in Raft thread
    //
    void OnAppendEntriesReply(int peer,
                              const AppendEntriesArgs& args,
                              const AppendEntriesReply& reply);

private:
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
    bool started_ = false;

    typedef std::unique_ptr<Raft> RaftPtr;
    RaftPtr raft_;

    typedef std::unique_ptr<RaftPeer> RaftPeerPtr;
    typedef std::vector<RaftPeerPtr> RaftPeerList;
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
