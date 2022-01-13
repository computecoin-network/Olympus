#pragma once

#include <mcp/common/EVMSchedule.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/TrieCommon.h>

#include <blake2/blake2.h>
#include <mcp/core/log_entry.hpp>
#include <mcp/core/blocks.hpp>
#include <mcp/core/overlay_db.hpp>

#include <unordered_map>
#include <set>


using namespace dev;
namespace mcp
{
	class dag_account_info
	{
	public:
		dag_account_info() = default;
		dag_account_info(bool & error_a, dev::RLP const & r);
		void stream_RLP(dev::RLPStream & s) const;

		mcp::block_hash latest_stable_block;
	};

	/**
	* Latest information about an account
	*/
	class account_info
	{
	public:
		account_info() :latest_stable_block(0), latest_linked(0){};
        account_info(bool & error_a, dev::RLP const & r);
        void stream_RLP(dev::RLPStream & s) const;

		mcp::block_hash latest_stable_block;
		mcp::block_hash latest_linked;
	};

	class free_key
	{
	public:
		free_key(uint64_t const &, uint64_t const &, mcp::block_hash const &);
		free_key(dev::Slice const & val_a);
		bool operator== (mcp::free_key const &) const;
		void serialize(mcp::stream & stream_a) const;
		void deserialize(mcp::stream & stream_a);
		uint64_t witnessed_level_desc;
		uint64_t level_desc;
		mcp::block_hash hash_asc;
	};

	enum class TransactionException
	{
		None = 0,
		Unknown,
		BadRLP,
		InvalidFormat,
		OutOfGasIntrinsic,		///< Too little gas to pay for the base transaction cost.
		InvalidSignature,
		InvalidNonce,
		NotEnoughCash,
		OutOfGasBase,			///< Too little gas to pay for the base transaction cost.
		BlockGasLimitReached,
		BadInstruction,
		BadJumpDestination,
		OutOfGas,				///< Ran out of gas executing code of the transaction.
		OutOfStack,				///< Ran out of stack executing code of the transaction.
		StackUnderflow,
		RevertInstruction,
		InvalidZeroSignatureFormat,
		AddressAlreadyUsed
	};
	TransactionException toTransactionException(Exception const& _e);

	std::string to_transaction_exception_messge(mcp::TransactionException const & exception_a);

	enum class CodeDeposit
	{
		None = 0,
		Failed,
		Success
	};

	struct ExecutionResult
	{
		u256 gasUsed = 0;
		TransactionException excepted = TransactionException::Unknown;
		mcp::account newAddress;
		dev::bytes output;
		CodeDeposit codeDeposit = CodeDeposit::None;		///< Failed if an attempted deposit failed due to lack of gas.
		u256 gasRefunded = 0;
		unsigned depositSize = 0; 							///< Amount of code of the creation's attempted deposit.
		u256 gasForDeposit;			 						///< Amount of gas remaining for the code deposit phase.
		std::set<mcp::account> modified_accounts;			///< The accounts that have been modified by the transaction.
	};

	enum class Permanence
	{
    	Reverted,
    	Committed,
    	Uncommitted  ///< Uncommitted state for change log readings in tests.
	};    

	enum class block_status : uint8_t
    {
		unknown = 255,
        ok = 0,
        fork = 1,
        invalid = 2,
        fail = 3,
    };

	class block_child_key
	{
	public:
		block_child_key(mcp::block_hash const &, mcp::block_hash const &);
		block_child_key(dev::Slice const &);
		bool operator== (mcp::block_child_key const &) const;
		dev::Slice val() const;
		mcp::block_hash hash;
		mcp::block_hash child_hash;
	};

	class hash_tree_info
	{
	public:
		hash_tree_info();
		hash_tree_info(mcp::block_hash const &, mcp::summary_hash const &);
		hash_tree_info(dev::Slice const &);
		dev::Slice val() const;
		mcp::block_hash b_hash;
		mcp::summary_hash s_hash;
	};

	class Executive;
	class account_state
	{
	public:
		/// Changedness of account to create.
    	enum Changedness
    	{
        	/// Account starts as though it has been changed.
        	Changed,
        	/// Account starts as though it has not been changed.
        	Unchanged
    	};
	
		account_state();
		account_state(mcp::account const & account_a, mcp::block_hash const & block_hash_a, mcp::account_state_hash const & previous_a, u256 nonce_a, mcp::amount const & balance_a, Changedness _c = Changed);
		account_state(mcp::account const & account_a, mcp::block_hash const & block_hash_a, mcp::account_state_hash const & previous_a, u256 nonce_a, mcp::amount const & balance_a, h256 storageRoot_a, h256 codeHash_a, Changedness _c);
		account_state(bool & error_a, dev::RLP const & r, Changedness _c = Unchanged);
		void stream_RLP(dev::RLPStream & s) const;
		mcp::account_state_hash hash();
		mcp::account account;
		mcp::block_hash block_hash;
		mcp::account_state_hash previous;
		mcp::amount balance;

		mcp::account_state_hash init_hash = 0;

		void record_init_hash();

		bool has_code() const;

		/// Kill this account. Useful for the suicide opcode. Following this call, isAlive() returns
    	/// false.
    	void kill()
    	{
        	m_isAlive = false;
        	m_storageOverlay.clear();
        	m_storageOriginal.clear();
	    	m_codeHash = dev::EmptySHA3;
	    	m_storageRoot = dev::EmptyTrie;
        	balance = 0;
            changed();
    	}

		/// @returns true iff this object represents an account in the state. Returns false if this object
    	/// represents an account that should no longer exist in the trie (an account that never existed or was
    	/// suicided).
    	bool isAlive() const { return m_isAlive; }

		/// @returns true if the account is unchanged from creation.
    	bool isDirty() const { return !m_isUnchanged; }

		void untouch() { m_isUnchanged = true; }

		/// @returns true if the balance and code is zero / empty. Code is considered empty
    	/// during creation phase.
    	bool isEmpty() const { return balance == 0 && codeHash() == EmptySHA3; }

		/// @returns the nonce of the account.
		u256 nonce() const { return m_nonce; }

		/// Increment the nonce of the account by one.
		void incNonce() { ++m_nonce; changed(); }

		/// Set nonce to a new value. This is used when reverting changes made to
		/// the account.
		void setNonce(u256 const& _nonce) { m_nonce = _nonce; }

		/// @returns the root of the trie (whose nodes are stored in the state db externally to this class)
    	/// which encodes the base-state of the account's storage (upon which the storage is overlaid).
    	h256 baseRoot() const { assert_x(m_storageRoot); return m_storageRoot; }

		bool hasNewCode() const { return m_hasNewCode; }

		void addBalance(mcp::amount const& _amount);

		h256 codeHash() const { return m_codeHash; }

		size_t codeSize(mcp::account const& _contract) const;

		/// @returns account's storage value corresponding to the @_key
    	/// taking into account overlayed modifications
		mcp::uint256_t storageValue(mcp::uint256_t const& _key, mcp::overlay_db const& _db) const
		{
			auto mit = m_storageOverlay.find(_key);
			if (mit != m_storageOverlay.end())
				return mit->second;

			return originalStorageValue(_key, _db);
		}

		/// @returns account's original storage value corresponding to the @_key
		/// not taking into account overlayed modifications
		mcp::uint256_t originalStorageValue(mcp::uint256_t const& _key, mcp::overlay_db const& _db) const;

		/// @returns the storage overlay as a simple hash map.
		std::unordered_map<mcp::uint256_t, mcp::uint256_t> const& storageOverlay() const { return m_storageOverlay; }

		/// Set a key/value pair in the account's storage. This actually goes into the overlay, for committing
    	/// to the trie later.
    	void setStorage(uint256_t _p, uint256_t _v) { m_storageOverlay[_p] = _v; changed(); }

		/// Empty the storage.  Used when a contract is overwritten.
    	void clearStorage()
    	{
        	m_storageOverlay.clear();
        	m_storageOriginal.clear();
        	m_storageRoot = dev::EmptyTrie;
        	changed();
    	}

		/// Set the storage root.  Used when clearStorage() is reverted.
    	void setStorageRoot(uint256_t const& _root)
    	{
        	m_storageOverlay.clear();
        	m_storageOriginal.clear();
        	m_storageRoot = _root;
        	changed();
    	}

		/// Sets the code of the account. Used by "create" messages.
		void setCode(dev::bytes&& _code);

		/// Specify to the object what the actual code is for the account. @a _code must have a SHA3
		/// equal to codeHash().
		void noteCode(bytesConstRef _code) { assert_x(sha3(_code) == m_codeHash); m_codeCache = _code.toBytes(); }

		/// @returns the account's code.
    	dev::bytes const& code() const { return m_codeCache; }

		//clear temp state to make it just like the state get from db
		void clear_temp_state()
		{
			m_isUnchanged = true;
			m_hasNewCode = false;
			m_storageOverlay.clear();
			m_storageOriginal.clear();
			m_codeCache.clear();
		}

	private:
		/// Note that we've altered the account.
    	void changed() { m_isUnchanged = false; }

		/// Is this account existant? If not, it represents a deleted account.
    	bool m_isAlive = false;

    	/// True if we've not made any alteration to the account having been given it's properties directly.
    	bool m_isUnchanged = false;

		/// True if new code was deployed to the account
    	bool m_hasNewCode = false;

		/// Account's nonce.
		u256 m_nonce;

		/// The base storage root. Used with the state DB to give a base to the storage. m_storageOverlay is
    	/// overlaid on this and takes precedence for all values set.
    	h256 m_storageRoot = EmptyTrie;

		/** If c_contractConceptionCodeHash then we're in the limbo where we're running the initialisation code.
     	* We expect a setCode() at some point later.
     	* If EmptySHA3, then m_code, which should be empty, is valid.
     	* If anything else, then m_code is valid iff it's not empty, otherwise, State::ensureCached() needs to
     	* be called with the correct args.
     	*/
    	h256 m_codeHash = EmptySHA3;
		
		/// The map with is overlaid onto whatever storage is implied by the m_storageRoot in the trie.
		mutable std::unordered_map<u256, u256> m_storageOverlay;

		/// The cache of unmodifed storage items
    	mutable std::unordered_map<u256, u256> m_storageOriginal;

    	/// The associated code for this account. The SHA3 of this should be equal to m_codeHash unless
    	/// m_codeHash equals c_contractConceptionCodeHash.
    	dev::bytes m_codeCache;

    	/// Value for m_codeHash when this account is having its code determined.
    	static const u256 c_contractConceptionCodeHash;
	};

	using AccountMap = std::unordered_map<mcp::account, std::shared_ptr<mcp::account_state>>;
	// using AccountMaskMap = std::unordered_map<Address, AccountMask>;

	class transaction_receipt
	{
	public:
		transaction_receipt(account_state_hash from_state_a, std::set<account_state_hash> to_state_a,
			u256 gas_used_a, log_entries log_a);
        transaction_receipt(bool & error_a, dev::RLP const & r);
        void stream_RLP(dev::RLPStream & s) const;
		void serialize_json(mcp::json & json_a);
		void hash(blake2b_state &) const;
		bool contains_bloom(dev::h256 const & h_a);
		static void serialize_null_json(mcp::json & json_a);

		mcp::account_state_hash from_state;
		std::set<mcp::account_state_hash> to_state;
		u256 gas_used;
		log_bloom bloom;
		log_entries log;
	};

	class block_state
	{
	public:
		block_state();
        block_state(bool & error_a, dev::RLP const & r);
        void stream_RLP(dev::RLPStream & s) const;
		void serialize_json(mcp::json & json_a, mcp::account const & contract_account_a);

		mcp::block_type block_type;
        mcp::block_status status;
		bool is_stable;
		uint64_t stable_index;
		boost::optional<uint64_t> main_chain_index;
		uint64_t level;
		uint64_t mc_timestamp;
		uint64_t stable_timestamp;

		//dag
		bool is_free;
		bool is_on_main_chain;
		boost::optional<uint64_t> earliest_included_mc_index;
		boost::optional<uint64_t> latest_included_mc_index;
		boost::optional<uint64_t> bp_included_mc_index;
		boost::optional<uint64_t> earliest_bp_included_mc_index;
		boost::optional<uint64_t> latest_bp_included_mc_index;
		uint64_t witnessed_level;
		mcp::block_hash best_parent;

		//light
		boost::optional<transaction_receipt> receipt;
	};

	class skiplist_info
	{
	public:
		skiplist_info();
		skiplist_info(std::set<mcp::block_hash> const &);
		skiplist_info(dev::RLP const & r);
		void stream_RLP(dev::RLPStream & s) const;

		std::set<mcp::block_hash> list;
	};

	class summary
	{
	public:
        static mcp::summary_hash gen_summary_hash(mcp::block_hash const & block_hash, mcp::summary_hash const & previous_hash,
            std::list<mcp::summary_hash> const & parent_hashs, std::list<mcp::summary_hash> const & link_hashs,
            std::set<mcp::summary_hash> const & skip_list, mcp::block_status const & status, uint64_t const& stable_index_a, uint64_t const& mc_timestamp_a,
            boost::optional<transaction_receipt> receipt);
	};

	class fork_successor_key
	{
	public:
		fork_successor_key(mcp::block_hash const & previous_a, mcp::block_hash const & successor_a);
		fork_successor_key(dev::Slice const & val_a);
		dev::Slice val() const;

		mcp::block_hash previous;
		mcp::block_hash successor;
	};

	class advance_info
	{
	public:
		advance_info();
		advance_info(uint64_t const & mci_a, mcp::block_hash const & witness_block_a);
		advance_info(dev::Slice const & val_a);
		dev::Slice val() const;

		uint64_union mci;
		mcp::block_hash witness_block;
	};

	class min_wl_result
	{
	public:
		uint64_t min_wl;

		// Updated to 512, Daniel
		std::unordered_set<mcp::account> witnesses;
	};

	class unlink_info
	{
	public:
		unlink_info() :earliest_unlink(0), latest_unlink(0) {}
		unlink_info(dev::Slice const& val_a);
		dev::Slice val() const;
	
		mcp::block_hash earliest_unlink;
		mcp::block_hash latest_unlink;
	};

	class next_unlink
	{
	public:
		next_unlink() :hash(0), next(0) {}
		next_unlink(mcp::block_hash const& hash_a, mcp::block_hash const& next_a) :hash(hash_a), next(next_a) {}
		next_unlink(dev::Slice const& val_a);
		dev::Slice val() const;

		mcp::block_hash hash;
		mcp::block_hash next;
	};

	class head_unlink
	{
	public:
		head_unlink() :hash(0), time(0) {}
		head_unlink(mcp::uint64_union const& time_a, mcp::block_hash const& hash_a) :time(time_a), hash(hash_a){}
		head_unlink(dev::Slice const& val_a);
		dev::Slice val() const;

		mcp::uint64_union time;
		mcp::block_hash hash;
	};

	class unlink_block
	{
	public:
		unlink_block() :time(0), block(nullptr){}
		unlink_block(mcp::uint64_union const& time_a, std::shared_ptr<mcp::block> const& block_a) :time(time_a), block(block_a) {}
		unlink_block(bool & error_a, dev::RLP const & r);
		void stream_RLP(dev::RLPStream & s) const;

		std::shared_ptr<mcp::block> block;
		mcp::uint64_union time;
	};

	//trace
	class trace_action
	{
	public:
		virtual void stream_RLP(dev::RLPStream & s) const = 0;
		virtual void serialize_json(mcp::json & json_a) const = 0;
	};

	class trace_result
	{
	public:
		virtual void stream_RLP(dev::RLPStream & s) const = 0;
		virtual void serialize_json(mcp::json & json_a) const = 0;
	};

	class call_trace_action : public trace_action
	{
	public:
		call_trace_action() = default;
		call_trace_action(bool & error_a, dev::RLP const & r);
		void stream_RLP(dev::RLPStream & s) const;
		void serialize_json(mcp::json & json_a) const;

		std::string call_type;
		mcp::account from;
		mcp::uint256_t gas;
		bytes data;
		mcp::account to;
		mcp::uint256_t amount;
	};

	class call_trace_result : public trace_result
	{
	public:
		call_trace_result() = default;
		call_trace_result(bool & error_a, dev::RLP const & r);
		void stream_RLP(dev::RLPStream & s) const;
		void serialize_json(mcp::json & json_a) const;

		mcp::uint256_t gas_used;
		dev::bytes output;
	};

	class create_trace_action : public trace_action
	{
	public:
		create_trace_action() = default;
		create_trace_action(bool & error_a, dev::RLP const & r);
		void stream_RLP(dev::RLPStream & s) const;
		void serialize_json(mcp::json & json_a) const;

		mcp::account from;
		mcp::uint256_t gas;
		dev::bytes init;
		mcp::uint256_t amount;
	};

	class create_trace_result : public trace_result
	{
	public:
		create_trace_result() = default;
		create_trace_result(bool & error_a, dev::RLP const & r);
		void stream_RLP(dev::RLPStream & s) const;
		void serialize_json(mcp::json & json_a) const;

		mcp::uint256_t gas_used;
		mcp::account contract_account;
		dev::bytes code;
	};

	class suicide_trace_action : public trace_action
	{
	public:
		suicide_trace_action() = default;
		suicide_trace_action(bool & error_a, dev::RLP const & r);
		void stream_RLP(dev::RLPStream & s) const;
		void serialize_json(mcp::json & json_a) const;

		mcp::account contract_account;
		mcp::account refund_account;
		mcp::uint256_t balance;
	};

	enum class trace_type : uint8_t
	{
		call = 0,
		create = 1,
		suicide = 2
	};

	class trace
	{
	public:
		trace() = default;
		trace(bool & error_a, dev::RLP const & r);
		void stream_RLP(dev::RLPStream & s) const;
		void serialize_json(mcp::json & json_a) const;

		mcp::trace_type type;
		std::shared_ptr<mcp::trace_action> action;
		std::string error_message;
		std::shared_ptr<mcp::trace_result> result;
		uint32_t depth;
	};


	dev::Slice uint64_to_slice(mcp::uint64_union const & value);
	mcp::uint64_union slice_to_uint64(dev::Slice const & slice);

	dev::Slice uint256_to_slice(mcp::uint256_union const & value);
	mcp::uint256_union slice_to_uint256(dev::Slice const & slice);

	// Added by Daniel
	dev::Slice uint512_to_slice(mcp::uint512_union const & value);
	mcp::uint512_union slice_to_uint512(dev::Slice const & slice);

	mcp::account toAddress(mcp::account const& _from, u256 const& _nonce);

	// OS-specific way of finding a path to a home directory.
	boost::filesystem::path working_path();
	// Get a unique path within the home directory, used for testing
	boost::filesystem::path unique_path();
}
