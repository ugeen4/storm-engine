#include "entity_container_cache.h"

#if (! defined __LCC__ )                    
#include <ranges>
#endif

void EntityContainerCache::Add(hash_t hash, entid_t id)
{
    // insert ordered
    auto &entry = cache_[hash];
#if (! defined __LCC__ )            
    const auto it = std::ranges::upper_bound(entry, id);
#else
    const auto it = std::upper_bound(entry.begin(), entry.end(), id);
#endif
    entry.insert(it, id);
}

void EntityContainerCache::UpdateAdd(hash_t hash, entid_t id)
{
    // check if exists
    if (auto entry = cache_.find(hash); entry != cache_.end())
    {
        // high dword is a timestamp. therefore newly added ent_id should always be greater than existing values
        entry->second.push_back(id);
    }
}

void EntityContainerCache::UpdateErase(hash_t hash, entid_t id)
{
    // check if exists
    if (auto entry = cache_.find(hash); entry != cache_.end())
    {
        // erase ordered
#if (! defined __LCC__ )            
        const auto it = std::ranges::lower_bound(entry->second, id);
#else
        const auto it = std::lower_bound(entry->second.begin(),entry->second.end(), id);
#endif        
        if (it != std::end(entry->second) && *it == id)
        {
            entry->second.erase(it);
        }
    }
}

void EntityContainerCache::Clear()
{
    cache_.clear();
}

bool EntityContainerCache::Contains(hash_t hash) const
{
#if (! defined __LCC__ )            
    return cache_.contains(hash);
#else
    auto search = cache_.find(hash);
    if (search != cache_.end()) return true;
    else                        return false;
#endif    
}

entity_container_cref EntityContainerCache::Get(hash_t hash)
{
    // no bound check
    return cache_.find(hash)->second;
}
