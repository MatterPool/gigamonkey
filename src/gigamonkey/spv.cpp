// Copyright (c) 2019 Daniel Krawisz
// Distributed under the Open BSV software license, see the accompanying file LICENSE.

#include <gigamonkey/spv.hpp>

namespace Gigamonkey::Bitcoin {
        
    headers headers::attach(const Bitcoin::header& h) const {
        auto d = digest<32>(h.Previous);
        list<header> prev = Headers[d];
        if (data::empty(prev)) return {};
        ordered_list<chain> chains = Chains;
        list<chain> chx{};
        chain next;
        while(true) {
            if (data::empty(chains)) {
                next = chain{prev}.add(h);
                chains = Chains;
                break;
            }
            if (chains.first().Chain == prev) {
                next = chains.first().add(h);
                chains = chains.rest();
                while (!data::empty(chx)) {
                    chains = chains.insert(chx.first());
                    chx = chx.rest();
                }
                break;
            }
        }
        return headers{chains.insert(next), Headers.insert(next.Chain.first().Hash, next.Chain)};
    }
    
}
