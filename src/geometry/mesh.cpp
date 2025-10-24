#include "geometry/mesh.hpp"
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>




// 从解析出的数据构建最终的顶点缓冲区和 Mesh 对象
std::unique_ptr<Mesh> Mesh::buildMesh(
    const std::vector<Eigen::Vector3f>& positions,
    const std::vector<Eigen::Vector2f>& texcoords,
    const std::vector<Eigen::Vector3f>& normals,
    const std::vector<Eigen::Vector4f>& tangents,
    const std::vector<Eigen::Vector4f>& joints,
    const std::vector<Eigen::Vector4f>& weights,
    const std::vector<int>& position_indices,
    const std::vector<int>& texcoord_indices,
    const std::vector<int>& normal_indices) {

    Eigen::Vector3f bbox_min(+1e6f, +1e6f, +1e6f);
    Eigen::Vector3f bbox_max(-1e6f, -1e6f, -1e6f);
    size_t num_indices = position_indices.size();

    std::vector<vertex_attribs> final_vertices;
    final_vertices.reserve(num_indices);

    for (size_t i = 0; i < num_indices; ++i) {
        vertex_attribs v;
        int pos_idx = position_indices[i];
        int uv_idx = texcoord_indices[i];
        int n_idx = normal_indices[i];

        v.position = positions.at(pos_idx);
        v.texcoord = texcoords.at(uv_idx);
        v.normal = normals.at(n_idx);

        v.tangent = (tangents.empty()) ? Eigen::Vector4f(1, 0, 0, 1) : tangents.at(pos_idx);
        v.joint = (joints.empty()) ? Eigen::Vector4f(0, 0, 0, 0) : joints.at(pos_idx);
        v.weight = (weights.empty()) ? Eigen::Vector4f(0, 0, 0, 0) : weights.at(pos_idx);

        bbox_min = bbox_min.cwiseMin(v.position);
        bbox_max = bbox_max.cwiseMax(v.position);

        final_vertices.push_back(v);
    }

    Eigen::Vector3f center = (bbox_min + bbox_max) / 2.0f;

    return std::unique_ptr<Mesh>(new Mesh(std::move(final_vertices), center));
}

Mesh::Mesh(std::vector<vertex_attribs>&& vertices, Eigen::Vector3f center)
    : m_vertices(std::move(vertices)), m_center(center) {}

std::unique_ptr<Mesh> Mesh::load(const std::string& filename) {
    size_t dot_pos = filename.rfind('.');
    if (dot_pos == std::string::npos) {
        std::cerr << "Error: No file extension found in " << filename << std::endl;
        return nullptr;
    }

    std::string extension = filename.substr(dot_pos + 1);
    if (extension == "obj") {
        return loadObj(filename);
    } else {
        std::cerr << "Error: Unsupported mesh format '" << extension << "'" << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Mesh> Mesh::loadObj(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return nullptr;
    }

    std::vector<Eigen::Vector3f> positions;
    std::vector<Eigen::Vector2f> texcoords;
    std::vector<Eigen::Vector3f> normals;
    std::vector<Eigen::Vector4f> tangents;
    std::vector<Eigen::Vector4f> joints;
    std::vector<Eigen::Vector4f> weights;
    std::vector<int> position_indices, texcoord_indices, normal_indices;

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            Eigen::Vector3f p;
            ss >> p.x() >> p.y() >> p.z();
            positions.push_back(p);
        } else if (prefix == "vt") {
            Eigen::Vector2f uv;
            ss >> uv.x() >> uv.y();
            texcoords.push_back(uv);
        } else if (prefix == "vn") {
            Eigen::Vector3f n;
            ss >> n.x() >> n.y() >> n.z();
            normals.push_back(n);
        } else if (prefix == "f") {
            for (int i = 0; i < 3; ++i) {
                int p_idx, uv_idx, n_idx;
                char slash;
                ss >> p_idx >> slash >> uv_idx >> slash >> n_idx;
                position_indices.push_back(p_idx - 1);
                texcoord_indices.push_back(uv_idx - 1);
                normal_indices.push_back(n_idx - 1);
            }
        } else if (prefix == "#" && line.find("ext.tangent") != std::string::npos) {
            Eigen::Vector4f t;
            ss >> prefix >> t.x() >> t.y() >> t.z() >> t.w(); // "ext.tangent" is consumed
            tangents.push_back(t);
        } else if (prefix == "#" && line.find("ext.joint") != std::string::npos) {
            Eigen::Vector4f j;
            ss >> prefix >> j.x() >> j.y() >> j.z() >> j.w();
            joints.push_back(j);
        } else if (prefix == "#" && line.find("ext.weight") != std::string::npos) {
            Eigen::Vector4f w;
            ss >> prefix >> w.x() >> w.y() >> w.z() >> w.w();
            weights.push_back(w);
        }
    }

    return buildMesh(positions, texcoords, normals, tangents, joints, weights,
                     position_indices, texcoord_indices, normal_indices);
}

size_t Mesh::getNumFaces() const {
    return m_vertices.size() / 3;
}

const std::vector<vertex_attribs>& Mesh::getVertices() const {
    return m_vertices;
}

Eigen::Vector3f Mesh::getCenter() const {
    return m_center;
}