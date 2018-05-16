//
// Created by frank on 18-5-10.
//

#include <tinyev/Logger.h>
#include <jackson/StringWriteStream.h>
#include <jackson/Writer.h>
#include <jackson/Document.h>

#include <raft/Storage.h>

using namespace raft;

namespace
{

const int kInitialTerm = 0;
const int kVotedForNull = -1;
const int kInitialIndex = 0;

// leading space makes the keys < index
const char* kCurrentTermKey = " currentTerm";
const char* kVotedForKey    = " votedFor";
const char* kFirstIndexKey  = " firstIndex";
const char* kLastIndexKey   = " lastIndex";

json::Value ParseSlice(const leveldb::Slice& slice)
{
    json::Document doc;
    std::string_view view(slice.data(), slice.size());
    json::ParseError ret = doc.parse(view);
    assert(ret == json::PARSE_OK); (void)ret;
    return doc;
}

}

Storage::Storage(const std::string& path)
{
    leveldb::Options options;
    leveldb::Status status = leveldb::DB::Open(options, path, &db_);
    if (status.ok()) {
        InitNoneEmptyDB();
    }
    else  {
        INFO("creating new database...");
        options.create_if_missing = true;
        status = leveldb::DB::Open(options, path, &db_);
        if (!status.ok())
            FATAL("leveldb create error: %s", status.ToString().c_str());
        InitEmptyDB();
    }
}

Storage::~Storage()
{ delete db_; }

std::vector<json::Value>
        Storage::GetEntries() const
{
    char first[11], last[11];
    snprintf(first, sizeof first, "%010d", firstIndex_);
    snprintf(last, sizeof last, "%010d", lastIndex_);

    auto it = db_->NewIterator(leveldb::ReadOptions());
    it->Seek(first);

    std::vector<json::Value> vec;
    for (; it->Valid(); it->Next()) {
        auto key = it->key().ToString();
        if (key > last)
            break;
        vec.push_back(ParseSlice(it->value()));
    }
    delete it;
    assert(!vec.empty());
    return vec;
}

void Storage::PutCurrentTerm(int currentTerm)
{
    if (currentTerm_ != currentTerm) {
        currentTerm_ = currentTerm;
        Put(kCurrentTermKey, currentTerm);
    }
}

void Storage::PutVotedFor(int votedFor)
{
    if (votedFor_ != votedFor) {
        votedFor_ = votedFor;
        Put(kVotedForKey, votedFor);
    }
}

void Storage::PutFirstIndex(int firstIndex)
{
    if (firstIndex_ != firstIndex) {
        firstIndex_ = firstIndex;
        Put(kFirstIndexKey, firstIndex);
    }
}

void Storage::PutLastIndex(int lastIndex)
{
    if (lastIndex_ != lastIndex) {
        lastIndex_ = lastIndex;
        Put(kLastIndexKey, lastIndex);
    }
}

void Storage::PrepareEntry(int index, const json::Value& entry)
{
    //
    // add leading zero, so we can iterate keys in order
    // fixme: snprintf() may be very slow!!!
    //
    char key[11];
    snprintf(key, sizeof key, "%010d", index);

    json::StringWriteStream os;
    json::Writer writer(os);
    entry.writeTo(writer);
    auto value = os.get();

    batch_.Put(key, leveldb::Slice(value.data(), value.size()));
    preparing = true;
}

void Storage::PutPreparedEntries()
{
    assert(preparing);
    db_->Write(leveldb::WriteOptions(), &batch_);
    batch_.Clear();
    preparing = false;
}

void Storage::InitEmptyDB()
{
    currentTerm_ = kInitialTerm;
    votedFor_ = kVotedForNull;
    firstIndex_ = kInitialIndex;
    lastIndex_ = kInitialIndex;

    Put(kCurrentTermKey, currentTerm_);
    Put(kVotedForKey, votedFor_);
    Put(kFirstIndexKey, firstIndex_);
    Put(kLastIndexKey, lastIndex_);

    json::Value entry(json::TYPE_OBJECT);
    entry.addMember("term", kInitialTerm);
    entry.addMember("command", "leveldb initialized");
    PrepareEntry(kInitialIndex, entry);
    PutPreparedEntries();
}

void Storage::InitNoneEmptyDB()
{
    currentTerm_ = Get(kCurrentTermKey);
    votedFor_ = Get(kVotedForKey);
    firstIndex_ = Get(kFirstIndexKey);
    lastIndex_ = Get(kLastIndexKey);
}

void Storage::Put(const leveldb::Slice& key, int value)
{
    auto status = db_->Put(leveldb::WriteOptions(), key,
            std::to_string(value));
    if (!status.ok()) {
        FATAL("levedb::Put failed: %s", status.ToString().c_str());
    }
}

int Storage::Get(const leveldb::Slice& key)
{
    std::string value;
    auto status = db_->Get(leveldb::ReadOptions(), key, &value);
    if (!status.ok()) {
        FATAL("leveldb::Get failed: %s", status.ToString().c_str());
    }
    return std::stoi(value);
}

