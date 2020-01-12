// Copyright (c) 2019 Daniel Krawisz
// Distributed under the Open BSV software license, see the accompanying file LICENSE.

#ifndef GIGAMONKEY_SCRIPT_ENGINE
#define GIGAMONKEY_SCRIPT_ENGINE

#include <gigamonkey/script.hpp>

namespace gigamonkey::bitcoin::script {
    
    class engine {
        // a class that caches signature hashes. 
        struct transaction {
            bytes_view Transaction;
        };
        
    public:
        struct item { // TODO need a new name for this. 
            bytes* Data;
            engine* Unevaluated;
            
            item(bytes_view b) : Data{new bytes{b}}, Unevaluated{nullptr} {}
            item(list<bytes_view>, program p) {
                throw data::method::unimplemented{"item{list<bytes_view>, program}"};
            }
            
            bytes evaluate(bytes_view tx) const {
                throw data::method::unimplemented{"item::evaluate(bytes_view)"};
            }
            
            ~item() {
                if (Data != nullptr) delete Data;
                if (Unevaluated != nullptr) delete Unevaluated;
            }
            
            bool operator==(const item& i) const {
                throw data::method::unimplemented{"item=="};
            }
            
            bool operator!=(const item& i) const {
                return !operator==(i);
            }
            
        private:
            item(const engine& e) : Data{nullptr}, Unevaluated{new engine{e}} {}
            friend class engine;
        };
        
        list<item> Stack;
        list<bytes> AltStack;
        program Program;
        
        engine(program) {
            throw data::method::unimplemented{"engine{program}"};
        }
        
        bool halted() const {
            return data::empty(Program);
        }
        
        bool evaluate(bytes_view tx);
        
        item evaluate() const {
            throw data::method::unimplemented{"engine::evaluate"};
        }
    };
    
    inline engine::item run(program p) {
        return engine{p}.evaluate();
    }
    
    inline engine::item run(bytes_view scr) {
        return run(decompile(scr));
    }
    
    inline engine::item run(bytes_view script_signature, bytes_view script_pubkey) {
        return run(decompile(script_signature) + decompile(script_pubkey));
    }
    
}

#endif
