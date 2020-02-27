#ifndef GIGAMONKEY_BOOST_HASHPOWER
#define GIGAMONKEY_BOOST_HASHPOWER

#include <gigamonkey/stratum/stratum.hpp>

namespace Gigamonkey::economy {
    // See: https://github.com/MatterPool/Tech-Roadmap/wiki/Upgrading-BTC-Pool-to-Support-Boost#step-2-hashpower
    
    // hashpower has units of difficulty per second. 
    using hashpower = double;

    struct working {
        Stratum::job Assignment;
        hashpower Hashpower;
    };

    struct unassigned {
        Stratum::worker Worker;
        hashpower Hashpower;
    };

    struct work_snapshot {
        std::list<working> Workers;

        const map<Stratum::job, hashpower> distribution() const;
    };
    
    struct assignment {
        Stratum::worker Worker;
        work::puzzle Puzzle;
    };
    
    using assignments = list<assignment>;
    
    struct job_manager {
        // Notify the job manager of a job completed. 
        // Job manager figures out what the new job is. 
        virtual assignments job_complete(Stratum::job, work_snapshot, list<unassigned>) = 0;
        // Notify of a new miner and receive information on how that miner should be assigned. 
        virtual assignments new_miner(work_snapshot, list<unassigned>) const = 0;
        // Receive information about rebalancing. 
        virtual assignments rebalance(work_snapshot) const = 0;
    };
    
}

#endif


