# pragma once

#include <eigen3/Eigen/Eigen>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <iostream>

class Mesh;


struct vertex_attribs {
    Eigen::Vector3f position;
    Eigen::Vector2f texcoord;
    Eigen::Vector3f normal;
    Eigen::Vector4f tangent;
    Eigen::Vector4f joint;
    Eigen::Vector4f weight;
};
class Mesh {
public:

    static std::unique_ptr<Mesh> load(const std::string& filename);

    // 禁用拷贝构造和拷贝赋值
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // 默认的移动构造和移动赋值是安全的
    Mesh(Mesh&&) = default;
    Mesh& operator=(Mesh&&) = default;
    ~Mesh() = default; 

    size_t getNumFaces() const;

    const std::vector<vertex_attribs>& getVertices() const;

    Eigen::Vector3f getCenter() const;

private:
    // 私有构造函数，强制通过静态 load 方法创建实例
    Mesh(std::vector<vertex_attribs>&& vertices, Eigen::Vector3f center);

    // 加载器
    static std::unique_ptr<Mesh> buildMesh(
        const std::vector<Eigen::Vector3f>& positions,
        const std::vector<Eigen::Vector2f>& texcoords,
        const std::vector<Eigen::Vector3f>& normals,
        const std::vector<Eigen::Vector4f>& tangents,
        const std::vector<Eigen::Vector4f>& joints,
        const std::vector<Eigen::Vector4f>& weights,
        const std::vector<int>& position_indices,
        const std::vector<int>& texcoord_indices,
        const std::vector<int>& normal_indices);
    static std::unique_ptr<Mesh> loadObj(const std::string& filename);

    // 成员变量
    std::vector<vertex_attribs> m_vertices;
    Eigen::Vector3f m_center;
};

class MeshCache{
public:
    static MeshCache& get_instance()
    {
        static MeshCache instance;
        return instance;
    }
    // 获取资源的 shared_ptr
    std::shared_ptr<Mesh> acquire(const std::string& filename)
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
        auto unique_resource = Mesh::load(filename);
        if (!unique_resource)
        {
            std::cerr << "Error: Failed to load resource " << filename << std::endl;
            return nullptr;
        }
        auto new_resource = std::shared_ptr<Mesh>(std::move(unique_resource));
        cache_[filename] = new_resource; // 存入 weak_ptr
        return new_resource;
    }
private:
    MeshCache() = default;
    ~MeshCache() = default;
    MeshCache(const MeshCache&) = delete;
    MeshCache& operator=(const MeshCache&) = delete;

    std::unordered_map<std::string, std::weak_ptr<Mesh>> cache_;
    std::mutex cache_mutex_;
};


inline std::shared_ptr<Mesh> cache_acquire_mesh(const std::string& filename) {
    return MeshCache::get_instance().acquire(filename);
}

