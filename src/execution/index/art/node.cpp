#include "duckdb/execution/index/art/node.hpp"

#include "duckdb/common/limits.hpp"
#include "duckdb/execution/index/art/art.hpp"
#include "duckdb/execution/index/art/leaf.hpp"
#include "duckdb/execution/index/art/node15_leaf.hpp"
#include "duckdb/execution/index/art/node16.hpp"
#include "duckdb/execution/index/art/node256.hpp"
#include "duckdb/execution/index/art/node256_leaf.hpp"
#include "duckdb/execution/index/art/node4.hpp"
#include "duckdb/execution/index/art/node48.hpp"
#include "duckdb/execution/index/art/node7_leaf.hpp"
#include "duckdb/execution/index/art/prefix.hpp"
#include "duckdb/storage/table_io_manager.hpp"

namespace duckdb {

//===--------------------------------------------------------------------===//
// New / Free
//===--------------------------------------------------------------------===//

void Node::New(ART &art, Node &node, NType type) {
	switch (type) {
	case NType::NODE_7_LEAF:
		Node7Leaf::New(art, node);
		break;
	case NType::NODE_15_LEAF:
		Node15Leaf::New(art, node);
		break;
	case NType::NODE_256_LEAF:
		Node256Leaf::New(art, node);
		break;
	case NType::NODE_4:
		Node4::New<Node4>(art, node, NType::NODE_4);
		break;
	case NType::NODE_16:
		Node16::New(art, node);
		break;
	case NType::NODE_48:
		Node48::New(art, node);
		break;
	case NType::NODE_256:
		Node256::New(art, node);
		break;
	default:
		throw InternalException("Invalid node type for New.");
	}
}

void Node::Free(ART &art, Node &node) {
	// Early-out, if the node is empty.
	if (!node.HasMetadata()) {
		return node.Clear();
	}

	// Free the children.
	auto type = node.GetType();
	switch (type) {
	case NType::PREFIX:
		return Prefix::Free(art, node);
	case NType::LEAF:
		return Leaf::DeprecatedFree(art, node);
	case NType::NODE_4:
		Node4::Free<Node4>(art, node);
		break;
	case NType::NODE_16:
		Node16::Free(art, node);
		break;
	case NType::NODE_48:
		Node48::Free(art, node);
		break;
	case NType::NODE_256:
		Node256::Free(art, node);
		break;
	case NType::LEAF_INLINED:
		return node.Clear();
	case NType::PREFIX_INLINED:
	case NType::NODE_7_LEAF:
	case NType::NODE_15_LEAF:
	case NType::NODE_256_LEAF:
		break;
	}

	GetAllocator(art, type).Free(node);
	node.Clear();
}

//===--------------------------------------------------------------------===//
// Get Allocators
//===--------------------------------------------------------------------===//

FixedSizeAllocator &Node::GetAllocator(const ART &art, const NType type) {
	return *(*art.allocators)[GetAllocatorIdx(type)];
}

uint8_t Node::GetAllocatorIdx(const NType type) {
	switch (type) {
	case NType::PREFIX:
		return 0;
	case NType::LEAF:
		return 1;
	case NType::NODE_4:
		return 2;
	case NType::NODE_16:
		return 3;
	case NType::NODE_48:
		return 4;
	case NType::NODE_256:
		return 5;
	case NType::PREFIX_INLINED:
		return 6;
	case NType::NODE_7_LEAF:
		return 7;
	case NType::NODE_15_LEAF:
		return 8;
	case NType::NODE_256_LEAF:
		return 9;
	default:
		throw InternalException("Invalid node type for GetAllocatorIdx.");
	}
}

//===--------------------------------------------------------------------===//
// Inserts
//===--------------------------------------------------------------------===//

void Node::ReplaceChild(const ART &art, const uint8_t byte, const Node child) const {
	D_ASSERT(HasMetadata());
	auto type = GetType();
	switch (GetType()) {
	case NType::NODE_4: {
		auto &n4 = Ref<Node4>(art, *this, type);
		return Node4::ReplaceChild(n4, byte, child);
	}
	case NType::NODE_16:
		return Ref<Node16>(art, *this, type).ReplaceChild(byte, child);
	case NType::NODE_48:
		return Ref<Node48>(art, *this, type).ReplaceChild(byte, child);
	case NType::NODE_256:
		return Ref<Node256>(art, *this, type).ReplaceChild(byte, child);
	default:
		throw InternalException("Invalid node type for ReplaceChild.");
	}
}

void Node::InsertChild(ART &art, Node &node, const uint8_t byte, const Node child) {
	D_ASSERT(node.HasMetadata());

	switch (node.GetType()) {
	case NType::NODE_4:
		return Node4::InsertChild(art, node, byte, child);
	case NType::NODE_16:
		return Node16::InsertChild(art, node, byte, child);
	case NType::NODE_48:
		return Node48::InsertChild(art, node, byte, child);
	case NType::NODE_256:
		return Node256::InsertChild(art, node, byte, child);
	case NType::NODE_7_LEAF:
		return Node7Leaf::InsertByte(art, node, byte);
	case NType::NODE_15_LEAF:
		return Node15Leaf::InsertByte(art, node, byte);
	case NType::NODE_256_LEAF:
		return Node256Leaf::InsertByte(art, node, byte);
	default:
		throw InternalException("Invalid node type for InsertChild.");
	}
}

//===--------------------------------------------------------------------===//
// Deletes
//===--------------------------------------------------------------------===//

void Node::DeleteChild(ART &art, Node &node, Node &prefix, const uint8_t byte) {
	D_ASSERT(node.HasMetadata());

	switch (node.GetType()) {
	case NType::NODE_4:
		return Node4::DeleteChild(art, node, prefix, byte);
	case NType::NODE_16:
		return Node16::DeleteChild(art, node, byte);
	case NType::NODE_48:
		return Node48::DeleteChild(art, node, byte);
	case NType::NODE_256:
		return Node256::DeleteChild(art, node, byte);
	case NType::NODE_7_LEAF:
		return Node7Leaf::DeleteByte(art, node, prefix, byte);
	case NType::NODE_15_LEAF:
		return Node15Leaf::DeleteByte(art, node, byte);
	case NType::NODE_256_LEAF:
		return Node256Leaf::DeleteByte(art, node, byte);
	default:
		throw InternalException("Invalid node type for DeleteChild.");
	}
}

//===--------------------------------------------------------------------===//
// Get functions
//===--------------------------------------------------------------------===//

const Node *Node::GetChild(ART &art, const uint8_t byte) const {
	D_ASSERT(HasMetadata());
	auto type = GetType();

	switch (GetType()) {
	case NType::NODE_4:
		return GetChildInternal<const Node4, const Node>(art, *this, byte, type);
	case NType::NODE_16:
		return GetChildInternal<const Node16, const Node>(art, *this, byte, type);
	case NType::NODE_48:
		return GetChildInternal<const Node48, const Node>(art, *this, byte, type);
	case NType::NODE_256:
		return GetChildInternal<const Node256, const Node>(art, *this, byte, type);
	default:
		throw InternalException("Invalid node type for GetChild.");
	}
}

Node *Node::GetChildMutable(ART &art, const uint8_t byte) const {
	D_ASSERT(HasMetadata());
	auto type = GetType();

	switch (GetType()) {
	case NType::NODE_4:
		return GetChildInternal<Node4, Node>(art, *this, byte, type);
	case NType::NODE_16:
		return GetChildInternal<Node16, Node>(art, *this, byte, type);
	case NType::NODE_48:
		return GetChildInternal<Node48, Node>(art, *this, byte, type);
	case NType::NODE_256:
		return GetChildInternal<Node256, Node>(art, *this, byte, type);
	default:
		throw InternalException("Invalid node type for GetChildMutable.");
	}
}

const Node *Node::GetNextChild(ART &art, uint8_t &byte) const {
	D_ASSERT(HasMetadata());
	auto type = GetType();

	switch (type) {
	case NType::NODE_4:
		return GetNextChildInternal<const Node4, const Node>(art, *this, byte, type);
	case NType::NODE_16:
		return GetNextChildInternal<const Node16, const Node>(art, *this, byte, type);
	case NType::NODE_48:
		return GetNextChildInternal<const Node48, const Node>(art, *this, byte, type);
	case NType::NODE_256:
		return GetNextChildInternal<const Node256, const Node>(art, *this, byte, type);
	default:
		throw InternalException("Invalid node type for GetNextChild.");
	}
}

Node *Node::GetNextChildMutable(ART &art, uint8_t &byte) const {
	D_ASSERT(HasMetadata());
	auto type = GetType();

	switch (GetType()) {
	case NType::NODE_4:
		return GetNextChildInternal<Node4, Node>(art, *this, byte, type);
	case NType::NODE_16:
		return GetNextChildInternal<Node16, Node>(art, *this, byte, type);
	case NType::NODE_48:
		return GetNextChildInternal<Node48, Node>(art, *this, byte, type);
	case NType::NODE_256:
		return GetNextChildInternal<Node256, Node>(art, *this, byte, type);
	default:
		throw InternalException("Invalid node type for GetNextChildMutable.");
	}
}

bool Node::GetNextByte(ART &art, uint8_t &byte) const {
	D_ASSERT(HasMetadata());

	switch (GetType()) {
	case NType::NODE_7_LEAF: {
		auto &n7_leaf = Ref<const Node7Leaf>(art, *this, NType::NODE_7_LEAF);
		return Node7Leaf::GetNextByte(n7_leaf, byte);
	}
	case NType::NODE_15_LEAF:
		return Ref<const Node15Leaf>(art, *this, NType::NODE_15_LEAF).GetNextByte(byte);
	case NType::NODE_256_LEAF:
		return Ref<Node256Leaf>(art, *this, NType::NODE_256_LEAF).GetNextByte(byte);
	default:
		throw InternalException("Invalid node type for GetNextByte.");
	}
}

//===--------------------------------------------------------------------===//
// Utility
//===--------------------------------------------------------------------===//

idx_t GetCapacity(NType type) {
	switch (type) {
	case NType::NODE_4:
		return Node::NODE_4_CAPACITY;
	case NType::NODE_7_LEAF:
		return Node::NODE_7_LEAF_CAPACITY;
	case NType::NODE_15_LEAF:
		return Node::NODE_15_LEAF_CAPACITY;
	case NType::NODE_16:
		return Node::NODE_16_CAPACITY;
	case NType::NODE_48:
		return Node::NODE_48_CAPACITY;
	case NType::NODE_256_LEAF:
		return Node::NODE_256_CAPACITY;
	case NType::NODE_256:
		return Node::NODE_256_CAPACITY;
	default:
		throw InternalException("Invalid node type for GetCapacity.");
	}
}

string Node::VerifyAndToString(ART &art, const bool only_verify) const {
	D_ASSERT(HasMetadata());

	auto type = GetType();
	switch (type) {
	case NType::LEAF_INLINED:
		return only_verify ? "" : "Inlined Leaf [count: 1, row ID: " + to_string(GetRowId()) + "]";
	case NType::LEAF:
		return Leaf::DeprecatedVerifyAndToString(art, *this, only_verify);
	case NType::PREFIX: {
		auto str = Prefix::VerifyAndToString(art, *this, only_verify);
		if (IsGate()) {
			str = "Gate [" + str + "]";
		}
		return only_verify ? "" : "\n" + str;
	}
	case NType::PREFIX_INLINED: {
		Prefix prefix(art, *this);
		string str = " Inlined Prefix:[";
		for (idx_t i = 0; i < prefix.data[Prefix::Count(art)]; i++) {
			str += to_string(prefix.data[i]) + "-";
		}
		str += "] ";
		D_ASSERT(!IsGate());
		return only_verify ? "" : "\n" + str;
	}
	default:
		break;
	}

	string str = "Node" + to_string(GetCapacity(type)) + ": [";
	uint8_t byte = 0;

	if (IsLeafNode()) {
		str = "Leaf " + str;
		auto has_byte = GetNextByte(art, byte);
		while (has_byte) {
			str += to_string(byte) + "-";
			if (byte == NumericLimits<uint8_t>::Maximum()) {
				break;
			}
			byte++;
			has_byte = GetNextByte(art, byte);
		}
	} else {
		auto child = GetNextChild(art, byte);
		while (child) {
			str += "(" + to_string(byte) + ", " + child->VerifyAndToString(art, only_verify) + ")";
			if (byte == NumericLimits<uint8_t>::Maximum()) {
				break;
			}
			byte++;
			child = GetNextChild(art, byte);
		}
	}

	if (IsGate()) {
		str = "Gate [" + str + "]";
	}
	return only_verify ? "" : "\n" + str + "]";
}

void Node::VerifyAllocations(ART &art, unordered_map<uint8_t, idx_t> &node_counts) const {
	D_ASSERT(HasMetadata());

	auto type = GetType();
	switch (type) {
	case NType::PREFIX:
		return Prefix::VerifyAllocations(art, *this, node_counts);
	case NType::LEAF:
		return Ref<Leaf>(art, *this, type).DeprecatedVerifyAllocations(art, node_counts);
	case NType::LEAF_INLINED:
		return;
	case NType::NODE_4:
		VerifyAllocationsInternal(art, Ref<Node4>(art, *this, type), node_counts);
		break;
	case NType::NODE_16:
		VerifyAllocationsInternal(art, Ref<Node16>(art, *this, type), node_counts);
		break;
	case NType::NODE_48:
		VerifyAllocationsInternal(art, Ref<Node48>(art, *this, type), node_counts);
		break;
	case NType::NODE_256:
		VerifyAllocationsInternal(art, Ref<Node256>(art, *this, type), node_counts);
		break;
	case NType::PREFIX_INLINED:
	case NType::NODE_7_LEAF:
	case NType::NODE_15_LEAF:
	case NType::NODE_256_LEAF:
		break;
	}

	auto idx = GetAllocatorIdx(type);
	node_counts[idx]++;
}

NType Node::GetNodeLeafType(idx_t count) {
	if (count <= NODE_7_LEAF_CAPACITY) {
		return NType::NODE_7_LEAF;
	} else if (count <= NODE_15_LEAF_CAPACITY) {
		return NType::NODE_15_LEAF;
	}
	return NType::NODE_256_LEAF;
}

NType Node::GetNodeType(idx_t count) {
	if (count <= NODE_4_CAPACITY) {
		return NType::NODE_4;
	} else if (count <= NODE_16_CAPACITY) {
		return NType::NODE_16;
	} else if (count <= NODE_48_CAPACITY) {
		return NType::NODE_48;
	}
	return NType::NODE_256;
}

//===--------------------------------------------------------------------===//
// Vacuum
//===--------------------------------------------------------------------===//

void Node::Vacuum(ART &art, const unordered_set<uint8_t> &indexes) {
	D_ASSERT(HasMetadata());

	auto node_type = GetType();
	switch (node_type) {
	case NType::LEAF_INLINED:
		return;
	case NType::PREFIX:
		return Prefix::Vacuum(art, *this, indexes);
	case NType::LEAF: {
		auto idx = GetAllocatorIdx(node_type);
		if (indexes.find(idx) == indexes.end()) {
			return;
		}
		return Leaf::DeprecatedVacuum(art, *this);
	}
	default:
		break;
	}

	auto idx = GetAllocatorIdx(node_type);
	auto &allocator = GetAllocator(art, node_type);
	auto needs_vacuum = indexes.find(idx) != indexes.end() && allocator.NeedsVacuum(*this);
	if (needs_vacuum) {
		*this = allocator.VacuumPointer(*this);
		SetMetadata(static_cast<uint8_t>(node_type));
	}

	switch (node_type) {
	case NType::NODE_4:
		return VacuumInternal(art, Ref<Node4>(art, *this, node_type), indexes);
	case NType::NODE_16:
		return VacuumInternal(art, Ref<Node16>(art, *this, node_type), indexes);
	case NType::NODE_48:
		return VacuumInternal(art, Ref<Node48>(art, *this, node_type), indexes);
	case NType::NODE_256:
		return VacuumInternal(art, Ref<Node256>(art, *this, node_type), indexes);
	case NType::NODE_7_LEAF:
	case NType::NODE_15_LEAF:
	case NType::NODE_256_LEAF:
	case NType::PREFIX_INLINED:
		return;
	default:
		throw InternalException("Invalid node type for Vacuum.");
	}
}

//===--------------------------------------------------------------------===//
// TransformToDeprecated
//===--------------------------------------------------------------------===//

void Node::TransformToDeprecated(ART &art, Node &node, unsafe_unique_ptr<FixedSizeAllocator> &allocator) {
	D_ASSERT(node.HasMetadata());

	if (node.IsGate()) {
		return Leaf::TransformToDeprecated(art, node);
	}

	auto node_type = node.GetType();
	switch (node_type) {
	case NType::PREFIX:
		return Prefix::TransformToDeprecated(art, node, allocator);
	case NType::LEAF_INLINED:
		return;
	case NType::LEAF:
		return;
	case NType::NODE_4: {
		auto ptr = InMemoryRef<Node4>(art, node, node_type);
		return TransformToDeprecatedInternal(art, ptr, allocator);
	}
	case NType::NODE_16: {
		auto ptr = InMemoryRef<Node16>(art, node, node_type);
		return TransformToDeprecatedInternal(art, ptr, allocator);
	}
	case NType::NODE_48: {
		auto ptr = InMemoryRef<Node48>(art, node, node_type);
		return TransformToDeprecatedInternal(art, ptr, allocator);
	}
	case NType::NODE_256: {
		auto ptr = InMemoryRef<Node256>(art, node, node_type);
		return TransformToDeprecatedInternal(art, ptr, allocator);
	}
	default:
		throw InternalException("Invalid node type for TransformToDeprecated.");
	}
}

} // namespace duckdb
