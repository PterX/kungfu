// demo_td.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#pragma warning(disable : 4996)

#include "kf_time.h"
#include <IMSTradeAPI.h>
#include <csignal>
#include <fstream>
#include <iostream>
#include <pack_types.h>
#include <spdlog/spdlog.h>

using namespace CICC::API;
using namespace CICC::enums;

// constexpr int log_nums = 2000 * 30;
constexpr int log_nums = 2000 * 60 * 5;

constexpr int order_per_second = 10;

// constexpr int seconds_send = 60*3;
constexpr int seconds_send = 60 * 60 * 8;

constexpr int total_num = order_per_second * 60 * 30;
// constexpr int total_num = order_per_second * seconds_send;

constexpr int order_sleep = 2 * 1e5;

std::unordered_map<uint64_t, uint64_t> InputSendTime(total_num + 100);
std::unordered_map<uint64_t, uint64_t> CancelSendTime(total_num + 100);
// std::unordered_map<uint64_t, uint64_t>  ServerOrderInputTime(total_num + 100);
// std::unordered_map<uint64_t, uint64_t>  ServerOrderTime(total_num + 100);
std::unordered_map<uint64_t, uint64_t> ServerTradeTime(total_num + 100);

// std::unordered_map<uint64_t, uint64_t>  OrderRecvTime(total_num + 100);
std::unordered_map<uint64_t, uint64_t> TradeRecvTime(total_num + 100);
std::unordered_map<uint64_t, uint64_t> OrderInputRecvTime(total_num + 100);

std::unordered_map<uint64_t, uint64_t> OrderInputInsertTime(total_num + 100);
std::unordered_map<uint64_t, uint64_t> OrderInsertTime(total_num + 100);
// std::unordered_map<uint64_t, uint64_t>  OrderUpdateTime(total_num + 100);
std::unordered_map<uint64_t, uint64_t> TradeTime(total_num + 100);
std::unordered_map<uint64_t, uint64_t> order_request_map(total_num + 100);

uint64_t orderinput_num = 0;
uint64_t order_num = 0;
uint64_t trade_num = 0;

static uint64_t start_time = 0;
static uint64_t end_time = 0;

using namespace CICC::enums;
using namespace CICC::types;
using namespace kungfu::yijinjing;

bool g_quit = false;
void signal_handler(int signal) {
  if (signal == SIGINT) {
    std::cout << "thread_id:" << std::this_thread::get_id() << " Ctrl+C detected. wait to write file....." << std::endl;
    g_quit = true;
  }
}

struct ServerConfig {
  std::string address;
  std::string group;
  std::string name;
};
ServerConfig read_config(std::string filename) {
  ServerConfig config;
  std::ifstream file(filename);
  std::cout << "read_config" << std::endl;
  ;
  while (1) {
    if (!file.is_open()) {
      std::cout << "open file  error:" << filename << std::endl;
      ;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      continue;
    }
    // 从文件中读取每行数据，并将其依次存储到 ServerConfig 结构体的成员变量中
    std::getline(file, config.address);
    std::getline(file, config.group);
    std::getline(file, config.name);

    return config;
  }
  return config;
}

bool is_final_status(OrderStatus status) {
  return status == OrderStatus::Cancelled || status == OrderStatus::Filled ||
         status == OrderStatus::PartialFilledNotActive;
}

class my_spi : public IMSTradeSPI {
  void OnRspNewOrder(OrderInput *input) override {
    OrderInputRecvTime.try_emplace(input->order_id, time::now_in_nano());
    // ServerOrderInputTime.try_emplace(input->order_id, input->parent_id);
    order_request_map.try_emplace(input->order_id, input->request_id);
    OrderInputInsertTime.try_emplace(input->order_id, input->insert_time);
    orderinput_num++;
    if (orderinput_num % log_nums == 0) [[unlikely]] {
      std::cout << "orderinput:" << orderinput_num << "timestamp:" << time::now_in_nano() << std::endl;
    }
  };
  void OnRspCancelOrder(OrderAction *action) override{
      // std::cout << " OnRspCancelOrder order_action_id: " << action->order_action_id << " order_id: " <<
      // action->order_id << std::endl;
  };
  void OnNotiOrder(Order *order) override {
    // if(is_final_status(order->status)){
    // OrderRecvTime.try_emplace(order->order_id, time::now_in_nano());
    //}
    // ServerOrderTime.try_emplace(order->order_id, order->parent_id);
    // OrderInsertTime.try_emplace(order->order_id, order->insert_time);
    order_num++;
    if (order_num % log_nums == 0) [[unlikely]] {
      std::cout << "order:" << order_num << "timestamp:" << time::now_in_nano() << std::endl;
    }
  };
  void OnNotiKnock(Trade *trade) override {
    TradeRecvTime.try_emplace(trade->order_id, time::now_in_nano());
    TradeTime.try_emplace(trade->order_id, trade->trade_time);
    ServerTradeTime.try_emplace(trade->order_id, trade->parent_order_id);

    trade_num++;
    if (trade_num % log_nums == 0) [[unlikely]] {
      std::cout << "trade:" << trade_num << "timestamp:" << time::now_in_nano() << std::endl;
    }
  };
};

int main() {

  std::cout << "OrderInput size:" << sizeof(CICC::types::OrderInput) << std::endl;
  std::cout << "Order size:" << sizeof(CICC::types::Order) << std::endl;
  std::cout << "Trade size:" << sizeof(CICC::types::Trade) << std::endl;
  // std::cout << "PackOrderInput size:" << sizeof(CICC::types::PackOrderInput) << std::endl;
  // std::cout << "PackOrder size:" << sizeof(CICC::types::PackOrder) << std::endl;
  // std::cout << "PackTrade size:" << sizeof(CICC::types::PackTrade) << std::endl;
  signal(SIGINT, signal_handler);
  std::ios::sync_with_stdio(false);
  // std::string address = "ws://169.254.70.182:9080/test";
  // std::string address = "ws://192.168.100.87:9080/test";
  // std::string address = "ws://192.168.100.128:9080/test";
  // std::string group = "CppSim";
  // std::string name = "123";
  auto config = read_config("./init.config");

  IMSTradeAPI *api = IMSTradeAPI::CreateApi();
  my_spi *spi = new my_spi();
  api->Initial(config.address.c_str());
  api->SetTradeSPI(spi);
  api->TryLogin(config.group.c_str(), config.name.c_str());
  Sleep(5000);
  OrderInput input{};
  input.request_id = 1;
  strcpy(input.instrument_id, "000001");
  strcpy(input.exchange_id, "SZE");
  input.instrument_type = InstrumentType::Stock;
  input.limit_price = 0.02;
  input.frozen_price = 0.02;
  input.volume = 200;
  input.price_type = PriceType::Limit;
  input.side = Side::Buy;
  input.offset = Offset::Open;
  input.hedge_flag = HedgeFlag::Speculation;
  input.block_id = 0;
  input.parent_id = 0;
  input.is_swap = false;
  // input.contract_id = contract_id.c_str();
  // input.insert_time = now();

  OrderAction action{};
  action.action_flag = OrderActionFlag::Cancel;

  start_time = time::now_in_nano();

  int i = 0;
  while (i < total_num) {
    auto second_start = time::now_in_nano();
    std::cout << "send:" << i << std::endl;
    for (int j = 0; j < order_per_second; j++) {
      auto order_start = time::now_in_nano();
      uint64_t id = i++;
      input.request_id = id;

      InputSendTime.try_emplace(id, time::now_in_nano());
      api->ReqNewOrder(input);

      CancelSendTime.try_emplace(id, time::now_in_nano());

      action.order_action_id = id;
      api->ReqCancelOrder(action);
      auto order_end = order_start + order_sleep;
      while (time::now_in_nano() < order_end) {
      }
    }
    auto second_end = second_start + 1e6;
    while (time::now_in_nano() < second_end) {
    }
  }
  auto end_time = time::now_in_nano();

  std::cout << "end use:" << end_time - start_time << std::endl;
  while (!g_quit) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  std::ofstream inputsend_time_record;
  inputsend_time_record.open("InputSendTime.csv",
                             std::ios::out | std::ios::trunc); // 如果不存在则创建该文件，如果有则清空内容
  inputsend_time_record << "request_id,timestamp" << std::endl;
  for (const auto &pair : InputSendTime) {
    if (pair.second > 0) { // 检验数据有效性
      inputsend_time_record << pair.first << "," << pair.second << std::endl;
    }
  }
  inputsend_time_record.close();

  std::ofstream cancelsend_time_record;
  cancelsend_time_record.open("CancelSendTime.csv",
                              std::ios::out | std::ios::trunc); // 如果不存在则创建该文件，如果有则清空内容
  cancelsend_time_record << "request_id,timestamp" << std::endl;
  for (const auto &pair : CancelSendTime) {
    if (pair.second > 0) { // 检验数据有效性
      cancelsend_time_record << pair.first << "," << pair.second << std::endl;
    }
  }
  cancelsend_time_record.close();

  std::ofstream inputrecv_time_record;
  inputrecv_time_record.open("OrderInputRecvTime.csv",
                             std::ios::out | std::ios::trunc); // 如果不存在则创建该文件，如果有则清空内容
  inputrecv_time_record << "order_id,timestamp" << std::endl;
  for (const auto &pair : OrderInputRecvTime) {
    if (pair.second > 0) { // 检验数据有效性
      inputrecv_time_record << pair.first << "," << pair.second << std::endl;
    }
  }
  inputrecv_time_record.close();

  std::ofstream traderecv_time_record;
  traderecv_time_record.open("TradeRecvTime.csv",
                             std::ios::out | std::ios::trunc); // 如果不存在则创建该文件，如果有则清空内容
  traderecv_time_record << "order_id,timestamp" << std::endl;
  for (const auto &pair : TradeRecvTime) {
    if (pair.second > 0) { // 检验数据有效性
      traderecv_time_record << pair.first << "," << pair.second << std::endl;
    }
  }
  traderecv_time_record.close();

  std::ofstream orderinput_time_record;
  orderinput_time_record.open("OrderInputInsertTime.csv",
                              std::ios::out | std::ios::trunc); // 如果不存在则创建该文件，如果有则清空内容
  orderinput_time_record << "order_id,timestamp" << std::endl;
  for (const auto &pair : OrderInputInsertTime) {
    if (pair.second > 0) { // 检验数据有效性
      orderinput_time_record << pair.first << "," << pair.second << std::endl;
    }
  }
  orderinput_time_record.close();

  std::ofstream orderinsert_time_record;
  orderinsert_time_record.open("OrderInsertTime.csv",
                               std::ios::out | std::ios::trunc); // 如果不存在则创建该文件，如果有则清空内容
  orderinsert_time_record << "order_id,timestamp" << std::endl;
  for (const auto &pair : OrderInsertTime) {
    if (pair.second > 0) { // 检验数据有效性
      orderinsert_time_record << pair.first << "," << pair.second << std::endl;
    }
  }
  orderinsert_time_record.close();

  std::ofstream trade_time_record;
  trade_time_record.open("TradeTime.csv", std::ios::out | std::ios::trunc); // 如果不存在则创建该文件，如果有则清空内容
  trade_time_record << "order_id,timestamp" << std::endl;
  for (const auto &pair : TradeTime) {
    if (pair.second > 0) { // 检验数据有效性
      trade_time_record << pair.first << "," << pair.second << std::endl;
    }
  }
  trade_time_record.close();

  std::ofstream server_trade_time_record;
  server_trade_time_record.open("ServerTradeTime.csv",
                                std::ios::out | std::ios::trunc); // 如果不存在则创建该文件，如果有则清空内容
  server_trade_time_record << "order_id,timestamp" << std::endl;
  for (const auto &pair : ServerTradeTime) {
    if (pair.second > 0) { // 检验数据有效性
      server_trade_time_record << pair.first << "," << pair.second << std::endl;
    }
  }
  server_trade_time_record.close();

  std::ofstream order_request_record;
  order_request_record.open("order_request_map.csv",
                            std::ios::out | std::ios::trunc); // 如果不存在则创建该文件，如果有则清空内容
  order_request_record << "order_id,request_id" << std::endl;
  for (const auto &pair : order_request_map) {
    order_request_record << pair.first << "," << pair.second << std::endl;
  }
  order_request_record.close();

  system("pause");
}
