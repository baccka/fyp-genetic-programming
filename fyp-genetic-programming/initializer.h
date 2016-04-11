#pragma once

#include "genome.h"
#include <functional>

namespace genetic {

/// Contains the options that are used to initialize the GP population.
struct InitializationOptions {
	int maxTreeGenomeDepth;
	size_t populationSize;
};

/// An abstract initializer.
class Initializer {
public:
	virtual void initialize(const InitializationOptions &options, std::function<void (TreeGenome)> consumer) = 0;
};
	
} // end namespace genetic