#include <iostream>
#include <vector>
#include <numa.h>   // This is the only header needed for libnuma
#include <sched.h>
#include <iomanip>

void print_separator(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
}

int main() {
    // 1. HARDWARE DISCOVERY
    // Always check if NUMA is supported by the kernel/hardware first
    if (numa_available() == -1) {
        std::cerr << "NUMA not supported on this system!\n";
        return 1;
    }

    print_separator("TOPOLOGY DISCOVERY");
    int max_node = numa_max_node(); // Highest node ID
    int num_cpus = numa_num_configured_cpus();
    
    std::cout << "Max Node ID: " << max_node << std::endl;
    std::cout << "Total Configured CPUs: " << num_cpus << std::endl;

    for (int i = 0; i <= max_node; i++) {
        long long free_mem;
        long long total_mem = numa_node_size64(i, &free_mem);
        if (total_mem > 0) {
            // Total and free memory per node in Megabytes
            std::cout << "Node " << i << ": Total " << (total_mem >> 20) 
                      << " MB, Free " << (free_mem >> 20) << " MB\n";
        }
    }

    // 2. THREAD AFFINITY
    print_separator("THREAD AFFINITY");
    int target_node = 0; // Usually the primary socket
    std::cout << "Binding current thread to Node " << target_node << "...\n";
    numa_run_on_node(target_node);
    
    // 3. MEMORY ALLOCATION FACILITIES
    print_separator("ALLOCATION STRATEGIES");
    size_t alloc_size = 1024 * 1024; // 1MB buffer

    // Specific Node Allocation
    void* node_mem = numa_alloc_onnode(alloc_size, target_node);
    std::cout << "[Alloc On Node] Created 1MB on Node " << target_node << " at " << node_mem << std::endl;

    // Interleaved Allocation (Distributes pages across all available nodes)
    void* interleaved_mem = numa_alloc_interleaved(alloc_size);
    std::cout << "[Alloc Interleaved] Created 1MB striped across nodes at " << interleaved_mem << std::endl;

    // 4. BITMASK FACILITIES
    print_separator("BITMASK MANAGEMENT");
    // Bitmasks are used to define sets of nodes for policies
    struct bitmask* mask = numa_allocate_nodemask();
    numa_bitmask_setbit(mask, 0); 
    
    std::cout << "Bitmask created. Using it to set process-wide preference.\n";
    numa_set_bind_policy(0); // 0 = preferred, 1 = strict
    numa_set_membind(mask);

    // 5. CLEANUP
    print_separator("CLEANUP");
    numa_free(node_mem, alloc_size);
    numa_free(interleaved_mem, alloc_size);
    numa_bitmask_free(mask);
    
    std::cout << "All NUMA resources freed. Demo complete.\n";

    return 0;
}
