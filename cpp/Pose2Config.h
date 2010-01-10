/**
 * @file    VectorConfig.cpp
 * @brief   Pose2Graph Configuration
 * @author  Frank Dellaert
 */

#pragma once

#include <map>
#include <string>
#include "Pose2.h"
#include "Testable.h"

namespace gtsam {

  class VectorConfig;

  class Pose2Config: public Testable<Pose2Config> {

  private:
    std::map<std::string, Pose2> values_;

  public:

    typedef std::map<std::string, Pose2>::const_iterator iterator;
    typedef iterator const_iterator;

    Pose2Config() {}

    Pose2Config(const Pose2Config &config) :values_(config.values_) {}

    virtual ~Pose2Config() {}

    const Pose2& get(std::string key) const;

    void insert(const std::string& name, const Pose2& val);

    inline Pose2Config& operator=(const Pose2Config& rhs) {
      values_ = rhs.values_;
      return (*this);
    }

    bool equals(const Pose2Config& expected, double tol) const;

    void print(const std::string &s) const;

    void clear() { values_.clear(); }

    int size() { return values_.size(); };

    inline const_iterator begin() const {return values_.begin();}
    inline const_iterator end() const {return values_.end();}
    inline iterator begin() {return values_.begin();}
    inline iterator end() {return values_.end();}

    friend Pose2Config expmap(const Pose2Config& c, const VectorConfig& delta);
  };

  /**
   * Add a delta config, needed for use in NonlinearOptimizer
   */
  Pose2Config expmap(const Pose2Config& c, const VectorConfig& delta);

  /**
   * Create a circle of n 2D poses tangent to circle of radius R, first pose at (R,0)
   * @param n number of poses
   * @param R radius of circle
   * @param c character to use for keys
   * @return circle of n 2D poses
   */
	Pose2Config pose2Circle(size_t n, double R, char c = 'p');

} // namespace
