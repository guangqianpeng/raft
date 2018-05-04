//
// Created by frank on 18-4-19.
//

#ifndef RAFT_RAFT_H
#define RAFT_RAFT_H

#include <memory>
#include <vector>
#include <cassert>

#include <tinyev/EventLoopThread.h>
#include <tinyev/EventLoop.h>
#include <tinyev/InetAddress.h>

#include <jackson/Value.h>

#include <raft/Random.h>
#include <raft/Callback.h>
#include <raft/LogSlice.h>

class RaftPeer;

class Raft: ev::noncopyable
{
public:
    Raft(int me,
         int heartbeatTimeout = 1,
         int electionTimeout = 5);

    ~Raft();

    //
    // used by raft peers to schedule tasks
    //
    ev::EventLoop* GetEventLoop() const { return loop_; }

    //
    // add a raft peer before start, thread safe
    //
    void AddRaftPeer(const ev::InetAddress& serverAddress);

    //
    // set callback of apply, thread safe
    //
    void SetApplyCallback(const ApplyCallback& cb);

    //
    // start the raft instance, thread safe
    //
    void Start();

    //
    // thread safe, return:
    //   1. current term
    //   2. whether this server believes it is the leader
    //
    struct GetStateResult {
        int currentTerm;
        bool isLeader;
    };
    GetStateResult GetState();

    //
    // the service using Raft (e.g. a k/v server) wants to start
    // agreement on the next command to be appended to Raft's log. if this
    // server isn't the leader, returns false. Otherwise start the
    // agreement and return immediately. there is no guarantee that this
    // command will ever be committed to the Raft log, since the leader
    // may fail or lose an election. Thread safe.
    //
    // return:
    //   1. the index that the command will appear if it's ever committed.
    //   2. current term.
    //   3. true if this server believes it is the leader.
    //
    struct ProposeResult {
        int expectIndex;
        int currentTerm;
        bool isLeader;
    };
    ProposeResult Propose(const json::Value& command);

    //
    // RequestVote RPC handler, thread safe
    //
    void RequestVote(const RequestVoteArgs& args,
                     const RequestVoteCallback& done);

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
                       const AppendEntriesCallback& done);

    //
    // AppendEntries RPC handler, thread safe
    // In current implementation, it is only called in Raft thread
    //
    void OnAppendEntriesReply(int peer,
                              const AppendEntriesArgs& args,
                              const AppendEntriesReply& reply);

private:
    enum Role
    {
        kLeader,
        kCandidate,
        kFollower,
    };

    const char* RoleString() const
    {
        return role_ == kLeader ?  "leader" :
               role_ == kFollower? "follower" :
                                   "candidate";
    }

    template <typename Task>
    void RunTaskInLoop(Task&& task);
    template <typename Task>
    void QueueTaskInLoop(Task&& task);
    template <typename Task>
    void RunTaskInLoopAndWait(Task&& task);

    void StartInLoop();

    void Tick();
    void TickOnElection();
    void TickOnHeartbeat();

    void ToFollower(bool termIncreased);
    void ToCandidate();
    void ToLeader();

    void OnNewInputTerm(int term);

    void ResetTimer();

    void StartRequestVote();
    void StartAppendEntries();

    void RequestVoteInLoop(const RequestVoteArgs& args,
                           RequestVoteReply& reply);
    void OnRequestVoteReplyInLoop(int peer,
                                  const RequestVoteArgs& args,
                                  const RequestVoteReply& reply);

    void AppendEntriesInLoop(const AppendEntriesArgs& args,
                             AppendEntriesReply& reply);
    void OnAppendEntriesReplyInLoop(int peer,
                                    const AppendEntriesArgs& args,
                                    const AppendEntriesReply& reply);

    void OnApplyDefault(const ApplyMsg& msg);

    void ApplyLog();

    void AssertInLoop() const
    { loop_->assertInLoopThread(); }

    void AssertStarted() const
    { assert(started_); }

    void AssertNotStarted() const
    { assert(!started_); }

    bool IsSingleNode() const
    { return peerNum_ == 1; }

private:
    constexpr static int kVotedForNull = -1;
    constexpr static int kInitialTerm = 0;
    constexpr static int kInitialCommitIndex = 0;
    constexpr static int kInitialLastApplied = 0;
    constexpr static int kInitialMatchIndex = 0;
    constexpr static int kTimeUnitInMilliseconds = 100;
    constexpr static int kMaxEntriesSendOneTime = 100;

    const int me_;
    int peerNum_ = 0;
    int currentTerm_ = kInitialTerm;     // todo: persistent
    Role role_ = kFollower;
    int votedFor_ = kVotedForNull;       // todo: persistent
    int votesGot_ = 0;

    int commitIndex_ = kInitialCommitIndex;
    int lastApplied_ = kInitialLastApplied;
    LogSlice log_;                       // todo: persistent

    int timeElapsed_ = 0;
    const int heartbeatTimeout_;
    const int electionTimeout_;
    int randomizedElectionTimeout_ = 0;
    Random randomGen_;

    std::vector<int> nextIndex_;
    std::vector<int> matchIndex_;

    bool started_ = false;

    typedef std::unique_ptr<RaftPeer> RaftPeerPtr;
    typedef std::vector<RaftPeerPtr> RaftPeerList;
    RaftPeerList peers_;

    ApplyCallback applyCallback_;

    ev::EventLoopThread loopThread_;
    ev::EventLoop* loop_;
};

struct RequestVoteArgs
{
    int term;
    int candidateId;
    int lastLogIndex;
    int lastLogTerm;
};

struct RequestVoteReply
{
    int term;
    bool voteGranted;
};

struct AppendEntriesArgs
{
    int term;
    int prevLogIndex;
    int prevLogTerm;
    json::Value entries;
    int leaderCommit;
};

struct AppendEntriesReply
{
    int term;
    bool success;
};

struct ApplyMsg
{
    ApplyMsg(int index_, const json::Value& command_)
            : index(index_)
            , command(command_)
    {}

    int index;
    json::Value command;
};

#endif //RAFT_RAFT_H
