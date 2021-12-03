#pragma once

enum class NodeType
{
    Branch = 0,
    IBLProbe = 1,
    SHLightField = 2,
    LocalLight = 3
};

class Node
{
public:
    NodeType type{};
    void* data{};
    Eigen::AlignedVector3f center;
    float radius{};
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
};

typedef void TraverseCallback(void* userData, const Node& node);

class NodeBVH
{
    struct BuildCacheItem
    {
        NodeType type {};
        void* data {};
        AABB aabb;
        Eigen::AlignedVector3f center;
    };

    std::vector<BuildCacheItem> buildCache;
    std::unique_ptr<Node> node;

    static std::unique_ptr<Node> build(const std::vector<const BuildCacheItem*>& items);
    static void traverse(const Frustum& frustum, const Node* node, void* userData, TraverseCallback* callback);

public:
    void add(NodeType nodeType, void* data, const AABB& aabb);
    void build();
    void reset();

    void traverse(const Frustum& frustum, void* userData, TraverseCallback* callback) const;
};
