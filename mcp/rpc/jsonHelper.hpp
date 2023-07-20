#pragma once
#include <libdevcore/CommonJS.h>

#include <mcp/common/mcp_json.hpp>
#include <mcp/core/common.hpp>
#include <mcp/core/transaction_receipt.hpp>
#include "exceptions.hpp"

namespace mcp
{
	TransactionSkeleton toTransactionSkeletonForEth(mcp::json const& _json);

	inline Address jsToAddress(std::string const& _s) { return jsToFixed<20>(_s); }

	inline h256 jsToHash(std::string const& _s) { return jsToFixed<32>(_s);  }

	inline Signature jsToSignature(std::string const& _s) { return jsToFixed<65>(_s); }

	inline u128 jsToU128(std::string const& _s) { 
		uint64_t ans = (uint64_t)jsToInt<16>(_s);
		if(ans == 0 && _s != "0"){
			BOOST_THROW_EXCEPTION(NEW_RPC_Eth_Error_InvalidParams("Invalid Argument: not an uint_128 number"));
		}
		return ans;
	}

	inline u64 jsToU64(std::string const& _s) { 
		uint64_t ans = (uint64_t)jsToInt<8>(_s);
		if(ans == 0 && _s != "0"){
			BOOST_THROW_EXCEPTION(NEW_RPC_Eth_Error_InvalidParams("Invalid Argument: not an uint_64 number"));
		}
		return ans;
	}

	inline uint64_t jsToULl(std::string const & _s){
		uint64_t ans = (uint64_t)jsToU64(_s);
		return ans;
	}

	inline BlockNumber jsToBlockNumber(std::string const& _js)
	{
		if (_js == "latest")
			return LatestBlock;
		else if (_js == "earliest")
			return 0;
		else if (_js == "pending")
			return PendingBlock;
		else
			return (BlockNumber)jsToInt(_js);
	}

	uint64_t jsToULl(std::string const& _s);

	mcp::json toJson(Transaction const& _t);

	mcp::json toJson(LocalisedTransaction const& _t);

	mcp::json toJson(dev::eth::LocalisedTransactionReceipt const& _t);

	mcp::json toJson(mcp::localised_log_entries const& _e);

	mcp::json toJson(mcp::localised_log_entry const& _e);

	mcp::json toJson(mcp::log_entry const& _e);

	mcp::json toJson(mcp::block & _b, bool is_eth = false);
}