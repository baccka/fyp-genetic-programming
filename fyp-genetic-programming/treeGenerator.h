#pragma once

#include "genome.h"
#include "grammar.h"
#include <random>

namespace genetic {
	
/// Generates random GP trees with the given grammar.
/// It's the base class for the GP tree initializers.
template<class RNG>
struct TreeGenerator {
private:
	const grammar::Grammar &grammar;
	RNG &rng;
	TreeGenomeValue terminalLimit, nodeLimit;

	TreeGenomeValue random(TreeGenomeValue min, TreeGenomeValue max) const {
		return std::uniform_int_distribution<TreeGenomeValue>(min, max)(rng);
	}
public:

	TreeGenerator(const grammar::Grammar &grammar, RNG &rng) : grammar(grammar), rng(rng) {
		terminalLimit = grammar.getTerminalLimit();
		nodeLimit = grammar.getNodeLimit();
		assert(nodeLimit == (grammar.getTerminalLimit() + grammar.getFunctionLimit()));
		assert(terminalLimit != 0);
	}

	/// Return a random tree node value that represents some terminal node.
	TreeGenomeValue randomTerminalValue() {
		return random(0, terminalLimit - 1);
	}

	/// Return a random tree node value that represents some function node.
	TreeGenomeValue randomFunctionValue() {
		return random(terminalLimit, nodeLimit - 1);
	}

	/// Return a random tree node value that represents either a terminal or a function node.
	TreeGenomeValue randomNodeValue() {
		return random(0, nodeLimit - 1);
	}
	
	TreeGenomeValue randomTerminalValueForDefinitonSet(const grammar::Grammar::TypeDefinitionSet &set) {
		return set.nodeValueForTypeConstrainedNodeValue(random(0, set.typeConstrainedTerminalLimit() - 1));
	}
	
	TreeGenomeValue randomFunctionValueForDefinitonSet(const grammar::Grammar::TypeDefinitionSet &set) {
		return set.nodeValueForTypeConstrainedNodeValue(random(set.typeConstrainedTerminalLimit(), set.typeConstrainedFunctionLimit() - 1));
	}
	
	TreeGenomeValue randomNodeValueForDefinitonSet(const grammar::Grammar::TypeDefinitionSet &set) {
		return set.nodeValueForTypeConstrainedNodeValue(random(0, set.typeConstrainedFunctionLimit() - 1));
	}

	enum class Strategy {
		Full,
		Grow
	};
	
	void generate(TreeGenome::Builder &builder, int maxDepth, Strategy strategy, TreeGenomeType type = grammar::Type::invalidTypeId) {
		bool hasType = type != grammar::Type::invalidTypeId;
		bool hasTerminals = hasType ? grammar.definitionSetForType(type).hasTerminals() : true;
		if (maxDepth <= 1 && hasTerminals) {
			builder.add(hasType ? randomTerminalValueForDefinitonSet(grammar.definitionSetForType(type)) : randomTerminalValue());
			return;
		}
		// FIXME: what if type has no functions?
		auto value = strategy == Strategy::Full ? (hasType ? randomFunctionValueForDefinitonSet(grammar.definitionSetForType(type)) : randomFunctionValue()) : (hasType ? randomNodeValueForDefinitonSet(grammar.definitionSetForType(type)) : randomNodeValue());
		const auto &def = grammar[grammar.definitionIdForTreeGenomeValue(value)];
		if (def.isTerminal()) {
			builder.add(value);
			return;
		}
		assert(def.isFunction() && def.getNumArguments() > 0);
		builder.push(value);
		for (unsigned i = 0, e = def.getNumArguments(); i < e; ++i) {
			generate(builder, maxDepth - 1, strategy, def.getTypeForArgument(i));
		}
		builder.pop();
	}

	/// Generate a tree that grows fully until it reaches the specified depth.
	void generateFull(TreeGenome::Builder &builder, int maxDepth, TreeGenomeType type = grammar::Type::invalidTypeId) {
		generate(builder, maxDepth, Strategy::Full, type);
	}

	/// Generate a tree that can grow until max depth, but doesn't have to.
	void generateGrow(TreeGenome::Builder &builder, int maxDepth, TreeGenomeType type = grammar::Type::invalidTypeId) {
		generate(builder, maxDepth, Strategy::Grow, type);
	}
};

}
