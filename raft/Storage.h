//
// Created by frank on 18-5-10.
//

#ifndef RAFT_STORAGE_H
#define RAFT_STORAGE_H

#include <string>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <jackson/Value.h>
#include <tinyev/noncopyable.h>

namespace raft
{

class Storage : ev::noncopyable
{
public:
    explicit
    Storage(const std::string& path);

    ~Storage();

    void PutCurrentTerm(int currentTerm);

    void PutVotedFor(int votedFor);

    void PutFirstIndex(int firstIndex);

    void PutLastIndex(int lastIndex);

    void PrepareEntry(int index, const json::Value& entry);

    void PutPreparedEntries();

    int GetCurrentTerm() const
    { return currentTerm_; }

    int GetVotedFor() const
    { return votedFor_; }

    int GetFirstIndex() const
    { return firstIndex_; }

    int GetLastIndex() const
    { return lastIndex_; }

    std::vector<json::Value> GetEntries() const;

private:
    void InitEmptyDB();

    void InitNoneEmptyDB();

    void Put(const leveldb::Slice& key, int value);

    int Get(const leveldb::Slice& key);

private:
    int currentTerm_;
    int votedFor_;
    int firstIndex_;
    int lastIndex_;
    leveldb::DB* db_;
    leveldb::WriteBatch batch_;
    bool preparing = false;
};

}

#endif //RAFT_STORAGE_H
