# InstructionPrefetchers-And-CacheReplacementPolicies
We are a URS Team researching the effects that combining instruction prefetchers with cache replacement policies has on speedup. 

Alex is responsible for the Instruction Prefetchers and David is focused on Cache Replacement Policies.

Our project utilizes ChampSim, a trace-based microarchitecture simulator, to simulate processors running instructions with different prefetching and cache replacement hardware. We also employ Texas A&M's Computer Engineering and Systems Group (CESG) high performance computing cluster to run our simulations. The CESG cluster runs linux and is connected to via SSH. 

We are using competition instruction prefetchers and traces from IPC-1 at NCSU. Additionally, our cache replacement policies came from CRC2. Our goal is to use these prefetchers and cache replacement policies to develop a practical hybrid policy that produces an even higher speedup. To do this, we take cache replacement policies meant for the lower level cache and port them to the L1I cache. After we combine them, we will work on reducing interference between the prefetcher model and cache replacement policy.