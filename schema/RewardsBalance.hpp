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

    struct RewardsBalance
    {
        uint64_t id;
        name account;
        uint64_t pool_id;
        asset quantity;

        uint64_t primary_key() const { return id; }
        uint64_t by_account() const { return account.value; }
    };
    EOSIO_REFLECT(RewardsBalance, id, account, pool_id, quantity);
}  // namespace endlessnftft