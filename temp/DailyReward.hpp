#pragma once

#include <eosio/eosio.hpp>
#include <eosio/name.hpp>
#include <eosio/asset.hpp>

#include <string>
#include <vector>

namespace endlessnftft
{
    using eosio::asset;
    using eosio::name;
    using std::string;
    using std::vector;
    using std::map;

    struct Reward
    {
        uint64_t id;
        // asset reward_token;
        // map<uint32_t, uint8_t> template_to_percent;

        uint64_t primary_key() const { return id; }
    };
    EOSIO_REFLECT(Reward, id);
}  // namespace endlessnftft