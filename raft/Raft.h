//
// Created by frank on 18-4-19.
//

#ifndef RAFT_RAFT_H
#define RAFT_RAFT_H

#include <memory>
#include <vector>
#include <cassert>

#include <jackson/Value.h>

#include <raft/Random.h>
#include <raft/Callback.h>
#include <raft/Log.h>
#include <raft/Struct.h>
#include <raft/Storage.h>
#include <raft/Config.h>
#include <raft/RaftPeer.h>

namespace raft
{

class Raft : ev::noncopyable
{
public:
    Raft(const Config& config,
         const std::vector<RaftPeer*>& peers);

    //
    // return
    // struct RaftState
    // {
    //    int currentTerm; // current term
    //    bool isLeader;   // whether this node believes it is the leader
    // };
    //
    RaftState GetState() const;

    //
    // the service using Raft (e.g. a k/v serverAddress) wants to start
    // agreement on the next command to be appended to Raft's log. if this
    // serverAddress isn't the leader, returns false. Otherwise propose the
    // agreement and return immediately. there is no guarantee that this
    // command will ever be committed to the Raft log, since the leader
    // may fail or lose an election.
    //
    // Thread safe, return
    // struct ProposeResult
    // {
    //    int expectIndex;  // the index that the command will appear if it's ever committed.
    //    int currentTerm;  // current term
    //    bool isLeader;    // true if this node believes it is the leader
    // };
    //
    ProposeResult Propose(const json::Value& command);

    //
    // RequestVote RPC handler
    //
    void RequestVote(const RequestVoteArgs& args,
                     RequestVoteReply& reply);

    //
    // RequestVote reply callback
    //
    void OnRequestVoteReply(int peer,
                            const RequestVoteArgs& args,
                            const RequestVoteReply& reply);

    //
    // AppendEntries RPC handler
    //
    void AppendEntries(const AppendEntriesArgs& args,
                       AppendEntriesReply& reply);

    //
    // AppendEntries reply callback
    //
    void OnAppendEntriesReply(int peer,
                              const AppendEntriesArgs& args,
                              const AppendEntriesReply& reply);

    //
    // external timer input, the frequency is determined by config.timeUnit
    //
    void Tick();

    void DebugOutput() const;

private:
    enum Role
    {
        kLeader,
        kCandidate,
        kFollower,
    };

    void TickOnElection();

    void TickOnHeartbeat();

    void ToFollower(int targetTerm);

    void ToCandidate();

    void ToLeader();

    void OnNewInputTerm(int term);

    void ResetTimer();

    void StartRequestVote();

    void StartAppendEntries();

    void ApplyLog();

    bool IsStandalone() const
    { return peerNum_ == 1; }

    void SetCurrentTerm(int term);

    void SetVotedFor(int votedFor);

    const char* RoleString() const
    {
        return role_ == kLeader ? "leader" :
               role_ == kFollower ? "follower" :
               "candidate";
    }

private:
    constexpr static int kVotedForNull = -1;
    constexpr static int kInitialTerm = 0;
    constexpr static int kInitialCommitIndex = 0;
    constexpr static int kInitialLastApplied = 0;
    constexpr static int kInitialMatchIndex = 0;
    constexpr static int kMaxEntriesSendOneTime = 100;

    const int id_;
    const int peerNum_;

    Storage storage_;
    int currentTerm_ = kInitialTerm;     // persistent
    Role role_ = kFollower;
    int votedFor_ = kVotedForNull;       // persistent
    int votesGot_ = 0;

    int commitIndex_ = kInitialCommitIndex;
    int lastApplied_ = kInitialLastApplied;
    Log log_;                            // persistent

    int timeElapsed_ = 0;
    const int heartbeatTimeout_;
    const int electionTimeout_;
    int randomizedElectionTimeout_ = 0;
    Random randomGen_;

    std::vector<int> nextIndex_;
    std::vector<int> matchIndex_;

    typedef std::vector<RaftPeer*> RaftPeerList;
    const RaftPeerList peers_;

    ApplyCallback applyCallback_;
    SnapshotCallback snapshotCallback_;
};

}

#endif //RAFT_RAFT_H
