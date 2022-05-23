#pragma once
#include <eosio/asset.hpp>

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/name.hpp>
#include <eosio/singleton.hpp>
#include <string>
#include <vector>

#include "Pool.hpp"
#include "PoolTemplates.hpp"
#include "PoolSchemas.hpp"
#include "PoolAttrs.hpp"
#include "AssetBurners.hpp"
#include "RewardsBalance.hpp"

#include "./atomicassets/atomicassets-interface.hpp"
#include "./atomicassets/atomicdata.hpp"

namespace endlessnftft
{
    // using endlessnftft::Pool;
    using eosio::check;
    using eosio::contract;
    using eosio::datastream;
    using eosio::name;
    using eosio::print;
    using eosio::asset;
    using std::string;
    using std::map;

    // Ricardian contracts live in ricardian/class1-ricardian.cpp
    extern const char* sayhi_ricardian;
    extern const char* sayhialice_ricardian;
    extern const char* ricardian_clause;

    // The account this contract is normally deployed to
    inline constexpr auto default_contract_account = "endlessnftft"_n;
    inline constexpr auto pools_table_name = "poolsx"_n;
    inline constexpr auto pool_templates_table_name = "pooltemplsa"_n;
    inline constexpr auto pool_schemas_table_name = "poolschemas"_n;
    inline constexpr auto pool_attrs_table_name = "poolattrs"_n;
    inline constexpr auto asset_burners_table_name = "assetburn"_n;
    inline constexpr auto rewards_balance_table_name = "rewardsbal"_n;

    struct account
    {
        eosio::asset balance;

        bool operator==(const account&) const = default;
        bool operator!=(const account&) const = default;
        uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };
    EOSIO_REFLECT(account, balance)

    typedef eosio::multi_index<"accounts"_n, account> accounts;

    static eosio::asset get_balance(eosio::name token_contract_account,
                                    eosio::name owner,
                                    eosio::symbol_code sym_code)
    {
        accounts accountstable(token_contract_account, owner.value);
        const auto& ac = accountstable.get(sym_code.raw(), "Contract Balance is empty");
        return ac.balance;
    }

    typedef eosio::multi_index<pools_table_name, Pool> PoolTable;

    typedef eosio::multi_index<pool_templates_table_name, PoolTemplates,
            eosio::indexed_by<"bypool"_n,
                eosio::const_mem_fun<PoolTemplates, uint64_t, &PoolTemplates::by_pool_id>
            >,
            eosio::indexed_by<"bytempl"_n,
                eosio::const_mem_fun<PoolTemplates, uint64_t, &PoolTemplates::by_template_id>
            >
        >
        PoolTemplatesTable;

    typedef eosio::multi_index<pool_schemas_table_name, PoolSchemas,
            eosio::indexed_by<"bypool"_n,
            eosio::const_mem_fun<PoolSchemas, uint64_t, &PoolSchemas::by_pool_id>>>
        PoolSchemasTable;

    typedef eosio::multi_index<pool_attrs_table_name, PoolAttrs,
            eosio::indexed_by<"bypool"_n, eosio::const_mem_fun<PoolAttrs, uint64_t, &PoolAttrs::by_pool_id>>>
        PoolAttrsTable;

    typedef eosio::multi_index<asset_burners_table_name, AssetBurners,
            eosio::indexed_by<"bypool"_n, eosio::const_mem_fun<AssetBurners, uint64_t, &AssetBurners::by_pool_id>>,
            eosio::indexed_by<"bysubpool"_n, eosio::const_mem_fun<AssetBurners, uint64_t, &AssetBurners::by_subpool_id>>
        >
        AssetBurnersTable;

    typedef eosio::multi_index<rewards_balance_table_name, RewardsBalance,
            eosio::indexed_by<"byaccount"_n, eosio::const_mem_fun<RewardsBalance, uint64_t, &RewardsBalance::by_account>>>
        RewardsBalanceTable;

   
    static vector<RewardsBalance> get_rewards(name account)
    {
        RewardsBalanceTable rt(default_contract_account, default_contract_account.value);
        auto rtba = rt.get_index<"byaccount"_n>();
        auto rtitr = rtba.lower_bound(account.value);
        vector<RewardsBalance> rewards;
        while (rtitr != rtba.end() && rtitr->account == account)
        {
            rewards.push_back(*rtitr);
            rtitr++;
        }
        return rewards;
    }

    static void check_pool(uint64_t pool_id, name pool_type) {
        PoolTable pools(default_contract_account, default_contract_account.value);
        auto pool_itr = pools.find(pool_id);
        check(pool_itr != pools.end(), "Pool not found");
        check(pool_itr->started_at == pool_itr->started_at.min(), "Pool already started");
        check(pool_itr->pool_type == pool_type, "Pool must be of type schemas");
    }

    static void askburn_template(name account, uint64_t pool_id, int32_t template_id) {
        PoolTemplatesTable pt(default_contract_account, default_contract_account.value);
        auto ptbt = pt.get_index<"bytempl"_n>();

        uint64_t subpool_id = 0;
        bool subpool_found = false;
        auto ptitr = ptbt.lower_bound(template_id);
        while (ptitr != ptbt.end() && ptitr->template_id == template_id) {
            if (ptitr->pool_id == pool_id) {
                subpool_id = ptitr->id;
                subpool_found = true;
                break;
            }
            ptitr++;
        }
        check(subpool_found, "Template not found in pool " + std::to_string(template_id));

        AssetBurnersTable ab(default_contract_account, default_contract_account.value);
        auto abbp = ab.get_index<"bypool"_n>();
        auto abitr = abbp.lower_bound(pool_id);
        bool found = false;
        while (abitr != abbp.end() && abitr->pool_id == pool_id) {
            if(abitr->account == account && abitr->pool_sub_id == subpool_id) {
                ab.modify(ab.find(abitr->id), default_contract_account, [&](auto& abrow) {
                    abrow.amount += 1;
                });
                found = true;
            }
            abitr++;
        }

        if (!found) {
            ab.emplace(default_contract_account, [&](auto& abrow) {
                abrow.id = ab.available_primary_key();
                abrow.pool_id = pool_id;
                abrow.pool_sub_id = ptitr->id;
                abrow.account = account;
                abrow.amount = 1;
            });
        }    
    }


    class endlessnftft_contract : public contract
    {
       public:
        using eosio::contract::contract;

        endlessnftft_contract(name receiver, name code, datastream<const char*> ds);

        // void sayhi();

        // void sayhialice(const name& someone);

        void initpool(uint64_t pool_id, asset token_quantity, name token_contract, name pool_type, uint8_t period_days);

        void settemplate(uint64_t pool_id, int32_t template_id, uint8_t percent);
        
        void setschema(uint64_t pool_id, name schema, uint8_t percent);

        void setattr(uint64_t pool_id, name schema, name attr_name, string attr_value, uint8_t percent);

        void startpool(uint64_t pool_id);

        void testaskburn(name account, uint64_t pool_id, int32_t template_id);

        void askburn(name account, uint64_t pool_id, uint64_t asset_id);

        void giverewards(uint64_t pool_id);

        // void editpool(uint32_t template_id);

        // void burnnft(uint64_t asset_id);

        // void claimrewards(name account);

        using PoolTable = endlessnftft::PoolTable;

        using PoolTemplatesTable = endlessnftft::PoolTemplatesTable;   

        using PoolSchemasTable = endlessnftft::PoolSchemasTable;

        using PoolAttrsTable = endlessnftft::PoolAttrsTable;

        using AssetBurnersTable = endlessnftft::AssetBurnersTable;
        
        using RewardsBalanceTable = endlessnftft::RewardsBalanceTable;

        // using Schema1Table = eosio::multi_index<"schema1"_n, Schema1>;
    };

    // This macro:
    // * Creates a part of the dispatcher
    // * Defines action wrappers which make it easy for other contracts and for test cases to invoke
    //   this contract's actions
    // * Optional: provides the names of actions to the ABI generator. Without this, the ABI
    //   generator will make up names (e.g. arg0, arg1, arg2, ...).
    // * Optional: provides ricardian contracts to the ABI generator. Without this, the ABI generator
    //   will leave the ricardian contracts blank.
    EOSIO_ACTIONS(endlessnftft_contract,
                  default_contract_account,
                  action(initpool, pool_id, token_quantity, token_contract, pool_type),
                  action(settemplate, pool_id, template_id, percent),
                  action(setschema, pool_id, schema, percent),
                  action(setattr, pool_id, schema, attr_name, attr_value, percent),
                  action(startpool, pool_id),
                  action(testaskburn, account, pool_id, template_id),
                  action(askburn, account, pool_id, asset_id),
                  action(giverewards, pool_id)
                    //   action(sayhi, ricardian_contract(sayhi_ricardian))
                )

    // See https://github.com/eoscommunity/demo-clsdk/ for another example, including how to listen for a token transfer

}  // namespace endlessnftft