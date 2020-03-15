#include <gigamonkey/boost/boost.hpp>
#include <gigamonkey/script.hpp>
#include <data/encoding/halves.hpp>
#include <iostream>

namespace Gigamonkey::Bitcoin {
    // The Bitcoin script pattern which takes a target in 
    // exponential format and converts it to expanded format. 
    const program expand_target = program{
        OP_SIZE, push_value(4), OP_EQUALVERIFY, push_value(3), OP_SPLIT,
        OP_DUP, push_value(3), OP_GREATERTHANOREQUAL, OP_VERIFY,
        OP_DUP, push_value(32), OP_LESSTHANOREQUAL, OP_VERIFY, OP_TOALTSTACK,
        push_hex("0000000000000000000000000000000000000000000000000000000000"), 
        OP_CAT, OP_FROMALTSTACK, push_value(3), OP_SUB, OP_RSHIFT};
}

namespace Gigamonkey::Boost {
    
    input_script input_script::read(bytes b) {
        using namespace Bitcoin;
        input_script x{};
        bytes MinerPubkey;
        bytes Timestamp;
        bytes Nonce;
        bytes ExtraNonce1;
        bytes ExtraNonce2;
        bytes MinerAddress;
        
        if (!pattern{
            push{x.Signature.Data}, 
            pubkey_pattern(MinerPubkey), 
            push_size{4, Nonce}, 
            push_size{4, Timestamp}, 
            push_size{8, ExtraNonce2}, 
            push_size{4, ExtraNonce1}, 
            optional{push_size{20, MinerAddress}}}.match(b)) return {};
        
        x.Type = MinerAddress.size() == 0 ? Boost::contract : Boost::bounty;
        
        if (x.Type == Boost::bounty) std::copy(
            MinerAddress.begin(), 
            MinerAddress.end(), 
            x.MinerAddress.begin());
        
        x.Pubkey.Value.resize(MinerPubkey.size());
        std::copy(
            MinerPubkey.begin(), 
            MinerPubkey.end(), 
            x.Pubkey.Value.begin());
        
        std::copy(
            Timestamp.begin(), 
            Timestamp.end(), 
            x.Timestamp.data());
        
        std::copy(
            Nonce.begin(), 
            Nonce.end(), 
            x.Nonce.data());
        
        std::copy(
            ExtraNonce1.begin(), 
            ExtraNonce1.end(), 
            x.ExtraNonce1.data());
        
        std::copy(
            ExtraNonce2.begin(), 
            ExtraNonce2.end(), 
            x.ExtraNonce2.data());
        
        return x;
    }
    
    output_script output_script::read(bytes b) {
        using namespace Bitcoin;
        output_script x{};
        bytes Category{};
        bytes Content{};
        bytes Target{};
        bytes UserNonce{};
        bytes MinerAddress{};
        
        pattern output_script_pattern = pattern{
            push{bytes{0x62, 0x6F, 0x6F, 0x73, 0x74, 0x70, 0x6F, 0x77}}, OP_DROP, 
            optional{push_size{20, MinerAddress}},
            push_size{4, Category},
            push_size{32, Content},
            push_size{4, Target},
            push{x.Tag}, 
            push_size{4, UserNonce}, 
            push{x.AdditionalData}, OP_CAT, OP_SWAP, 
            // copy mining pool’s pubkey hash to alt stack. A copy remains on the stack.
            push{5}, OP_ROLL, OP_DUP, OP_TOALTSTACK, OP_CAT,              
            // expand compact form of target and push to altstack. 
            push{2}, OP_PICK, expand_target, OP_TOALTSTACK, 
            push{5}, OP_ROLL, OP_SIZE, push{4}, OP_EQUALVERIFY, OP_CAT,   // check size of extra_nonce_1
            push{5}, OP_ROLL, OP_SIZE, push{8}, OP_EQUALVERIFY, OP_CAT,   // check size of extra_nonce_2
            // create metadata document and hash it.
            OP_SWAP, OP_CAT, OP_HASH256,                       
            OP_SWAP, OP_TOALTSTACK, OP_CAT, OP_CAT,              // target to altstack. 
            OP_SWAP, OP_SIZE, push{4}, OP_EQUALVERIFY, OP_CAT,   // check size of timestamp.
            OP_FROMALTSTACK, OP_CAT,                             // attach target
            // check size of nonce. Boost POW string is constructed. 
            OP_SWAP, OP_SIZE, push{4}, OP_EQUALVERIFY, OP_CAT,            
            // check that the hash of the Boost POW string is less than the target
            OP_HASH256, OP_FROMALTSTACK, OP_LESSTHAN, OP_VERIFY,
            // check that the given address matches the pubkey and check signature.
            OP_DUP, OP_HASH256, OP_FROMALTSTACK, OP_EQUALVERIFY, OP_CHECKSIG};
        
        if (!output_script_pattern.match(b)) return {};
        
        std::copy(
            Category.begin(), 
            Category.end(), 
            x.Category.data());
        
        if (x.Tag.size() > 20) return {};
        
        x.Type = MinerAddress.size() == 0 ? Boost::bounty : Boost::contract;
        
        if (x.Type == Boost::contract) std::copy(
            MinerAddress.begin(), 
            MinerAddress.end(), 
            x.MinerAddress.begin());
        
        std::copy(
            Target.begin(), 
            Target.end(), 
            x.Target.data());
        
        std::copy(
            UserNonce.begin(), 
            UserNonce.end(), 
            x.UserNonce.data());
        
        std::copy(
            Content.begin(), 
            Content.end(), 
            x.Content.data());
        
        return x;
    }
    
    script input_script::write() const {
        using namespace Bitcoin;
        if (Type == Boost::invalid) return {};
        program p{
            push_data(bytes_view(Signature)), 
            push_data(Pubkey),
            push_data(Nonce),
            push_data(bytes_view(Timestamp)),
            push_data(ExtraNonce2),
            push_data(ExtraNonce1)};
        if (Type == Boost::bounty) p = p << push_data(MinerAddress);
        return compile(p);
    }
    
    script output_script::write() const {
        using namespace Bitcoin;
        if (Type == Boost::invalid) return {};
        program boost_output_script = program{push_hex("626F6F7374706F77"), OP_DROP}; // "boostpow"
        
        if (Type == Boost::contract) 
            boost_output_script = boost_output_script.append(push_data(MinerAddress));
        
        boost_output_script = boost_output_script.append(
            push_data(Category),
            push_data(Content), 
            push_data(Target), 
            push_data(Tag), 
            push_data(UserNonce), 
            push_data(bytes_view(AdditionalData)), 
            OP_CAT, OP_SWAP, 
            // copy mining pool’s pubkey hash to alt stack. A copy remains on the stack.
            OP_5, OP_ROLL, OP_DUP, OP_TOALTSTACK, OP_CAT,              
            // expand compact form of target and push to altstack. 
            OP_2, OP_PICK, expand_target, OP_TOALTSTACK, 
            OP_5, OP_ROLL, OP_SIZE, OP_4, OP_EQUALVERIFY, OP_CAT,   // check size of extra_nonce_1
            OP_5, OP_ROLL, OP_SIZE, OP_8, OP_EQUALVERIFY, OP_CAT,   // check size of extra_nonce_2
            // create metadata document and hash it.
            OP_SWAP, OP_CAT, OP_HASH256,    
            OP_SWAP, OP_TOALTSTACK, OP_CAT, OP_CAT,                 // target to altstack. 
            OP_SWAP, OP_SIZE, OP_4, OP_EQUALVERIFY, OP_CAT,         // check size of timestamp.
            OP_FROMALTSTACK, OP_CAT,                                // attach target
            // check size of nonce. Boost POW string is constructed. 
            OP_SWAP, OP_SIZE, OP_4, OP_EQUALVERIFY, OP_CAT,            
            // check that the hash of the Boost POW string is less than the target
            OP_HASH256, OP_FROMALTSTACK, OP_LESSTHAN, OP_VERIFY,
            // check that the given address matches the pubkey and check signature.
            OP_DUP, OP_HASH256, OP_FROMALTSTACK, OP_EQUALVERIFY, OP_CHECKSIG);
        
        return compile(boost_output_script);
    }
    
    work::puzzle puzzle(Boost::output_script o, uint160 miner) { 
        if (o.Type == Boost::invalid || (o.Type == Boost::contract && o.MinerAddress != miner)) return {};
        
        return work::puzzle{
            o.Category, o.Content, o.Target, 
            Merkle::path{list<digest256>{}, 0}, 
            write(o.Tag.size() + 168, o.Tag, o.UserNonce, o.MinerAddress), 
            o.AdditionalData};
    }
    
    inline bool between_inclusive(int x, int y, int z) {
        return x <= y && y <= z;
    }
    
    uint160 job::miner_address() const {
        size_t puzzle_header_size = Puzzle.Header.size();
        if (puzzle_header_size < 28) return 0;
        uint160 x;
        std::copy(Puzzle.Header.end() - 28, 
                  Puzzle.Header.end() - 8, 
                  x.begin());
        return x;
    }
    
    input_script from_solution(
                Bitcoin::signature signature, 
                Bitcoin::pubkey pubkey, 
                work::solution x, Boost::type t) {
        if (t == Boost::invalid || !x.valid()) return input_script{};
        
        input_script in = t == Boost::bounty ? 
            input_script::bounty(signature, pubkey, x.Nonce, x.Timestamp, 0, 0, pubkey.hash()) : 
            input_script::contract(signature, pubkey, x.Nonce, x.Timestamp, 0, 0);
        
        std::copy(
            x.ExtraNonce.begin(), 
            x.ExtraNonce.begin() + 4, 
            in.ExtraNonce1.data());
        
        std::copy(
            x.ExtraNonce.begin() + 4, 
            x.ExtraNonce.end(), 
            in.ExtraNonce2.data());
        
        return in;
    }
    
    input_script::input_script(
        Bitcoin::signature signature, 
        Bitcoin::pubkey pubkey, 
        work::solution x, Boost::type t) : input_script{from_solution(signature, pubkey, x, t)} {}
    
    Boost::output_script job::output_script() const {
        if (Type == invalid) return Boost::output_script();
        
        size_t puzzle_header_size = Puzzle.Header.size();
        size_t puzzle_body_size = Puzzle.Body.size();
        
        if (puzzle_header_size < 20) return Boost::output_script();
        if (puzzle_body_size < 4) return Boost::output_script();
        
        size_t tag_size = puzzle_header_size - 20;
        size_t data_size = puzzle_body_size - 4;
        
        Boost::output_script out{Type, Puzzle.Category, Puzzle.Digest, Puzzle.Target, 
            bytes(tag_size), 0, bytes(puzzle_body_size - 4), 
            Type == contract ? miner_address() : uint160{}};
        
        std::copy(Puzzle.Header.begin(), Puzzle.Header.begin() + tag_size, out.Tag.begin());
        std::copy(Puzzle.Body.begin(), Puzzle.Body.begin() + 4, out.UserNonce.begin());
        std::copy(Puzzle.Body.begin() + 4, Puzzle.Body.end(), out.AdditionalData.begin());
        
        return out;
    }
    
}

std::ostream& operator<<(std::ostream& o, const Gigamonkey::Boost::output_script s) {
    using namespace Gigamonkey::Boost;
    if (s.Type == invalid) return o << "BoostOutputScript{Type : invalid}";
    o << "BoostOutputScript{Type : ";
    if (s.Type == bounty) o << "bounty, MinerAddress : " << s.MinerAddress;
    else o << "contract";
    return o << 
        ", Category : " << s.Category << 
        ", Content : " << s.Content << 
        ", Target : " << s.Target << 
        ", Tag : " << data::encoding::hexidecimal::write(s.Tag, data::endian::little) << 
        ", UserNonce : " << s.UserNonce << 
        ", AdditionalData : " << data::encoding::hexidecimal::write(s.AdditionalData, data::endian::little) << "}";
}

std::ostream& operator<<(std::ostream& o, const Gigamonkey::Boost::input_script s) {
    using namespace Gigamonkey::Boost;
    if (s.Type == invalid) return o << "BoostInputScript{Type : invalid}";
    o << "BoostInputScript{Type : " << (s.Type == contract ? "contract" : "bounty") << 
        ", Signature : " << s.Signature << 
        ", Pubkey : " << s.Pubkey << 
        ", Nonce : " << s.Nonce << 
        ", Timestamp : " << s.Timestamp << 
        ", ExtraNonce2 : " << s.ExtraNonce2 << 
        ", ExtraNonce1 : " << s.ExtraNonce1;
    if (s.Type == bounty) o << ", MinerAddress: " << s.MinerAddress;
    return o << "}";
}

namespace Gigamonkey::Boost::database {
    void outputs::to_json(json& j, const script& p) {
        throw data::method::unimplemented{"to_json script"}; // TODO
    }
    
    void outputs::from_json(const json& j, script& p) {
        throw data::method::unimplemented{"from_json script"}; // TODO
    } 
    
    void contents::to_json(json& j, const content& p) {
        throw data::method::unimplemented{"to_json content"}; // TODO
    }
    
    void contents::from_json(const json& j, content& p) {
        throw data::method::unimplemented{"from_json content"}; // TODO
    } 
}

namespace Gigamonkey::Boost {
    
    void sales_manager::to_json(json& j, const payment_request& p) {
        throw data::method::unimplemented{"to_json payment_request"}; // TODO
    }
    
    void sales_manager::from_json(const json& j, payment_request& p) {
        throw data::method::unimplemented{"from_json payment_request"}; // TODO
    } 
};

