#pragma once

#include <mcp/core/common.hpp>
#include <mcp/core/block_store.hpp>
#include <mcp/common/lruc_cache.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>

namespace mcp
{
class iblock_cache
{
public:
	virtual bool block_exists(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a) = 0;
	virtual std::shared_ptr<mcp::block> block_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a) = 0;
	virtual std::shared_ptr<mcp::block_state> block_state_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a) = 0;
	virtual std::shared_ptr<mcp::account_state> latest_account_state_get(mcp::db::db_transaction & transaction_a, mcp::account const & account_a) = 0;
	virtual bool unlink_block_exists(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a) = 0;
	virtual std::shared_ptr<mcp::unlink_block> unlink_block_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a) = 0;
	virtual std::shared_ptr<mcp::account_info> account_get(mcp::db::db_transaction & transaction_a, mcp::account const & account_a) = 0;
	virtual bool successor_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & root_a, mcp::block_hash & successor_a) = 0;
	virtual bool block_summary_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a, mcp::summary_hash & summary_a) = 0;
	virtual std::set<mcp::account> validator_list_get(mcp::db::db_transaction & transaction_a) = 0;
};

class block_cache : public mcp::iblock_cache
{
  public:
	block_cache(mcp::block_store &store_a);

	bool block_exists(mcp::db::db_transaction & transaction_a, mcp::block_hash const &block_hash_a);
	std::shared_ptr<mcp::block> block_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const &block_hash_a);
	void block_put(mcp::block_hash const & block_hash_a, std::shared_ptr<mcp::block> blocks_a);
	void block_earse(std::unordered_set<mcp::block_hash> const & block_hashs_a);
	void mark_block_as_changing(std::unordered_set<mcp::block_hash> const & block_hashs_a);
	void clear_block_changing();

	std::shared_ptr<mcp::block_state> block_state_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const &block_hash_a);
	void block_state_put(mcp::block_hash const & block_hash_a, std::shared_ptr<mcp::block_state> block_state_a);
	void block_state_earse(std::unordered_set<mcp::block_hash> const & block_hashs_a);
	void mark_block_state_as_changing(std::unordered_set<mcp::block_hash> const & block_hashs_a);
	void clear_block_state_changing();

	std::shared_ptr<mcp::account_state> latest_account_state_get(mcp::db::db_transaction & transaction_a, mcp::account const & account_a);
	void latest_account_state_put(mcp::account const & account_a, std::shared_ptr<mcp::account_state> account_state_a);
	void latest_account_state_earse(std::unordered_set<mcp::account> const & accounts_a);
	void mark_latest_account_state_as_changing(std::unordered_set<mcp::account> const & accounts_a);
	void clear_latest_account_state_changing();

	bool unlink_block_exists(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a);
	std::shared_ptr<mcp::unlink_block> unlink_block_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const &block_hash_a);
	void unlink_block_put(mcp::block_hash const & block_hash_a, std::shared_ptr<mcp::unlink_block> unlink_block_a);
	void unlink_block_earse(std::unordered_set<mcp::block_hash> const & block_hashs_a);
	void mark_unlink_block_as_changing(std::unordered_set<mcp::block_hash> const & block_hashs_a);
	void clear_unlink_block_changing();

	std::shared_ptr<mcp::account_info> account_get(mcp::db::db_transaction & transaction_a, mcp::account const & account_a);
	void account_put(mcp::account const & account_a, std::shared_ptr<mcp::account_info> account_info_a);
	void account_earse(std::unordered_set<mcp::account> const & accounts_a);
	void mark_account_as_changing(std::unordered_set<mcp::account> const & accounts_a);
	void clear_account_changing();

	bool successor_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & root_a, mcp::block_hash & successor_a);
	void successor_put(mcp::block_hash const & root_a, mcp::block_hash const & summary_a);
	void successor_earse(std::unordered_set<mcp::block_hash> const & roots_a);
	void mark_successor_as_changing(std::unordered_set<mcp::account> const & roots_a);
	void clear_successor_changing();

	bool block_summary_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a, mcp::summary_hash & summary_a);
	void block_summary_put(mcp::block_hash const & block_hash_a, mcp::block_hash const & summary_a);
	void block_summary_earse(std::unordered_set<mcp::block_hash> const & block_hashs_a);
	void mark_block_summary_as_changing(std::unordered_set<mcp::block_hash> const & block_hashs_a);
	void clear_block_summary_changing();

	//validators
	std::set<mcp::account> validator_list_get(mcp::db::db_transaction & transaction_a);
	bool validator_list_put(mcp::account const & account_a);
	void validator_list_erase(mcp::account const & account_a);

	std::string report_cache_size();

private:
	mcp::block_store & m_store;

	std::mutex m_block_mutex;
	std::unordered_set<mcp::block_hash> m_block_changings;
	mcp::Cache<mcp::block_hash, std::shared_ptr<mcp::block>> m_blocks;

	std::mutex m_block_state_mutex;
	std::unordered_set<mcp::block_hash> m_block_state_changings;
	mcp::Cache<mcp::block_hash, std::shared_ptr<mcp::block_state>> m_block_states;

	std::mutex m_latest_account_state_mutex;
	std::unordered_set<mcp::account> m_latest_account_state_changings;
	mcp::Cache<mcp::account, std::shared_ptr<mcp::account_state>> m_latest_account_states;

	std::mutex m_unlink_block_mutex;
	std::unordered_set<mcp::block_hash> m_unlink_block_changings;
	mcp::Cache<mcp::block_hash, std::shared_ptr<mcp::unlink_block>> m_unlink_blocks;

	std::mutex m_account_mutex;
	std::unordered_set<mcp::account> m_account_changings;
	mcp::Cache<mcp::account, std::shared_ptr<mcp::account_info>> m_accounts;

	std::mutex m_successor_mutex;
	std::unordered_set<mcp::block_hash> m_successor_changings;
	mcp::Cache<mcp::block_hash, mcp::block_hash> m_successors;

	std::mutex m_block_summary_mutex;
	std::unordered_set<mcp::block_hash> m_block_summary_changings;
	mcp::Cache<mcp::block_hash, mcp::summary_hash> m_block_summarys;

	//validators
	std::mutex m_validator_list_mutex;
	std::set<mcp::account> m_validator_list;	// 100MB = 3495253 validators
};
} // namespace mcp
