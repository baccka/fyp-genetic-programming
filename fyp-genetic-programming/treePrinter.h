#pragma once

#include "genome.h"
#include "grammar.h"
#include <iostream>

namespace genetic {

// An abstract class that allows the users to provide custom printing behaviour, like custom printing for terminal values.
class TreeGenomePrinterDelegate {
public:
	virtual ~TreeGenomePrinterDelegate() {}
	// The tree printer calls this method before printing the given terminal node. If this method returns true, the tree printer won't print the given terminal node using the default behaviour.
	virtual bool printTerminal(const grammar::Definition &definition, const TreeGenome::Node &node, std::ostream &os) = 0;
};

// Prints out the GP tree.
struct TreeGenomePrinter {
	const grammar::Grammar &defs;
public:

	TreeGenomePrinter(const grammar::Grammar &definitions) : defs(definitions) { }

	void print(const TreeGenome::Node &node, std::ostream &os, TreeGenomePrinterDelegate *delegate = nullptr) {
		const auto &definition = defs[node];
		if (definition.isTerminal()) {
			assert(node.isEmpty());
			if (delegate && delegate->printTerminal(definition, node, os)) {
				return;
			}
			os << definition.getName();
			return;
		}
		assert(node.size() == definition.getNumArguments());
		os << "(" << definition.getName();
		for (auto child : node) {
			os << " ";
			print(child, os, delegate);
		}
		os << ")";
	}

	void print(const TreeGenome &tree, std::ostream &os, TreeGenomePrinterDelegate *delegate = nullptr) {
		for (auto node : tree) {
			print(node, os, delegate);
		}
	}
};

} // end namespace genetic
