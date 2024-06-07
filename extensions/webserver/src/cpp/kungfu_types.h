#pragma once
#include <cstdint>
#include <string>
#include "kungfu_enums.h"

namespace CICC{
namespace types {
    static constexpr int INSTRUMENT_ID_LEN = 32;
    static constexpr int PRODUCT_ID_LEN = 128;
    static constexpr int ALGO_TYPE_ID_LEN = 128;
    static constexpr int ALGO_ID_LEN = 128;
    static constexpr int DATE_LEN = 9;
    static constexpr int EXCHANGE_ID_LEN = 16;
    static constexpr int TRAIDNG_PHASE_CODE_LEN = 8;
    static constexpr int ERROR_MSG_LEN = 256;
    static constexpr int EXTERNAL_ID_LEN = 32;
    static constexpr int OPPONENT_SEAT_LEN = 16;
    static constexpr int CONTRACT_OPENINGDATE_LEN = 16;
    static constexpr int CONTRACT_EXPIRATIONDATE_LEN = 16;
    static constexpr int CONTRACT_ID_LEN = 64;

    struct OrderInput {             //
        uint64_t request_id;                                // api请求ID
        uint64_t order_id;                                  // 订单ID
        uint64_t parent_id;                                 // 母单号

        char instrument_id[INSTRUMENT_ID_LEN]; // 合约代码
        char exchange_id[EXCHANGE_ID_LEN];     // 交易所代码

        char contract_id[CONTRACT_ID_LEN]; // 两融合约唯一标识
        CICC::enums::InstrumentType instrument_type;            // 合约类型

        double limit_price;  // 价格
        double frozen_price; // 冻结价格

        double volume; // 数量

        bool is_swap;                            // 互换单
        CICC::enums::Side side;                        // 买卖方向
        CICC::enums::Offset offset;                    // 开平方向
        CICC::enums::HedgeFlag hedge_flag;             // 投机套保标识
        CICC::enums::PriceType price_type;             // 价格类型
        CICC::enums::VolumeCondition volume_condition; // 成交量类型
        CICC::enums::TimeCondition time_condition;     // 成交时间类型
        uint64_t block_id;                       // 大宗交易信息id 非大宗交易则为0

        int64_t insert_time; // 写入时间
    };

    struct Order {
        uint64_t order_id;                                     // 订单ID
        char external_order_id[EXTERNAL_ID_LEN]; // 柜台订单id
        uint64_t parent_id;                                     // 母单号

        int64_t insert_time; // 订单写入时间
        int64_t update_time; // 订单更新时间
        int64_t restore_time; // 根据这个时间决定是否要恢复该数据, 主要针对期货夜盘
        char trading_day[DATE_LEN]; // 针对模拟盘交易日与实际时间不对应

        char instrument_id[INSTRUMENT_ID_LEN]; // 合约ID
        char exchange_id[EXCHANGE_ID_LEN];    // 交易所ID
        char contract_id[CONTRACT_ID_LEN];     // 两融合约唯一标识

        CICC::enums::InstrumentType instrument_type; // 合约类型

        double limit_price;  // 价格
        double frozen_price; // 冻结价格 市价单冻结价格为0

        double volume;      // 数量
        double volume_left; // 剩余数量

        double tax;        // 税
        double commission; // 手续费

        CICC::enums::OrderStatus status; // 订单状态

        int32_t error_id;                             // 错误ID
        char error_msg[ERROR_MSG_LEN]; // 错误信息

        bool is_swap;                            // 互换单
        CICC::enums::Side side;                        // 买卖方向
        CICC::enums::Offset offset;                    // 开平方向
        CICC::enums::HedgeFlag hedge_flag;             // 投机套保标识
        CICC::enums::PriceType price_type;             // 价格类型
        CICC::enums::VolumeCondition volume_condition; // 成交量类型
        CICC::enums::TimeCondition time_condition;      // 成交时间类型
    };

    struct Trade { 
        uint64_t trade_id;                           // 成交ID

        uint64_t order_id;                                      // 订单ID
        uint64_t parent_order_id;                               // 母单号
        char external_order_id[EXTERNAL_ID_LEN]; // 柜台订单id
        char external_trade_id[EXTERNAL_ID_LEN]; // 柜台成交编号id

        int64_t trade_time; // 成交时间
        int64_t restore_time; // 根据这个时间决定是否要恢复该数据, 主要针对期货夜盘
        char trading_day[DATE_LEN]; // 针对模拟盘交易日与实际时间不对应


        char instrument_id[INSTRUMENT_ID_LEN]; // 合约ID
        char exchange_id[EXCHANGE_ID_LEN];     // 交易所ID
        char contract_id[CONTRACT_ID_LEN];     // 两融合约唯一标识

        CICC::enums::InstrumentType instrument_type; // 合约类型

        CICC::enums::Side side;            // 买卖方向
        CICC::enums::Offset offset;        // 开平方向
        CICC::enums::HedgeFlag hedge_flag; // 投机套保标识

        double price;  // 成交价格
        double volume; // 成交量

        double tax;       // 税
        double commission; // 手续费
    };

    struct OrderAction{
        uint64_t order_id;                                       // 订单ID
        uint64_t order_action_id;                                   // 订单操作ID
        CICC::enums::OrderActionFlag action_flag;                         // 订单操作类型
        int64_t insert_time;                                         // 写入时间
    };

    struct OrderActionError{
        uint64_t order_id;                                              // 订单ID
        char external_order_id[EXTERNAL_ID_LEN]; // 撤单原委托柜台订单id 新生成撤单委托编号不记录
        uint64_t order_action_id;                               // 订单操作ID
        int32_t error_id;                                       // 错误ID
        char error_msg[ERROR_MSG_LEN]; // 错误信息
        int64_t insert_time;                           // 写入时间
    };


    struct AlgoOrderInput{
        uint64_t order_id;                                      // 算法单ID
        uint64_t origin_order_id;                               // 原算法单ID
        int64_t insert_time;                                    // 下单时间
        int64_t begin_time;                                     // 开始时间
        int64_t end_time;                                       // 结束时间

        char instrument_id[INSTRUMENT_ID_LEN]; // 合约代码
        char exchange_id[EXCHANGE_ID_LEN];     // 交易所代码
        CICC::enums::InstrumentType instrument_type;                // 合约类型

        uint32_t basket_uid; // basket订单的id

        CICC::enums::Side side;              // 买卖方向
        CICC::enums::Offset offset;          // 开平方向
        CICC::enums::PriceType price_type;   // 价格类型
        CICC::enums::PriceLevel price_level; // 价格级别
        double price_offset;           // 价格偏移量

        double volume; // 目标量

        char algo_type_id[ALGO_TYPE_ID_LEN]; // 算法类型
        char algo_id[ALGO_ID_LEN];           // 算法id

        std::string args; // 自定义参数json的形式
        bool is_local;     // 是否为一个本地算法单
    };

    struct AlgoOrder {
        uint64_t order_id;                                      // 算法单ID
            char external_order_id[EXTERNAL_ID_LEN]; // 柜台算法单id
            int64_t insert_time;                                    // 下单时间
            int64_t update_time;                                    // 更新时间
            int64_t begin_time;                                     // 开始时间
            int64_t end_time;                                       // 结束时间

            char instrument_id[INSTRUMENT_ID_LEN]; // 合约代码
            char exchange_id[EXCHANGE_ID_LEN];     // 交易所代码
            CICC::enums::InstrumentType instrument_type;                // 合约类型

            uint32_t basket_uid; // basket订单的id

            CICC::enums::Side side;              // 买卖方向
            CICC::enums::Offset offset;          // 开平方向
            CICC::enums::PriceType price_type;   // 价格类型
            CICC::enums::PriceLevel price_level; // 价格级别
            double price_offset;           // 价格偏移量

            double volume;      // 目标量
            double volume_left; // 剩余数量

            char algo_type_id[ALGO_TYPE_ID_LEN]; // 算法类型
            char algo_id[ALGO_ID_LEN];           // 算法id

            CICC::enums::OrderStatus status;                    // 订单状态
            char error_msg[ERROR_MSG_LEN]; // 错误信息
            bool is_local;                                // 是否为一个本地算法单
    };

    struct AlgoOrderAction{
        uint64_t order_id;                                              // 订单ID
        uint64_t order_action_id;                                       // 订单操作ID

        CICC::enums::AlgoOrderActionFlag action_flag; // 订单操作类型
        int64_t insert_time;                     // 写入时间
    };

    struct AlgoOrderActionError{
        uint64_t order_id;                                                   // 订单ID
        char external_order_id[EXTERNAL_ID_LEN];              // 柜台算法单id
        uint64_t order_action_id;                                            // 订单操作ID
        int32_t error_id;                                                    // 错误ID
        char error_msg[ERROR_MSG_LEN];                        // 错误信息
        int64_t insert_time;                                                  // 写入时间
    };
}
}