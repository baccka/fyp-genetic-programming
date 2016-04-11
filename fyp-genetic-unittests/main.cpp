#include "geneticProgramming.h"
#include "grammar.h"
#include "treePrinter.h"
#include "rampedHalfAndHalfInitializer.h"

#include <iostream>
#include <sstream>
#include <cassert>

namespace treeGenomeTest {

void testTreeIteration() {
	using namespace genetic::core;
	Tree<int> tree;

	// Construct a tree that looks like this:
	//               2
	//            /  |  \
	//           /   |   \
	//          11   42  90
	//              /|\
	//             / | \
	//            /  |  \
	//           13  0  9
	//                  |
	//                  7
	Tree<int>::Builder builder(tree);
	builder.push(2);
	builder.add(11);
	builder.push(42);
	builder.add(13);
	builder.add(0);
	builder.push(9);
	builder.add(7);
	builder.pop();
	builder.pop();
	builder.add(90);
	builder.pop();
	
	// Accesses.
	// Root depth.
	auto root = tree.first();
	assert(root.value == 2);
	assert(root.size() == 3);
	assert(root.first().value == 11);
	// Level 1.
	auto x0 = root[0];
	assert(x0.value == 11);
	assert(x0.size() == 0);
	assert(x0.isEmpty());
	auto x1 = root[1];
	assert(x1.value == 42);
	assert(x1.size() == 3);
	auto x2 = root[2];
	assert(x2.value == 90);
	assert(x2.size() == 0);
	assert(x2.isEmpty());
	// Level 2.
	auto y1 = x1[0];
	auto y2 = x1[1];
	auto y3 = x1[2];
	assert(y1.value == 13);
	assert(y1.isEmpty());
	assert(y2.value == 0);
	assert(y2.isEmpty());
	assert(y3.value == 9);
	assert(y3.size() == 1);
	// Level 3.
	auto z1 = y3[0];
	assert(z1.value == 7);
	assert(z1.isEmpty());
	
	
	// Depth first iteration
	struct Walker {
		std::vector<int> result;
		
		void fn(const Tree<int>::Node &node) {
			result.push_back(node.value);
			for (auto child : node) {
				fn(child);
			}
		}
	};
	Walker visitor;
	for (auto v : tree) {
		visitor.fn(v);
	}
	auto result = visitor.result;
	assert(result.size() == 8);
	assert(result[0] == 2);
	assert(result[1] == 11);
	assert(result[2] == 42);
	assert(result[3] == 13);
	assert(result[4] == 0);
	assert(result[5] == 9);
	assert(result[6] == 7);
	assert(result[7] == 90);
}
	
void testTreeGenomeGrammar() {
	using namespace genetic;
	using namespace genetic::grammar;

	const Type t = type("int");
	Grammar grammar = Grammar({ t }, {
		terminal("x", t, 10),
		terminal("y", t, 10),
		binaryFunction("+", t, {t, t}, 5),
		binaryFunction("*", t, {t, t}, 11),
		unaryFunction("sin", t, t, 3),
	});
	// Definitions.
	auto definitionDictionary = GrammarDefinitionAccessor(grammar);
	const auto &x = definitionDictionary["x"];
	const auto &y = definitionDictionary["y"];
	const auto &add = definitionDictionary["+"];
	const auto &mul = definitionDictionary["*"];
	const auto &sin = definitionDictionary["sin"];
	
	assert(grammar.getTerminalLimit() == 20);
	assert(grammar.getFunctionLimit() == 19);
	assert(grammar.getNodeLimit() == 39);
	
	assert(x.getName() == std::string("x"));
	assert(x.getDefinitionId() == 0);
	assert(x.getNodeValue() == 0);
	assert(x.getNumArguments() == 0);
	assert(x.isTerminal());
	assert(grammar.definitionIdForTreeGenomeValue(x.getNodeValue()) == x.getDefinitionId());

	assert(y.getName() == std::string("y"));
	assert(y.getDefinitionId() == 1);
	assert(y.getNodeValue() == 10);
	assert(y.getNumArguments() == 0);
	assert(y.isTerminal());
	assert(grammar.definitionIdForTreeGenomeValue(y.getNodeValue()) == y.getDefinitionId());
	
	assert(add.getName() == std::string("+"));
	assert(add.getDefinitionId() == 2);
	assert(add.getNodeValue() == 20);
	assert(add.getNumArguments() == 2);
	assert(add.isFunction());
	assert(grammar.definitionIdForTreeGenomeValue(add.getNodeValue()) == add.getDefinitionId());

	assert(mul.getName() == std::string("*"));
	assert(mul.getDefinitionId() == 3);
	assert(mul.getNodeValue() == 25);
	assert(mul.getNumArguments() == 2);
	assert(mul.isFunction());
	assert(grammar.definitionIdForTreeGenomeValue(mul.getNodeValue()) == mul.getDefinitionId());
	
	assert(sin.getName() == std::string("sin"));
	assert(sin.getDefinitionId() == 4);
	assert(sin.getNodeValue() == 36);
	assert(sin.getNumArguments() == 1);
	assert(sin.isFunction());
	assert(grammar.definitionIdForTreeGenomeValue(sin.getNodeValue()) == sin.getDefinitionId());
}
	
void testTreeGenomeTypedGrammar() {
	using namespace genetic;
	using namespace genetic::grammar;
	
	const Type scalar = type("float");
	const Type vec = type("float3");
	Grammar grammar = Grammar({ scalar, vec }, {
		terminal("x", scalar, 10),
		terminal("randomColor", vec, 5),
		terminal("y", scalar, 10),
		terminal("orange", vec, 1),

		binaryFunction("+", scalar, {scalar, scalar}, 5),
		ternaryFunction("rgb", vec, { scalar, scalar, scalar }, 5),
		binaryFunction("darker", vec, { vec, scalar }, 2),
		binaryFunction("*", scalar, {scalar, scalar}, 11),
		binaryFunction("lighter", vec, { vec, scalar }, 2),
		unaryFunction("sin", scalar, scalar, 3),
		unaryFunction("grayscale", vec, vec, 8),
		unaryFunction("cos", scalar, scalar, 6)
	});
	TreeGenomeType scalarType = grammar.typeByName("float");
	TreeGenomeType vectorType = grammar.typeByName("float3");
	assert(scalarType == 0);
	assert(vectorType == 1);
	assert(grammar.typeCount() == 2);

	// Verify that the definitions are partioned by kind and types in the expected order.
	std::vector<std::string> definitionOrder = {
		// Scalar terminals.
		"x", "y",
		// Vector terminals.
		"randomColor", "orange",
		// Scalar functions.
		"+", "*", "sin", "cos",
		// Vector functions.
		"rgb", "darker", "lighter", "grayscale"
	};
	unsigned definitionId = 0;
	TreeGenomeValue value = 0;
	auto definitionDictionary = GrammarDefinitionAccessor(grammar);
	for (auto &&name : definitionOrder) {
		const auto &def = definitionDictionary[name];
		assert(def.getName() == name);
		assert(def.getDefinitionId() == definitionId);
		assert(def.getNodeValue() == value);
		if (definitionId < 2) {
			assert(def.getType() == scalarType);
		} else if (definitionId < 4) {
			assert(def.getType() == vectorType);
		} else if (definitionId < 8) {
			assert(def.getType() == scalarType);
		} else {
			assert(def.getType() == vectorType);
		}
		definitionId += 1;
		value += def.getWeight();
	}

	auto vectorize = [](IteratorRange<std::vector<Definition>::const_iterator> range) -> std::vector<Definition> {
		return std::vector<Definition>(range.begin(), range.end());
	};
	auto scalarTerminals = vectorize(grammar.terminalsForType(scalarType));
	auto vectorTerminals = vectorize(grammar.terminalsForType(vectorType));
	auto scalarFunctions = vectorize(grammar.functionsForType(scalarType));
	auto vectorFunctions = vectorize(grammar.functionsForType(vectorType));

	assert(scalarTerminals.size() == 2);
	assert(scalarTerminals[0].getName() == std::string("x"));
	assert(scalarTerminals[1].getName() == std::string("y"));
	assert(vectorTerminals.size() == 2);
	assert(vectorTerminals[0].getName() == std::string("randomColor"));
	assert(vectorTerminals[1].getName() == std::string("orange"));
	assert(scalarFunctions.size() == 4);
	assert(scalarFunctions[0].getName() == std::string("+"));
	assert(scalarFunctions[1].getName() == std::string("*"));
	assert(scalarFunctions[2].getName() == std::string("sin"));
	assert(scalarFunctions[3].getName() == std::string("cos"));
	assert(vectorFunctions.size() == 4);
	assert(vectorFunctions[0].getName() == std::string("rgb"));
	assert(vectorFunctions[1].getName() == std::string("darker"));
	assert(vectorFunctions[2].getName() == std::string("lighter"));
	assert(vectorFunctions[3].getName() == std::string("grayscale"));
	
	// TODO: global definition set.
	auto globalSet = grammar.definitionSetForType(Type::invalidTypeId);
	assert(globalSet.typeConstrainedTerminalLimit() == 26);
	assert(globalSet.typeConstrainedFunctionLimit() == 68);
	auto scalarSet = grammar.definitionSetForType(scalarType);
	auto vectorSet = grammar.definitionSetForType(vectorType);
	assert(scalarSet.hasTerminals() && scalarSet.hasFunctions());
	assert(scalarSet.typeConstrainedTerminalLimit() == 20);
	assert(scalarSet.typeConstrainedFunctionLimit() == 45);
	assert(scalarSet.nodeValueForTypeConstrainedNodeValue(0) == definitionDictionary["x"].getNodeValue());
	assert(scalarSet.nodeValueForTypeConstrainedNodeValue(10) == definitionDictionary["y"].getNodeValue());
	assert(scalarSet.nodeValueForTypeConstrainedNodeValue(20) == definitionDictionary["+"].getNodeValue());
	assert(vectorSet.hasTerminals() && vectorSet.hasFunctions());
	assert(vectorSet.typeConstrainedTerminalLimit() == 6);
	assert(vectorSet.typeConstrainedFunctionLimit() == 23);
	assert(vectorSet.nodeValueForTypeConstrainedNodeValue(0) == definitionDictionary["randomColor"].getNodeValue());
	assert(vectorSet.nodeValueForTypeConstrainedNodeValue(6) == definitionDictionary["rgb"].getNodeValue());
}

void testTreeGenomePrinter() {
	using namespace genetic;
	using namespace genetic::grammar;
	
	const Type t = type("int");
	Grammar grammar = Grammar({ t }, {
		terminal("x", t, 10),
		terminal("y", t, 10),
		binaryFunction("+", t, {t, t}, 5),
		binaryFunction("*", t, {t, t}, 11),
		unaryFunction("sin", t, t, 3)
	});
	auto definitionDictionary = GrammarDefinitionAccessor(grammar);
	unsigned x = definitionDictionary["x"].getNodeValue();
	unsigned y = definitionDictionary["y"].getNodeValue();
	unsigned add = definitionDictionary["+"].getNodeValue();
	unsigned mul = definitionDictionary["*"].getNodeValue();
	unsigned sin = definitionDictionary["sin"].getNodeValue();
	
	TreeGenome tree;
	TreeGenome::Builder builder(tree);
	builder.push(add);
	builder.push(sin); builder.add(x); builder.pop();
	builder.push(mul); builder.add(y); builder.push(sin); builder.add(y); builder.pop(); builder.pop();
	builder.pop();

	std::ostringstream ss;
	TreeGenomePrinter printer(grammar);
	printer.print(tree, ss);
	assert(ss.str() == "(+ (sin x) (* y (sin y)))");
}
	
void testRampedHalfAndHalfInitializer() {
	using namespace genetic;
	using namespace genetic::grammar;
	
	const Type t = type("int");
	Grammar grammar = Grammar({ t }, {
		terminal("x", t, 10),
		terminal("y", t, 10),
		binaryFunction("+", t, {t, t}, 5),
		binaryFunction("*", t, {t, t}, 5),
		ternaryFunction("rgb", t, {t, t, t}, 2)
	});
	auto definitionDictionary = GrammarDefinitionAccessor(grammar);
	auto rgb = definitionDictionary["rgb"].getNodeValue();
	
	class Delegate: public RampedHalfAndHalfInitializerDelegate {
	public:
		unsigned rgb;

		Delegate(unsigned rgb) : rgb(rgb) { }

		bool generateFull(TreeGenerator<EvolutionParameters::RNG> &generator, TreeGenome::Builder &builder, int maxDepth) override {
			builder.push(rgb);
			for (unsigned i = 0; i < 3; ++i) {
				generator.generateFull(builder, maxDepth);
			}
			builder.pop();
			return true;
		}
		bool generateGrow(TreeGenerator<EvolutionParameters::RNG> &generator, TreeGenome::Builder &builder, int maxDepth) override {
			builder.push(rgb);
			for (unsigned i = 0; i < 3; ++i) {
				generator.generateGrow(builder, maxDepth);
			}
			builder.pop();
			return true;
		}
	};

	auto rng = std::mt19937(11);
	auto delegate = Delegate(rgb);
	{
		RampedHalfAndHalfInitializer<EvolutionParameters::RNG> init(grammar, rng, &delegate);
		InitializationOptions options;
		options.maxTreeGenomeDepth = 1;
		options.populationSize = 2;
		unsigned count = 0;
		init.initialize(options, [&] (TreeGenome genome) {
			assert(count < 2);
			auto root = genome.first();
			assert(root.value == rgb);
			assert(root.size() == 3);
			for (auto &&node : root) {
				assert(node.size() == 0);
			}
			++count;
		});
		assert(count == 2);
	}
}

enum TestNode {
	Plus,
	One,
	Zero
};
typedef genetic::core::Tree<TestNode> TestTreeType;

std::string description(const TestTreeType::Node &node) {
	switch (node.value) {
		case Plus: {
			std::string result = "(+ ";
			for (size_t i = 0; i < node.size(); ++i) {
				if (i) result += " ";
				result += description(node[i]);
			}
			result += ")";
			return result;
		}
		case One:
			return "1";
		case Zero:
			return "0";
	}
}

std::string description(const TestTreeType &tree) {
	return description(tree.first());
}

void testGenome() {
    {
        TestTreeType genome;
        TestTreeType::Builder builder(genome);
        builder.push(Plus); builder.add(One); builder.add(Zero); builder.pop();
        assert(genome.getNodeCount() == 3);
        assert(description(genome) == "(+ 1 0)");
    }
    {
        TestTreeType genome;
        TestTreeType::Builder builder(genome);
        builder.push(Plus); builder.push(Plus); builder.add(One); builder.add(One); builder.pop(); builder.add(Zero); builder.pop();
        assert(genome.getNodeCount() == 5);
        assert(description(genome) == "(+ (+ 1 1) 0)");
    }
    {
        TestTreeType genome;
        TestTreeType::Builder builder(genome);
        builder.push(Plus); builder.push(Plus); builder.add(One); builder.add(One); builder.pop(); builder.add(Zero); builder.pop();
        auto subTree = genome.getSubTree(1);
        assert(genome.getNodeCount() == 5);
        assert(description(genome) == "(+ (+ 1 1) 0)");
        assert(subTree.getNodeCount() == 3);
        assert(description(subTree) == "(+ 1 1)");
        genome.replace(4, subTree);
        assert(genome.getNodeCount() == 7);
        assert(description(genome) == "(+ (+ 1 1) (+ 1 1))");
        assert(subTree.getNodeCount() == 3);
        assert(description(subTree) == "(+ 1 1)");
        genome.replace(0, subTree);
        assert(genome.getNodeCount() == 3);
        assert(description(genome) == "(+ 1 1)");
        assert(subTree.getNodeCount() == 3);
        assert(description(subTree) == "(+ 1 1)");
        TestTreeType zero;
        TestTreeType::Builder builder2(zero);
        builder2.add(Zero);
        genome.replace(2, zero);
        assert(genome.getNodeCount() == 3);
        assert(description(genome) == "(+ 1 0)");
        genome.replace(1, zero);
        assert(genome.getNodeCount() == 3);
        assert(description(genome) == "(+ 0 0)");
        genome.replace(2, subTree);
        assert(genome.getNodeCount() == 5);
        assert(description(genome) == "(+ 0 (+ 1 1))");
        auto zero2 = genome.getSubTree(1);
        assert(zero2.getNodeCount() == 1);
        assert(description(zero2) == "0");
        genome.replace(2, zero2);
        assert(genome.getNodeCount() == 3);
        assert(description(genome) == "(+ 0 0)");
    }
    {
        TestTreeType genome;
        TestTreeType::Builder builder(genome);
        builder.push(Plus); builder.push(Plus); builder.add(One); builder.add(One); builder.pop(); builder.add(Zero); builder.pop();
        assert(genome.getNodeCount() == 5);
        assert(description(genome) == "(+ (+ 1 1) 0)");
        auto rootSubTree = genome.getSubTree(0);
        assert(rootSubTree.getNodeCount() == 5);
        assert(description(rootSubTree) == "(+ (+ 1 1) 0)");
        auto subTree = genome.getSubTree(1);
        genome.replace(2, subTree);
        assert(genome.getNodeCount() == 7);
        assert(description(genome) == "(+ (+ (+ 1 1) 1) 0)");
        auto rootSubTree2 = genome.getSubTree(0);
        assert(rootSubTree2.getNodeCount() == 7);
        assert(description(rootSubTree2) == "(+ (+ (+ 1 1) 1) 0)");
    }
}

} // end namespace treeGenomeTest

void testFunctionSolver();
void testMultiFunctionSolver();

int main(int argc, const char * argv[]) {
	// Core tree tests.
    treeGenomeTest::testGenome();
	treeGenomeTest::testTreeIteration();
	treeGenomeTest::testTreeGenomeGrammar();
	treeGenomeTest::testTreeGenomeTypedGrammar();
	treeGenomeTest::testTreeGenomePrinter();
	treeGenomeTest::testRampedHalfAndHalfInitializer();

	// Test GP solvers.
    testFunctionSolver();
	testMultiFunctionSolver();
    return 0;
}
