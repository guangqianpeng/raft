//
// Created by frank on 18-4-20.
//

#ifndef RAFT_CALLBACK_H
#define RAFT_CALLBACK_H

#include <functional>
#include <jackson/Value.h>

namespace raft
{

struct RequestVoteArgs;
struct RequestVoteReply;

struct AppendEntriesArgs;
struct AppendEntriesReply;

struct ApplyMsg;

typedef std::function<void(const RequestVoteReply&)> RequestVoteDoneCallback;
typedef std::function<void(const AppendEntriesReply&)> AppendEntriesDoneCallback;
typedef std::function<void(const RequestVoteArgs&,
                           const RequestVoteDoneCallback&)> DoRequestVoteCallback;
typedef std::function<void(const AppendEntriesArgs& args,
                           const AppendEntriesDoneCallback& done)> DoAppendEntriesCallback;
typedef std::function<void(int,
                           const RequestVoteArgs&,
                           const RequestVoteReply&)> RequestVoteReplyCallback;
typedef std::function<void(int,
                           const AppendEntriesArgs&,
                           const AppendEntriesReply&)> AppendEntriesReplyCallback;
typedef std::function<void(const ApplyMsg&)> ApplyCallback;
typedef std::function<void(const json::Value&)> SnapshotCallback;

}

#endif //RAFT_CALLBACK_H
