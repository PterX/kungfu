#include <cstdint>
#include <iostream>

enum class Method : int8_t {
    round,
    direct,
};

enum class RoundReqType: int8_t{
    All,
    OrderInput,
    Order,
    Trade,
};

static constexpr int ACCOUNT_NAME_LEN = 32;


constexpr uint32_t AccountInfoType	= 11110701;
constexpr uint32_t ReqType			= 11110702;
constexpr uint32_t ReqEndType		= 11110703;

struct AccountInfo{
	char group[ACCOUNT_NAME_LEN];
	char name[ACCOUNT_NAME_LEN];
	Method method;
};


struct ReqEnd {
};

struct RoundReq {
	RoundReqType req_type;
	uint32_t limit;
};