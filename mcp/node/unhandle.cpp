#include "unhandle.hpp"
#include "arrival.hpp"
#include <queue>
mcp::unhandle_item::unhandle_item(
	mcp::block_hash const &unhandle_hash_a,
	std::shared_ptr<mcp::block_processor_item> item_a,
	std::unordered_set<mcp::block_hash> const &dependency_hashs_a,
	h256Hash const &transactions_a,
	h256Hash const &approves_a
):
	unhandle_hash(unhandle_hash_a),
	item(item_a),
	dependency_hashs(dependency_hashs_a),
	transactions(transactions_a),
	approves(approves_a)
{
}

mcp::unhandle_cache::unhandle_cache(std::shared_ptr<TransactionQueue> tq, std::shared_ptr<ApproveQueue> aq, size_t const &capacity_a) :
	m_tq(tq),
	m_aq(aq),
	m_capacity(capacity_a)
{
}

/// hash_a must be block hash
mcp::unhandle_add_result mcp::unhandle_cache::add(mcp::block_hash const &hash_a,
	std::unordered_set<mcp::block_hash> const &dependency_hashs_a, 
	h256Hash const &transactions,
	h256Hash const &approves,
	std::shared_ptr<mcp::block_processor_item> item_a)
{
	std::lock_guard<std::mutex> lock (m_mutux);

	assert_x(!dependency_hashs_a.count(hash_a));

	/// Determine if a transaction exists
	/// validation maybe missing,but transaction maybe arrived before add unhandle.
	h256Hash trs;
	for (h256 const &h : transactions)
	{
		if (!m_tq->exist(h))
			trs.insert(h);
	}
	/// Determine if a approve exists
	/// validation maybe missing,but approve maybe arrived before add unhandle.
	h256Hash aps;
	for (h256 const &h : approves)
	{
		if (!m_aq->exist(h))
			aps.insert(h);
	}
	if (dependency_hashs_a.empty() && trs.empty() && aps.empty())
		return unhandle_add_result::Retry;

    bool exists(m_unhandles.count(hash_a));
    if (exists)
    {
		unhandle_exist_count++;
        return unhandle_add_result::Exist;
    }

	/// only accept unknown dependencies when size reach at half of capacity
	if (m_unhandles.size() >= m_capacity / 2)
	{
		unhandle_full_count++;
        return unhandle_add_result::Exist;
	}
	
	add_unhandle_ok_count++;

	mcp::unhandle_item u_item(hash_a, item_a, dependency_hashs_a, trs, aps);
	m_unhandles[hash_a] = u_item;

	/// add tip
	if (!m_dependencies.count(hash_a))
		m_tips.insert(hash_a);

	bool allExist = true;
	for (mcp::block_hash const &dependency_hash : u_item.dependency_hashs)
	{
		std::shared_ptr<std::unordered_set<mcp::block_hash>> unhandle_hashs;
		auto const it = m_dependencies.find(dependency_hash);
		if (it != m_dependencies.end())
		{
			unhandle_hashs = it->second;
		}
		else
		{
			unhandle_hashs = std::make_shared<std::unordered_set<mcp::block_hash>>();
			m_dependencies.insert(std::make_pair(dependency_hash, unhandle_hashs));
		}
		unhandle_hashs->insert(hash_a);

		/// add unknown dependency
		if (!m_unhandles.count(dependency_hash))
		{
			m_missings.insert(dependency_hash);
			allExist = false;
		}

		/// delete tip
		if (m_tips.count(dependency_hash))
		{
			m_tips.erase(dependency_hash);
		}
	}

	for (h256 const &h : u_item.transactions)
	{
		std::shared_ptr<std::unordered_set<mcp::block_hash>> unhandle_hashs;
		auto const it = m_transactions.find(h);
		if (it != m_transactions.end())
		{
			unhandle_hashs = it->second;
		}
		else
		{
			unhandle_hashs = std::make_shared<std::unordered_set<mcp::block_hash>>();
			m_transactions.insert(std::make_pair(h, unhandle_hashs));
		}
		unhandle_hashs->insert(hash_a);

		/// add unknown dependency
		m_light_missings.insert(h);/// maybe failed because exist,unimportance.
	}

	for (h256 const &h : u_item.approves)
	{
		std::shared_ptr<std::unordered_set<mcp::block_hash>> unhandle_hashs;
		auto const it = m_approves.find(h);
		if (it != m_approves.end())
		{
			unhandle_hashs = it->second;
		}
		else
		{
			unhandle_hashs = std::make_shared<std::unordered_set<mcp::block_hash>>();
			m_approves.insert(std::make_pair(h, unhandle_hashs));
		}
		unhandle_hashs->insert(hash_a);

		/// add unknown dependency
		m_approve_missings.insert(h);/// maybe failed because exist,unimportance.
	}

	
	if (m_unhandles.size() > m_capacity)
	{
		auto tip_it = m_tips.begin();
        int current_search_count = 0;
		while (true)
		{
            assert_x(tip_it != m_tips.end());
            mcp::block_hash const & delete_hash(*tip_it);

            assert_x(m_unhandles.count(delete_hash));
			mcp::unhandle_item const & delete_item = m_unhandles[delete_hash];            
			bool has_missings(false);
			for (mcp::block_hash const &dependency_hash : delete_item.dependency_hashs)
			{
				if (m_missings.count(dependency_hash))
				{
					has_missings = true;
					break;
				}
			}
			if (!has_missings)
			{
				for (h256 const &dependency_hash : delete_item.transactions)
				{
					if (m_light_missings.count(dependency_hash))
					{
						has_missings = true;
						break;
					}
				}
			}
			

			//skip tip if it has unknown dependencies
			//but if all tips has unknown dependencies, remove the last tip
            if (current_search_count < m_max_search_count)
            {
                if (has_missings)
                {
                    tip_it++;
                    if (tip_it != m_tips.end())
                    {
                        current_search_count++;
                        continue;
                    }                      
                }
            }
           
			//delete dependencies
             del_unhandle_in_dependencies(delete_hash);
			 m_unhandles.erase(delete_hash);
             m_tips.erase(delete_hash);
			 BlockArrival.remove(delete_hash);
             break;
		}
	}
	///dependice block,transaction,approves exist,need request existed missings.
	if (allExist && !u_item.transactions.size()&& !u_item.approves.size())
	{
		return unhandle_add_result::Exist;
	}
    return unhandle_add_result::Success;
}

std::unordered_set<std::shared_ptr<mcp::block_processor_item>> mcp::unhandle_cache::release_dependency(mcp::block_hash const &dependency_hash_a)
{
	std::lock_guard<std::mutex> lock(m_mutux);

	std::unordered_set<std::shared_ptr<mcp::block_processor_item>> result;

	if (m_dependencies.count(dependency_hash_a))
	{
		std::shared_ptr<std::unordered_set<mcp::block_hash>> unhandle_hashs = m_dependencies[dependency_hash_a];
		for (auto i(unhandle_hashs->begin()), n(unhandle_hashs->end()); i != n; i++)
		{
			mcp::block_hash const &unhandle_hash = *i;

            if (m_unhandles.count(unhandle_hash) == 0)
            {
                LOG(m_log.info) << "m_unhandles dont have:hash:" << unhandle_hash.hex()<<",dependency_hash:"<< dependency_hash_a.hex();
            }
			assert_x(m_unhandles.count(unhandle_hash));
			auto &unhandle = m_unhandles[unhandle_hash];
			unhandle.dependency_hashs.erase(dependency_hash_a);
			if (unhandle.dependency_hashs.empty() && unhandle.transactions.empty() && unhandle.approves.empty())
			{
				result.insert(unhandle.item);

				del_unhandle_in_dependencies(unhandle_hash);
				m_unhandles.erase(unhandle_hash);
				//LOG(m_log.info) << "release_dependency:m_unhandles.erase to process hash:" << unhandle_hash.to_string();
				m_tips.erase(unhandle_hash);
			}
		}
        
		//delete dependency
		m_dependencies.erase(dependency_hash_a);
		//delete unknown dependencies
		m_missings.erase(dependency_hash_a);

        if (m_unhandles.count(dependency_hash_a))
        {
            del_unhandle_in_dependencies(dependency_hash_a);
            m_unhandles.erase(dependency_hash_a);
        }
	}

	return result;
}

std::unordered_set<std::shared_ptr<mcp::block_processor_item>> mcp::unhandle_cache::release_transaction_dependency(h256Hash const &hashs)
{
	std::lock_guard<std::mutex> lock(m_mutux);
	std::unordered_set<std::shared_ptr<mcp::block_processor_item>> result;
	for (auto h : hashs)
	{
		if (!m_transactions.count(h))
			continue;

		std::shared_ptr<std::unordered_set<mcp::block_hash>> unhandle_hashs = m_transactions[h];
		for (auto it : *unhandle_hashs)
		{
			mcp::block_hash const &unhandle_hash = it;

			if (!m_unhandles.count(unhandle_hash))
			{
				LOG(m_log.info) << "m_unhandles dont have:hash:" << unhandle_hash.hex() << ",dependency_hash:" << h.hex();
			}
			assert_x(m_unhandles.count(unhandle_hash));
			auto &unhandle = m_unhandles[unhandle_hash];
			unhandle.transactions.erase(h);
			if (unhandle.dependency_hashs.empty() && unhandle.transactions.empty() && unhandle.approves.empty())
			{
				result.insert(unhandle.item);

				del_unhandle_in_dependencies(unhandle_hash);
				m_unhandles.erase(unhandle_hash);
				//LOG(m_log.info) << "release_dependency:m_unhandles.erase to process hash:" << unhandle_hash.to_string();
				m_tips.erase(unhandle_hash);
			}
		}

		//delete dependency
		m_transactions.erase(h);
		//delete unknown dependencies
		m_light_missings.erase(h);
	}

	return result;
}

std::unordered_set<std::shared_ptr<mcp::block_processor_item>> mcp::unhandle_cache::release_approve_dependency(h256 const &h)
{
	std::lock_guard<std::mutex> lock(m_mutux);

	std::unordered_set<std::shared_ptr<mcp::block_processor_item>> result;
	if (!m_approves.count(h))
		return result;

	std::shared_ptr<std::unordered_set<mcp::block_hash>> unhandle_hashs = m_approves[h];
	for (auto it : *unhandle_hashs)
	{
		mcp::block_hash const &unhandle_hash = it;

		if (!m_unhandles.count(unhandle_hash))
		{
			LOG(m_log.info) << "m_unhandles dont have:hash:" << unhandle_hash.hex() << ",dependency_hash:" << h.hex();
		}
		assert_x(m_unhandles.count(unhandle_hash));
		auto &unhandle = m_unhandles[unhandle_hash];
		unhandle.approves.erase(h);
		if (unhandle.dependency_hashs.empty() && unhandle.transactions.empty() && unhandle.approves.empty())
		{
			result.insert(unhandle.item);

			del_unhandle_in_dependencies(unhandle_hash);
			m_unhandles.erase(unhandle_hash);
			//LOG(m_log.info) << "release_dependency:m_unhandles.erase to process hash:" << unhandle_hash.to_string();
			m_tips.erase(unhandle_hash);
		}
	}

	//delete dependency
	m_approves.erase(h);
	//delete unknown dependencies
	m_approve_missings.erase(h);
	return result;
}

void mcp::unhandle_cache::get_missings(size_t const & missing_limit_a, std::vector<mcp::block_hash> & missings_a, std::vector<h256> & light_missings_a, std::vector<h256>& approve_missings_a)
{
	std::lock_guard<std::mutex> lock(m_mutux);

	if (m_missings.size() == 0 && m_unhandles.size() > 0)
	{
		auto it = m_unhandles.begin();
		size_t offset = mcp::random_pool.GenerateWord32(0, m_unhandles.size() - 1);
		for (; offset > 0; offset--)
			it++;
		mcp::block_hash start(0);
		while (missings_a.size() < 50 && start != it->first)
		{
			missings_a.push_back(it->first);
			if (start == mcp::block_hash(0))
				start = it->first;
			it++;
			if (it == m_unhandles.end())
				it = m_unhandles.begin();
		}
	}
	else
	{
		if (m_missings.size() <= missing_limit_a)
		{
			for (auto const &d : m_missings)
			{
				if (!m_unhandles.count(d))
					missings_a.push_back(d);
			}
		}
		else
		{
			auto it = m_missings.begin();
			size_t offset = mcp::random_pool.GenerateWord32(0, m_missings.size() - 1);
			for (; offset > 0; offset--)
				it++;
			mcp::block_hash start(0);
			while (missings_a.size() < missing_limit_a && start != *it)
			{
				if (!m_unhandles.count(*it))
					missings_a.push_back(*it);
				if (start == mcp::block_hash(0))
					start = *it;
				it++;
				if (it == m_missings.end())
					it = m_missings.begin();
			}
		}
	}

	size_t light_missing_limit = missing_limit_a - missings_a.size();

	h256Hash knowns;
	{
		/// will locked transaction queue
		knowns = m_tq->knownTransactions();
	}
	if (m_light_missings.size() <= light_missing_limit)
	{
		for (auto const &d : m_light_missings)
		{
			if (!knowns.count(d))
				light_missings_a.push_back(d);
		}
	}
	else
	{
		auto it = m_light_missings.begin();
		size_t offset = mcp::random_pool.GenerateWord32(0, m_light_missings.size() - 1);
		for (; offset > 0; offset--)
		{
			it++;
		}

		h256 start(0);
		while (light_missings_a.size() < light_missing_limit && start != *it)
		{
			if (!knowns.count(*it))
				light_missings_a.push_back(*it);
			if (start == h256(0))
				start = *it;
			it++;
			if (it == m_light_missings.end())
				it = m_light_missings.begin();
		}
	}

	size_t approve_missing_limit = missing_limit_a / 4;

	if (m_approve_missings.size() <= approve_missing_limit)
	{
		for (auto const &d : m_approve_missings)
		{
			//if (!m_unhandles.count(d))
				approve_missings_a.push_back(d);
		}
	}
	else
	{
		auto it = m_approve_missings.begin();
		size_t offset = mcp::random_pool.GenerateWord32(0, m_approve_missings.size() - 1);
		for (; offset > 0; offset--)
		{
			it++;
		}

		h256 start(0);
		while (approve_missings_a.size() < approve_missing_limit && start != *it)
		{
			//if (!m_unhandles.count(*it))
				approve_missings_a.push_back(*it);
			if (start == h256(0))
				start = *it;
			it++;
			if (it == m_approve_missings.end())
				it = m_approve_missings.begin();
		}
	}
}

bool mcp::unhandle_cache::exists(mcp::block_hash const & block_hash_a)
{
	std::lock_guard<std::mutex> lock(m_mutux);

	auto it(m_unhandles.find(block_hash_a));
	return it != m_unhandles.end();
}

void mcp::unhandle_cache::del_unhandle_in_dependencies(mcp::block_hash const & unhandle_a)
{
    mcp::unhandle_item const & delete_item = m_unhandles[unhandle_a];
    auto dependency_hashs_p = delete_item.dependency_hashs;
    for (mcp::block_hash const &dependency_hash : dependency_hashs_p)
    {
        assert_x(m_dependencies.count(dependency_hash));
        std::shared_ptr<std::unordered_set<mcp::block_hash>> unhandle_hashs = m_dependencies[dependency_hash];
        assert_x(unhandle_hashs->count(unhandle_a));
        unhandle_hashs->erase(unhandle_a);
        if (unhandle_hashs->empty())
        {
            //delete dependency
            m_dependencies.erase(dependency_hash);
            //delete unknown dependencies
            m_missings.erase(dependency_hash);
            //check if dependency is tip now
            if (m_unhandles.count(dependency_hash))
                m_tips.insert(dependency_hash);
        }
    }

	for (h256 const &dependency_hash : delete_item.transactions)
	{
		assert_x(m_transactions.count(dependency_hash));
		std::shared_ptr<std::unordered_set<mcp::block_hash>> unhandle_hashs = m_transactions[dependency_hash];
		assert_x(unhandle_hashs->count(unhandle_a));
		unhandle_hashs->erase(unhandle_a);
		if (unhandle_hashs->empty())
		{
			//delete dependency
			m_transactions.erase(dependency_hash);
			//delete unknown dependencies
			m_light_missings.erase(dependency_hash);
		}
	}

	for (h256 const &dependency_hash : delete_item.approves)
	{
		assert_x(m_approves.count(dependency_hash));
		std::shared_ptr<std::unordered_set<mcp::block_hash>> unhandle_hashs = m_approves[dependency_hash];
		assert_x(unhandle_hashs->count(unhandle_a));
		unhandle_hashs->erase(unhandle_a);
		if (unhandle_hashs->empty())
		{
			//delete dependency
			m_approves.erase(dependency_hash);
			//delete unknown dependencies
			m_approve_missings.erase(dependency_hash);
		}
	}
}

size_t mcp::unhandle_cache::unhandlde_size() const
{
	return m_unhandles.size();
}

size_t mcp::unhandle_cache::dependency_size() const
{
	return m_dependencies.size();
}

size_t mcp::unhandle_cache::missing_size() const
{
    return m_missings.size();
}

size_t mcp::unhandle_cache::light_missing_size() const
{
	return m_light_missings.size();
}

size_t mcp::unhandle_cache::approve_missing_size() const
{
	return m_approve_missings.size();
}

size_t mcp::unhandle_cache::tips_size() const
{
    return m_tips.size();
}

