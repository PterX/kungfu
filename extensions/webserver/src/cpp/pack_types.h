#include <cstdint>
#include "kungfu_types.h"

namespace CICC{
namespace types{
constexpr uint32_t OrderInputType			= 111201;
constexpr uint32_t OrderType				= 111202;
constexpr uint32_t TradeType				= 111203;
constexpr uint32_t OrderActionType			= 111204;
constexpr uint32_t OrderActionErrorType		= 111205;
constexpr uint32_t AlgoOrderInputType		= 111213;
constexpr uint32_t AlgoOrderType			= 111214;
constexpr uint32_t AlgoOrderActionType		= 111215;
constexpr uint32_t AlgoOrderActionErrorType	= 111216;
constexpr uint32_t AccountInfoType			= 11110701;
constexpr uint32_t ReqType					= 11110702;
constexpr uint32_t ReqEndType				= 11110703;



static constexpr int KF_ACCOUNT_NAME_LEN = 32;


struct PackOrderInput {
	uint32_t type = OrderInputType;
	CICC::types::OrderInput data;
};

struct PackOrder {
	uint32_t type = OrderType;
	CICC::types::Order data;
}; 

struct PackTrade {
	uint32_t type = TradeType;
	CICC::types::Trade data;
};

struct PackOrderAction {
	uint32_t type = OrderActionType;
	CICC::types::OrderAction data;
};

struct PackOrderActionError {
	uint32_t type = OrderActionErrorType;
	CICC::types::OrderActionError data;
};

struct PackAlgoOrderInput {
	uint32_t type = AlgoOrderInputType;
	CICC::types::AlgoOrderInput data;
};

struct PackAlgoOrder {
	uint32_t type = AlgoOrderType;
	CICC::types::AlgoOrder data;
};

struct PackAlgoOrderAction {
	uint32_t type = AlgoOrderActionType;
	CICC::types::AlgoOrderAction data;
};

struct PackAlgoOrderActionError {
	uint32_t type = AlgoOrderActionErrorType;
	CICC::types::AlgoOrderActionError data;
};

struct PackAccountInfo {
	uint32_t type = AccountInfoType;
	uint64_t stream_id;
	char group[KF_ACCOUNT_NAME_LEN];
	char name[KF_ACCOUNT_NAME_LEN];
	CICC::enums::Method method;
};

struct PackReqEnd {
	uint32_t type = ReqEndType;
};

struct PackRoundReq {
	uint32_t type = ReqType;
	CICC::enums::RoundReqType req_type;
	uint32_t limit;
};

}
}