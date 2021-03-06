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

    struct AssetBurners
    {
        uint64_t id;
        uint64_t pool_id;
        uint64_t pool_sub_id; // cannot be primary index.. can be multiple subpools of same id..
        uint64_t amount;
        name account;

        uint64_t primary_key() const { return id; }
        uint64_t by_pool_id() const { return pool_id; }
        uint64_t by_subpool_id() const { return pool_sub_id; }
    };
    EOSIO_REFLECT(AssetBurners, id, pool_id, pool_sub_id, amount, account);
}  // namespace endlessnftft