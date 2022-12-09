#pragma once

#include <mcp/p2p/common.hpp>
#include <mcp/p2p/capability.hpp>
#include <mcp/p2p/handshake.hpp>
#include <mcp/p2p/node_table.hpp>
#include <mcp/p2p/frame_coder.hpp>
#include <mcp/p2p/peer.hpp>
#include <mcp/p2p/peer_manager.hpp>
#include <mcp/p2p/upnp.hpp>

#include <unordered_map>


namespace mcp
{
    namespace p2p
    {
        class peer_metrics;
		class RLPXFrameCoder;
		class hankshake;
        class host : public std::enable_shared_from_this<host>
        {
        public:
            host(bool & error_a, p2p_config const & config_a, boost::asio::io_service & io_service_a, dev::Secret const & node_key, 
				boost::filesystem::path const & application_path_a);
			~host() { stop(); }
            void start();
            void stop();
            void register_capability(std::shared_ptr<icapability> cap);
            void on_node_table_event(node_id const & node_id_a, node_table_event_type const & type_a);
            std::unordered_map<node_id, bi::tcp::endpoint> peers() const;
            std::list<node_info> nodes() const;

			node_id id() const { return alias.pub(); }

			std::list<capability_desc> caps() const { std::list<capability_desc> ret; for (auto const& i : capabilities) ret.push_back(i.first); return ret; }
			void start_peer(mcp::p2p::node_id const& _id, dev::RLP const& _hello, std::unique_ptr<mcp::p2p::RLPXFrameCoder>&& _io, std::shared_ptr<bi::tcp::socket> const & socket);

			KeyPair alias;
			
            std::map<std::string,uint64_t> get_peers_write_queue_size();
            std::map<std::string, std::shared_ptr<mcp::p2p::peer_metrics> > get_peers_metrics();

			bool is_started() { return is_run; };
			size_t get_peers_count() { return m_peers.size(); };
			void replace_bootstrap(node_id const& old_a, node_id new_a);

			/// Set a handshake failure reason for a peer
			void onHandshakeFailed(node_id const& _n, HandshakeFailureReason _r);
			std::shared_ptr<peer_manager> peerManager() { return m_peer_manager; }
        private:
            enum class peer_type
            {
                egress = 0,
                ingress = 1
            };

            void run();
			bool is_handshaking(node_id const& _id) const;
			bool have_peer(node_id const& _id) const;
            bool resolve_host(std::string const & addr, bi::tcp::endpoint & ep);
            void connect(std::shared_ptr<node_info> const & ne);
            size_t avaliable_peer_count(peer_type const & type, bool b = true);
            uint32_t max_peer_size(peer_type const & type);
            void keep_alive_peers();
            void try_connect_nodes();
            void start_listen(bi::address const & listen_ip, uint16_t const & port);
            void accept_loop();

            void map_public(bi::address const & listen_ip, uint16_t port);

            node_info node_info_from_node_table(node_id const& node_id) const;

            p2p_config const & config;
            boost::asio::io_service & io_service;
            std::map<capability_desc, std::shared_ptr<icapability>> capabilities;

            std::unique_ptr<bi::tcp::acceptor> acceptor;
            std::unordered_map<node_id, std::weak_ptr<peer>> m_peers;
            mutable std::mutex m_peers_mutex;

			/// Pending connections. Completed handshakes are garbage-collected in run() (a handshake is
			/// complete when there are no more shared_ptrs in handlers)
			std::list<std::weak_ptr<hankshake>> m_connecting;
			mutable Mutex x_connecting;													///< Mutex for m_connecting.

            std::shared_ptr<node_table> m_node_table;

            dev::bytes restore_network_bytes;

            std::atomic<bool> is_run;
            std::unique_ptr<boost::asio::deadline_timer> run_timer;
            const boost::posix_time::milliseconds run_interval = boost::posix_time::milliseconds(100);

            std::chrono::seconds const keep_alive_interval = std::chrono::seconds(30);
            std::chrono::steady_clock::time_point last_ping;

            std::chrono::seconds const try_connect_interval = std::chrono::seconds(3);
			std::chrono::seconds const try_connect_interval_exemption = std::chrono::seconds(30);
            std::chrono::steady_clock::time_point last_try_connect;
			std::chrono::steady_clock::time_point last_try_connect_exemption;
            std::chrono::seconds node_fallback_interval = std::chrono::seconds(20);

            std::chrono::steady_clock::time_point start_time;
            std::vector<std::shared_ptr<node_info>> bootstrap_nodes;
            std::vector<std::shared_ptr<node_info>> exemption_nodes;

            boost::posix_time::milliseconds const handshake_timeout = boost::posix_time::milliseconds(5000);

			std::shared_ptr<peer_manager> m_peer_manager;
            std::unique_ptr<upnp> up;
            mcp::log m_log = { mcp::log("p2p") };
        };

        class host_node_table_event_handler : public node_table_event_handler
        {
        public:
            host_node_table_event_handler(host & host_a)
                :m_host(host_a)
            {
            }

            virtual void process_event(node_id const & node_id_a, node_table_event_type const & type_a)
            {
                m_host.on_node_table_event(node_id_a, type_a);
            }

            host & m_host;
        };
    }
}