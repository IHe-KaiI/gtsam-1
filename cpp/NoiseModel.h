/*
 * NoiseModel.h
 *
 *  Created on: Jan 13, 2010
 *      Author: Richard Roberts
 *      Author: Frank Dellaert
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include "Testable.h"
#include "Vector.h"
#include "Matrix.h"

namespace gtsam {	namespace noiseModel {

  /**
   * noiseModel::Base is the abstract base class for all noise models.
   *
   * Noise models must implement a 'whiten' function to normalize an error vector,
   * and an 'unwhiten' function to unnormalize an error vector.
   */
  class Base : public Testable<Base> {

  protected:

  	size_t dim_;

  public:

  	Base(size_t dim):dim_(dim) {}
  	virtual ~Base() {}

    /**
     * Whiten an error vector.
     */
    virtual Vector whiten(const Vector& v) const = 0;

    /**
     * Unwhiten an error vector.
     */
    virtual Vector unwhiten(const Vector& v) const = 0;

    /** in-place whiten, override if can be done more efficiently */
    virtual void whitenInPlace(Vector& v) const {
    	v = whiten(v);
    }

    /** in-place unwhiten, override if can be done more efficiently */
    virtual void unwhitenInPlace(Vector& v) const {
    	v = unwhiten(v);
    }
    };

  /**
	 * Gaussian implements the mathematical model
	 *  |R*x|^2 = |y|^2 with R'*R=inv(Sigma)
	 * where
	 *   y = whiten(x) = R*x
	 *   x = unwhiten(x) = inv(R)*y
	 * as indeed
	 *   |y|^2 = y'*y = x'*R'*R*x
	 * Various derived classes are available that are more efficient.
	 */
	struct Gaussian: public Base {

	protected:

		// TODO: store as boost upper-triangular or whatever is passed from above
		/* Matrix square root of information matrix (R) */
		Matrix sqrt_information_;

		/** protected constructor takes square root information matrix */
		Gaussian(const Matrix& sqrt_information) :
			Base(sqrt_information.size1()), sqrt_information_(sqrt_information) {
		}

	public:

		typedef boost::shared_ptr<Gaussian> shared_ptr;

    /**
     * A Gaussian noise model created by specifying a square root information matrix.
     */
    static shared_ptr SqrtInformation(const Matrix& R) {
    	return shared_ptr(new Gaussian(R));
    }

    /**
     * A Gaussian noise model created by specifying a covariance matrix.
     */
    static shared_ptr Covariance(const Matrix& covariance) {
    	return shared_ptr(new Gaussian(inverse_square_root(covariance)));
    }

    /**
     * A Gaussian noise model created by specifying an information matrix.
     */
    static shared_ptr Information(const Matrix& Q)  {
    	return shared_ptr(new Gaussian(square_root_positive(Q)));
    }

		virtual void print(const std::string& name) const;
		virtual bool equals(const Base& expected, double tol) const;
    virtual Vector whiten(const Vector& v) const;
		virtual Vector unwhiten(const Vector& v) const;

    /**
		 * Mahalanobis distance v'*R'*R*v = <R*v,R*v>
		 */
		virtual double Mahalanobis(const Vector& v) const;

		/**
		 * Multiply a derivative with R (derivative of whiten)
		 * Equivalent to whitening each column of the input matrix.
		 */
		virtual Matrix Whiten(const Matrix& H) const;

		/**
		 * In-place version
		 */
		virtual void WhitenInPlace(Matrix& H) const;

    /**
     * Whiten a system, in place as well
     */
    inline void WhitenSystem(Matrix& A, Vector& b) const {
  		WhitenInPlace(A);
  		whitenInPlace(b);
    }

		/**
		 * Return R itself, but note that Whiten(H) is cheaper than R*H
		 */
		const Matrix& R() const {
			return sqrt_information_;
		}

	}; // Gaussian

	// FD: does not work, ambiguous overload :-(
  // inline Vector operator*(const Gaussian& R, const Vector& v) {return R.whiten(v);}

  /**
   * A diagonal noise model implements a diagonal covariance matrix, with the
   * elements of the diagonal specified in a Vector.  This class has no public
   * constructors, instead, use the static constructor functions Sigmas etc...
   */
  class Diagonal : public Gaussian {
  protected:

  	/** sigmas and reciprocal */
    Vector sigmas_, invsigmas_;

    /** protected constructor takes sigmas */
    Diagonal(const Vector& sigmas);

  public:

		typedef boost::shared_ptr<Diagonal> shared_ptr;

    /**
     * A diagonal noise model created by specifying a Vector of sigmas, i.e.
     * standard devations, the diagonal of the square root covariance matrix.
     */
    static shared_ptr Sigmas(const Vector& sigmas) {
    	return shared_ptr(new Diagonal(sigmas));
    }

    /**
     * A diagonal noise model created by specifying a Vector of variances, i.e.
     * i.e. the diagonal of the covariance matrix.
     */
    static shared_ptr Variances(const Vector& variances) {
    	return shared_ptr(new Diagonal(esqrt(variances)));
    }

    /**
     * A diagonal noise model created by specifying a Vector of precisions, i.e.
     * i.e. the diagonal of the information matrix, i.e., weights
     */
    static shared_ptr Precisions(const Vector& precisions) {
    	return Variances(reciprocal(precisions));
    }

		virtual void print(const std::string& name) const;
    virtual Vector whiten(const Vector& v) const;
    virtual Vector unwhiten(const Vector& v) const;
		virtual Matrix Whiten(const Matrix& H) const;
		virtual void WhitenInPlace(Matrix& H) const;

		/**
     * Return standard deviations (sqrt of diagonal)
     */
    inline const Vector& sigmas() const { return sigmas_; }
    inline double sigma(size_t i) const { return sigmas_(i); }

  }; // Diagonal

  /**
   * A Constrained constrained model is a specialization of Diagonal which allows
   * some or all of the sigmas to be zero, forcing the error to be zero there.
   * All other Gaussian models are guaranteed to have a non-singular square-root
   * information matrix, but this class is specifically equipped to deal with
   * singular noise models, specifically: whiten will return zero on those
   * components that have zero sigma *and* zero error, infinity otherwise.
   */
  class Constrained : public Diagonal {
  protected:

    /** protected constructor takes sigmas */
  	Constrained(const Vector& sigmas) :Diagonal(sigmas) {}

  public:

		typedef boost::shared_ptr<Constrained> shared_ptr;

    /**
     * A diagonal noise model created by specifying a Vector of
     * standard devations, some of which might be zero
     */
    static shared_ptr Mixed(const Vector& sigmas) {
    	return shared_ptr(new Constrained(sigmas));
    }

    /**
     * Fully constrained. TODO: subclass ?
     */
    static shared_ptr All(size_t dim) {
    	return Mixed(repeat(dim,0));
    }

		virtual void print(const std::string& name) const;
    virtual Vector whiten(const Vector& v) const;

		// Whitening Jacobians does not make sense for possibly constrained
		// noise model and will throw an exception.

		virtual Matrix Whiten(const Matrix& H) const;
		virtual void WhitenInPlace(Matrix& H) const;

  }; // Constrained

  /**
   * An isotropic noise model corresponds to a scaled diagonal covariance
   * To construct, use one of the static methods
   */
  class Isotropic : public Diagonal {
  protected:
    double sigma_, invsigma_;

    /** protected constructor takes sigma */
    Isotropic(size_t dim, double sigma) :
			Diagonal(repeat(dim, sigma)),sigma_(sigma),invsigma_(1.0/sigma) {}

  public:

		typedef boost::shared_ptr<Isotropic> shared_ptr;

    /**
     * An isotropic noise model created by specifying a standard devation sigma
     */
    static shared_ptr Sigma(size_t dim, double sigma) {
    	return shared_ptr(new Isotropic(dim, sigma));
    }

    /**
     * An isotropic noise model created by specifying a variance = sigma^2.
     */
    static shared_ptr Variance(size_t dim, double variance)  {
    	return shared_ptr(new Isotropic(dim, sqrt(variance)));
    }

    /**
     * An isotropic noise model created by specifying a precision
     */
    static shared_ptr Precision(size_t dim, double precision)  {
    	return Variance(dim, 1.0/precision);
    }

		virtual void print(const std::string& name) const;
		virtual double Mahalanobis(const Vector& v) const;
    virtual Vector whiten(const Vector& v) const;
    virtual Vector unwhiten(const Vector& v) const;
		virtual Matrix Whiten(const Matrix& H) const;
		virtual void WhitenInPlace(Matrix& H) const;

    /**
     * Return standard deviation
     */
    inline double sigma() const { return sigma_; }
  };

  /**
   * Unit: i.i.d. unit-variance noise on all m dimensions.
   */
  class Unit : public Isotropic {
  protected:

    Unit(size_t dim): Isotropic(dim,1.0) {}

  public:

    typedef boost::shared_ptr<Unit> shared_ptr;

    /**
     * Create a unit covariance noise model
     */
    static shared_ptr Create(size_t dim) {
    	return shared_ptr(new Unit(dim));
    }

		virtual void print(const std::string& name) const;
		virtual double Mahalanobis(const Vector& v) const {return inner_prod(v,v); }
    virtual Vector whiten(const Vector& v) const { return v; }
    virtual Vector unwhiten(const Vector& v) const { return v; }
    virtual Matrix Whiten(const Matrix& H) const { return H; }
	  virtual void WhitenInPlace(Matrix& H) const {}
  };

	} // namespace noiseModel

	using namespace noiseModel;

	// A useful convenience class to refer to a shared Gaussian model
	// Define GTSAM_MAGIC_GAUSSIAN to desired dimension to have access to slightly
	// dangerous and non-shared (inefficient, wasteful) noise models. Only in tests!
	struct sharedGaussian : public Gaussian::shared_ptr {
		sharedGaussian() {}
		sharedGaussian(const    Gaussian::shared_ptr& p):Gaussian::shared_ptr(p) {}
		sharedGaussian(const    Diagonal::shared_ptr& p):Gaussian::shared_ptr(p) {}
		sharedGaussian(const Constrained::shared_ptr& p):Gaussian::shared_ptr(p) {}
		sharedGaussian(const   Isotropic::shared_ptr& p):Gaussian::shared_ptr(p) {}
		sharedGaussian(const        Unit::shared_ptr& p):Gaussian::shared_ptr(p) {}
#ifdef GTSAM_MAGIC_GAUSSIAN
		sharedGaussian(const Matrix& covariance):Gaussian::shared_ptr(Gaussian::Covariance(covariance)) {}
		sharedGaussian(const Vector& sigmas):Gaussian::shared_ptr(Diagonal::Sigmas(sigmas)) {}
		sharedGaussian(const double& s):Gaussian::shared_ptr(Isotropic::Sigma(GTSAM_MAGIC_GAUSSIAN,s)) {}
#endif
		};

	typedef Diagonal::shared_ptr sharedDiagonal;


} // namespace gtsam
