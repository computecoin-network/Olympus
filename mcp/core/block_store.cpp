#include "block_store.hpp"
#include <mcp/common/utility.hpp>

mcp::block_store::block_store(bool & error_a, boost::filesystem::path const & path_a) :
	m_db(std::make_shared<mcp::db::database>(path_a)),
	dag_account_info(0),
	account_info(0),
	account_state(0),
	latest_account_state(0),
	blocks(0),
	blocks_data(0),
	block_state(0),
	contract_main(0),
	contract_aux(0),
	block_child(0),
	dag_free(0),
	main_chain(0),
	stable_block(0),
	block_summary(0),
	summary_block(0),
	skiplist(0),
	successor(0),
	prop(0),
	catchup_chain_summaries(0),
	catchup_chain_block_summary(0),
	catchup_chain_summary_block(0),
	hash_tree_summary(0),
	unlink_block(0),
	unlink_info(0),
	next_unlink(0),
	next_unlink_index(0),
	head_unlink(0),
	traces(0)
{
	if (error_a)
		return;

	auto tbops_prefix = mcp::db::db_column::default_table_options(mcp::db::database::get_table_cache());
	if (rocksdb::BlockBasedTableOptions::IndexType::kBinarySearch == tbops_prefix->index_type)
		tbops_prefix->index_type = rocksdb::BlockBasedTableOptions::IndexType::kHashSearch;
	
	auto cfops_prefix = mcp::db::db_column::default_column_family_options(tbops_prefix);
	cfops_prefix->prefix_extractor.reset(rocksdb::NewFixedPrefixTransform(3));
	cfops_prefix->memtable_prefix_bloom_size_ratio = 0.02;
	//cfops_prefix->max_write_buffer_number = 6;
	//cfops_prefix->min_write_buffer_number_to_merge = 2;
	//default
	int default_col = m_db->create_column_family(rocksdb::kDefaultColumnFamilyName, cfops_prefix);
	dag_account_info = m_db->set_column_family(default_col, "001");
	account_info = m_db->set_column_family(default_col, "002");
	account_state = m_db->set_column_family(default_col, "003");
	latest_account_state = m_db->set_column_family(default_col, "004");
	blocks = m_db->set_column_family(default_col, "005");
	blocks_data = m_db->set_column_family(default_col, "006");
	block_state = m_db->set_column_family(default_col, "007");
	successor = m_db->set_column_family(default_col, "008");
	main_chain = m_db->set_column_family(default_col, "009");
	skiplist = m_db->set_column_family(default_col, "010");
	block_summary = m_db->set_column_family(default_col, "011");
	summary_block = m_db->set_column_family(default_col, "012");
	stable_block = m_db->set_column_family(default_col, "013");
	contract_main = m_db->set_column_family(default_col, "014");
	prop = m_db->set_column_family(default_col, "015");
	catchup_chain_summaries = m_db->set_column_family(default_col, "016");
	catchup_chain_block_summary = m_db->set_column_family(default_col, "017");
	catchup_chain_summary_block = m_db->set_column_family(default_col, "018");
	hash_tree_summary = m_db->set_column_family(default_col, "019");
	unlink_block = m_db->set_column_family(default_col, "020");
	traces = m_db->set_column_family(default_col, "021");
	next_unlink = m_db->set_column_family(default_col, "022");
	next_unlink_index = m_db->set_column_family(default_col, "023");
	contract_aux = m_db->set_column_family(default_col, "024");

	//use iterator
	dag_free = m_db->set_column_family(default_col, "101");
	block_child = m_db->set_column_family(default_col, "102");
	unlink_info = m_db->set_column_family(default_col, "103");
	head_unlink = m_db->set_column_family(default_col, "104");


	////column have used iterator 
	//auto tbops_iter = mcp::db::db_column::default_table_options(mcp::db::database::get_table_cache());
	//if (rocksdb::BlockBasedTableOptions::IndexType::kBinarySearch == tbops_iter->index_type)
	//	tbops_iter->index_type = rocksdb::BlockBasedTableOptions::IndexType::kHashSearch;

	//auto cfops_iter = mcp::db::db_column::default_column_family_options(tbops_iter);
	//cfops_iter->prefix_extractor.reset(rocksdb::NewFixedPrefixTransform(3));
	//cfops_iter->memtable_prefix_bloom_size_ratio = 0.02;
	////cfops_iter->max_write_buffer_number = 6;
	////cfops_iter->min_write_buffer_number_to_merge = 2;
	//int iter_col = m_db->create_column_family("iter", cfops_iter);

	//dag_free = m_db->set_column_family(iter_col, "001");
	//block_child = m_db->set_column_family(iter_col, "002");
	//unlink_info = m_db->set_column_family(iter_col, "003");
	//head_unlink = m_db->set_column_family(iter_col, "004");

	error_a = !m_db->open();
	if (error_a)
	{
		std::cerr << "Block store db open error" << std::endl;
		return;
	}

	error_a = !upgrade();
	if (error_a)
	{
		std::cerr << "Block store db upgrade error" << std::endl;
		return;
	}
}

bool mcp::block_store::upgrade()
{
	bool ok(true);
	if (version_get() == 1)
	{
	}

	return ok;
}

std::string mcp::block_store::get_rocksdb_state(uint64_t limit)
{
	std::string str = "";
	if (m_db)
	{
		str = m_db->get_rocksdb_state(limit);
	}
	return str;
}

void mcp::block_store::version_put(mcp::db::db_transaction & transaction_a, int version_a)
{
	mcp::uint256_union version_value(version_a);
	transaction_a.put(prop, mcp::uint256_to_slice(version_key), mcp::uint256_to_slice(version_value));
}

int mcp::block_store::version_get()
{
	mcp::db::db_transaction transaction(create_transaction());
	std::string data;
	bool exists(transaction.get(prop, mcp::uint256_to_slice(version_key), data));
	int result;
	if (!exists)
	{
		result = 1;
	}
	else
	{
		mcp::uint256_union version_value(mcp::slice_to_uint256(dev::Slice(data)));
		assert_x(version_value.qwords[2] == 0 && version_value.qwords[1] == 0 && version_value.qwords[0] == 0);
		result = version_value.number().convert_to<int>();
	}
	return result;
}

std::shared_ptr<mcp::block> mcp::block_store::block_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & hash_a)
{
	std::string value;
	bool exists(transaction_a.get(blocks, mcp::uint256_to_slice(hash_a), value));
	std::shared_ptr<mcp::block> result;
	if (exists)
	{
		dev::RLP r(value);
		bool error(false);
		result = std::make_shared<mcp::block>(error, r, false);
		assert_x_msg(!error, "hash:" + hash_a.to_string() + " ,data:" + value);
		assert_x_msg(result != nullptr, "hash:" + hash_a.to_string() + " ,data:" + value);

		bool data_exists(!block_data_get(transaction_a, result->hashables->data_hash, result->data));
		if (data_exists)
			assert_x(result->hashables->data_hash == mcp::block::data_hash(result->data));
	}
	return result;
}

bool mcp::block_store::block_data_get(mcp::db::db_transaction & transaction_a, mcp::data_hash const & hash_a, dev::bytes & data_a)
{
	std::string value;
	bool exists(transaction_a.get(blocks_data, mcp::uint256_to_slice(hash_a), value));
	if (exists)
	{
		dev::RLP r(value);
		assert_x(r.itemCount() == 1)
		data_a = r[0].toBytes();
	}
	return !exists;
}

bool mcp::block_store::block_exists(mcp::db::db_transaction & transaction_a, mcp::block_hash const & hash_a)
{
	std::string result;
	bool exists(transaction_a.get(blocks, mcp::uint256_to_slice(hash_a), result));
	return exists;
}

void mcp::block_store::block_put(mcp::db::db_transaction & transaction_a, mcp::block_hash const & hash_a, mcp::block const & block_a)
{
	// all parts of block except data
	dev::bytes b_value;
	{
		dev::RLPStream s;
		block_a.stream_RLP_nodata(s);
		s.swapOut(b_value);
	}

	dev::Slice s_value((char *)b_value.data(), b_value.size());
	transaction_a.put(blocks, mcp::uint256_to_slice(hash_a), s_value);
	transaction_a.count_add("block", 1);

	// block data
	if (!block_a.data.empty())
		block_data_put(transaction_a, block_a.hashables->data_hash, block_a.data);
}

void mcp::block_store::block_data_put(mcp::db::db_transaction & transaction_a, mcp::data_hash const & hash_a, dev::bytes const & data_a)
{
	dev::bytes b_value;
	{
		dev::RLPStream s;
		s.appendList(1);
		s << data_a;
		s.swapOut(b_value);
	}

	dev::Slice s_value((char *)b_value.data(), b_value.size());
	transaction_a.put(blocks_data, mcp::uint256_to_slice(hash_a), s_value);
}

void mcp::block_store::block_data_del(mcp::db::db_transaction & transaction_a, mcp::data_hash const & hash_a)
{
	transaction_a.del(blocks_data, mcp::uint256_to_slice(hash_a));
}

///////////
mcp::db::forward_iterator mcp::block_store::block_begin(mcp::db::db_transaction & transaction_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	mcp::db::forward_iterator result(transaction_a.begin(blocks, snapshot_a));
	return result;
}

size_t mcp::block_store::block_count(mcp::db::db_transaction & transaction_a)
{
	return transaction_a.count_get("block");
}

bool mcp::block_store::dag_account_get(mcp::db::db_transaction & transaction_a, mcp::account_512 const & account_a, mcp::dag_account_info & info_a)
{
	std::string value;
	bool exists(transaction_a.get(dag_account_info, mcp::uint512_to_slice(account_a), value));
	if (exists)
	{
		dev::RLP r(value);
		bool error(false);
		info_a = mcp::dag_account_info(error, r);
		assert_x(!error);
	}
	return !exists;
}

void mcp::block_store::dag_account_put(mcp::db::db_transaction & transaction_a, mcp::account_512 const & account_a, mcp::dag_account_info const & info_a)
{
	dev::bytes b_value;
	{
		dev::RLPStream s;
		info_a.stream_RLP(s);
		s.swapOut(b_value);
	}
	dev::Slice s_value((char *)b_value.data(), b_value.size());

	transaction_a.put(dag_account_info, mcp::uint512_to_slice(account_a), s_value);
}

std::shared_ptr<mcp::account_info> mcp::block_store::account_get(mcp::db::db_transaction & transaction_a, mcp::account const & account_a)
{
	std::shared_ptr<mcp::account_info> info;
	std::string value;
	bool exists(transaction_a.get(account_info, mcp::uint256_to_slice(account_a), value));
	if (exists)
	{
		dev::RLP r(value);
		bool error(false);
		info = std::make_shared<mcp::account_info>(error, r);
		assert_x(!error);
	}
	return info;
}

void mcp::block_store::account_put(mcp::db::db_transaction & transaction_a, mcp::account const & account_a, mcp::account_info & info_a)
{
	dev::bytes b_value;
	{
		dev::RLPStream s;
		info_a.stream_RLP(s);
		s.swapOut(b_value);
	}
	dev::Slice s_value((char *)b_value.data(), b_value.size());
	transaction_a.put(account_info, mcp::uint256_to_slice(account_a), s_value);
}

std::shared_ptr<mcp::account_state> mcp::block_store::account_state_get(mcp::db::db_transaction & transaction_a, mcp::account_state_hash const & hash_a)
{
	std::shared_ptr<mcp::account_state> result;
	std::string value;
	bool exists(transaction_a.get(account_state, mcp::uint256_to_slice(hash_a), value));
	if (exists)
	{
		dev::RLP r(value);
		bool error(false);
		result = std::make_shared<mcp::account_state>(error, r);
		assert_x(!error);
	}
	return result;
}

void mcp::block_store::account_state_put(mcp::db::db_transaction & transaction_a, mcp::account_state_hash const & hash_a, mcp::account_state const & value_a)
{
	dev::bytes b_value;
	{
		dev::RLPStream s;
		value_a.stream_RLP(s);
		s.swapOut(b_value);
	}
	dev::Slice s_value((char *)b_value.data(), b_value.size());
	transaction_a.put(account_state, mcp::uint256_to_slice(hash_a), s_value);
}


bool mcp::block_store::latest_account_state_get(mcp::db::db_transaction & transaction_a, mcp::account const & account_a, mcp::account_state_hash & hash_a)
{
	std::string value;
	bool exists(transaction_a.get(latest_account_state, mcp::uint256_to_slice(account_a), value));
	if (exists)
		hash_a = mcp::slice_to_uint256(value);
	return !exists;
}


// Updated from 256 to 512, Daniel
void mcp::block_store::latest_account_state_put(mcp::db::db_transaction & transaction_a, mcp::account_512 const & account_a, mcp::account_state_hash const & hash_a)
{
	transaction_a.put(latest_account_state, mcp::uint512_to_slice(account_a), mcp::uint512_to_slice(hash_a));
}


bool mcp::block_store::block_summary_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a, mcp::summary_hash & summary_hash_a)
{
	std::string value;
	bool exists(transaction_a.get(block_summary, mcp::uint256_to_slice(block_hash_a), value));
	if (exists)
	{
		summary_hash_a = mcp::slice_to_uint256(value);
	}
	return !exists;
}

void mcp::block_store::block_summary_put(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a, mcp::summary_hash const & summary_hash_a)
{
	transaction_a.put(block_summary, mcp::uint256_to_slice(block_hash_a), mcp::uint256_to_slice(summary_hash_a));
}


bool mcp::block_store::summary_block_get(mcp::db::db_transaction & transaction_a, mcp::summary_hash const &  summary_hash_a, mcp::block_hash & block_hash_a)
{
	std::string value;
	bool exists(transaction_a.get(summary_block, mcp::uint256_to_slice(summary_hash_a), value));
	if (exists)
	{
		block_hash_a = mcp::slice_to_uint256(value);
	}
	return !exists;
}

void mcp::block_store::summary_block_put(mcp::db::db_transaction & transaction_a, mcp::summary_hash const & summary_hash_a, mcp::block_hash const & block_hash_a)
{
	transaction_a.put(summary_block, mcp::uint256_to_slice(summary_hash_a), mcp::uint256_to_slice(block_hash_a));
}

std::shared_ptr<mcp::block_state> mcp::block_store::block_state_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & hash_a)
{
	std::string value;
	bool exists(transaction_a.get(block_state, mcp::uint256_to_slice(hash_a), value));
	std::shared_ptr<mcp::block_state> result;
	if (exists)
	{
		dev::RLP r(value);
		bool error(false);
		result = std::make_shared<mcp::block_state>(error, r);
		assert_x(!error);
		assert_x(result != nullptr);
	}
	return result;
}

void mcp::block_store::block_state_put(mcp::db::db_transaction & transaction_a, mcp::block_hash const & hash_a, mcp::block_state const & state_a)
{
	dev::bytes b_value;
	{
		dev::RLPStream s;
		state_a.stream_RLP(s);
		s.swapOut(b_value);
	}
	dev::Slice s_value((char *)b_value.data(), b_value.size());
	transaction_a.put(block_state, mcp::uint256_to_slice(hash_a), s_value);
}

mcp::db::forward_iterator mcp::block_store::dag_free_begin(mcp::db::db_transaction & transaction_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	mcp::db::forward_iterator result(transaction_a.begin(dag_free, snapshot_a));
	return result;
}

mcp::db::forward_iterator mcp::block_store::dag_free_begin(mcp::db::db_transaction & transaction_a, mcp::free_key const & key_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	dev::bytes b_key;
	{
		mcp::vectorstream stream(b_key);
		key_a.serialize(stream);
	}
	dev::Slice s_key((char*)b_key.data(), b_key.size());
	mcp::db::forward_iterator result(transaction_a.begin(dag_free, s_key, snapshot_a));
	return result;
}

mcp::db::backward_iterator mcp::block_store::dag_free_rbegin(mcp::db::db_transaction & transaction_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	mcp::db::backward_iterator result(transaction_a.rbegin(dag_free, snapshot_a));
	return result;
}

void mcp::block_store::dag_free_put(mcp::db::db_transaction & transaction_a, mcp::free_key const & key_a)
{
	dev::bytes b_key;
	{
		mcp::vectorstream stream(b_key);
		key_a.serialize(stream);
	}
	dev::Slice s_key((char*)b_key.data(), b_key.size());
	transaction_a.put(dag_free, s_key, dev::Slice());
	transaction_a.count_add("dag_free", 1);
}

void mcp::block_store::dag_free_del(mcp::db::db_transaction & transaction_a, mcp::free_key const & key_a)
{
	dev::bytes b_key;
	{
		mcp::vectorstream stream(b_key);
		key_a.serialize(stream);
	}
	dev::Slice s_key((char*)b_key.data(), b_key.size());

	{
		std::string result;
		bool exists(transaction_a.get(dag_free, s_key, result));
		if (!exists)
		{
			assert(false);
		}
	}

	transaction_a.del(dag_free, s_key);

	transaction_a.count_reduce("dag_free", 1);
}

size_t mcp::block_store::dag_free_count(mcp::db::db_transaction & transaction_a)
{
	return transaction_a.count_get("dag_free");
}


bool mcp::block_store::main_chain_get(mcp::db::db_transaction & transaction_a, uint64_t const & mci_a, mcp::block_hash & hash_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	std::string value;
	mcp::uint64_union mci(mci_a);
	bool exists(transaction_a.get(main_chain, mcp::uint64_to_slice(mci), value, snapshot_a));
	if (exists)
	{
		hash_a = mcp::slice_to_uint256(value);
	}
	return !exists;
}

void mcp::block_store::main_chain_put(mcp::db::db_transaction & transaction_a, uint64_t const & mci_a, mcp::block_hash const & hash_a)
{
	mcp::uint64_union mci(mci_a);
	transaction_a.put(main_chain, mcp::uint64_to_slice(mci), mcp::uint256_to_slice(hash_a));
}

void mcp::block_store::main_chain_del(mcp::db::db_transaction & transaction_a, uint64_t const & mci_a)
{
	mcp::uint64_union mci(mci_a);
	transaction_a.del(main_chain, mcp::uint64_to_slice(mci));
}

size_t mcp::block_store::stable_block_count(mcp::db::db_transaction & transaction_a)
{
	return last_stable_index_get(transaction_a) + 1;
}

bool mcp::block_store::stable_block_get(mcp::db::db_transaction & transaction_a, uint64_t const & index_a, mcp::block_hash & hash_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	std::string value;
	mcp::uint64_union index(index_a);
	bool exists(transaction_a.get(stable_block, mcp::uint64_to_slice(index), value, snapshot_a));
	if (exists)
	{
		hash_a = mcp::slice_to_uint256(value);
	}
	return !exists;
}

void mcp::block_store::stable_block_put(mcp::db::db_transaction & transaction_a, uint64_t const & index_a, mcp::block_hash const & hash_a)
{
	mcp::uint64_union index(index_a);
	transaction_a.put(stable_block, mcp::uint64_to_slice(index), mcp::uint256_to_slice(hash_a));
}

size_t mcp::block_store::light_unstable_count(mcp::db::db_transaction & transaction_a)
{
	return transaction_a.count_get("light_unstable");
}

void mcp::block_store::light_unstable_count_add(mcp::db::db_transaction & transaction_a)
{
	transaction_a.count_add("light_unstable", 1);
}

void mcp::block_store::light_unstable_count_reduce(mcp::db::db_transaction & transaction_a)
{
	transaction_a.count_reduce("light_unstable", 1);
}


uint64_t mcp::block_store::last_mci_get(mcp::db::db_transaction & transaction_a)
{
	std::string value;
	bool exists(transaction_a.get(prop, mcp::uint256_to_slice(last_mci_key), value));
	uint64_t result(0);
	if (exists)
		result = mcp::slice_to_uint64(value).number();
	return result;
}

void mcp::block_store::last_mci_put(mcp::db::db_transaction & transaction_a, uint64_t const & last_mci_a)
{
	mcp::uint64_union last_mci(last_mci_a);
	transaction_a.put(prop, mcp::uint256_to_slice(last_mci_key), mcp::uint64_to_slice(last_mci));
}


uint64_t mcp::block_store::last_stable_mci_get(mcp::db::db_transaction & transaction_a)
{
	std::string value;
	bool exists(transaction_a.get(prop, mcp::uint256_to_slice(last_stable_mci_key), value));
	uint64_t result(0);
	if (exists)
		result = mcp::slice_to_uint64(value).number();
	return result;
}

void mcp::block_store::last_stable_mci_put(mcp::db::db_transaction & transaction_a, uint64_t const & last_stable_mci_a)
{
	mcp::uint64_union last_stable_mci(last_stable_mci_a);
	transaction_a.put(prop, mcp::uint256_to_slice(last_stable_mci_key), mcp::uint64_to_slice(last_stable_mci));
}


mcp::advance_info mcp::block_store::advance_info_get(mcp::db::db_transaction & transaction_a)
{
	std::string value;
	bool exists(transaction_a.get(prop, mcp::uint256_to_slice(advance_info_key), value));
	mcp::advance_info result;
	if (exists)
		result = mcp::advance_info(value);
	return result;
}

void mcp::block_store::advance_info_put(mcp::db::db_transaction & transaction_a, mcp::advance_info const & value_a)
{
	transaction_a.put(prop, mcp::uint256_to_slice(advance_info_key), value_a.val());
}


uint64_t mcp::block_store::last_stable_index_get(mcp::db::db_transaction & transaction_a)
{
	std::string value;
	bool exists(transaction_a.get(prop, mcp::uint256_to_slice(last_stable_index_key), value));
	uint64_t result(0);
	if (exists)
		result = mcp::slice_to_uint64(value).number();
	return result;
}

void mcp::block_store::last_stable_index_put(mcp::db::db_transaction & transaction_a, uint64_t const & last_stable_index_a)
{
	mcp::uint64_union last_stable_index(last_stable_index_a);
	transaction_a.put(prop, mcp::uint256_to_slice(last_stable_index_key), mcp::uint64_to_slice(last_stable_index));
}


void mcp::block_store::block_children_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & p_hash_a, std::list<mcp::block_hash> & c_hashs_a)
{
	mcp::block_child_key key(p_hash_a, 0);
	mcp::db::forward_iterator it(transaction_a.begin(block_child, key.val()));
	while (true)
	{
		if (!it.valid())
			break;
		mcp::block_child_key child_key(it.key());
		if (child_key.hash != p_hash_a)
			break;

		c_hashs_a.push_back(child_key.child_hash);

		++it;
	}
}

mcp::db::forward_iterator mcp::block_store::block_child_begin(mcp::db::db_transaction & transaction_a, mcp::block_child_key const & key_a)
{
	mcp::db::forward_iterator result(transaction_a.begin(block_child, key_a.val()));
	return result;
}

void mcp::block_store::block_child_put(mcp::db::db_transaction & transaction_a, mcp::block_child_key const & key_a)
{
	transaction_a.put(block_child, key_a.val(), dev::Slice());
}


bool mcp::block_store::skiplist_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & hash_a, mcp::skiplist_info & skiplist_a)
{
	std::string value;
	bool exists(transaction_a.get(skiplist, mcp::uint256_to_slice(hash_a), value));
	if (exists)
	{
		dev::RLP r(value);
		skiplist_a = mcp::skiplist_info(r);
	}
	return !exists;
}

void mcp::block_store::skiplist_put(mcp::db::db_transaction & transaction_a, mcp::block_hash const & hash_a, mcp::skiplist_info const & skiplist_a)
{
	dev::bytes b_value;
	{
		dev::RLPStream s;
		skiplist_a.stream_RLP(s);
		s.swapOut(b_value);
	}
	dev::Slice s_value((char *)b_value.data(), b_value.size());
	transaction_a.put(skiplist, mcp::uint256_to_slice(hash_a), s_value);
}

bool mcp::block_store::successor_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & root_a, mcp::block_hash & successor_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	std::string value;
	bool exists(transaction_a.get(successor, mcp::uint256_to_slice(root_a), value, snapshot_a));
	if (exists)
	{
		successor_a = mcp::slice_to_uint256(value);
	}
	return !exists;
}

void mcp::block_store::successor_put(mcp::db::db_transaction & transaction_a, mcp::block_hash const & root_a, mcp::block_hash const & successor_a)
{
	transaction_a.put(successor, mcp::uint256_to_slice(root_a), mcp::uint256_to_slice(successor_a));
}

void mcp::block_store::successor_del(mcp::db::db_transaction & transaction_a, mcp::block_hash const & root_a)
{
	transaction_a.del(successor, mcp::uint256_to_slice(root_a));
}

bool mcp::block_store::genesis_hash_get(mcp::db::db_transaction & transaction_a, mcp::block_hash & genesis_hash)
{
	std::string value;
	bool exists(transaction_a.get(prop, mcp::uint256_to_slice(genesis_hash_key), value));
	if (exists)
	{
		genesis_hash = mcp::slice_to_uint256(value);
	}
	return !exists;
}

void mcp::block_store::genesis_hash_put(mcp::db::db_transaction & transaction_a, mcp::block_hash const & genesis_hash)
{
	transaction_a.put(prop, mcp::uint256_to_slice(genesis_hash_key), mcp::uint256_to_slice(genesis_hash));
}

// sync db

bool mcp::block_store::catchup_chain_summaries_get(mcp::db::db_transaction & transaction_a, uint64_t const & index_a, mcp::summary_hash & hash_a)
{
	std::string value;
	mcp::uint64_union index(index_a);
	bool exists(transaction_a.get(catchup_chain_summaries, mcp::uint64_to_slice(index), value));
	if (exists)
	{
		hash_a = mcp::slice_to_uint256(value);
	}
	return !exists;
}

void mcp::block_store::catchup_chain_summaries_put(mcp::db::db_transaction & transaction_a, uint64_t const & index_a, mcp::summary_hash const & hash_a)
{
	mcp::uint64_union index(index_a);
	transaction_a.put(catchup_chain_summaries, mcp::uint64_to_slice(index), mcp::uint256_to_slice(hash_a));
}

void mcp::block_store::catchup_chain_summaries_del(mcp::db::db_transaction & transaction_a, uint64_t const & index_a)
{
	mcp::uint64_union index(index_a);
	transaction_a.del(catchup_chain_summaries, mcp::uint64_to_slice(index));
}

void mcp::block_store::catchup_chain_summaries_clear(std::shared_ptr<rocksdb::WriteOptions> write_option_a)
{
	dev::Slicebytes start(8,0);
	dev::Slicebytes end(8, 0xFF);
	m_db->del_range(catchup_chain_summaries, dev::Slice(start.data(), start.size()), dev::Slice(end.data(), end.size()), write_option_a);
}

bool mcp::block_store::catchup_chain_block_summary_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a, mcp::summary_hash & summary_hash_a)
{
	std::string value;
	bool exists(transaction_a.get(catchup_chain_block_summary, mcp::uint256_to_slice(block_hash_a), value));
	if (exists)
	{
		summary_hash_a = mcp::slice_to_uint256(value);
	}
	return !exists;
}

void mcp::block_store::catchup_chain_block_summary_put(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a, mcp::summary_hash const & summary_hash_a)
{
	transaction_a.put(catchup_chain_block_summary, mcp::uint256_to_slice(block_hash_a), mcp::uint256_to_slice(summary_hash_a));
}

void mcp::block_store::catchup_chain_block_summary_del(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a)
{
	transaction_a.del(catchup_chain_block_summary, mcp::uint256_to_slice(block_hash_a));
}

void mcp::block_store::catchup_chain_block_summary_clear(std::shared_ptr<rocksdb::WriteOptions> write_option_a)
{
	mcp::summary_hash min(0);
	mcp::summary_hash max;
	max.bytes.fill(255);

	m_db->del_range(catchup_chain_block_summary, mcp::uint256_to_slice(min), mcp::uint256_to_slice(max), write_option_a);
}


bool mcp::block_store::catchup_chain_summary_block_get(mcp::db::db_transaction & transaction_a, mcp::summary_hash const & summary_hash_a, mcp::block_hash & block_hash_a)
{
	std::string value;
	bool exists(transaction_a.get(catchup_chain_summary_block, mcp::uint256_to_slice(summary_hash_a), value));
	if (exists)
	{
		block_hash_a = mcp::slice_to_uint256(value);
	}
	return !exists;
}

void mcp::block_store::catchup_chain_summary_block_put(mcp::db::db_transaction & transaction_a, mcp::summary_hash const & summary_hash_a, mcp::block_hash const & block_hash_a)
{
	transaction_a.put(catchup_chain_summary_block, mcp::uint256_to_slice(summary_hash_a), mcp::uint256_to_slice(block_hash_a));
}

void mcp::block_store::catchup_chain_summary_block_del(mcp::db::db_transaction & transaction_a, mcp::summary_hash const & summary_hash_a)
{
	transaction_a.del(catchup_chain_summary_block, mcp::uint256_to_slice(summary_hash_a));
}

void mcp::block_store::catchup_chain_summary_block_clear(std::shared_ptr<rocksdb::WriteOptions> write_option_a)
{
	mcp::summary_hash min(0);
	mcp::summary_hash max;
	max.bytes.fill(255);

	m_db->del_range(catchup_chain_summary_block, mcp::uint256_to_slice(min), mcp::uint256_to_slice(max), write_option_a);
}

bool mcp::block_store::hash_tree_summary_exists(mcp::db::db_transaction & transaction_a, mcp::summary_hash const & summary_a)
{
	std::string value;
	bool exists(transaction_a.get(hash_tree_summary, mcp::uint256_to_slice(summary_a), value));
	return exists;
}

void mcp::block_store::hash_tree_summary_put(mcp::db::db_transaction & transaction_a, mcp::summary_hash const & summary_a)
{
	transaction_a.put(hash_tree_summary, mcp::uint256_to_slice(summary_a), dev::Slice());
}

void mcp::block_store::hash_tree_summary_del(mcp::db::db_transaction & transaction_a, mcp::summary_hash const & summary_a)
{
	transaction_a.del(hash_tree_summary, mcp::uint256_to_slice(summary_a));
}

void mcp::block_store::hash_tree_summary_clear(std::shared_ptr<rocksdb::WriteOptions> write_option_a)
{
	mcp::summary_hash min(0);
	mcp::summary_hash max;
	max.bytes.fill(255);

	m_db->del_range(hash_tree_summary, mcp::uint256_to_slice(min), mcp::uint256_to_slice(max), write_option_a);
}

bool mcp::block_store::contract_main_trie_node_get(mcp::db::db_transaction & transaction_a, mcp::code_hash const & hash_a, std::string & value_a)
{
	bool exists(transaction_a.get(contract_main, mcp::uint256_to_slice(hash_a), value_a));
	return !exists;
}

void mcp::block_store::contract_main_trie_node_put(mcp::db::db_transaction & transaction_a, mcp::code_hash const & hash_a, std::string const & value_a)
{
	transaction_a.put(contract_main, mcp::uint256_to_slice(hash_a), dev::Slice(value_a));
}

bool mcp::block_store::contract_aux_state_key_get(mcp::db::db_transaction & transaction_a, dev::bytes const & key_a, dev::bytes & value_a)
{
	std::string value;
	bool exists(transaction_a.get(contract_aux, dev::Slice((char*)key_a.data(), key_a.size()), value));
	if (exists)
	{
		dev::RLP r(value);
		assert_x(r.itemCount() == 1)
		value_a = r[0].toBytes();
	}
	return !exists;
}

void mcp::block_store::contract_aux_state_key_put(mcp::db::db_transaction & transaction_a, dev::bytes const & key_a, dev::bytes const & value_a)
{
	dev::bytes b_value;
	{
		dev::RLPStream s;
		s.appendList(1);
		s << value_a;
		s.swapOut(b_value);
	}

	dev::Slice s_value((char *)b_value.data(), b_value.size());
	transaction_a.put(contract_aux, dev::Slice((char*)key_a.data(), key_a.size()), s_value);
}

bool mcp::block_store::catchup_index_get(mcp::db::db_transaction & transaction_a, uint64_t & _v)
{
	std::string value;
	bool exists(transaction_a.get(prop, mcp::uint256_to_slice(catchup_index), value));
	if (exists)
	{
		_v = mcp::slice_to_uint64(value).number();
	}
	return !exists;
}

void mcp::block_store::catchup_index_put(mcp::db::db_transaction & transaction_a, uint64_t const& _v)
{
	mcp::uint64_union v(_v);
	transaction_a.put(prop, mcp::uint256_to_slice(catchup_index), mcp::uint64_to_slice(v));
}

void mcp::block_store::catchup_index_del(mcp::db::db_transaction & transaction_a)
{
	transaction_a.del(prop, mcp::uint256_to_slice(catchup_index));
}

bool mcp::block_store::catchup_max_index_get(mcp::db::db_transaction & transaction_a, uint64_t & _v)
{
	std::string value;
	bool exists(transaction_a.get(prop, mcp::uint256_to_slice(catchup_max_index), value));
	if (exists)
	{
		_v = mcp::slice_to_uint64(value).number();
	}
	return !exists;
}

void mcp::block_store::catchup_max_index_put(mcp::db::db_transaction & transaction_a, uint64_t const& _v)
{
	mcp::uint64_union v(_v);
	transaction_a.put(prop, mcp::uint256_to_slice(catchup_max_index), mcp::uint64_to_slice(v));
}

void mcp::block_store::catchup_max_index_del(mcp::db::db_transaction & transaction_a)
{
	transaction_a.del(prop, mcp::uint256_to_slice(catchup_max_index));
}

std::shared_ptr<mcp::unlink_block> mcp::block_store::unlink_block_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & hash_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	std::string value;
	bool exists(transaction_a.get(unlink_block, mcp::uint256_to_slice(hash_a), value, snapshot_a));
	std::shared_ptr<mcp::unlink_block> result = nullptr;
	if (exists)
	{
		dev::RLP r(value);
		bool error(false);
		result = std::make_shared<mcp::unlink_block>(error, r);
		assert_x(!error);
		assert_x(result != nullptr);
	}
	return result;
}

bool mcp::block_store::unlink_block_exists(mcp::db::db_transaction & transaction_a, mcp::block_hash const & hash_a)
{
	std::string result;
	bool exists(transaction_a.get(unlink_block, mcp::uint256_to_slice(hash_a), result));
	return exists;
}

void mcp::block_store::unlink_block_put(mcp::db::db_transaction & transaction_a, mcp::block_hash const & hash_a, mcp::unlink_block const & block_a)
{
	dev::bytes b_value;
	{
		dev::RLPStream s;
		block_a.stream_RLP(s);
		s.swapOut(b_value);
	}

	bool exist = unlink_block_exists(transaction_a, hash_a);

	dev::Slice s_value((char *)b_value.data(), b_value.size());
	transaction_a.put(unlink_block, mcp::uint256_to_slice(hash_a), s_value);

	if (!exist)
		transaction_a.count_add("unlink_block", 1);
}

void mcp::block_store::unlink_block_del(mcp::db::db_transaction & transaction_a, mcp::block_hash const & hash_a)
{
	bool exist = unlink_block_exists(transaction_a, hash_a);

	if (exist)
	{
		transaction_a.del(unlink_block, mcp::uint256_to_slice(hash_a));
		transaction_a.count_reduce("unlink_block", 1);
	}
}

size_t mcp::block_store::unlink_block_count(mcp::db::db_transaction & transaction_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	return transaction_a.count_get("unlink_block", snapshot_a);
}

std::shared_ptr<mcp::unlink_info> mcp::block_store::unlink_info_get(mcp::db::db_transaction & transaction_a, mcp::account const & account_a)
{
	std::string value;
	bool exists(transaction_a.get(unlink_info, mcp::uint256_to_slice(account_a), value));
	std::shared_ptr<mcp::unlink_info> result = nullptr;
	if (exists)
	{
		result = std::make_shared<mcp::unlink_info>(value);
		assert_x(result != nullptr);
	}

	return result;
}

bool mcp::block_store::unlink_info_exists(mcp::db::db_transaction & transaction_a, mcp::account const & account_a)
{
	std::string result;
	bool exists(transaction_a.get(unlink_info, mcp::uint256_to_slice(account_a), result));
	return exists;
}


bool unlink_sametime = true;

void mcp::block_store::unlink_info_put(mcp::db::db_transaction & transaction_a, mcp::account const & account_a, mcp::unlink_info const & unlink_a)
{
	if (!unlink_sametime)
		assert_x(false);

	unlink_sametime = false;
	bool exists = false;
	if (unlink_info_exists(transaction_a, account_a))
		exists = true;

	transaction_a.put(unlink_info, mcp::uint256_to_slice(account_a), unlink_a.val());
	if (!exists)
	{
		transaction_a.count_add("unlink_info", 1);
	}
	unlink_sametime = true;
}

void mcp::block_store::unlink_info_del(mcp::db::db_transaction & transaction_a, mcp::account const & account_a)
{
	if (!unlink_sametime)
		assert_x(false);

	unlink_sametime = false;
	if (unlink_info_exists(transaction_a, account_a))
	{
		transaction_a.del(unlink_info, mcp::uint256_to_slice(account_a));
		transaction_a.count_reduce("unlink_info", 1);
	}
	unlink_sametime = true;
}

mcp::db::forward_iterator mcp::block_store::unlink_info_begin(mcp::db::db_transaction & transaction_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	mcp::db::forward_iterator result(transaction_a.begin(unlink_info, snapshot_a));
	return result;
}

mcp::db::forward_iterator mcp::block_store::unlink_info_begin(mcp::db::db_transaction & transaction_a, mcp::account const & account_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	mcp::db::forward_iterator result(transaction_a.begin(unlink_info, mcp::uint256_to_slice(account_a), snapshot_a));
	return result;
}

size_t mcp::block_store::unlink_info_count(mcp::db::db_transaction & transaction_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	return transaction_a.count_get("unlink_info", snapshot_a);
}

bool mcp::block_store::next_unlink_exists(mcp::db::db_transaction & transaction_a, mcp::next_unlink const & next_unlink_a)
{
	std::string result;
	bool exists(transaction_a.get(next_unlink, next_unlink_a.val(), result));
	return exists;
}

void mcp::block_store::next_unlink_put(mcp::db::db_transaction & transaction_a, mcp::next_unlink const & next_unlink_a)
{
	mcp::block_hash const & root(next_unlink_a.hash);
	std::string const & root_key(std::string((char*)root.bytes.data(), root.bytes.size()));
	std::string const & root_index_key(root_key + "idx");
	transaction_a.count_add(root_key, 1);
	transaction_a.count_add(root_index_key, 1);

	uint64_t index(transaction_a.count_get(root_index_key));
	mcp::uint64_union index_union(index);
	dev::Slice index_slice(mcp::uint64_to_slice(index_union));

	transaction_a.put(next_unlink, next_unlink_a.val(), index_slice);

	dev::Slicebytes next_index_key;
	next_index_key.resize(root.bytes.size() + index_union.bytes.size());
	dev::SliceRef next_index_key_ref(&next_index_key);

	dev::Slice root_slice(mcp::uint256_to_slice(root));
	root_slice.copyTo(next_index_key_ref.cropped(0, root.bytes.size()));
	index_slice.copyTo(next_index_key_ref.cropped(root.bytes.size(), index_union.bytes.size()));

	transaction_a.put(next_unlink_index, dev::Slice(next_index_key.data(), next_index_key.size()), mcp::uint256_to_slice(next_unlink_a.next));
}

void mcp::block_store::next_unlink_del(mcp::db::db_transaction & transaction_a, mcp::next_unlink const & next_unlink_a)
{
	std::string value;
	bool exists(transaction_a.get(next_unlink, next_unlink_a.val(), value));
	if (exists)
	{
		mcp::block_hash const & root(next_unlink_a.hash);
		std::string const & root_key(std::string((char*)root.bytes.data(), root.bytes.size()));
		transaction_a.count_reduce(root_key, 1);

		transaction_a.del(next_unlink, next_unlink_a.val());

		mcp::uint64_union index_union(mcp::slice_to_uint64(value));
		dev::Slice index_slice(mcp::uint64_to_slice(index_union));

		dev::Slicebytes next_index_key;
		next_index_key.resize(root.bytes.size() + index_union.bytes.size());
		dev::SliceRef next_index_key_ref(&next_index_key);

		dev::Slice root_slice(mcp::uint256_to_slice(root));
		root_slice.copyTo(next_index_key_ref.cropped(0, root.bytes.size()));
		index_slice.copyTo(next_index_key_ref.cropped(root.bytes.size(), index_union.bytes.size()));

		transaction_a.del(next_unlink_index, dev::Slice(next_index_key.data(), next_index_key.size()));

		uint64_t count(transaction_a.count_get(root_key));
		if (count == 0)
		{
			transaction_a.count_del(root_key);
			std::string const & root_index_key(root_key + "idx");
			transaction_a.count_del(root_index_key);
		}
	}
}

void mcp::block_store::next_unlink_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & hash_a, std::list<mcp::next_unlink>& li)
{
	mcp::block_hash const & root(hash_a);
	std::string const & root_key(std::string((char*)root.bytes.data(), root.bytes.size()));
	std::string const & root_index_key(root_key + "idx");
	uint64_t index(transaction_a.count_get(root_index_key));

	for (uint64_t i = 1; i <= index; i++)
	{
		mcp::uint64_union index_union(i);
		dev::Slice index_slice(mcp::uint64_to_slice(index_union));

		dev::Slicebytes next_index_key;
		next_index_key.resize(root.bytes.size() + index_union.bytes.size());
		dev::SliceRef next_index_key_ref(&next_index_key);

		dev::Slice root_slice(mcp::uint256_to_slice(root));
		root_slice.copyTo(next_index_key_ref.cropped(0, root.bytes.size()));
		index_slice.copyTo(next_index_key_ref.cropped(root.bytes.size(), index_union.bytes.size()));

		std::string value;
		bool exists(transaction_a.get(next_unlink_index, dev::Slice(next_index_key.data(), next_index_key.size()), value));
		if (exists) //may not exist
		{
			mcp::block_hash next(mcp::slice_to_uint256(value));
			li.push_back(mcp::next_unlink(hash_a,next));
		}
	}
}

bool mcp::block_store::head_unlink_exists(mcp::db::db_transaction & transaction_a, mcp::head_unlink const & head_unlink_a)
{
	std::string result;
	bool exists(transaction_a.get(head_unlink, head_unlink_a.val(), result));
	return exists;
}

void mcp::block_store::head_unlink_put(mcp::db::db_transaction & transaction_a, mcp::head_unlink const & head_unlink_a)
{
	bool exist = head_unlink_exists(transaction_a, head_unlink_a);
	std::string _v = "";
	transaction_a.put(head_unlink, head_unlink_a.val(), _v);
	if (!exist)
		transaction_a.count_add("head_unlink", 1);
}

void mcp::block_store::head_unlink_del(mcp::db::db_transaction & transaction_a, mcp::head_unlink const & head_unlink_a)
{
	bool exist = head_unlink_exists(transaction_a, head_unlink_a);
	if (exist)
	{
		transaction_a.del(head_unlink, head_unlink_a.val());
		transaction_a.count_reduce("head_unlink", 1);
	}
}

mcp::db::forward_iterator mcp::block_store::head_link_begin(mcp::db::db_transaction & transaction_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	mcp::db::forward_iterator result(transaction_a.begin(head_unlink, snapshot_a));
	return result;
}

size_t mcp::block_store::head_unlink_count(mcp::db::db_transaction & transaction_a, std::shared_ptr<rocksdb::ManagedSnapshot> snapshot_a)
{
	return transaction_a.count_get("head_unlink", snapshot_a);
}

bool mcp::block_store::traces_get(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a, std::list<std::shared_ptr<mcp::trace>> & traces_a)
{
	std::string value;
	bool exists(transaction_a.get(traces, mcp::uint256_to_slice(block_hash_a), value));
	if (exists)
	{
		dev::RLP r_list(value);
		bool error(false);
		for (dev::RLP r : r_list)
		{
			std::shared_ptr<mcp::trace> trace(std::make_shared<mcp::trace>(error, r));
			assert_x(!error);
			traces_a.push_back(trace);
		}
	}
	return !exists;
}

void mcp::block_store::traces_put(mcp::db::db_transaction & transaction_a, mcp::block_hash const & block_hash_a, std::list<std::shared_ptr<mcp::trace>> const & traces_a)
{
	dev::bytes b_value;
	{
		dev::RLPStream s;
		s.appendList(traces_a.size());
		for (std::shared_ptr<mcp::trace> trace : traces_a)
		{
			trace->stream_RLP(s);
		}
		s.swapOut(b_value);
	}

	dev::Slice s_value((char *)b_value.data(), b_value.size());
	transaction_a.put(traces, mcp::uint256_to_slice(block_hash_a), s_value);
}


mcp::uint256_union const mcp::block_store::version_key(0);
mcp::uint256_union const mcp::block_store::genesis_hash_key(1);
mcp::uint256_union const mcp::block_store::last_mci_key(2);
mcp::uint256_union const mcp::block_store::last_stable_mci_key(3);
mcp::uint256_union const mcp::block_store::advance_info_key(4);
mcp::uint256_union const mcp::block_store::last_stable_index_key(5);
mcp::uint256_union const mcp::block_store::catchup_index(6);
mcp::uint256_union const mcp::block_store::catchup_max_index(7);