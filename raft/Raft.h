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

    RaftState GetState() const;

    ProposeResult Propose(const json::Value& command);

    void RequestVote(const RequestVoteArgs& args,
                     RequestVoteReply& reply);

    void OnRequestVoteReply(int peer,
                            const RequestVoteArgs& args,
                            const RequestVoteReply& reply);

    void AppendEntries(const AppendEntriesArgs& args,
                       AppendEntriesReply& reply);

    void OnAppendEntriesReply(int peer,
                              const AppendEntriesArgs& args,
                              const AppendEntriesReply& reply);

    //
    // external timer input
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
