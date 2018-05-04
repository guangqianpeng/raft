//
// Created by frank on 18-4-20.
//

#ifndef RAFT_CALLBACK_H
#define RAFT_CALLBACK_H

#include <functional>

struct RequestVoteArgs;
struct RequestVoteReply;

struct AppendEntriesArgs;
struct AppendEntriesReply;

struct ApplyMsg;

typedef std::function<void(const RequestVoteReply&)> RequestVoteCallback;
typedef std::function<void(const AppendEntriesReply&)> AppendEntriesCallback;
typedef std::function<void(const ApplyMsg&)> ApplyCallback;

#endif //RAFT_CALLBACK_H
