// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_RNG_DISCRETE_H
#define SST_CORE_RNG_DISCRETE_H

#include "sst/core/rng/distrib.h"
#include "math.h"
#include "sst/core/rng/mersenne.h"
#include "sst/core/rng/rng.h"
#include "sst/core/shared/sharedMap.h"

#include <cstdlib> // for malloc/free

/**
    \class DiscreteDistribution discrete.h "sst/core/rng/discrete.h"

    Creates a discrete distribution for use within SST. This distribution is the same across
    platforms and compilers.
*/
class DiscreteSharedDistribution : public SST::RNG::RandomDistribution
{

public:
    /**
        Creates a discrete probability distribution
        \param probs An array of probabilities for each outcome
        \param probsCount The number of discrete outcomes
    */
    DiscreteSharedDistribution(std::string name, std::map<uint64_t, double> probs, SST::RNG::Random* baseDist)
    {
        probabilities.initialize(name);

        double prob_sum = 0;

        for ( auto probs_kv: probs ) {
            probabilities.write(probs_kv.first, prob_sum);
            prob_sum += probs_kv.second;
        }

	probabilities.publish();

        baseDistrib = baseDist;
    }

    DiscreteSharedDistribution(std::string name, SST::RNG::Random* baseDist) {
        probabilities.initialize(name);

	probabilities.publish();

        baseDistrib = baseDist;
    }

    /**
        Destroys the exponential distribution
    */
    ~DiscreteSharedDistribution() { }

    /**
        Gets the next (random) double value in the distribution
        \return The next random double from the discrete distribution, this is the double converted of the index where
       the probability is located
    */
    double getNextDouble()
    {
        const double nextD = baseDistrib->nextUniform();

        uint32_t res = 0;

        for ( auto probs_kv: probabilities ) {
            if ( probs_kv.second >= nextD ) { res = probs_kv.first; break; }
        }

        return (double)res;
    }

protected:
    /**
        Sets the base random number generator for the distribution.
    */
    SST::RNG::Random* baseDistrib;

    /**
        The discrete probability list
    */
    SST::Shared::SharedMap<uint64_t, double> probabilities;
};

#endif // SST_CORE_RNG_DISCRETE_H
