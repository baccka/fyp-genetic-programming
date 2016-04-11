#include "geneticProgramming.h"
#include "grammar.h"
#include "treePrinter.h"
#include "treeEvaluator.h"
#include "rampedHalfAndHalfInitializer.h"
#include <iostream>
#include <sstream>
#include <cassert>

using namespace genetic;
using namespace grammar;

namespace {

// The functions that we're trying to find.
int f0(int x, int y) { return x * y - (y * y + x); }
int f(int x, int y) { return f0(x + 1 + 1, f0(x, y)) - f0(y, x * y); }

static const Type baseType = type("int-base");
static const Type fnType = type("int");
static const Type setType = type("function-set");

static const Grammar fnGrammar = Grammar({
	baseType, fnType, setType
}, {
	terminal("x", fnType, 25),
	terminal("y", fnType, 25),
	terminal("1", fnType, 50),
	binaryFunction("+", fnType, { fnType, fnType }, 50),
	binaryFunction("-", fnType, { fnType, fnType }, 50),
	binaryFunction("*", fnType, { fnType, fnType }, 50),
	binaryFunction("call", fnType, {fnType, fnType}, 200),
	
	terminal("x", baseType, 25),
	terminal("y", baseType, 25),
	terminal("1", baseType, 50),
	binaryFunction("+", baseType, { baseType, baseType }, 50),
	binaryFunction("-", baseType, { baseType, baseType }, 50),
	binaryFunction("*", baseType, { baseType, baseType }, 50),
	
	// F0 - base type, F1 - set type.
	binaryFunction("functions", setType, { baseType, fnType }, 50)
});

struct FnEvaluator : TreeGenomeEvaluator<int> {
	DefinitionSet x, y, one, add, sub, mul, call;
	int px, py;
	TreeGenome::Node baseFN;
public:
	FnEvaluator(TreeGenome::Node baseFN, int px, int py) : TreeGenomeEvaluator<int>(fnGrammar), px(px), py(py), baseFN(baseFN) {
		x = fnGrammar["x"];
		y = fnGrammar["y"];
		one = fnGrammar["1"];
		add = fnGrammar["+"];
		sub = fnGrammar["-"];
		mul = fnGrammar["*"];
		call = fnGrammar["call"];
	}
	
	int evaluteTerminal(unsigned definitionId, const TreeGenome::Node &node) override {
		if (x.contains(definitionId)) {
			return px;
		} else if (y.contains(definitionId)) {
			return py;
		}
		assert(one.contains(definitionId));
		return 1;
	}
	
	int evaluateBinaryFunction(unsigned definitionId, const TreeGenome::Node &node, int x, int y) override {
		if (add.contains(definitionId)) {
			return x + y;
		} else if (sub.contains(definitionId)) {
			return x - y;
		} else if (call.contains(definitionId)) {
			FnEvaluator evaluator(baseFN, x, y);
			return evaluator(baseFN);
		}
		assert(mul.contains(definitionId));
		return x * y;
	}
};
	
class InitDelegate: public RampedHalfAndHalfInitializerDelegate {
public:
	TreeGenomeType rootType;
	
	InitDelegate(TreeGenomeType rootType) : rootType(rootType) { }
	
	bool generateFull(TreeGenerator<EvolutionParameters::RNG> &generator, TreeGenome::Builder &builder, int maxDepth) override {
		generator.generateFull(builder, maxDepth, rootType);
		return true;
	}
	bool generateGrow(TreeGenerator<EvolutionParameters::RNG> &generator, TreeGenome::Builder &builder, int maxDepth) override {
		generator.generateGrow(builder, maxDepth, rootType);
		return true;
	}
};

class FnEvolver: public EvolvingPopulationDelegate {
public:
	EvolutionParameters &params;
	
	FnEvolver(EvolutionParameters &params) : params(params) {
	}
	
	TreeGenome generateRandomTreeOfType(TreeGenomeType type) override {
		TreeGenome genome;
		TreeGenerator<EvolutionParameters::RNG> generator(fnGrammar, params.rng);
		TreeGenome::Builder builder(genome);
		generator.generateGrow(builder, 2, type);
		return genome;
	}
	
	int evaluate(const TreeGenome &tree, const std::vector<int> &parameters) {
		assert(parameters.size() == 2);
		auto root = tree.first();
		const auto &rootDef = fnGrammar[root];
		assert(rootDef.getDefinitionId() == GrammarDefinitionAccessor(fnGrammar)["functions"].getDefinitionId());
		assert(rootDef.getNumArguments() == 2);
		FnEvaluator eval(root[0], parameters[0], parameters[1]);
		return eval(root[1]);
	}
	
	float computeFitnessForIndividual(const TreeGenome &i) {
		static const std::vector<std::vector<int>> parameters = { {1,2}, {4,5}, {6,7}, {8,9}, {10, 11}, {45, 11}, {450, 660}, {2017, 13} };
		float fitness = 0.0;
		for (const auto &p : parameters) {
			auto expectedAnswer = f(p[0], p[1]);
			auto answer = evaluate(i, p);
			fitness += 1.0f - (float(abs(answer - expectedAnswer)) / 1000.0f);
		}
		fitness /= float(parameters.size());
		// Penalize large trees.
		fitness -= log10f(ceil(float(i.getNodeCount()) / 30.0f));
		return fitness;
	}
	
	void computeFitness(const std::vector<TreeGenome> &individuals, std::vector<float> &fitnesses) override {
		for (size_t i = 0; i < individuals.size(); ++i) {
			fitnesses[i] = computeFitnessForIndividual(individuals[i]);
		}
	}
	
	const grammar::Grammar &genomeGrammar() override {
		return fnGrammar;
	}
};

} // end anonymous namespace
		
void testMultiFunctionSolver() {
	// Set up the parameters.
	EvolutionParameters params;
	params.rng = std::mt19937(42);
	params.mutationRate = 0.1;
	params.crossoverRate = 0.895;
	// Run the evolution.
	FnEvolver controller(params);
	Population population(100, params, controller);
	InitDelegate delegate(fnGrammar.typeByName("function-set"));
	{
		RampedHalfAndHalfInitializer<EvolutionParameters::RNG> init(fnGrammar, params.rng, &delegate);
		population.initialize(6, init);
	}
	population.dump();
	for (int i = 0; i < 100; ++i) {
		population.nextGeneration();
	}
	population.evaluateGeneration();
	population.dump(false);
	auto stats = population.getStats();
	
	assert(population.generation == 100);
	assert(stats.bestFitness > -50.0f);
	
	const auto &best = population[stats.bestIndividual];
	TreeGenomePrinter printer(fnGrammar);
	std::stringstream ss;
	printer.print(best, ss, controller.printerDelegate.get());
}
