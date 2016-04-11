#pragma once

#include "genome.h"
#include "grammar.h"
#include <iostream>

namespace genetic {
	
// An abstract class that allows the users to provide custom compilation behaviour, like custom printing for terminal values.
class TreeGenomeCompilerDelegate {
public:
	virtual ~TreeGenomeCompilerDelegate() {}
	// The tree compiler calls this method before printing the given terminal node. If this method returns true, the tree printer won't print the given terminal node using the default behaviour.
	virtual bool printTerminal(const grammar::Definition &definition, const TreeGenome::Node &node, std::ostream &os) = 0;

	// The tree compiler calls this method before printing the given function node. If this method returns true, the tree printer won't print the given function node using the default behaviour.
	virtual bool printFunction(const grammar::Definition &definition, const TreeGenome::Node &node, std::ostream &os) = 0;

	virtual bool treeGenomeCompilerShouldPrintFunctionAsOperator(const grammar::Definition &definition) = 0;
};

// Converts the GP tree into a textual represenation of the stored program.
struct TreeGenomeCompiler {
	const grammar::Grammar &defs;
protected:
	TreeGenomeCompilerDelegate *delegate;
public:

	TreeGenomeCompiler(const grammar::Grammar &definitions, TreeGenomeCompilerDelegate *delegate = nullptr) : defs(definitions), delegate(delegate) { }
	
	void print(const TreeGenome::Node &node, std::ostream &os) {
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
		if (delegate && delegate->printFunction(definition, node, os))
			return;
		if (delegate && delegate->treeGenomeCompilerShouldPrintFunctionAsOperator(definition)) {
			/// Print an operator: `op X` or `X op Y`
			switch (node.size()) {
				case 1:
					os << "(" << definition.getName() << " ";
					print(node[0], os);
					os << ")";
					break;
				case 2:
					os << "(";
					print(node[0], os);
					os << " " << definition.getName() << " ";
					print(node[1], os);
					os << ")";
					break;
				default:
					assert(false && "Invalid operator grammar");
			}
			return;
		}
		/// Print a function call: `fn(arguments...)`
		os << definition.getName() << "(";
		bool isFirstArgument = true;
		for (auto child : node) {
			if (!isFirstArgument)
				os << ", ";
			print(child, os);
			isFirstArgument = false;
		}
		os << ")";
	}

	void print(const TreeGenome &tree, std::ostream &os) {
		for (auto node : tree) {
			print(node, os);
		}
	}
};
	
} // end namespace genetic
