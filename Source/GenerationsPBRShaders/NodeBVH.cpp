#include "NodeBVH.h"

std::unique_ptr<Node> NodeBVH::build(const std::vector<const BuildCacheItem*>& items)
{
    if (items.empty())
        return nullptr;

    AABB aabb;

    for (auto& item : items)
        aabb.extend(item->aabb);

    const Eigen::AlignedVector3f center = aabb.center();

    std::unique_ptr<Node> node = std::make_unique<Node>();
    node->center = center;
    node->radius = getAABBRadius(aabb);

    if (items.size() == 1)
    {
        node->type = items[0]->type;
        node->data = items[0]->data;
        return node;
    }

    std::vector<const BuildCacheItem*> left;
    std::vector<const BuildCacheItem*> right;

    size_t dimIndex;
    aabb.sizes().maxCoeff(&dimIndex);

    for (auto& item : items)
    {
        if (item->center[dimIndex] < center[dimIndex])
            left.push_back(item);
        else
            right.push_back(item);
    }

    if (left.empty())
        std::swap(left, right);

    if (right.empty())
    {
        for (size_t i = 0; i < left.size(); i++)
        {
            if ((i & 1) == 0)
                continue;

            right.push_back(left.back());
            left.pop_back();
        }
    }

    node->left = build(left);
    node->right = build(right);

    if (node->left != nullptr && node->right == nullptr)
        return std::move(node->left);

    if (node->left == nullptr && node->right != nullptr)
        return std::move(node->right);

    return node;
}

void NodeBVH::traverse(const Frustum& frustum, const Node* node, void* userData, TraverseCallback* callback)
{
    if (node == nullptr || !frustum.intersects(node->center, node->radius))
        return;

    if (node->type != NodeType::Branch)
        callback(userData, *node);

    traverse(frustum, node->left.get(), userData, callback);
    traverse(frustum, node->right.get(), userData, callback);
}

void NodeBVH::add(const NodeType nodeType, void* data, const AABB& aabb)
{
    buildCache.push_back({nodeType, data, aabb, aabb.center() });
}

void NodeBVH::build()
{
    std::vector<const BuildCacheItem*> items;
    for (auto& item : buildCache)
        items.push_back(&item);

    node = build(items);

    std::vector<BuildCacheItem>().swap(buildCache);
}

void NodeBVH::reset()
{
    std::vector<BuildCacheItem>().swap(buildCache);
    node = nullptr;
}

void NodeBVH::traverse(const Frustum& frustum, void* userData, TraverseCallback* callback) const
{
    traverse(frustum, node.get(), userData, callback);
}
