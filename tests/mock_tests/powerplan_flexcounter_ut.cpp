#include "gtest/gtest.h"

#include "flexcounterorch.h"
#include "mock_orch_test.h"

using namespace std;
using namespace swss;
using namespace mock_orch_test;

namespace flexcounter_test
{
    bool checkFlexCounterGroup(const std::string group, std::vector<swss::FieldValueTuple> entries);
    void _hook_sai_switch_api();
    void _unhook_sai_switch_api();
}

class PowerPlanFlexCounterTest : public MockOrchTest
{
protected:
    void ApplySaiMock() override
    {
        flexcounter_test::_hook_sai_switch_api();
    }

    void PreTearDown() override
    {
        flexcounter_test::_unhook_sai_switch_api();
    }
};

TEST_F(PowerPlanFlexCounterTest, FlexCounterPollIntervalScalesWithPowerPlan)
{
    vector<string> flex_counter_tables = {
        CFG_FLEX_COUNTER_TABLE_NAME,
        CFG_DEVICE_METADATA_TABLE_NAME,
        "POWER_PLAN_PROFILE"
    };

    auto *flexCounterOrch = new FlexCounterOrch(m_config_db.get(), flex_counter_tables);
    Table flexCounterCfg(m_config_db.get(), CFG_FLEX_COUNTER_TABLE_NAME);
    Table deviceMetadata(m_config_db.get(), CFG_DEVICE_METADATA_TABLE_NAME);
    Table powerPlanProfile(m_config_db.get(), "POWER_PLAN_PROFILE");

    deviceMetadata.set("localhost", {{"power_plan", "balanced"}});
    powerPlanProfile.set("balanced", {{"polling_interval_multiplier", "2.0"}});
    flexCounterCfg.set("PORT", {{FLEX_COUNTER_STATUS_FIELD, "enable"}, {POLL_INTERVAL_FIELD, "1000"}});

    flexCounterOrch->addExistingData(&deviceMetadata);
    flexCounterOrch->addExistingData(&powerPlanProfile);
    flexCounterOrch->addExistingData(&flexCounterCfg);
    static_cast<Orch *>(flexCounterOrch)->doTask();

    /*
    ASSERT_TRUE(flexcounter_test::checkFlexCounterGroup(PORT_STAT_COUNTER_FLEX_COUNTER_GROUP,
                                                        {
                                                            {POLL_INTERVAL_FIELD, "2000"},
                                                        }));
    */
    std::string effectiveInterval;
    ASSERT_TRUE(flexCounterCfg.hget("PORT", "EFFECTIVE_POLL_INTERVAL", effectiveInterval));
    ASSERT_EQ(effectiveInterval, "2000");
}
