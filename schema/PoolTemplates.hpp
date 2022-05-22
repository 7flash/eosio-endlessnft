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

    struct PoolTemplates
    {
        uint64_t id;
        uint64_t pool_id;
        int32_t template_id;
        uint8_t percent;

        uint64_t primary_key() const { return id; }

        uint64_t by_pool_id() const { return pool_id; }
        uint64_t by_template_id() const { return (uint64_t)template_id; }
    };
    EOSIO_REFLECT(PoolTemplates, id, pool_id, template_id, percent);
}  // namespace endlessnftft