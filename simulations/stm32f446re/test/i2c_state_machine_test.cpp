#include "gtest/gtest.h"

extern "C" {
    #include "i2c_state_machine.h"
}

extern "C" {
    void _test_fixture_i2c_state_machine_set_state(i2c_state_t state);
    i2c_state_t _test_fixture_i2c_state_machine_get_state();
}

class I2CStateMachineTest : public ::testing::Test {
protected:
    void SetUp() override {
        _test_fixture_i2c_state_machine_set_state(I2C_STATE_TRANSMISSION_NOT_STARTED);
    }

    void TearDown() override {
    }
};

TEST_F(I2CStateMachineTest, FromNotStartedToStartRequested)
{
    // Arrange
    ASSERT_EQ(I2C_STATE_TRANSMISSION_NOT_STARTED, _test_fixture_i2c_state_machine_get_state());
    // Act
    ASSERT_EQ(I2C_OP_INITIATE_TRANSFER, i2c_state_machine_get_operation(I2C_EVENT_START_CONDITION_REQUEST));
    // Assert
    ASSERT_EQ(I2C_STATE_START_CONDITION_REQUESTED, _test_fixture_i2c_state_machine_get_state());
}

TEST_F(I2CStateMachineTest, FromNotStartedToFault)
{
    // Arrange
    ASSERT_EQ(I2C_STATE_TRANSMISSION_NOT_STARTED, _test_fixture_i2c_state_machine_get_state());
    // Act : Out of order event
    ASSERT_EQ(I2C_OP_FAULT, i2c_state_machine_get_operation(I2C_EVENT_START_CONDITION_CONFIRMED));
    // Assert
    ASSERT_EQ(I2C_STATE_FAULT, _test_fixture_i2c_state_machine_get_state());
}
