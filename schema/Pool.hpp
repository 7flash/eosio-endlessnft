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

    // struct CustomMap {
    //     vector<uint32_t> keys;
    //     vector<uint8_t> values;

    //     uint8_t operator[](const uint32_t key) const {
    //         auto it = std::find(keys.begin(), keys.end(), key);
    //         return values[std::distance(keys.begin(), it)];
    //     }
    // };
    // EOSIO_REFLECT(CustomMap, keys, values);

    constexpr name templates_pool = "pooltemplate"_n;
    constexpr name schemas_pool = "poolschema"_n;
    constexpr name attributes_pool = "poolattr"_n;

    struct Pool
    {
        uint64_t id;
        asset token_quantity;
        name token_contract;
        block_timestamp started_at;
        name pool_type;
        uint8_t period_days;

        // CustomMap templates;

        uint64_t primary_key() const { return id; }
    };
    EOSIO_REFLECT(Pool, id, token_quantity, token_contract, started_at, pool_type, period_days);
}  // namespace endlessnftft