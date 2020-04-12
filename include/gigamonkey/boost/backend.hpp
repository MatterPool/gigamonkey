#ifndef GIGAMONKEY_BOOST_BACKEND
#define GIGAMONKEY_BOOST_BACKEND

#include "boost.hpp"
#include <gigamonkey/wallet.hpp>
#include <set>

// hashpower has units of difficulty per second. 
using hashpower = double;

using profitability = double;

namespace Gigamonkey::Boost {
    
    struct output {
        Bitcoin::outpoint Reference;
        output_script Script;
        satoshi Value;
        digest256 ID;
        
        output() : Reference{}, Script{}, Value{0}, ID{} {}
        output(const Bitcoin::outpoint& o, const output_script& s, satoshi v) : 
            Reference{o}, Script{s}, Value{v}, ID{s.hash()} {}
            
        bool valid() const {
            return Reference.valid() && Script.valid() && ID.valid();
        }
        
    private:
        output(const Bitcoin::outpoint& o, const output_script& j, satoshi v, const uint256& id) : 
            Reference{o}, Script{j}, Value{v}, ID{id} {}
    };
    
    struct contracts_queue {
        struct job {
            output Output;
            Bitcoin::secret Secret;
            
            job() : Output{}, Secret{} {}
            job(const output& o, const Bitcoin::secret& s) : Output{o}, Secret{s} {}
            
            bool valid() const {
                return Output.valid() && Secret.valid();
            }
        };
        
        list<job> Jobs;
        double Difficulty; // Total difficulty of the queue.
        
        bool empty() const {
            return Jobs.empty();
        }
        
        void push(const job& j) {
            if (!j.valid() || j.Output.Script.Type != contract) return;
            Jobs = Jobs << j;
            Difficulty += double(j.Output.Script.Target.difficulty());
        }
        
        job pop() {
            if (empty()) return {};
            job j = Jobs.first();
            Jobs = Jobs.rest();
            Difficulty -= double(j.Output.Script.Target.difficulty());
            return j;
        }
    };
    
    struct bounties_queue {
        struct entry {
            output_script Script;
            satoshi Bounty;
            double Difficulty;
            double BountyPerDifficulty;
            digest256 ID;
            
            bool operator>=(const entry& p) const {
                return BountyPerDifficulty >= p.BountyPerDifficulty;
            }
            
            bool operator<=(const entry& p) const {
                return BountyPerDifficulty <= p.BountyPerDifficulty;
            }
            
            bool valid() const {
                return Script.valid();
            }
            
            entry() : Script{}, Bounty{0}, Difficulty{0}, BountyPerDifficulty{}, ID{} {}
            
            entry(const output_script& o, satoshi b) : entry(o, b, double(o.Target.difficulty())) {}
            
        private:
            entry(const output_script& o, satoshi b, double difficulty) : 
                Script{o}, Bounty{b}, Difficulty{difficulty}, 
                BountyPerDifficulty{double(b) / difficulty}, ID{o.hash()} {}
                
            entry(const output_script& o, satoshi b, double difficulty, double bpd, const uint256& id) : 
                Script{o}, Bounty{b}, Difficulty{difficulty}, 
                BountyPerDifficulty{bpd}, ID{id} {}
            
            friend struct bounties_queue;
        };
        
        // scripts ordered by satoshis / difficulty
        data::ordered_list<entry> List;
        
        // script ids associated with outpoints.
        std::map<uint256, list<Bitcoin::outpoint>> Entries;
        
        // keep track of total difficulty in the queue. 
        double Difficulty;
        
        bounties_queue() : List{}, Difficulty{0} {}
        
        bool empty() const {
            return List.empty();
        }
        
        bool contains(const uint256& id) const {
            return Entries.find(id) != Entries.end();
        }
        
        list<Bitcoin::outpoint> remove(const uint256& id);
        
        void push(const output& o) {
            if (o.Script.Type != bounty) return;
            auto i = Entries.find(o.ID);
            if (i == Entries.end()) {
                double d = double(o.Script.Target.difficulty());
                push(entry{o.Script, o.Value, d, double(o.Value / d), o.ID}, o.Reference);
            } else {
                
            }
        }
        
        // Random is supposed to be [0, 1)
        output_script select(double random, double cost_per_difficulty, double constant_cost) const;
        
    private:
        void push(const entry& e, const Bitcoin::outpoint& o) {
            List = List.insert(e);
            Difficulty += double(e.Difficulty);
            Entries.insert({e.ID, list<Bitcoin::outpoint>{o}});
        }
    };
    
    struct in_progress {
        struct job {
            digest256 ID;
            Boost::job Job;
            Bitcoin::secret Secret;
            
            bool valid() const {
                return Job.valid() && Secret.valid();
            }
            
            job() : Job{}, Secret{} {}
            job(const Boost::job& j, const Bitcoin::secret& s) : Job{j}, Secret{s} {}
        };
        
        struct worker {
            digest256 ID;
            virtual hashpower estimateHashpower() = 0;
        };
        
        struct completed {
            list<Bitcoin::spendable> Spendable;
            
            // workers that need to be reassigned. 
            list<worker&> Workers;
            
            completed() : Spendable{}, Workers{} {}
            completed(list<Bitcoin::spendable> v, list<worker&> w) : Spendable{v}, Workers{w} {}
        };
        
        // max number of elements in queue. 
        uint32 MaxSize;
        
        // max total difficulty in queue. 
        double MaxDifficulty;
        
        // minimum expected time per job. 
        uint32 MinTime; 
        
        std::list<std::pair<job, list<worker&>>> Jobs;
        
        // script ids associated with outpoints.
        std::map<uint256, list<Bitcoin::outpoint>> Entries;
        
        void assign(worker& w);
        
        // if a solution is not given, it is assumed that the output was 
        // redeemed by someone else. 
        completed complete(const uint256& id, const work::solution& x);
        completed complete(const uint256& id);
        
    private: 
        virtual job get() = 0;
    };
    
    struct ideal_hashpower_assignment {
        constexpr static double BlocksMinimum{.25};
        
        double Blocks;
        double BoostBounty;
        double BoostContract;
        
        ideal_hashpower_assignment(
            profitability blocks, 
            profitability boost_bounty, 
            profitability boost_contract);
    };
    
    struct assignments {
        hashpower Blocks;
        hashpower BoostBounty;
        hashpower BoostContract;
        
        assignments() : Blocks{0}, BoostBounty{0}, BoostContract{0} {}
        assignments(hashpower blocks, hashpower boost_contract, hashpower boost_bounty) : 
            Blocks{blocks}, BoostBounty{boost_bounty}, BoostContract{boost_contract} {}
    };
    
    struct goal {
        constexpr static double ContractQueueTime{10 * 60};
        
        ideal_hashpower_assignment IdealAssignment;
        double ContractPricePerDifficulty;
        
        // Do this with every new block candidate. 
        // IdealAssignment and ContractPricePerDifficulty
        // are both updated. 
        goal reasses(
            profitability blocks, 
            double hashpower_price_satoshis_per_difficulty, 
            double bounty_queue_profitable_difficulty, 
            double contract_queue_difficulty, 
            assignments current_assignments) const;
        
        goal(ideal_hashpower_assignment i, double p) : IdealAssignment{i}, ContractPricePerDifficulty{p} {}
    };
}

#endif


