#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>

template <typename Resource>
class ResourceCache
{
public:
    // 获取单例实例
    static ResourceCache& get_instance()
    {
        static ResourceCache instance;
        return instance;
    }

    // 获取资源的 shared_ptr
    std::shared_ptr<Resource> acquire(const std::string& filename)
    {
        if (filename.empty())
        {
            return nullptr;
        }

        // 加锁以保证线程安全
        std::lock_guard<std::mutex> lock(cache_mutex_);

        auto it = cache_.find(filename);
        if (it != cache_.end())
        {
            if (auto shared = it->second.lock())
            {
                // 缓存命中且对象仍然存活
                return shared;
            }
        }
        auto unique_resource = Resource::load(filename);
        if (!unique_resource)
        {
            std::cerr << "Error: Failed to load resource " << filename << std::endl;
            return nullptr;
        }
        auto new_resource = std::shared_ptr<Resource>(std::move(unique_resource));
        cache_[filename] = new_resource; // 存入 weak_ptr
        return new_resource;
    }

protected:
    ResourceCache() = default;
    ~ResourceCache() = default;
    ResourceCache(const ResourceCache&) = delete;
    ResourceCache& operator=(const ResourceCache&) = delete;

    std::unordered_map<std::string, std::weak_ptr<Resource>> cache_;
    std::mutex cache_mutex_;
};