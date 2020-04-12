#include <gigamonkey/boost/backend.hpp>

namespace Gigamonkey::Boost {
    
    list<Bitcoin::outpoint> bounties_queue::remove(const uint256& id) {
        if (!contains(id)) return {};
        
        data::ordered_list<entry> l = List;
        data::functional::stack::linked<entry> behind{};
        entry removed{};
        bool found = false;
        while (!l.empty()) {
            entry p = l.first();
            l = l.rest();
            if (p.ID == id) {
                removed = p;
                found = true;
                break;
            }
            behind = behind << p;
        }
        
        if (found) {
            while(!behind.empty()) {
                l = l << behind.first();
                behind = behind.rest();
            }
            List = l;
            Difficulty -= removed.Difficulty;
        }
        
        list<Bitcoin::outpoint> returned = Entries[id];
        Entries.erase(id);
        return returned;
    }
    
    output_script bounties_queue::select(double random, double cost_per_difficulty, double constant_cost) const {
        if (empty()) return {};
        double normalization = 0;
        data::functional::stack::linked<double> accumulated{};
        data::ordered_list<entry> list = List;
        while (!list.empty()) {
            entry next = list.first();
            double cost = constant_cost + cost_per_difficulty * next.Difficulty;
            double profitability = (double(next.Bounty) - cost) / cost;
            if (profitability < 0) break;
            normalization += profitability;
            accumulated = accumulated << normalization;
        }
        
        if (normalization == 0) return {};
        double r = random * normalization;
        int index{0};
        
        while(true) {
            double running_total = accumulated.first();
            if (r < running_total) break;
            index++;
            accumulated = accumulated.rest();
        }
        
        data::ordered_list<entry> selected = List;
        for(; index > 0; --index) selected = selected.rest();
        
        return selected.first().Script;
    }
        
    ideal_hashpower_assignment::ideal_hashpower_assignment(
        profitability blocks, 
        profitability boost_bounty, 
        profitability boost_contract) {
        if (blocks < 0) blocks = 0;
        if (boost_bounty < 0) boost_bounty = 0;
        if (boost_contract < 0) boost_contract = 0;
        
        // we always mine blocks even if it's unprofitable. 
        if (boost_bounty == 0 && boost_contract == 0) blocks = 1;
        
        profitability sum = blocks + boost_bounty + boost_contract;
        Blocks = blocks / sum;
        BoostBounty = boost_bounty / sum;
        BoostContract = boost_contract / sum;
        
        if (Blocks < BlocksMinimum) {
            Blocks = BlocksMinimum;
            double factor = (1 - BlocksMinimum) / (boost_bounty + boost_contract);
            BoostBounty *= factor;
            BoostContract *= factor;
        }
    } 
    
    // Do this with every new block candidate. 
    goal goal::reasses(
            profitability blocks, 
            double hashpower_price_satoshis_per_difficulty, 
            double bounty_queue_profitable_difficulty, 
            double contract_queue_difficulty, 
            assignments current_assignments) const {
        throw method::unimplemented{"controller::reasses"};
        // find profitability of bounty queue. 
        
        // find length of time to complete contract queue. 
        
        // adjust new price of boost contract. 
        
        // adjust ideal assignments. 
        // TODO
    }
}

