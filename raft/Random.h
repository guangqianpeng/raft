//
// Created by frank on 18-4-20.
//

#ifndef RAFT_RANDOM_H
#define RAFT_RANDOM_H

#include <random>

namespace raft
{

class Random
{
public:
    Random(int seed, int left, int right)
            : engine_(seed), dist_(left, right)
    {}

    int Generate()
    {
        return dist_(engine_);
    }

private:
    std::default_random_engine engine_;
    std::uniform_int_distribution<> dist_;
};

}

#endif //RAFT_RANDOM_H
