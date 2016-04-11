#pragma once

#include <vector>
#include <cassert>

namespace genetic {
namespace core {

/// Tree ADT.
template <typename T>
class Tree {
    struct NodeStorage {
        T value;
        // The number of children that this tree node has.
        unsigned childCount;
        // The number of nodes contained in this sub-tree, including the current node.
        unsigned subTreeSize;
        
        NodeStorage(T value) : value(value), childCount(0), subTreeSize(1) {
        }
    };
    std::vector<NodeStorage> nodes;
    
    Tree(std::vector<NodeStorage> nodes) : nodes(std::move(nodes)) { }
    
    size_t addNode(T value) {
        nodes.push_back(NodeStorage(value));
        return nodes.size() - 1;
    }

	// This helper struct is used to walk the tree.
	struct Enumerator {
		const Tree<T> &tree;
		size_t nodeId;
	public:
		Enumerator(const Tree<T> &tree) : tree(tree), nodeId(0) {}
		Enumerator(Enumerator &&other) : tree(other.tree), nodeId(other.nodeId) { other.nodeId = tree.nodes.size(); }
		
		struct Node {
			T value;
			unsigned childCount;
		};
		
		size_t currentNodeId() const { return nodeId; }
		
		Node advance() {
			Node n;
			n.value = tree.nodes[nodeId].value;
			n.childCount = tree.nodes[nodeId].childCount;
			++nodeId;
			return n;
		}
	};
	
	// Return an enumerator that starts with the root node.
	Enumerator root() const {
		return Enumerator(*this);
	}
public:
    Tree() {
        nodes.reserve(100);
    }
	
    Tree(Tree<T> &&other) : nodes(std::move(other.nodes)) {
    }
	
    Tree<T> copy() const {
        return Tree<T>(std::vector<NodeStorage>(nodes.begin(), nodes.end()));
    }
    
    // Return the number of nodes in a tree.
    size_t getNodeCount() const { return nodes.size(); }
	
	struct Iterator;
	
	// A reference to a node in a tree.
	struct Node {
		const Tree<T> &tree;
		size_t nodeId;
	public:
		T value;
		
		Node(const Tree<T> &tree, size_t nodeId) : tree(tree), nodeId(nodeId), value(tree.nodes[nodeId].value) {
		}
		
		// Return the iterator with the first child.
		Iterator begin() const {
			return Iterator(tree, nodeId + 1);
		}
		
		// Return the iterator that tells when to stop iterating through children.
		Iterator end() const {
			return Iterator(tree, nodeId + tree.nodes[nodeId].subTreeSize);
		}
		
		size_t size() const {
			return tree.nodes[nodeId].childCount;
		}
		
		bool isEmpty() const {
			return size() == 0;
		}
		
		Node operator[] (size_t i) const {
			assert(i < size());
			auto it = begin();
			for (size_t j = 0; j < i; ++j, ++it);
			return *it;
		}
		
		// Return the first child.
		Node first() const {
			assert(size() > 0);
			return *begin();
		}
	};
	
	struct Iterator {
		const Tree<T> &tree;
		size_t nodeId;
	public:
		Iterator(const Tree<T> &tree, size_t nodeId) : tree(tree), nodeId(nodeId) { }

		Node operator *() const {
			assert(nodeId < tree.nodes.size());
			return Node(tree, nodeId);
		}
		
		bool operator == (const Iterator &other) const {
			return nodeId == other.nodeId;
		}
		
		bool operator != (const Iterator &other) const {
			return nodeId != other.nodeId;
		}
		
		Iterator &operator ++() {
			nodeId += tree.nodes[nodeId].subTreeSize;
			return *this;
		}
	};
	
	Node first() const {
		assert(nodes.size() > 0);
		return *begin();
	}
	
	Iterator begin() const {
		return Iterator(*this, 0);
	}
	
	Iterator end() const {
		return Iterator(*this, nodes.size());
	}
	
	Node operator [](size_t nodeId) const {
		assert(nodeId < nodes.size());
		return Node(*this, nodeId);
	}
	
    // Return the sub-tree that uses the node with the given node id as root.
    Tree<T> getSubTree(size_t subRootNodeId) {
        assert(subRootNodeId < nodes.size());
        return Tree<T>(std::vector<NodeStorage>(nodes.begin() + subRootNodeId, nodes.begin() + subRootNodeId + nodes[subRootNodeId].subTreeSize));
    }
    
    // Replace a sub-tree with the given node id by the given sub-tree.
    void replace(size_t nodeId, const Tree<T> &subTree) {
        assert(nodeId < nodes.size());
        nodes.erase(nodes.begin() + nodeId, nodes.begin() + nodeId + nodes[nodeId].subTreeSize);
        nodes.insert(nodes.begin() + nodeId, subTree.nodes.begin(), subTree.nodes.end());
        auto i = root();
        recomputeSubTreeSizes(i, nodeId, subTree.nodes.size());
        assert(nodes[0].subTreeSize == nodes.size());
    }
    
    /// Helper class that constructs trees.
    class Builder {
        Tree<T> &tree;
        std::vector<size_t> stack;
    public:
        Builder(Tree<T> &tree) : tree(tree) { }
        
        void push(T value) {
            if (!stack.empty())
                tree.nodes[stack.back()].childCount++;
            stack.push_back(tree.addNode(value));
        }
        
        void add(T value) {
            tree.addNode(value);
            if (!stack.empty()) {
                tree.nodes[stack.back()].childCount++;
                tree.nodes[stack.back()].subTreeSize++;
            }
        }
        
        void pop() {
            assert(!stack.empty());
            size_t size = tree.nodes[stack.back()].subTreeSize;
            stack.pop_back();
            // Propagate the sub-tree size up to the parent.
            if (!stack.empty())
                tree.nodes[stack.back()].subTreeSize += size;
        }
    };

private:
    unsigned recomputeSubTreeSizes(Enumerator &tree, size_t nodeId, unsigned newSubTreeSize) {
        auto parentNodeId = tree.currentNodeId();
        auto node = tree.advance();
        if (!node.childCount) { return 1; }
        unsigned subTreeSize = 1;
        for (unsigned i = 0; i < node.childCount; ++i) {
            subTreeSize += recomputeSubTreeSizes(tree, nodeId, newSubTreeSize);
        }
        nodes[parentNodeId].subTreeSize = subTreeSize;
        return subTreeSize;
    }
};
	
} // end namespace core
} // end namespace genetic
