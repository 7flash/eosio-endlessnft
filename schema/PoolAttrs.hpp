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

    struct PoolAttrs
    {
        uint64_t id;
        uint64_t pool_id;

        name schema;
        name attr_name;
        string attr_value;

        uint8_t percent = 0;

        uint64_t primary_key() const { return id; }
        uint64_t by_pool_id() const { return pool_id; }
    };
    EOSIO_REFLECT(PoolAttrs, id, pool_id, schema, attr_name, attr_value, percent);
}  // namespace endlessnftft