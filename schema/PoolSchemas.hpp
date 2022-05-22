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

    struct PoolSchemas
    {
        uint64_t id;
        uint64_t pool_id;
        name schema;
        uint8_t percent = 0;

        uint64_t primary_key() const { return id; }
        uint64_t by_pool_id() const { return pool_id; }
    };
    EOSIO_REFLECT(PoolSchemas, id, pool_id, schema, percent);
}  // namespace endlessnftft