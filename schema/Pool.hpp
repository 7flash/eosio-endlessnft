#pragma once

#include <eosio/eosio.hpp>
#include <eosio/name.hpp>
#include <eosio/asset.hpp>

#include <string>
#include <vector>

namespace endlessnftft
{
    using eosio::asset;
    using eosio::block_timestamp;
    using eosio::asset;
    using eosio::name;
    using std::string;
    using std::vector;
    using std::map;

    constexpr name templates_pool = "pooltemplate"_n;
    constexpr name schemas_pool = "poolschema"_n;
    constexpr name attributes_pool = "poolattr"_n;

    struct Pool
    {
        uint64_t id;
        asset token_quantity;
        asset token_requirement;
        name token_contract;
        block_timestamp started_at;
        name pool_type;
        uint8_t period_days;

        uint64_t primary_key() const { return id; }
    };
    EOSIO_REFLECT(Pool, id, token_quantity, token_requirement, token_contract, started_at, pool_type, period_days);
}  // namespace endlessnftft