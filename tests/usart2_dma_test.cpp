#include <gtest/gtest.h>
#include <string>

// Include the production implementation once so fixtures can reset private
// state without adding test hooks to firmware. MCU headers resolve to fakes.
#include "../src/drivers/usart2.cpp"

namespace {
namespace uart = drivers::usart2;
namespace flags = mcu::dma1::hisr_bit;

class Usart2Dma : public testing::Test {
protected:
  void SetUp() override {
    fake_mcu::reset();
    mcu::interrupts::primask = 0U;
    tx_buffer = {};
    rx_buffer = {};
    dma_diagnostics = {};
    dropped_bytes = dropped_rx = hardware_overruns = 0U;
    dma_tx_active_length = 0U;
    recovery_budget_used = 0U;
    dma_tx_state = DmaTxState::idle;
    ASSERT_TRUE(uart::initialize());
  }

  void interrupt(std::uint32_t status) {
    // Hardware raises read-only status bits; firmware must clear them via HIFCR.
    fake_mcu::values[mcu::dma1::hisr] = status;
    if ((status & flags::tcif6) != 0U && !fake_mcu::stuck_enable) {
      fake_mcu::values[mcu::dma1::s6cr] &= ~mcu::dma1::cr_bit::enable;
      fake_mcu::values[mcu::dma1::s6ndtr] = 0U;
    }
    DMA1_Stream6_IRQHandler();
  }

  void expect_state(DmaTxState expected) {
    EXPECT_EQ(static_cast<DmaTxState>(dma_tx_state), expected);
  }

  void expect_active(const std::string &expected) {
    expect_state(DmaTxState::busy);
    ASSERT_EQ(dma_tx_active_length, expected.size());
    EXPECT_EQ(std::string(reinterpret_cast<const char *>(dma_tx_buffer),
                          dma_tx_active_length), expected);
    EXPECT_EQ(fake_mcu::values[mcu::dma1::s6ndtr], expected.size());
    EXPECT_NE(fake_mcu::values[mcu::dma1::s6cr] & mcu::dma1::cr_bit::enable, 0U);
    EXPECT_NE(fake_mcu::values[mcu::usart2::cr3] &
                  mcu::usart2::cr3_bit::dma_transmitter_enable, 0U);
  }
};

TEST_F(Usart2Dma, CompletionAdvancesQueue) {
  ASSERT_EQ(uart::write("first"), 5U);
  ASSERT_EQ(uart::write("next"), 4U);
  interrupt(flags::tcif6);
  expect_active("next");
  EXPECT_EQ(uart::dma_tx_diagnostics().completed_transfers, 1U);
  interrupt(flags::tcif6);
  expect_state(DmaTxState::idle);
  EXPECT_EQ(uart::dma_tx_diagnostics().completed_transfers, 2U);
  EXPECT_EQ(fake_mcu::values[mcu::dma1::hisr], 0U);
}

class Usart2DmaError : public Usart2Dma,
                      public testing::WithParamInterface<std::uint32_t> {};

TEST_P(Usart2DmaError, CountsOnlyObservedFlagAndResumesWithoutReplay) {
  uart::write("first");
  uart::write("next");
  interrupt(GetParam());
  expect_state(DmaTxState::error);
  const auto diagnostics = uart::dma_tx_diagnostics();
  EXPECT_EQ(diagnostics.transfer_errors, GetParam() == flags::teif6);
  EXPECT_EQ(diagnostics.direct_mode_errors, GetParam() == flags::dmeif6);
  EXPECT_EQ(diagnostics.fifo_errors, GetParam() == flags::feif6);
  EXPECT_EQ(diagnostics.uncertain_bytes, 5U);
  EXPECT_EQ(diagnostics.completed_transfers, 0U);
  EXPECT_EQ(uart::dropped_tx_bytes(), 0U);
  EXPECT_FALSE(fake_mcu::irq_enabled(mcu::nvic::dma1_stream6_irq));
  EXPECT_TRUE(fake_mcu::irq_enabled(mcu::nvic::usart2_irq));
  EXPECT_EQ(fake_mcu::values[mcu::usart2::cr3] &
                mcu::usart2::cr3_bit::dma_transmitter_enable, 0U);

  uart::service_tx();
  expect_active("next");
  EXPECT_TRUE(fake_mcu::irq_enabled(mcu::nvic::dma1_stream6_irq));
  EXPECT_EQ(fake_mcu::values[mcu::dma1::hisr], 0U);
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_attempts, 1U);
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_failures, 0U);
  interrupt(flags::tcif6);
  expect_state(DmaTxState::idle); // No replay remains in the queue.
}

INSTANTIATE_TEST_SUITE_P(IndividualFlags, Usart2DmaError,
                        testing::Values(flags::teif6, flags::dmeif6, flags::feif6));

TEST_F(Usart2Dma, SimultaneousErrorsOverrideCompletionAndAreCountedOnce) {
  uart::write("first");
  interrupt(flags::teif6 | flags::dmeif6 | flags::feif6 | flags::tcif6);
  DMA1_Stream6_IRQHandler(); // A stale invocation while already in error.
  const auto diagnostics = uart::dma_tx_diagnostics();
  EXPECT_EQ(diagnostics.transfer_errors, 1U);
  EXPECT_EQ(diagnostics.direct_mode_errors, 1U);
  EXPECT_EQ(diagnostics.fifo_errors, 1U);
  EXPECT_EQ(diagnostics.completed_transfers, 0U);
  EXPECT_EQ(diagnostics.uncertain_bytes, 5U);
  expect_state(DmaTxState::error);
}

TEST_F(Usart2Dma, StuckEnableHasBoundedAttemptsAndPreservesStagingBuffer) {
  uart::write("first");
  uart::write("next");
  fake_mcu::stuck_enable = true;
  interrupt(flags::teif6);
  fake_mcu::stream_reads = 0U;
  uart::service_tx();
  EXPECT_GT(fake_mcu::stream_reads, 0U);
  EXPECT_LE(fake_mcu::stream_reads, 12U);
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_attempts, 1U);
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_failures, 1U);
  for (int i = 0; i < 10; ++i) uart::service_tx();
  expect_state(DmaTxState::error);
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_attempts, 3U);
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_failures, 3U);
  EXPECT_EQ(std::string(reinterpret_cast<const char *>(dma_tx_buffer), 5), "first");
  EXPECT_FALSE(fake_mcu::irq_enabled(mcu::nvic::dma1_stream6_irq));
  fake_mcu::stuck_enable = false;
  const auto writes_before = fake_mcu::writes;
  uart::service_tx();
  EXPECT_EQ(fake_mcu::writes, writes_before); // Exhaustion is terminal.
  expect_state(DmaTxState::error);
}

TEST_F(Usart2Dma, TemporaryStopFailureRecoversOnLaterCall) {
  uart::write("first");
  uart::write("next");
  fake_mcu::stuck_enable = true;
  interrupt(flags::dmeif6);
  uart::service_tx();
  expect_state(DmaTxState::error);
  fake_mcu::stuck_enable = false;
  uart::service_tx();
  expect_active("next");
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_attempts, 2U);
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_failures, 1U);
}

TEST_F(Usart2Dma, UnclearedFlagsPreventIdleAndQueueRestart) {
  uart::write("first");
  uart::write("next");
  interrupt(flags::feif6);
  fake_mcu::stuck_flags = true;
  uart::service_tx();
  expect_state(DmaTxState::error);
  EXPECT_FALSE(fake_mcu::irq_enabled(mcu::nvic::dma1_stream6_irq));
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_failures, 1U);
  fake_mcu::stuck_flags = false;
  uart::service_tx();
  expect_active("next");
}

TEST_F(Usart2Dma, FailedLaunchCountsKnownLossWithoutUncertainBytes) {
  fake_mcu::values[mcu::dma1::s6cr] |= mcu::dma1::cr_bit::enable;
  fake_mcu::stuck_enable = true;
  uart::write("first");
  expect_state(DmaTxState::error);
  EXPECT_EQ(uart::dropped_tx_bytes(), 5U);
  EXPECT_EQ(uart::dma_tx_diagnostics().uncertain_bytes, 0U);
  fake_mcu::stuck_enable = false;
  uart::service_tx();
  expect_state(DmaTxState::idle);
}

TEST_F(Usart2Dma, CompletionStopFailureDoesNotCountSuccess) {
  uart::write("first");
  fake_mcu::stuck_enable = true;
  interrupt(flags::tcif6);
  expect_state(DmaTxState::error);
  EXPECT_EQ(uart::dma_tx_diagnostics().completed_transfers, 0U);
  EXPECT_EQ(uart::dma_tx_diagnostics().uncertain_bytes, 5U);
  fake_mcu::stuck_enable = false;
  uart::service_tx();
  expect_state(DmaTxState::idle);
}

TEST_F(Usart2Dma, SuccessfulRecoveryAloneDoesNotRenewBudget) {
  for (int i = 0; i < 4; ++i) {
    uart::write("x");
    interrupt(flags::teif6);
    uart::service_tx();
  }
  expect_state(DmaTxState::error);
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_attempts, 3U);
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_failures, 0U);
  EXPECT_EQ(uart::dma_tx_diagnostics().uncertain_bytes, 4U);
}

TEST_F(Usart2Dma, SuccessfulTransferRenewsBudgetButKeepsLifetimeCounters) {
  for (int i = 0; i < 3; ++i) {
    uart::write("x");
    interrupt(flags::teif6);
    uart::service_tx();
  }
  uart::write("ok");
  interrupt(flags::tcif6);
  uart::write("bad");
  interrupt(flags::teif6);
  uart::service_tx();
  expect_state(DmaTxState::idle);
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_attempts, 4U);
  EXPECT_EQ(uart::dma_tx_diagnostics().completed_transfers, 1U);
}

TEST_F(Usart2Dma, ServiceDoesNothingWhenIdleOrBusy) {
  uart::service_tx();
  expect_state(DmaTxState::idle);
  uart::write("first");
  const auto writes_before = fake_mcu::writes;
  uart::service_tx();
  expect_active("first");
  EXPECT_EQ(fake_mcu::writes, writes_before);
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_attempts, 0U);
  EXPECT_EQ(uart::dma_tx_diagnostics().recovery_failures, 0U);
}

TEST_F(Usart2Dma, RxContinuesDuringTxError) {
  uart::write("first");
  interrupt(flags::teif6);
  fake_mcu::values[mcu::usart2::sr] =
      mcu::usart2::sr_bit::receive_data_register_not_empty;
  fake_mcu::values[mcu::usart2::dr] = 'p';
  USART2_IRQHandler();
  std::uint8_t byte = 0U;
  ASSERT_TRUE(uart::read_byte(byte));
  EXPECT_EQ(byte, 'p');
  EXPECT_TRUE(fake_mcu::irq_enabled(mcu::nvic::usart2_irq));
  expect_state(DmaTxState::error);
}

class Usart2DmaPrimask : public Usart2Dma,
                        public testing::WithParamInterface<std::uint32_t> {};
TEST_P(Usart2DmaPrimask, RestoresPreviousMaskOnEveryServicePathAndSnapshot) {
  mcu::interrupts::primask = GetParam();
  uart::service_tx(); // idle
  EXPECT_EQ(mcu::interrupts::primask, GetParam());
  uart::write("first");
  uart::service_tx(); // busy
  EXPECT_EQ(mcu::interrupts::primask, GetParam());
  interrupt(flags::teif6);
  fake_mcu::stuck_flags = true;
  uart::service_tx(); // failed recovery
  EXPECT_EQ(mcu::interrupts::primask, GetParam());
  fake_mcu::stuck_flags = false;
  uart::service_tx(); // successful recovery
  EXPECT_EQ(mcu::interrupts::primask, GetParam());
  uart::write("next");
  interrupt(flags::teif6);
  fake_mcu::stuck_flags = true;
  uart::service_tx(); // third attempt
  uart::service_tx(); // exhausted
  EXPECT_EQ(mcu::interrupts::primask, GetParam());
  (void)uart::dma_tx_diagnostics();
  EXPECT_EQ(mcu::interrupts::primask, GetParam());
}
INSTANTIATE_TEST_SUITE_P(PreviousMask, Usart2DmaPrimask, testing::Values(0U, 1U));
} // namespace
