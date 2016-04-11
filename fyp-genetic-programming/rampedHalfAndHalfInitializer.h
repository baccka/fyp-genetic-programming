#pragma once

#include "treeGenerator.h"
#include "initializer.h"

namespace genetic {
	
/// An initializer delegate.
class RampedHalfAndHalfInitializerDelegate {
public:
	virtual bool generateFull(TreeGenerator<EvolutionParameters::RNG> &generator, TreeGenome::Builder &builder, int maxDepth) = 0;
	virtual bool generateGrow(TreeGenerator<EvolutionParameters::RNG> &generator, TreeGenome::Builder &builder, int maxDepth) = 0;
};

/// Implements ramped half and half GP tree initialization.
template<class RNG>
class RampedHalfAndHalfInitializer : public Initializer {
	TreeGenerator<RNG> gen;
	RampedHalfAndHalfInitializerDelegate *delegate;
public:

	RampedHalfAndHalfInitializer(const grammar::Grammar &grammar, RNG &rng, RampedHalfAndHalfInitializerDelegate *delegate = nullptr) : gen(grammar, rng), delegate(delegate) {
	}

	void initialize(const InitializationOptions &options, std::function<void (TreeGenome)> consumer) override {
		// Ramped half and half initialization.
		size_t size = options.populationSize, i = 0;
		float depth = 1;
		float depthDelta = float(options.maxTreeGenomeDepth) / (float(size)/2);
		for (; i < size/2; ++i, depth += depthDelta) {
			int currentDepth = int(floor(depth));
			TreeGenome genome;
			{
				TreeGenome::Builder builder(genome);
				if (delegate && delegate->generateFull(gen, builder, /*maxDepth=*/ currentDepth))
					;
				else
					gen.generateFull(builder, /*maxDepth=*/ currentDepth);
			}
			consumer(std::move(genome));
		}
		depth = 1;
		for (; i < size; ++i, depth += depthDelta) {
			int currentDepth = int(floor(depth));
			TreeGenome genome;
			{
				TreeGenome::Builder builder(genome);
				if (delegate && delegate->generateGrow(gen, builder, /*maxDepth=*/ currentDepth))
					;
				else
					gen.generateGrow(builder, /*maxDepth=*/ currentDepth);
			}
			consumer(std::move(genome));
		}
	}
};

} // end namespace genetic
