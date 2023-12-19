// SPDX-License-Identifier: Apache-2.0
#include <kungfu/wingchun/operator/runner.h>

using namespace kungfu::rx;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun::broker;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::practice;

namespace kungfu::wingchun::op {
Runner::Runner(locator_ptr locator, const std::string &group, const std::string &name, mode m, bool low_latency)
    : apprentice(location::make_shared(m, category::OPERATOR, group, name, std::move(locator)), low_latency),
      started_(m == mode::BACKTEST) {}

RuntimeContext_ptr Runner::get_context() const { return context_; }

RuntimeContext_ptr Runner::make_context() { return std::make_shared<RuntimeContext>(*this, events_); }

void Runner::add_operator(const Operator_ptr &op) { operators_.push_back(op); }

void Runner::on_exit() { post_stop(); }

void Runner::on_trading_day(const event_ptr &event, int64_t daytime) { invoke(&Operator::on_trading_day, daytime); }

void Runner::on_react() { context_ = make_context(); }

void Runner::on_start() {
  pre_start();
  enable(*context_);

  // TODO add skip_until for broker_states_requested_ == true later
  events_ | is_own<Deregister>(context_->get_broker_client()) |
      $$(invoke(&Operator::on_deregister, event->data<Deregister>(), get_location(event->source())));
  events_ | is_own<BrokerStateUpdate>(context_->get_broker_client()) |
      $$(invoke(&Operator::on_broker_state_change, event->data<BrokerStateUpdate>(), get_location(event->source())));
  events_ | is_own<OperatorStateUpdate>(context_->get_broker_client()) |
      $$(invoke(&Operator::on_operator_state_change, event->data<OperatorStateUpdate>(),
                get_location(event->source())));

  if (get_home()->mode == mode::LIVE) {
    auto start_events = events_ | skip_until(events_ | filter([&](auto e) { return started_; }));
    start_events | is(Deregister::tag) | $$(context_->check_dependency_state(event));
    start_events | is(OperatorStateUpdate::tag) | $$(context_->check_dependency_state(event));
    start_events | is(BrokerStateUpdate::tag) | $$(context_->check_dependency_state(event));
  }

  events_ | take_until(events_ | filter([&](auto e) { return started_; })) | $$(prepare(event));
  post_start();
}

void Runner::on_active() {
  if (not is_live()) {
    pre_stop();
  }
}

void Runner::pre_start() { invoke(&Operator::pre_start); }

void Runner::post_start() {
  if (not started_) {
    return;
  }

  invoke(&Operator::post_start);
  SPDLOG_INFO("operator {} started", get_io_device()->get_home()->name);

  events_ | is_own<Quote>(context_->get_broker_client()) |
      $$(invoke(&Operator::on_quote, event->data<Quote>(), get_location(event->source()), event->dest()));
  events_ | is_own<Entrust>(context_->get_broker_client()) |
      $$(invoke(&Operator::on_entrust, event->data<Entrust>(), get_location(event->source()), event->dest()));
  events_ | is_own<Transaction>(context_->get_broker_client()) |
      $$(invoke(&Operator::on_transaction, event->data<Transaction>(), get_location(event->source()), event->dest()));

  events_ | is(SyntheticData::tag) |
      $$(invoke(&Operator::on_synthetic_data, event->data<SyntheticData>(), get_location(event->source()),
                event->dest()));
}

void Runner::pre_stop() { invoke(&Operator::pre_stop); }

void Runner::post_stop() { invoke(&Operator::post_stop); }

void Runner::prepare(const event_ptr &event) {
  auto ledger_uid = ledger_home_location_->uid;
  if (not has_writer(ledger_uid)) {
    SPDLOG_INFO("not hast ledger writer");
    return;
  }
  auto writer = get_writer(ledger_uid);

  auto connected_test = [&](const auto &locations) {
    for (const auto &pair : locations) {
      if (not context_->get_broker_client().is_connected(pair.second->uid)) {
        return false;
      }
    }
    return true;
  };
  if (not broker_states_requested_ and connected_test(context_->list_md()) and connected_test(context_->list_op())) {

    writer->mark(now(), BrokerStateRequest::tag);
    writer->mark(now(), OperatorStateRequest::tag);
    broker_states_requested_ = true;
  }

  if (not context_->get_broker_client().enrolled_md_ready() or
      not context_->get_broker_client().enrolled_operator_ready()) {
    return;
  }
  started_ = true;

  OperatorStateUpdate state_update;
  state_update.state = OperatorState::Ready;
  context_->update_operator_state(state_update);
  post_start();
}
} // namespace kungfu::wingchun::op
