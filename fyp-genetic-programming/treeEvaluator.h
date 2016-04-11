#pragma once

#include "genome.h"
#include "grammar.h"
#include <vector>

namespace genetic {
	
// Evaluates the GP tree.
template<typename T>
struct TreeGenomeEvaluator {
	const grammar::Grammar &defs;
public:

	TreeGenomeEvaluator(const grammar::Grammar &definitions) : defs(definitions) {
	}

	T operator()(const TreeGenome::Node &node) {
		const auto &definition = defs[node];
		auto dID = definition.getDefinitionId();
		if (definition.isTerminal()) {
			assert(node.isEmpty());
			return evaluteTerminal(dID, node);
		}
		assert(node.size() == definition.getNumArguments());
		std::vector<T> arguments;
		for (auto child : node) {
			arguments.push_back((*this)(child));
		}
		switch (arguments.size()) {
			case 1:
				return evaluateUnaryFunction(dID, node, arguments[0]);
			case 2:
				return evaluateBinaryFunction(dID, node, arguments[0], arguments[1]);
			default:
				return evaluateFunction(dID, node, arguments);
		}
	}

	T operator()(const TreeGenome &tree){
		return (*this)(tree.first());
	}

	virtual T evaluteTerminal(unsigned definitionId, const TreeGenome::Node &node) = 0;
	virtual T evaluateUnaryFunction(unsigned definitionId, const TreeGenome::Node &node, T x) {
		return x;
	}
	virtual T evaluateBinaryFunction(unsigned definitionId, const TreeGenome::Node &node, T x, T y) {
		return T();
	}
	virtual T evaluateFunction(unsigned definitionId, const TreeGenome::Node &node, const std::vector<T> &arguments) {
		return T();
	}
};
	
} // end namespace genetic
