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

    check(
        expected_deposit == current_deposit,
        "Token deposit does not match. Expected: " + expected_deposit.to_string() + ", Actual: " + current_deposit.to_string()
    );

    action(
        permission_level{get_self(), "active"_n},
        pool_itr->token_contract,
        "transfer"_n,
        make_tuple(
            get_self(),
            bank_account,
            pool_itr->token_quantity,
            "startpool " + to_string(pool_id)
        )
    ).send();

    asset token_quantity_second = asset(
        pool_itr->token_quantity.amount - (pool_itr->token_quantity.amount / 10),
        pool_itr->token_quantity.symbol
    );

    pools.modify(pool_itr, _self, [&](auto& p) {
        p.token_quantity = token_quantity_second;
        p.started_at = current_block_time();
    });
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

void endlessnftft_contract::claimreward(uint64_t pool_id, name account) {
    RewardsBalanceTable rt(get_self(), get_self().value);
    auto rtba = rt.get_index<"byaccount"_n>();
    auto rtba_itr = rtba.lower_bound(account.value);
    
    while (rtba_itr != rtba.end() && rtba_itr->account == account) {
        if (rtba_itr->pool_id == pool_id) {
            break;
        }
        rtba_itr++;
    }
    check (rtba_itr != rtba.end(), "balance not found where user " +
        account.to_string() + " and pool " + to_string(pool_id)
    );

    PoolTable pt(get_self(), get_self().value);
    auto pt_itr = pt.find(rtba_itr->pool_id);
    check(pt_itr != pt.end(), "pool not found where id " + to_string(rtba_itr->pool_id));

    asset account_balance = get_balance(
        pt_itr->token_contract,
        account,
        (pt_itr->token_requirement).symbol.code()
    );
    check(account_balance >= pt_itr->token_requirement,
        "token_requirement defined as " + pt_itr->token_requirement.to_string()
    );

    action(
        permission_level{get_self(), "active"_n},
        pt_itr->token_contract,
        "transfer"_n,
        make_tuple(
            bank_account,
            account,
            rtba_itr->quantity,
            "claimreward " + to_string(pool_id)
        )
    );

    rt.modify(rt.find(rtba_itr->id), get_self(), [&](auto& row) {
        row.quantity = asset(0, row.quantity.symbol);
    });
}

void endlessnftft_contract::giverewards(uint64_t pool_id) {
    require_auth(get_self());

    map<uint64_t, uint64_t> burned_of;

    AssetBurnersTable abt(get_self(), get_self().value);
    auto abtp = abt.get_index<"bypool"_n>();
    auto abtp_itr = abtp.lower_bound(pool_id);
    check(abtp_itr != abtp.end(), "Pool must have burned tokens");

    while (abtp_itr != abtp.end()) {
        PoolTable pt(get_self(), get_self().value);
        auto pt_itr = pt.find(pool_id);
        check(pt_itr != pt.end(), "Pool not found");

        check (pt_itr->started_at > pt_itr->started_at.min(), "Pool is not started");
        check (pt_itr->pool_type == templates_pool, "Pool is not templates pool");

        PoolTemplatesTable pt_tbl(get_self(), get_self().value);
        auto pt_tbl_itr = pt_tbl.find(abtp_itr->pool_sub_id);
        check(pt_tbl_itr != pt_tbl.end(), "Template not found in subpool");

        asset total = pt_itr->token_quantity;
        uint8_t percent = pt_tbl_itr->percent;
        uint8_t days = pt_itr->period_days;

        if (burned_of.find(abtp_itr->pool_sub_id) == burned_of.end()) {
            auto abtsp = abt.get_index<"bysubpool"_n>();
            auto abtsp_itr = abtsp.lower_bound(abtp_itr->pool_sub_id);
            uint64_t burned = 0;
            if (abtsp_itr != abtsp.end()) {
                while (abtsp_itr != abtsp.end() && abtsp_itr->pool_sub_id == abtp_itr->pool_sub_id) {
                    burned += abtsp_itr->amount;
                    abtsp_itr++;
                }
            }
            burned_of[abtp_itr->pool_sub_id] = burned;
        }
        
        uint64_t template_burned_assets = burned_of[abtp_itr->pool_sub_id];

        uint64_t templately = total.amount * percent / 100;
        uint64_t templately_daily = templately / days;
        uint64_t assetly_templately_daily = templately_daily / template_burned_assets;
        uint64_t reward_amount = assetly_templately_daily * abtp_itr->amount;

        asset reward = asset(reward_amount, total.symbol);

        RewardsBalanceTable rt(get_self(), get_self().value);
        auto rtba = rt.get_index<"byaccount"_n>();
        auto rtba_itr = rtba.lower_bound(abtp_itr->account.value);
        bool rt_found = false;
        uint64_t rt_id = 0;
        while (rtba_itr != rtba.end() && rtba_itr->account == abtp_itr->account) {
            if (rtba_itr->pool_id == abtp_itr->pool_id) {
                rt_found = true;
                rt_id = rtba_itr->id;        
            }
            rtba_itr++;
        }

        if (rt_found == true) {
            rt.modify(rt.find(rt_id), get_self(), [&](auto& row) {
                row.quantity += reward;
            });
        } else {
            rt.emplace(get_self(), [&](auto& row) {
                row.id = rt.available_primary_key();
                row.account = abtp_itr->account;
                row.pool_id = abtp_itr->pool_id;
                row.quantity = reward;
            });
        }

        abtp_itr++;
    }
}

// void endlessnftft_contract::askreward(name account, ) {
//     if (has_auth(get_self())) {
//         require_auth(get_self());
//     } else {
//         require_auth(account);
//     }

//     AssetBurnersTable abt(get_self(), get_self().value);
//     auto abtp = abt.get_index<"byaccount"_n>();
//     auto abitr = abtp.lower_bound(account.value);
//     if (abitr == abtp.end() || abitr->account != account) {
//         abt.emplace(get_self(), [&](auto& ab) {
//             ab.id = abt.available_primary_key();
//             ab.account = account;
//             ab.reward_asked = true;
//         });
//     } else {
//         abt.modify(abitr, get_self(), [&](auto& ab) {
//             ab.reward_asked = true;
//         });
//     }

//     PoolTable pools(get_self(), get_self().value);
//     auto pool_itr = pools.find(abtp->pool_id);
//     check(pool_itr != pools.end(), "Pool not found");
//     check(pool_itr->started == true, "Pool not started");
//     check(pool_itr->finished == false, "Pool not finished");

//     if (pool_itr->pool_type == templates_pool) {
//         PoolTemplatesTable ptt(get_self(), get_self().value);

//         auto pttitr 
//     } else {
//         check(false, "Not implemented");
//     }

//     asset total_pool_reward = pool_itr->token_quantity;
//     uint32_t number_of_days = pool_itr->number_of_days;

//     asset reward_per_day = total_pool_reward / number_of_days;

//     PoolTemplatesTable pt(get_self(), get_self().value);
//     auto ptbp = pt.get_index<"bypool"_n>();


//     asset remaining = pool_itr->token_quantity - pool_itr->claimed_quantity;

//     asset daily_of_template = total_pool / number_of_days / template_percentage;

//     asset daily = daily_of_template;

//     asset for_user = daily / number_of_assets_of_template;

//     // distribute rewards based from the current remaining amount in the pool
//     // between every one participated since day one equally
//     // the amount per each is equal to be distributed.. accordingly to each template distribution..
//     // amount to be distributed per day per template.. is known..
//     // then we divide by the number of participants..
//     // 
// }

void endlessnftft_contract::testaskburn(name account, uint64_t pool_id, int32_t template_id) {
    require_auth(get_self());
    
    askburn_template(account, pool_id, template_id);
}

void endlessnftft_contract::askburn(name account, uint64_t pool_id, uint64_t asset_id) {
    require_auth(account);

    atomicassets::assets_t assets = atomicassets::get_assets(get_self());
    auto asset_itr = assets.find(asset_id);
    check(asset_itr != assets.end(), "Asset not found");

    PoolTable pool(get_self(), get_self().value);
    auto pool_itr = pool.find(pool_id);
    check(pool_itr != pool.end(), "Pool not found");

    asset account_balance = get_balance(
        pool_itr->token_contract,
        account,
        (pool_itr->token_requirement).symbol.code()
    );
    check(account_balance >= pool_itr->token_requirement,
        "token_requirement defined as " + pool_itr->token_requirement.to_string()
    );

    check(pool_itr->started_at > pool_itr->started_at.min(), "Pool not started");

    if (pool_itr->pool_type == templates_pool) {
        askburn_template(account, pool_id, asset_itr->template_id);
    } else if (pool_itr->pool_type == schemas_pool) {
        check(false, "Not implemented");
    } else if (pool_itr->pool_type == attributes_pool) {
        check(false, "Not implemented");
    }

    // TODO: actually burn the nft..
}

void endlessnftft_contract::initpool(uint64_t pool_id, asset token_quantity, asset token_requirement, name token_contract, name pool_type, uint8_t period_days)
{
    require_auth(default_contract_account);

    PoolTable pools(default_contract_account, default_contract_account.value);

    uint64_t id = pools.available_primary_key();
    if (id == 0) {
        id = 1;
    }
    
    check(id == pool_id, "Unexpected pool id");

    check(
        period_days == 7 || period_days == 30 || period_days == 90,
        "Invalid period days"
    );

    check(
        pool_type == templates_pool ||
        pool_type == schemas_pool ||
        pool_type == attributes_pool,
        "Invalid pool type"
    );

    pools.emplace(default_contract_account, [&](auto& p) {
        p.id = pool_id;
        p.token_quantity = token_quantity;
        p.token_requirement = token_requirement;
        p.token_contract = token_contract;
        p.pool_type = pool_type;
        p.period_days = period_days;
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
    table(rewards_balance_table_name,endlessnftft::RewardsBalance),
    ricardian_clause("Class 1 clause", endlessnftft::ricardian_clause))