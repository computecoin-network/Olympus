#pragma once

#include <mcp/common/alarm.hpp>
#include <mcp/wallet/wallet.hpp>
#include <mcp/node/chain.hpp>
#include <mcp/core/block_cache.hpp>

#include <memory>

namespace mcp
{
    class witness_config
    {
    public:
        witness_config();
        void serialize_json(mcp::json &) const;
        bool deserialize_json(mcp::json const &);
		bool parse_old_version_data(mcp::json const &, uint64_t const&);
        bool is_witness;
        std::string account_or_file;
        std::string password;
    };
	class witness : public std::enable_shared_from_this<mcp::witness>
	{
	public:
		witness(mcp::error_message & error_msg, 
			std::shared_ptr<mcp::key_manager> key_manager_a,
			mcp::block_store& store_a, std::shared_ptr<mcp::alarm> alarm_a,
			std::shared_ptr<mcp::composer> composer_a, std::shared_ptr<mcp::chain> chain_a,
			std::shared_ptr<mcp::block_processor> block_processor_a,
			std::shared_ptr<mcp::block_cache> cache_a, std::shared_ptr<TransactionQueue> tq,
			std::string const & account_text, std::string const & password_a
		);
		void start();
		void check_and_witness();
		dev::Secret witness_secret() { return m_secret; } 
		dev::Address witness_account() { return m_account; }

		std::string getInfo();

	private:
		void do_witness();

		mcp::block_store m_store;
		std::shared_ptr<mcp::alarm> m_alarm;
		std::shared_ptr<mcp::chain> m_chain;
		std::shared_ptr<mcp::block_cache> m_cache;
		std::shared_ptr<mcp::composer> m_composer;
		std::shared_ptr<mcp::block_processor> m_block_processor;
		std::shared_ptr<TransactionQueue> m_tq;
		dev::Address m_account;
		dev::Secret m_secret;

		std::chrono::steady_clock::time_point m_last_witness_time;
		uint32_t const m_min_witness_interval = 1000;
		uint32_t const m_max_witness_interval = 2000;
		std::chrono::milliseconds m_witness_interval ;

		static std::atomic_flag m_is_witnessing;
		static uint32_t const m_max_do_witness_interval;
		static uint64_t const m_threshold_distance;
        mcp::log m_log = { mcp::log("node") };

		///logs
		std::atomic<uint64_t> witness_interval_count = { 0 };
		std::atomic<uint64_t> witness_syncing_count = { 0 };
		std::atomic<uint64_t> witness_transaction_count = { 0 };
		std::atomic<uint64_t> witness_notwitness_count = { 0 };
		std::atomic<uint64_t> witness_majority_count = { 0 };
	};
}