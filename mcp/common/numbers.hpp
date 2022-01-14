#pragma once

#include <boost/multiprecision/cpp_int.hpp>

#include <cryptopp/osrng.h>
#include <unordered_set>

#include <libdevcore/Common.h>
#include <libdevcore/SHA3.h>

#include <mcp/common/assert.hpp>

namespace mcp
{
// We operate on streams of uint8_t by convention
using stream = std::basic_streambuf<uint8_t>;
// Random pool used by mcp.
// This must be thread_local as long as the AutoSeededRandomPool implementation requires it
extern thread_local CryptoPP::AutoSeededRandomPool random_pool;
using uint128_t = boost::multiprecision::uint128_t;
using uint256_t = boost::multiprecision::uint256_t;
using uint512_t = boost::multiprecision::uint512_t;

// Read a raw byte stream the size of `T' and fill value.
template <typename T>
bool read(mcp::stream & stream_a, T & value)
{
	static_assert (std::is_pod<T>::value, "Can't stream read non-standard layout types");
	auto amount_read(stream_a.sgetn(reinterpret_cast<uint8_t *> (&value), sizeof(value)));
	return amount_read != sizeof(value);
}
template <typename T>
void write(mcp::stream & stream_a, T const & value)
{
	static_assert (std::is_pod<T>::value, "Can't stream write non-standard layout types");
	auto amount_written(stream_a.sputn(reinterpret_cast<uint8_t const *> (&value), sizeof(value)));
	assert_x(amount_written == sizeof(value));
}

union uint64_union
{
public:
	uint64_union() = default;
	uint64_union(mcp::uint64_union const &) = default;
	explicit uint64_union(uint64_t const & value_a);
	bool operator== (mcp::uint64_union const &) const;
	bool operator!= (mcp::uint64_union const &) const;
	bool operator< (mcp::uint64_union const &) const;
	bool operator> (mcp::uint64_union const &) const;
	bool operator<= (mcp::uint64_union const &) const;
	bool operator>= (mcp::uint64_union const &) const;
	void encode_hex(std::string &) const;
	bool decode_hex(std::string const &);
	std::string to_string() const;
	uint64_t number() const;
	enum { size = 8 };
	std::array<uint8_t, 8> bytes;
	std::array<uint64_t, 1> qwords;
};

union uint128_union
{
public:
	uint128_union () = default;
	uint128_union (std::string const &);
	uint128_union (uint64_t);
	uint128_union (mcp::uint128_union const &) = default;
	uint128_union (mcp::uint128_t const &);
	bool operator== (mcp::uint128_union const &) const;
	bool operator!= (mcp::uint128_union const &) const;
	bool operator< (mcp::uint128_union const &) const;
	bool operator> (mcp::uint128_union const &) const;
	void encode_hex (std::string &) const;
	bool decode_hex (std::string const &);
	void encode_dec (std::string &) const;
	bool decode_dec (std::string const &);
	std::string format_balance (mcp::uint128_t scale, int precision, bool group_digits);
	std::string format_balance (mcp::uint128_t scale, int precision, bool group_digits, const std::locale & locale);
	mcp::uint128_t number () const;
	void clear ();
	bool is_zero () const;
	std::string to_string () const;
	std::string to_string_dec () const;
	enum { size = 16 };
	std::array<uint8_t, 16> bytes;
	std::array<char, 16> chars;
	std::array<uint32_t, 4> dwords;
	std::array<uint64_t, 2> qwords;
};
// Balances are 128 bit.

class raw_key;
union uint256_union
{
	uint256_union () = default;
	uint256_union (std::string const &);
	uint256_union (uint64_t);
	uint256_union (mcp::uint256_t const &);
	void encrypt (mcp::raw_key const &, mcp::raw_key const &, uint128_union const &);
	uint256_union & operator^= (mcp::uint256_union const &);
	uint256_union operator^ (mcp::uint256_union const &) const;
	bool operator== (mcp::uint256_union const &) const;
	bool operator!= (mcp::uint256_union const &) const;
	bool operator< (mcp::uint256_union const &) const;
	bool operator> (mcp::uint256_union const &) const;
	bool operator<= (mcp::uint256_union const &) const;
	bool operator>= (mcp::uint256_union const &) const;
	void encode_hex (std::string &) const;
	bool decode_hex (std::string const &);
	void encode_dec (std::string &) const;
	bool decode_dec (std::string const &);
	
	// void encode_account (std::string &) const;
	// std::string to_account () const;
	// std::string to_account_split () const;
	// bool decode_account (std::string const &);

	byte* data() { return bytes.data(); }
	byte const* data() const { return bytes.data(); }

	std::array<uint8_t, 32> bytes;
	std::array<char, 32> chars;
	std::array<uint32_t, 8> dwords;
	std::array<uint64_t, 4> qwords;
	std::array<uint128_union, 2> owords;

	void clear ();
	bool is_zero () const;
	std::string to_string () const;
	std::string to_string_dec () const;
	mcp::uint256_t number () const;
	enum { size = 32 };
	dev::bytesRef ref() { return dev::bytesRef(bytes.data(), 32); }
	dev::bytesConstRef ref() const { return dev::bytesConstRef(bytes.data(), 32); }
};

// All keys and hashes are 256 bit.
using amount = uint256_t;
using block_hash = uint256_union;
using summary_hash = uint256_union;
using account_state_hash = uint256_union;
// using account = uint256_union;
/// A hash set of mcp accounts
// using AccountHash = std::unordered_set<account>;
// using public_key = uint256_union;
using private_key = uint256_union;
using secret_ciphertext = uint256_union;
using data_hash = uint256_union;
using code_hash = uint256_union;
using state_root = uint256_union;
using sync_request_hash = uint256_union;
using seed_key = uint256_union;
using secret_encry = uint256_union;		//p2p encryption

class raw_key
{
public:
	raw_key () = default;
	raw_key(mcp::private_key const & prv_a);
	~raw_key ();
	void decrypt (mcp::uint256_union const &, mcp::raw_key const &, uint128_union const &);
	raw_key(mcp::raw_key const &) = delete;
	raw_key(mcp::raw_key const &&) = delete;
	mcp::raw_key & operator= (mcp::raw_key const &) = delete;
	bool operator== (mcp::raw_key const &) const;
	bool operator!= (mcp::raw_key const &) const;
	mcp::uint256_union data;
};

union uint512_union
{
	uint512_union () = default;
	uint512_union (mcp::uint512_t const &);
	bool operator== (mcp::uint512_union const &) const;
	bool operator!= (mcp::uint512_union const &) const;
	mcp::uint512_union & operator^= (mcp::uint512_union const &);
	uint512_union operator^ (mcp::uint512_union const &) const;

	// Added by Daniel
	bool operator< (mcp::uint512_union const &) const;
	bool operator> (mcp::uint512_union const &) const;
	bool operator<= (mcp::uint512_union const &) const;
	bool operator>= (mcp::uint512_union const &) const;

	void encode_hex (std::string &) const;
	bool decode_hex (std::string const &);
	bool is_zero () const;

	byte* data() { return bytes.data(); }
	byte const* data() const { return bytes.data(); }

	std::array<uint8_t, 64> bytes;
	std::array<uint32_t, 16> dwords;
	std::array<uint64_t, 8> qwords;
	std::array<uint256_union, 2> uint256s;
	void clear ();
	mcp::uint512_t number () const;
	std::string to_string () const;
	enum { size = 64 };
	dev::bytesRef ref() { return dev::bytesRef(bytes.data(), 64); }
	dev::bytesConstRef ref() const { return dev::bytesConstRef(bytes.data(), 64); }
};

// added by michael at 1/7
struct signature_struct
{
    signature_struct() = default;
    signature_struct(uint256_union const& _r, uint256_union const& _s, byte _v): r(_r), s(_s), v(_v) {}
    
    bool is_valid() const noexcept {
		static const uint256_union s_max("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");
    	static const uint256_union s_zero(0);
		return (v <= 1 && r > s_zero && s > s_zero && r < s_max && s < s_max);
	}

	bool decode_hex(std::string const & hex_a) {
		bool error(false);
		if (hex_a.empty() || hex_a.size() != size) {
			r.clear();
			s.clear();
			v = 0;
		} else {
			std::stringstream stream(hex_a);
			stream << std::hex << std::showbase;
			for (auto i(r.bytes.begin()), n(r.bytes.end()); i != n; ++i){
				stream >> *i;
			}
			for (auto i(s.bytes.begin()), n(s.bytes.end()); i != n; ++i){
				stream >> *i;
			}
			stream >> v;
		}
		return error;
	}

	std::string to_string() {
		std::stringstream stream;
		stream << std::hex << std::noshowbase << v;
		return r.to_string() + s.to_string() + stream.str();
	}

    uint256_union r;
    uint256_union s;
    byte v = 0;

	enum { size = 65 };
	dev::bytesRef ref() { return dev::bytesRef(r.data(), size); }
	dev::bytesConstRef ref() const { return dev::bytesConstRef(r.data(), size); }
};

// added by michael at 1/8
// using signature = uint512_union;
using signature = signature_struct;

// added by michael at 1/5
// using secret_key = uint512_union;
using secret_key = uint256_union;
using public_key = uint512_union;
using public_key_comp = uint256_union;

struct account20_struct {
	account20_struct()  = default;
	account20_struct(mcp::public_key const& pubkey) {
		dev::bytesConstRef bRef = sha3(pubkey.ref()).ref();
		dev::bytesConstRef(bRef.data() + 12, size).copyTo(ref());
	}
	account20_struct (mcp::uint256_t const &number_a) {
		if(number_a == 0) {
			memset(bytes.data(), 0, bytes.size());
		} else {
			mcp::uint256_t number_l(number_a);
			for (int i = 12; i < bytes.size(); i++) {
				bytes[i] = ((number_l) & 0xff).convert_to<uint8_t>();
				number_l >>= 8;
			}
		}
	}
	
	bool operator== (account20_struct const& other_a) const {
		return bytes == other_a.bytes;
	}
	bool operator!= (account20_struct const & other_a) const {
		return !(*this == other_a);
	}

	std::string to_account() const {
		std::stringstream stream;
		stream << std::hex << std::showbase;
		for (auto i(bytes.begin()), n(bytes.end()); i != n; ++i) {
			stream << *i;
		}
		return stream.str();
	}

	bool is_zero() const {
		bool is_zero(true);
		for (auto i(bytes.begin()), n(bytes.end()); i != n; ++i) {
			if (*i != 0) {
				is_zero = false;
				break;
			}
		}
		return is_zero;
	}

	bool decode_account(std::string const & source_a) {
		bool error(false);
		if (source_a.empty() || source_a.size() != 42) {
			bytes.fill(0);
		} else {
			std::stringstream stream(source_a);
			stream << std::hex << std::showbase;
			for (auto i(bytes.begin()), n(bytes.end()); i != n; ++i){
				stream >> *i;
			}
		}
		return error;
	}

	mcp::uint256_t number() {
		mcp::uint256_t result;
		auto shift(0);
		for (auto i(bytes.begin()), n(bytes.end()); i != n; ++i)
		{
			result <<= shift;
			result |= *i;
			shift = 8;
		}
		return result;
	}

	byte* data() { return bytes.data(); }
	byte const* data() const { return bytes.data(); }
	
	std::array<uint8_t, 20> bytes;
	void clear () { bytes.fill(0); }

	enum { size = 20 };
	dev::bytesRef ref() { return dev::bytesRef(bytes.data(), size); }
	dev::bytesConstRef ref() const { return dev::bytesConstRef(bytes.data(), size); }
};

using account = account20_struct;
using AccountHash = std::unordered_set<account>;

namespace p2p
{
	using node_id = mcp::uint256_union;
	using hash256 = mcp::uint256_union;
}

mcp::signature sign_message (mcp::raw_key const &, mcp::public_key const &, mcp::uint256_union const &);
bool validate_message (mcp::public_key const &, mcp::uint256_union const &, mcp::signature const &);
}

namespace std
{
template <>
struct hash<mcp::uint256_union>
{
	size_t operator() (mcp::uint256_union const & data_a) const
	{
		return *reinterpret_cast<size_t const *> (data_a.bytes.data ());
		//return XXH64(data_a.bytes.data(), data_a.bytes.size(), 0);
	}
};
template <>
struct hash<mcp::uint256_t>
{
	size_t operator() (mcp::uint256_t const & number_a) const
	{
		return number_a.convert_to<size_t> ();
	}
};

// Hash for uint512, Daniel
template <>
struct hash<mcp::uint512_t>
{
	size_t operator() (mcp::uint512_t const & number_a) const
	{
		return number_a.convert_to<size_t> ();
	}
};
template <>
struct hash<mcp::uint512_union>
{
	size_t operator() (mcp::uint512_union const & data_a) const
	{
		return *reinterpret_cast<size_t const *> (data_a.bytes.data ());
		//return XXH64(data_a.bytes.data(), data_a.bytes.size(), 0);
	}
};
template <>
struct hash<mcp::account20_struct>
{
	size_t operator() (mcp::account20_struct const & data_a) const
	{
		return *reinterpret_cast<size_t const *> (data_a.bytes.data ());
		//return XXH64(data_a.bytes.data(), data_a.bytes.size(), 0);
	}
};
}

namespace boost
{
    template <>
    struct hash<mcp::uint256_union>
    {
        size_t operator() (mcp::uint256_union const & value_a) const
        {
            std::hash<mcp::uint256_union> hash;
            return hash(value_a);
        }
    };
	template <>
    struct hash<mcp::account20_struct>
    {
        size_t operator() (mcp::account20_struct const & value_a) const
        {
            std::hash<mcp::account20_struct> hash;
            return hash(value_a);
        }
    };
}
