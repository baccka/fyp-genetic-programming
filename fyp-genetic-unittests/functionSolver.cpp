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

// The function that we're trying to find.
// (+ (* $0 $1) (- $1 (* $0 $0)))
int f(int x, int y) { return x * y + (y - x * x); }
	
static const Type fnType = type("int");

static const Grammar fnGrammar = Grammar({
	fnType
}, {
	terminal("parameter", fnType, 50),
	terminal("1", fnType, 50),
	binaryFunction("+", fnType, { fnType, fnType }, 50),
	binaryFunction("-", fnType, { fnType, fnType }, 50),
	binaryFunction("*", fnType, { fnType, fnType }, 50)
});

static const unsigned parameterCount = 2;

static int parameterId(const Definition &definition, const TreeGenome::Node &node) {
	assert(definition.getName() == std::string("parameter"));
	auto value = node.value - definition.getNodeValue();
	assert(value < definition.getWeight());
	auto rangeOfParameter = (definition.getWeight() / parameterCount);
	assert(rangeOfParameter * parameterCount == definition.getWeight());
	return value / rangeOfParameter;
}

// Custom printing for parameter terminals.
class PrinterDelegate: public TreeGenomePrinterDelegate {
public:
	bool printTerminal(const Definition &definition, const TreeGenome::Node &node, std::ostream &os) override {
		if (definition.getName() == std::string("parameter")) {
			os << "$" << parameterId(definition, node);
			return true;
		}
		return false;
	}
};

struct FnEvaluator : TreeGenomeEvaluator<int> {
	const std::vector<int> &parameters;
	unsigned parameter, one, add, sub, mul;
public:
	FnEvaluator(const std::vector<int> &parameters) : TreeGenomeEvaluator<int>(fnGrammar), parameters(parameters) {
		auto definitionDictionary = GrammarDefinitionAccessor(fnGrammar);
		parameter = definitionDictionary["parameter"].getDefinitionId();
		one = definitionDictionary["1"].getDefinitionId();
		add = definitionDictionary["+"].getDefinitionId();
		sub = definitionDictionary["-"].getDefinitionId();
		mul = definitionDictionary["*"].getDefinitionId();
	}

	int evaluteTerminal(unsigned definitionId, const TreeGenome::Node &node) override {
		if (definitionId == parameter) {
			return parameters[parameterId(fnGrammar[definitionId], node)];
		}
		assert(definitionId == one);
		return 1;
	}

	int evaluateBinaryFunction(unsigned definitionId, const TreeGenome::Node &node, int x, int y) override {
		if (definitionId == add) {
			return x + y;
		} else if (definitionId == sub) {
			return x - y;
		}
		assert(definitionId == mul);
		return x * y;
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
		FnEvaluator eval(parameters);
		return eval(tree);
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

void testFunctionSolver() {
	// Set up the parameters.
	EvolutionParameters params;
	params.rng = std::mt19937(42);
	params.mutationRate = 0.1;
	params.crossoverRate = 0.895;
	// Run the evolution.
	FnEvolver controller(params);
	controller.printerDelegate.reset(new PrinterDelegate());
	Population population(100, params, controller);
	RampedHalfAndHalfInitializer<EvolutionParameters::RNG> init(fnGrammar, params.rng);
	population.initialize(10, init);
	population.dump();
	for (int i = 0; i < 100; ++i) {
		population.nextGeneration();
	}
	population.evaluateGeneration();
	population.dump(false);
	auto stats = population.getStats();

	assert(population.generation == 100);
	assert(stats.bestFitness == 1.0f);

	const auto &best = population[stats.bestIndividual];
	TreeGenomePrinter printer(fnGrammar);
	std::stringstream ss;
	printer.print(best, ss, controller.printerDelegate.get());
	assert(ss.str() == "(+ (* (* (- $0 $0) (- $0 $0)) (- $0 $0)) (+ $0 (* (+ (* 1 1) $0) (- $1 $0))))");
}
