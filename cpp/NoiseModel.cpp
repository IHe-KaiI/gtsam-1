/*
 * NoiseModel
 *
 *  Created on: Jan 13, 2010
 *      Author: Richard Roberts
 *      Author: Frank Dellaert
 */

#include <limits>
#include <iostream>
#include "NoiseModel.h"

namespace ublas = boost::numeric::ublas;
typedef ublas::matrix_column<Matrix> column;

static double inf = std::numeric_limits<double>::infinity();

using namespace std;

namespace gtsam {
	namespace noiseModel {

		/* ************************************************************************* */
		void Gaussian::print(const string& name) const {
			gtsam::print(sqrt_information_, "Gaussian");
		}

		bool Gaussian::equals(const Base& m, double tol) const {
			const Gaussian* p = dynamic_cast<const Gaussian*> (&m);
			if (p == NULL) return false;
			return equal_with_abs_tol(sqrt_information_, p->sqrt_information_, sqrt(tol));
		}

		Vector Gaussian::whiten(const Vector& v) const {
			return sqrt_information_ * v;
		}

		Vector Gaussian::unwhiten(const Vector& v) const {
			return backSubstituteUpper(sqrt_information_, v);
		}

		double Gaussian::Mahalanobis(const Vector& v) const {
			// Note: for Diagonal, which does ediv_, will be correct for constraints
			Vector w = whiten(v);
			return inner_prod(w, w);
		}

		Matrix Gaussian::Whiten(const Matrix& H) const {
		  return sqrt_information_ * H;
		}

		void Gaussian::WhitenInPlace(Matrix& H) const {
		  H = sqrt_information_ * H;
		}

		/* ************************************************************************* */
		// TODO: can we avoid calling reciprocal twice ?
		Diagonal::Diagonal(const Vector& sigmas) :
			Gaussian(diag(reciprocal(sigmas))), invsigmas_(reciprocal(sigmas)),
					sigmas_(sigmas) {
		}

		void Diagonal::print(const string& name) const {
			gtsam::print(sigmas_, "Diagonal sigmas " + name);
		}

		Vector Diagonal::whiten(const Vector& v) const {
			return emul(v, invsigmas_);
		}

		Vector Diagonal::unwhiten(const Vector& v) const {
			return emul(v, sigmas_);
		}

		Matrix Diagonal::Whiten(const Matrix& H) const {
		  return vector_scale(invsigmas_, H);
		}

		void Diagonal::WhitenInPlace(Matrix& H) const {
		  H = vector_scale(invsigmas_, H);
		}

		/* ************************************************************************* */

		void Constrained::print(const std::string& name) const {
			gtsam::print(sigmas_, "Constrained sigmas " + name);
		}

		Vector Constrained::whiten(const Vector& v) const {
			// ediv_ does the right thing with the errors
			return ediv_(v, sigmas_);
		}

		Matrix Constrained::Whiten(const Matrix& H) const {
			throw logic_error("noiseModel::Constrained cannot Whiten");
		}

		void Constrained::WhitenInPlace(Matrix& H) const {
			throw logic_error("noiseModel::Constrained cannot Whiten");
		}

		/* ************************************************************************* */

		void Isotropic::print(const string& name) const {
			cout << "Isotropic sigma " << name << " " << sigma_ << endl;
		}

		double Isotropic::Mahalanobis(const Vector& v) const {
			double dot = inner_prod(v, v);
			return dot * invsigma_ * invsigma_;
		}

		Vector Isotropic::whiten(const Vector& v) const {
			return v * invsigma_;
		}

		Vector Isotropic::unwhiten(const Vector& v) const {
			return v * sigma_;
		}

		Matrix Isotropic::Whiten(const Matrix& H) const {
		  return invsigma_ * H;
		}

		void Isotropic::WhitenInPlace(Matrix& H) const {
		  H *= invsigma_;
		}

		/* ************************************************************************* */
		void Unit::print(const std::string& name) const {
			cout << "Unit " << name << endl;
		}

	/* ************************************************************************* */

	}
} // gtsam
