#ifndef GIGAMONKEY_BOOST_BOOST
#define GIGAMONKEY_BOOST_BOOST

#include <gigamonkey/stratum/stratum.hpp>
#include <gigamonkey/script.hpp>
#include <gigamonkey/boost/hashpower.hpp>
#include <gigamonkey/address.hpp>
#include <gigamonkey/signature.hpp>
#include <data/encoding/halves.hpp>

namespace Gigamonkey {
    
    using price = double; 
    
    // See https://github.com/MatterPool/Tech-Roadmap/wiki/POW-Boost
    
    namespace Boost {
        
        enum type {invalid, bounty, contract};
        
        struct output_script {
            Boost::type Type;
            int32_little Category;
            uint256 Content;
            work::target Target;
            bytes Tag;
            uint32_little UserNonce;
            bytes AdditionalData;
            uint160 MinerAddress;
            
            output_script() : Type{Boost::invalid}, 
                Category{}, Content{}, Target{}, 
                Tag{}, UserNonce{}, 
                AdditionalData{}, MinerAddress{} {} 
                
            output_script(
                Boost::type type, 
                int32_little category, 
                uint256 content, 
                work::target target, 
                bytes tag, 
                uint32_little user_nonce, 
                bytes data, 
                uint160 miner_address) : 
                output_script{from_data(type, category, content, target, tag, user_nonce, data, miner_address)} {}
            
            static output_script bounty(
                int32_little category,
                uint256 content,
                work::target target, 
                bytes_view tag, 
                uint32_little user_nonce, 
                bytes_view data) {
                if (tag.size() > 20) return output_script{};
                return output_script{category, 
                    content, target, tag, user_nonce, data};    
            }
            
            static output_script contract(
                int32_little category,
                uint256 content,
                work::target target, 
                bytes_view tag, 
                uint32_little user_nonce, 
                bytes_view data, 
                uint160 miner_address) {
                return output_script{category, 
                    content, target, tag, user_nonce, data, 
                    miner_address}; 
            }
            
            bool valid() const {
                return Type != Boost::invalid;
            }
            
            script write() const; 
            
            digest256 index() {
                return valid() ? Bitcoin::hash256(write()) : digest256{};
            }
            
            static output_script read(bytes);
            
            explicit output_script(bytes b) : output_script{read(b)} {}
        
            bool operator==(const output_script& o) const {
                return Type == o.Type && 
                    Category == o.Category && 
                    Content == o.Content && 
                    Target == o.Target && 
                    Tag == o.Tag && 
                    UserNonce == o.UserNonce && 
                    AdditionalData == o.AdditionalData && 
                    MinerAddress == o.MinerAddress;
            }
            
            bool operator!=(const output_script& o) const {
                return !operator==(o);
            }
            
            static Boost::type type(script x) {
                return read(x).Type;
            }
            
            static bool valid(script x) {
                return read(x).valid();
            }
            
            static uint256 index(script x) {
                return read(x).index();
            }
            
            static int32_little version(script x) {
                return read(x).Type;
            }
            
            static uint256 content(script x) {
                return read(x).Content;
            }
            
            static work::target target(script x) {
                return read(x).Target;
            }
            
            static bytes tag(script x) {
                return read(x).Tag;
            }
            
            static uint32_little user_nonce(script x) {
                return read(x).UserNonce;
            }
            
            static bytes additional_data(script x) {
                return read(x).AdditionalData;
            }
            
            static uint160 miner_address(script x) {
                return read(x).MinerAddress;
            }
            
        private:
            output_script(
                int32_little category, 
                uint256 content,
                work::target target, 
                bytes_view tag, 
                uint32_little user_nonce, 
                bytes_view data) : Type{Boost::bounty}, 
                Category{category},
                Content{content}, 
                Target{target}, 
                Tag{tag}, 
                UserNonce{user_nonce}, 
                AdditionalData{data}, 
                MinerAddress{} {}
            
            output_script(
                int32_little category, 
                uint256 content,
                work::target target, 
                bytes_view tag, 
                uint32_little user_nonce, 
                bytes_view data,
                uint160 miner_address) : Type{Boost::contract}, 
                Category{category},
                Content{content}, 
                Target{target}, 
                Tag{tag}, 
                UserNonce{user_nonce}, 
                AdditionalData{data}, 
                MinerAddress{miner_address} {} 
                
            static output_script from_data(
                Boost::type type, 
                int32_little category, 
                uint256 content, 
                work::target target, 
                bytes tag, 
                uint32_little user_nonce, 
                bytes data, 
                uint160 miner_address) {
                if (type == Boost::invalid) return {};
                if (type == Boost::bounty) return output_script::bounty(category, content, target, tag, user_nonce, data);
                return output_script::contract(category, content, target, tag, user_nonce, data, miner_address);
            }
            
        };
        
        struct input_script {
            Boost::type Type;
            Bitcoin::signature Signature;
            Bitcoin::pubkey Pubkey;
            uint32_little Nonce;
            Gigamonkey::timestamp Timestamp;
            uint64_little ExtraNonce2;
            uint32_little ExtraNonce1;
            uint160 MinerAddress;
            
        private:
            input_script(
                Bitcoin::signature signature, 
                Bitcoin::pubkey pubkey, 
                uint32_little nonce,
                Gigamonkey::timestamp timestamp,
                uint64_little extra_nonce_2,
                uint32_little extra_nonce_1,
                uint160 miner_address) : Type{Boost::bounty}, 
                Signature{signature}, 
                Pubkey{pubkey}, 
                Nonce{nonce},
                Timestamp{timestamp},
                ExtraNonce2{extra_nonce_2},
                ExtraNonce1{extra_nonce_1},
                MinerAddress{miner_address} {}
            
            input_script(
                Bitcoin::signature signature, 
                Bitcoin::pubkey pubkey, 
                uint32_little nonce,
                Gigamonkey::timestamp timestamp,
                uint64_little extra_nonce_2,
                uint32_little extra_nonce_1) : Type{Boost::contract}, 
                Signature{signature}, 
                Pubkey{pubkey}, 
                Timestamp{timestamp},
                ExtraNonce2{extra_nonce_2},
                ExtraNonce1{extra_nonce_1},
                MinerAddress{} {}
                
        public:
            input_script() = default;
            
            bool valid() const {
                return Type != Boost::invalid;
            }
            
            script write() const; 
            
            // construct a Boost bounty input script. 
            static input_script bounty(
                Bitcoin::signature signature, 
                Bitcoin::pubkey pubkey,  
                uint32_little nonce,
                Gigamonkey::timestamp timestamp,
                uint64_little extra_nonce_2,
                uint32_little extra_nonce_1, 
                uint160 miner_address) {
                return input_script{signature, pubkey, nonce, timestamp, extra_nonce_2, extra_nonce_1, miner_address};
            }
            
            // construct a Boost contract input script.
            static input_script contract(
                Bitcoin::signature signature, 
                Bitcoin::pubkey pubkey,  
                uint32_little nonce,
                Gigamonkey::timestamp timestamp,
                uint64_little extra_nonce_2,
                uint32_little extra_nonce_1) {
                return input_script{signature, pubkey, nonce, timestamp, extra_nonce_2, extra_nonce_1};
            }
            
            static input_script read(bytes); 
            
            explicit input_script(bytes b) : input_script{read(b)} {}
        
            bool operator==(const input_script& i) const {
                return Type == i.Type && 
                    Signature == i.Signature && 
                    Pubkey == i.Pubkey && 
                    Nonce == i.Nonce &&
                    Timestamp == i.Timestamp && 
                    ExtraNonce1 == i.ExtraNonce1 && 
                    ExtraNonce2 == i.ExtraNonce2 && 
                    MinerAddress == i.MinerAddress;
            }
            
            bool operator!=(const input_script& i) const {
                return !operator==(i);
            }
            
            static Boost::type type(script x) {
                return read(x).Type;
            }
            
            static bool valid(script x) {
                return read(x).valid();
            }
            
            static Bitcoin::signature signature(script x) {
                return read(x).Signature;
            }
            
            static Bitcoin::pubkey pubkey(script x) {
                return read(x).Pubkey;
            }
            
            static Gigamonkey::timestamp timestamp(script x) {
                return read(x).Timestamp;
            }
            
            static uint32_little nonce(script x) {
                return read(x).Nonce;
            }
            
            static uint160 miner_address(script x) {
                return read(x).MinerAddress;
            }
            
            input_script(
                Bitcoin::signature signature, 
                Bitcoin::pubkey pubkey, 
                work::solution, Boost::type);
        };
        
        // from a job, an output script can always be reconstructed and a solution can be found. 
        struct job {
            type Type;
            work::puzzle Puzzle;
            
            bool valid() const {
                return (Type == Boost::bounty || Type == Boost::contract) && Puzzle.valid();
            }
            
            job() : Type{invalid}, Puzzle{} {}
            job(type t, work::puzzle p) : Type{t}, Puzzle{p} {}
            
            job(Boost::output_script x, uint160 addr) : job(make(x, addr)) {}
            
            job(Boost::type type, 
                int32_little category, 
                uint256 content, 
                work::target target, 
                bytes tag, 
                uint32_little user_nonce, 
                bytes data, 
                uint160 miner_address) : 
                job(make(type, category, content, target, tag, user_nonce, data, miner_address)) {}
            
            bool operator==(const job& j) const {
                return Puzzle == j.Puzzle && Type == j.Type;
            }
            
            bool operator!=(const job& j) const {
                return !operator==(j);
            }
            
            Boost::output_script output_script() const; 
            
            uint160 miner_address() const;

        private:
            static job make(Boost::type type, 
                int32_little version, 
                uint256 content, 
                work::target target, 
                bytes tag, 
                uint32_little user_nonce, 
                bytes data, 
                uint160 miner_address) {
                if (type == invalid) return {};
                return job{type, work::puzzle{version, content, target, Merkle::path{}, 
                write(tag.size() + 20, tag, miner_address), 
                write(data.size() + 4, user_nonce, data)}};
            }
            
            static job make(Boost::output_script x, uint160 miner_address) {
                if (x.Type == invalid || (x.Type == contract && x.MinerAddress != miner_address)) return {};
                return make(x.Type, x.Category, x.Content, x.Target, x.Tag, 
                    x.UserNonce, x.AdditionalData, miner_address);
            }
        };
        
        inline work::proof work_proof(output_script out, input_script in) {
            return out.Type == invalid || in.Type == invalid || in.Type != out.Type ? work::proof{} : 
                work::proof{job{out, out.Type == bounty ? in.MinerAddress : out.MinerAddress}.Puzzle, 
                    work::solution{in.Timestamp, in.Nonce, write(12, in.ExtraNonce1, in.ExtraNonce2)}};
        }
        
    }
        
    namespace Boost::database {
        
        struct outpoint {
            uint256 Txid; 
            uint32_t Index;
        };
        
        struct output {
            satoshi Value;
            outpoint Reference;
            uint256 Redeemed;    // TXID of redeeming tx if it exists. 
        };
        
        // interface for outputs database. The id is the hash of the output script. 
        // see https://github.com/MatterPool/Tech-Roadmap/wiki/POW-Boost-API-Design#rest-api-for-jobs
        // and https://github.com/MatterPool/Tech-Roadmap/wiki/POW-Boost-Database#jobs
        struct outputs {
        
            // TODO Need json serialization for all these types. 
            enum status {
                unknown,             // we have not seen a script with the given hash. 
                unspent,             // a script we have offered to a client.
                unredeemed,          // outputs exist in the blockchain but have not been spent
                redeemed,            // all outputs have been spent
                partially_redeemed}; // some outputs have been spent (edge case)
            
            struct script {
                status Status;
                double ExpectedTime; // 0 for redeemed scripts. 
                list<output> Outputs;
                
                // sum of values of all outputs over difficulty. 
                Gigamonkey::price price() const;
            };
        
            void to_json(json& j, const script& p); 
            
            void from_json(const json& j, script& p); 
        
            script get(uint256 id) const;
            bool put(output_script);
            bool register_output(uint256 id, satoshi, outpoint);
        };
        
        // Interface for content database. 
        struct contents {
        
            struct content {
                uint256 Digest;
                list<outputs::script> Scripts;
                work::difficulty Total; 
            };
        
            void to_json(json& j, const content& p); 
            
            void from_json(const json& j, content& p); 
                
            content query(uint256 content_digest, 
                const uint256* title, const uint160* tag, 
                timestamp begin, timestamp end) const;
            
            list<content> query(const uint256* title, const uint160* tag, 
                timestamp begin, timestamp end, uint64 first_recrd, uint64 max_records) const;
        };
            
        // For internal use. Interface for boost jobs that we've worked with. 
        struct sales {
            enum status {not_ours, offered, accepted, working, completed};
            
            struct job {
                status Status;
                timestamp CreationTimestamp;
                timestamp ExpirationTimestamp;
                Boost::job Job;
            };
            
            void put(job);
            job search(uint256 id) const;
            void update(uint256 id, status s);
        };
        
        struct queue {
            virtual work::difficulty difficulty() const = 0;
            bool contains(uint256 id);
        };
        
        struct queue_end : queue {
            virtual void insert(job) = 0;
            work::difficulty difficulty() const final;
        };
        
        // queue of scripts for us to work on. 
        struct queue_front : queue, economy::job_manager {
            job Selected;
            
            economy::assignments new_miner(economy::work_snapshot, list<economy::unassigned> all) const final {
                work::puzzle Puzzle = Selected.Puzzle;
                return data::for_each([Puzzle](economy::unassigned x) -> economy::assignment {
                    return economy::assignment{x.Worker, Puzzle};
                }, all);
            }
            
            economy::assignments rebalance(economy::work_snapshot) const final {
                return {};
            }
        };
        
        struct contracts_end : queue_end {
            void insert(job) final;
        };
        
        struct bounties_end : queue_end {
            void insert(job) final;
        };
        
        // interface for a queue of Boost output scripts 
        // which we have agreed to work on. 
        // See https://github.com/MatterPool/Tech-Roadmap/wiki/POW-Boost-Jobs-Data-Structures#boost-contract-queue
        struct contracts_front : queue_front {
            void pop();
            
            work::difficulty difficulty() const final;
            
            economy::assignments job_complete(Stratum::job j, economy::work_snapshot w, list<economy::unassigned> all) final {
                if (Selected.Puzzle == Stratum::work_puzzle(j)) pop();
                return new_miner(w, all);
            };
        };
        
        // interface for a priority queue Boost output scripts 
        // which can be worked on by anyone. 
        // See https://github.com/MatterPool/Tech-Roadmap/wiki/POW-Boost-Jobs-Data-Structures#boost-bounty-priority-queue
        class bounties_front : queue_front {
            void select(price minimum_profitable);
            
            work::difficulty difficulty() const final;
        
            // TODO need a way to calculate minimum profitable price. 
            economy::assignments job_complete(Stratum::job, economy::work_snapshot, list<economy::unassigned>) final;
        };
    
    }
    
    namespace Boost {
    
        // This type should check every new output we see for Boost. 
        struct monitor {
            // sends us a new address that we can redeem. 
            // make sure the private key is stored somewhere!!! 
            struct addresses {
                virtual uint160 get_new() = 0;
            };
            
            database::outputs& Outputs;
            database::sales& Sales;
            database::contracts_end& Contracts;
            database::bounties_end& Bounties;
            addresses& Addresses;
            
            void new_output(bytes_view script, satoshi value, database::outpoint op) {
                output_script o(script);
                if (o.Type == Boost::invalid) return;
                digest256 id = Bitcoin::hash256(script);
                
                Outputs.register_output(id, value, op);
                
                if (o.Type == Boost::bounty) {
                    Bounties.insert(Boost::job{o, Addresses.get_new()});
                    return;
                }
                
                database::sales::job job{Sales.search(id)};
                if (job.Status == database::sales::offered) {
                    Sales.update(id, database::sales::accepted);
                    Contracts.insert(Boost::job{o, o.MinerAddress});
                }
                
                return;
            }
            
            void new_input(uint256 redeemed) {
                // TODO
            }
        };
    
        struct sales_manager {
            // See Bip270: https://github.com/moneybutton/bips/blob/master/bip-0270.mediawiki
            struct output {
                satoshi Amount; 
                bytes Script; 
                string Description; 
            };
            
            struct payment_request {
                list<output> Outputs;
                timestamp CreationTimestamp;
                timestamp ExpirationTimestamp;
                string PaymentUrl; 
                string Memo;
                string MerchantData;
            };
            
            void to_json(json& j, const payment_request& p); 
            
            void from_json(const json& j, payment_request& p); 
            
            payment_request offer(uint256 content, uint256 title, uint256 tag, work::target);
            
            void accept(payment_request);
        };
        
    }
    
}

std::ostream& operator<<(std::ostream& o, const Gigamonkey::Boost::output_script s);

std::ostream& operator<<(std::ostream& o, const Gigamonkey::Boost::input_script s);

#endif


