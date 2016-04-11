#pragma once

#include "genome.h"
#include "initializer.h"
#include "grammar.h"
#include "treePrinter.h"
#include <random>
#include <vector>
#include <iostream>

namespace genetic {
	
template<class RNGType>
struct GenericEvolutionParameters {
	typedef RNGType RNG;
	/// The random number generator used for all random numbers.
	RNG rng = RNG(std::random_device()());
	/// The mutation rate.
	float mutationRate = 0.0f;
	/// The crossover rate.
	float crossoverRate = 0.0f;
};

/// The parameters that control the evolutionary process.
typedef GenericEvolutionParameters<std::mt19937> EvolutionParameters;

namespace utils {

/// Return a node id of a random node in a given genome.
template<typename T>
size_t selectRandomNode(const TreeGenome &genome, GenericEvolutionParameters<T> &params) {
	return std::uniform_int_distribution<size_t>(0, genome.getNodeCount() - 1)(params.rng);
}

}

///
class EvolvingPopulationDelegate {
public:
	std::unique_ptr<TreeGenomePrinterDelegate> printerDelegate;

	virtual void computeFitness(const std::vector<TreeGenome> &individuals, std::vector<float> &fitnesses) = 0;

	virtual TreeGenome generateRandomTreeOfType(TreeGenomeType type) = 0;

	virtual const grammar::Grammar &genomeGrammar() = 0;
};

// This class represents a population of individuals that have a tree-genome of a given type.
class Population {
private:
    std::vector<TreeGenome> individuals;
    std::vector<float> fitnesses;
	EvolutionParameters &params;
    EvolvingPopulationDelegate &traits;
	
	template<typename U>
	U random(U max) {
		return std::uniform_int_distribution<U>(0, max)(params.rng);
	}
	
	size_t selectRandomNode(const TreeGenome &genome) {
		return utils::selectRandomNode(genome, params);
	}

	void mutate(TreeGenome &genome) {
		auto nodeId = selectRandomNode(genome);
		const auto &grammar = traits.genomeGrammar();
		// Replace the node only with the node of the same type.
		genome.replace(nodeId, traits.generateRandomTreeOfType(grammar[genome[nodeId]].getType()));
	}
	
	std::pair<size_t, bool> selectRandomNodeWithType(const TreeGenome &genome, TreeGenomeType type) {
		const auto &grammar = traits.genomeGrammar();
		std::vector<size_t> nodes;
		for (size_t i = 0, e = genome.getNodeCount(); i < e; ++i) {
			const auto &def = grammar[genome[i]];
			if (def.getType() == type) {
				nodes.push_back(i);
			}
		}
		if (nodes.empty()) {
			return std::make_pair(0, false);
		}
		auto index = std::uniform_int_distribution<size_t>(0, nodes.size() - 1)(params.rng);
		return std::make_pair(nodes[index], true);
	}
	
	// Return true if crossover was successful. False is returned when the other genome
	// doesn't have any nodes that have the same type.
	bool crossover(TreeGenome &genome, size_t i, TreeGenomeType type, TreeGenome &other) {
		auto selection = selectRandomNodeWithType(other, type);
		if (!selection.second) {
			return false;
		}
		auto j = selection.first;
		auto x = genome.getSubTree(i), y = other.getSubTree(j);
		genome.replace(i, y);
		other.replace(j, x);
		return true;
	}
	
	size_t currentBestIndividualId = 0;
	int evaluatedGeneration = -1;
public:
    int generation = 0;
	
	Population(size_t size, EvolutionParameters &params, EvolvingPopulationDelegate &delegate) : params(params), traits(delegate) {
        assert(size != 0);
        fitnesses.resize(size);
    }
	
	const TreeGenome &operator [](size_t i) {
		return individuals[i];
	}

    void initialize(int maxDepth, Initializer &init) {
		InitializationOptions opts;
		opts.maxTreeGenomeDepth = maxDepth;
		opts.populationSize = fitnesses.size();
		init.initialize(opts, [this] (TreeGenome genome) {
			individuals.push_back(std::move(genome));
		});
    }
    
    void select(std::vector<TreeGenome> &newGeneration, size_t count) {
        assert(count != 0 && count <= individuals.size());
        for (size_t i = 0; i < count; ++i) {
            std::uniform_int_distribution<size_t> sampler(0, individuals.size() - 1);
			// 3 Tournament selection.
            size_t s[3] = { sampler(params.rng), sampler(params.rng), sampler(params.rng) };
            auto maxFitness = fitnesses[s[0]];
            auto selectedIndividual = s[0];
            for (unsigned j = 1; j < 3; ++j) {
                if (fitnesses[s[j]] > maxFitness) {
                    maxFitness = fitnesses[s[j]];
                    selectedIndividual = s[j];
                }
            }
            newGeneration.push_back(individuals[selectedIndividual].copy());
        }
    }

    struct Stats {
        float averageFitness;
        float bestFitness;
        size_t bestIndividual;
        
        Stats() {}
    };
    
    Stats getStats() {
        Stats result;
        size_t bestIndividual = 0;
        float bestFitness = fitnesses[0];
        result.averageFitness = 0;
        for (size_t i = 0; i < individuals.size(); ++i) {
            result.averageFitness += fitnesses[i];
            if (bestFitness < fitnesses[i]) {
                bestFitness = fitnesses[i];
                bestIndividual = i;
            }
        }
        result.averageFitness /= float(individuals.size());
        result.bestFitness = bestFitness;
        result.bestIndividual = bestIndividual;
        return result;
    }
    
    void dump(bool printIndividuals = true) {
        auto stats = getStats();
        std::cout << "-----\n";
        std::cout << "Generation:\t" << generation << "\n";
        std::cout << "Average fitness:\t" << stats.averageFitness << "\n";
        std::cout << "Best fitness:\t" << stats.bestFitness << "\n";
        std::cout << "Best individual:\t";
		TreeGenomePrinter printer(traits.genomeGrammar());
		printer.print(individuals[stats.bestIndividual], std::cout, traits.printerDelegate.get());
        std::cout << "\n";
        if (printIndividuals) {
			size_t idx = 0;
            for (const auto &i : individuals) {
                std::cout << "\t#" << idx << ":\t";
				TreeGenomePrinter printer(traits.genomeGrammar());
				printer.print(i, std::cout, traits.printerDelegate.get());
                std::cout << "\n";
				idx++;
            }
        }
        std::cout << "-----\n";
    }
    
    size_t evaluateGeneration() {
		if (evaluatedGeneration == generation) {
			return currentBestIndividualId;
		}
        // Evaluate the individuals.
		traits.computeFitness(individuals, fitnesses);
		assert(fitnesses.size() == individuals.size());
		size_t bestIndividual = 0;
		float bestFitness = fitnesses[0];
        for (size_t i = 0; i < individuals.size(); ++i) {
			const float fitness = fitnesses[i];
			if (bestFitness < fitness) {
                bestFitness = fitness;
                bestIndividual = i;
            }
        }
		currentBestIndividualId = bestIndividual;
		evaluatedGeneration = generation;
        return bestIndividual;
    }
    
    void nextGeneration(bool doDump = true) {
        auto bestIndividual = evaluateGeneration();
		
		if (doDump)
			dump(false);
        
        // Selection.
        std::vector<TreeGenome> newGeneration; newGeneration.reserve(individuals.size());
        // Add two elites for mutation / crossover.
        newGeneration.push_back(individuals[bestIndividual].copy());
        newGeneration.push_back(individuals[bestIndividual].copy());
        assert(params.mutationRate + params.crossoverRate <= 1.0);
        // Performs tournament selection for the rest.
        select(newGeneration, individuals.size() - 3);
        // Do mutation / crossover.
        for (size_t i = 0; i < newGeneration.size(); ++i) {
            std::uniform_real_distribution<float> sampler(0, 1);
            auto p = sampler(params.rng);
            if (p <= params.mutationRate) {
				mutate(newGeneration[i]);
            }
            else if (p <= params.mutationRate + params.crossoverRate) {
                auto next = (i + 1) != newGeneration.size() ? i + 1 : random(newGeneration.size() - 1);
                if (next == i) {
                    next = i - 1;
                }
				auto genomeIndex = selectRandomNode(newGeneration[i]);
				const auto type = traits.genomeGrammar()[newGeneration[i][genomeIndex]].getType();
				// TODO: Try 3 times.
				if (!crossover(newGeneration[i], genomeIndex, type, newGeneration[next])) {
					std::cout << "Error: failed to crossover because types couldn't be matched";
				}
                ++i;
            }
        }
        // Add the elite without mutation / crossover.
        newGeneration.push_back(individuals[bestIndividual].copy());
        
        individuals = std::move(newGeneration);
        ++generation;
    }
};

} // end namespace genetic
