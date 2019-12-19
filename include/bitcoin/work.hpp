// Copyright (c) 2019 Daniel Krawisz
// Distributed under the Open BSV software license, see the accompanying file LICENSE.

#ifndef GIGAMONKEY_WORK
#define GIGAMONKEY_WORK

#include "hash.hpp"

namespace gigamonkey::work {
    
    using nonce = boost::endian::little_int64_t;
    
    using digest = gigamonkey::digest<32, little_endian>;
    
    integer<32, little_endian> difficulty_1_target{"0x00000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"};
    
    using difficulty = data::math::number::fraction<integer<32, little_endian>, digest>;

    struct target {
        uint32_little Encoded;
        
        static target encode(byte e, uint24 v);
        
        target() : Encoded{} {}
        target(uint32 x) : Encoded{x} {}
        target(byte e, uint24 v) : Encoded{encode(e, v)} {}
        
        byte exponent() const {
            return static_cast<byte>(Encoded & 0x000000ff);
        }
        
        uint24 digits() const {
            return uint24{Encoded >> 8};
        }
        
        bool valid() const {
            byte e = exponent();
            return e >= 3 && e <= 32 && digits() != 0;
        }
        
        digest expand() const{
            return digest{digits()} >> (exponent() - 3);
        }
        
        explicit operator uint32_little() const {
            return Encoded;
        }
        
        explicit operator digest() const {
            return expand();
        }
        
        bool operator==(target t) const {
            return Encoded == t.Encoded;
        } 
        
        bool operator!=(target t) const {
            return Encoded != t.Encoded;
        } 
        
        bool operator<(target t) const {
            return expand() < t.expand();
        } 
        
        bool operator<=(target t) const {
            return expand() <= t.expand();
        } 
        
        bool operator>(target t) const {
            return expand() > t.expand();
        } 
        
        bool operator>=(target t) const {
            return expand() >= t.expand();
        } 
        
        work::difficulty difficulty() const {
            return difficulty{difficulty_1_target, expand()};
        }
    };
    
    const target easy{32, 0xffffff}; 
    const target hard{3, 0x000001};
    
    const target success_half{32, 0x800000};
    const target success_quarter{32, 0x400000};
    const target success_eighth{32, 0x200000};
    const target success_sixteenth{32, 0x100000};
    
    const uint32 message_size = 68;
    
    using content = uint<message_size, little_endian>;
    
    struct order {
        content Message;
        target Target;
        
        bool valid() const {
            return Target.valid();
        }
        
        order(content m, target t) : Message{m}, Target{t} {}
        order() : Message{}, Target{} {}
    };
    
    bool satisfied(order, nonce);
    
    struct candidate {
        uint<80, little_endian> Data;
    
        static data::uint<80> encode(order, nonce);
        
        candidate() : Data{} {}
        candidate(uint<80, little_endian> d) : Data{d} {}
        candidate(order o, nonce n) : Data{encode(o, n)} {}
        
        bool operator==(const candidate& c) {
            return Data == c.Data;
        }
        
        digest hash() const {
            return bitcoin::hash256(Data);
        }
        
        work::nonce nonce() const;
        
        work::content content() const;
        
        work::target target() const {
            throw data::method::unimplemented{"work::candidate::target"};
        }
    
        bool valid() const {
            return hash() < target().expand();
        }
    };
    
    inline byte exponent(target t) {
        t.exponent();
    }
    
    inline uint24 digits(target t) {
        return t.digits();
    }
    
    inline bool valid(target t) {
        t.valid();
    }
    
    inline digest expand(target t) {
        t.expand();
    }
    
    nonce work(order);
    
}

#endif
