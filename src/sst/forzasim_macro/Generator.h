#include "sst/core/rng/uniform.h"
#include "sst/core/rng/expon.h"
#include "sst/forzasim_macro/discrete_shared.h"

class DistGenerator {
  public:
    virtual ~DistGenerator() = default;

    virtual double generate_objects_single() = 0;

  protected:
    SST::RNG::RandomDistribution* dist;
};

class UniformDistV2 : public DistGenerator {
  public:
    UniformDistV2(double min, double max, SST::RNG::Random* rng) : min(min), max(max) {
      dist = new SST::RNG::UniformDistribution(max, rng);
    }

    ~UniformDistV2() {
      delete dist;
    }

    double generate_objects_single() {
      double res = dist->getNextDouble();
      //printf("%.2f\n",res);
      return res;
    }

  private:
    double min;
    double max;
};

class UniformDist : public DistGenerator {
  public:
    UniformDist(double min, double max, SST::RNG::Random* rng) : min(min), max(max) {
      dist = new SST::RNG::UniformDistribution(1e4, rng);
    }

    ~UniformDist() {
      delete dist;
    }

    double generate_objects_single() {
      return min+(max - min)*dist->getNextDouble()/1e4;
    }

  private:
    double min;
    double max;
};

class ExponentialDist : public DistGenerator {
  public:
    ExponentialDist(double lambda, SST::RNG::Random* rng) {
      dist = new SST::RNG::ExponentialDistribution(lambda, rng);
    }

    ~ExponentialDist() {
      delete dist;
    }

    double generate_objects_single() {
      return dist->getNextDouble();
    }
};

class DiscreteDist : public DistGenerator {
  public:
    DiscreteDist(uint64_t id, std::string name, std::map<uint64_t, double> distMap, SST::RNG::Random* rng) {
      if (id == 0) {
        dist = new DiscreteSharedDistribution(name, distMap, rng);
      } else {
        dist = new DiscreteSharedDistribution(name, rng);
      }
    }

    DiscreteDist(uint64_t id, std::string name, uint64_t count, SST::RNG::Random* rng) {
      if (id == 0) {
        std::map<uint64_t, double> probs;

        for (unsigned i=0; i<count; i++) {
          probs.insert(std::make_pair(i, 1.0/count));
        }

        dist = new DiscreteSharedDistribution(name, probs, rng);
      } else {
        dist = new DiscreteSharedDistribution(name, rng);
      }
    }

    ~DiscreteDist() {
      delete dist;
    }

    double generate_objects_single() {
      double res = dist->getNextDouble();
      return res;
    }
};
