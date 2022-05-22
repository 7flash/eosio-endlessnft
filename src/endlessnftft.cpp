#include <eosio/eosio.hpp>
#include <eosio/name.hpp>
#include <string>

#include "endlessnftft.hpp"

using namespace eosio;
using std::string;
using namespace endlessnftft;

endlessnftft_contract::endlessnftft_contract(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds)
{
    /* NOP */
}

// void endlessnftft_contract::sayhi()
// {
//     print("Hi");
// }

// void endlessnftft_contract::sayhialice(const name& someone)
// {
//     check(someone == "alice"_n, "You may only say hi to Alice!");
//     print("Hi, Alice!");
// }

void endlessnftft_contract::startpool(uint64_t pool_id) {
    require_auth(get_self());

    // check token deposit

    PoolTable pools(_self, _self.value);
    auto pool_itr = pools.find(pool_id);
    check(pool_itr != pools.end(), "Pool not found");
    
    check(pool_itr->started_at == pool_itr->started_at.min(), "Pool already started");

    asset expected_deposit = pool_itr->token_quantity;
    asset current_deposit = get_balance(pool_itr->token_contract, get_self(), expected_deposit.symbol.code());

    check(
        expected_deposit.symbol == current_deposit.symbol,
        "Token deposit must be of type " + expected_deposit.symbol.code().to_string() + " but " + current_deposit.symbol.code().to_string() + " was found"
    );

    check(
        expected_deposit.amount == current_deposit.amount,
        "Token deposit must be " + std::to_string(expected_deposit.amount) + " but " + std::to_string(current_deposit.amount) + " was found"
    );

    // check(
    //     expected_deposit == current_deposit,
    //     "Token deposit does not match. Expected: " + expected_deposit.to_string() + ", Actual: " + current_deposit.to_string()
    // );

    pools.modify(pool_itr, _self, [&](auto& p) {
        p.started_at = current_block_time();
    });

    // PoolTemplatesTable pt(get_self(), get_self().value);
    // auto ptbp = pt.get_index<"bypool"_n>();
    // auto ptitr = ptbp.lower_bound(pool_id);
    // uint8_t ptpercent = 0;
    // while (ptitr != ptbp.end() && ptitr->pool_id == pool_id) {
    //     ptpercent += ptitr->percent;
    //     ptitr++;
    // }

    // PoolSchemasTable ps(get_self(), get_self().value);
    // auto psbp = ps.get_index<"bypool"_n>();
    // auto psitr = psbp.lower_bound(pool_id);
    // uint8_t pspercent = 0;
    // while (psitr != psbp.end() && psitr->pool_id == pool_id) {
    //     pspercent += psitr->percent;
    //     psitr++;
    // }

    // check(pspercent == 100 || ptpercent == 100, "Pool must have 100% of templates or 100% of schemas");
    // if (pspercent == 100) {
    //     check(ptpercent == 0, "Pool must have 0% of templates if it has 100% of schemas");
    // } else if (ptpercent == 100) {
    //     check(pspercent == 0, "Pool must have 0% of schemas if it has 100% of templates");
    // }
    
    // pools.modify(pool_itr, get_self(), [&](auto& pool) {
    //     pool.started = true;
        // pool.useschemas = pspercent == 100;
    // });
}

void endlessnftft_contract::setattr(uint64_t pool_id, name schema, name attr_name, string attr_value, uint8_t percent) {
    require_auth(get_self());

    check_pool(pool_id, attributes_pool);

    PoolAttrsTable pa(get_self(), get_self().value);
    auto pabp = pa.get_index<"bypool"_n>();
    auto paitr = pabp.lower_bound(pool_id);
    while (paitr != pabp.end() && paitr->pool_id == pool_id) {
        check(paitr->attr_name != attr_name || paitr->attr_value != attr_value, "Already exists");
        paitr++;
    }

    pa.emplace(get_self(), [&](auto& pa) {
        pa.pool_id = pool_id;
        pa.schema = schema;
        pa.attr_name = attr_name;
        pa.attr_value = attr_value;
        pa.percent = percent;
    });
}

void endlessnftft_contract::settemplate(uint64_t pool_id, int32_t template_id, uint8_t percent)
{
    require_auth(default_contract_account);

    check_pool(pool_id, templates_pool);

    PoolTemplatesTable pt(get_self(), get_self().value);
    auto ptbp = pt.get_index<"bypool"_n>();
    auto itr = ptbp.lower_bound(pool_id);
    while (itr != ptbp.end() && itr->pool_id == pool_id)
    {
        check(itr->template_id != template_id, "Template already exists with defined percent for this pool");
        itr++;
    }

    pt.emplace(default_contract_account, [&](auto& p) {
        p.id = pt.available_primary_key();
        p.pool_id = pool_id;
        p.template_id = template_id;
        p.percent = percent;
    });
}

void endlessnftft_contract::setschema(uint64_t pool_id, name schema, uint8_t percent)
{
    require_auth(default_contract_account);

    check_pool(pool_id, schemas_pool);

    PoolSchemasTable ps(default_contract_account, default_contract_account.value);
    auto psp = ps.get_index<"bypool"_n>();
    auto itr = psp.lower_bound(pool_id);
    while (itr != psp.end() && itr->pool_id == pool_id)
    {
        check(itr->schema != schema, "Schema already exists with defined percent for this pool");
        return;
    }

    ps.emplace(default_contract_account, [&](auto& p) {
        p.id = ps.available_primary_key();
        p.pool_id = pool_id;
        p.schema = schema;
        p.percent = percent;
    });
}

void endlessnftft_contract::askburn(name account, uint64_t pool_id, uint64_t asset_id) {
    if (has_auth(get_self())) {
        require_auth(get_self());
    } else {
        require_auth(account);
    }

    atomicassets::assets_t assets = atomicassets::get_assets(get_self());
    auto asset_itr = assets.find(asset_id);
    check(asset_itr != assets.end(), "Asset not found");

    PoolTemplatesTable pt(get_self(), get_self().value);
    auto ptbt = pt.get_index<"bytempl"_n>();


    // for (auto& pt_itr : pt) {
    //     if (pt_itr.pool_id == pool_id && pt_itr.template_id == asset_itr->template_id) {
    //         subpool_id = pt_itr.id;
    //         subpool_found = true;
    //     }
    // }

    PoolTable pool(get_self(), get_self().value);
    auto pool_itr = pool.find(pool_id);
    check(pool_itr != pool.end(), "Pool not found");

    check(pool_itr->started_at > pool_itr->started_at.min(), "Pool not started");

    if (pool_itr->pool_type == templates_pool) {
        uint64_t subpool_id = 0;
        bool subpool_found = false;
        auto ptitr = ptbt.lower_bound(asset_itr->template_id);
        while (ptitr != ptbt.end() && ptitr->template_id == asset_itr->template_id) {
            if (ptitr->pool_id == pool_id) {
                subpool_id = ptitr->id;
                subpool_found = true;
            }
            ptitr++;
        }
        check(subpool_found, "Template not found in pool " + std::to_string(asset_itr->template_id));

        AssetBurnersTable ab(get_self(), get_self().value);
        auto abbp = ab.get_index<"bypool"_n>();
        auto abitr = abbp.lower_bound(pool_id);
        bool found = false;
        while (abitr != abbp.end() && abitr->pool_id == pool_id) {
            if(abitr->account == account && abitr->pool_sub_id == ptitr->id) {
                ab.modify(ab.find(abitr->id), get_self(), [&](auto& abrow) {
                    abrow.amount += 1;
                });
                found = true;
            }
            abitr++;
        }

        if (!found) {
            ab.emplace(get_self(), [&](auto& abrow) {
                abrow.id = ab.available_primary_key();
                abrow.pool_id = pool_id;
                abrow.pool_sub_id = ptitr->id;
                abrow.account = account;
                abrow.amount = 1;
            });
        }
    } else if (pool_itr->pool_type == schemas_pool) {
        check(false, "Not implemented");
    } else if (pool_itr->pool_type == attributes_pool) {
        check(false, "Not implemented");
    }

    // TODO: actually burn the nft..
}


void endlessnftft_contract::initpool(uint64_t pool_id, asset token_quantity, name token_contract, name pool_type)
{
    require_auth(default_contract_account);

    PoolTable pools(default_contract_account, default_contract_account.value);

    uint64_t id = pools.available_primary_key();
    if (id == 0) {
        id = 1;
    }
    
    check(id == pool_id, "Unexpected pool id");

    check(
        pool_type == templates_pool ||
        pool_type == schemas_pool ||
        pool_type == attributes_pool,
        "Invalid pool type"
    );

    pools.emplace(default_contract_account, [&](auto& p) {
        p.id = pool_id;
        p.token_quantity = token_quantity;
        p.token_contract = token_contract;
        p.pool_type = pool_type;
    });
}

// void endlessnftft_contract::createpool(asset token)
// {
//     require_auth(get_self());

    // PoolTable pools(get_self(), get_self().value);
    // PoolTemplatesTable templates_table(get_self(), get_self().value);
    // PoolSchemasTable schemas_table(get_self(), get_self().value);

    // check(templates.size() > 0 || schemas.size() > 0, "You must specify at least one template or schema!");
    // check(templates.size() == 0 || schemas.size() == 0, "You must specify either templates or schemas, not both!"); 

    // if (templates.size() > 0) {
    //     uint8_t total_percent = 0;
    //     for (auto& template_pair : templates) {
    //         check(template_pair.first > 0, "Template IDs must be greater than 0!");
    //         check(template_pair.second > 0, "Template percentages must be greater than 0!");
    //         total_percent += template_pair.second;
    //     }
    //     check(total_percent == 100, "Template percentages must add up to 100!");
    
    //     for (auto& template_pair : templates) {
    //         templates_table.emplace(get_self(), [&](auto& t) {
    //             t.id = templates_table.available_primary_key();
    //             t.template_id = template_pair.first;
    //             t.percent = template_pair.second;
    //         });
    //     }
    // } else if (schemas.size() > 0) {
    //     uint8_t total_percent = 0;
    //     for (auto& schema_pair : schemas) {
    //         check(schema_pair.first.value > 0, "Schema IDs must be greater than 0!");
    //         check(schema_pair.second > 0, "Schema percentages must be greater than 0!");
    //         total_percent += schema_pair.second;
    //     }
    //     check(total_percent == 100, "Schema percentages must add up to 100!");
    
    //     for (auto& schema_pair : schemas) {
    //         schemas_table.emplace(get_self(), [&](auto& schema) {
    //             schema.id = schemas_table.available_primary_key();
    //             schema.schema = schema_pair.first;
    //             schema.percent = schema_pair.second;
    //         });
    //     }
    // }

    // pools.emplace(get_self(), [&](auto& pool) {
    //     pool.id = pools.available_primary_key();
    //     pool.token = token;
    //     // pool.schemas = schemas;
    //     // pool.templates = templates;
    //     // pool.useschemas = schemas.size() > 0;
    // });
// }

// Final part of the dispatcher
EOSIO_ACTION_DISPATCHER(endlessnftft::actions)

// Things to populate the ABI with
EOSIO_ABIGEN(actions(endlessnftft::actions),
    table(pools_table_name,endlessnftft::Pool),
    table(pool_templates_table_name,endlessnftft::PoolTemplates),
    table(pool_schemas_table_name,endlessnftft::PoolSchemas),
    table(pool_attrs_table_name,endlessnftft::PoolAttrs),
    table(asset_burners_table_name,endlessnftft::AssetBurners),
    ricardian_clause("Class 1 clause", endlessnftft::ricardian_clause))