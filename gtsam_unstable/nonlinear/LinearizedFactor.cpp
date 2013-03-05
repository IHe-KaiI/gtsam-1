/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file LinearizedFactor.cpp
 * @brief A dummy factor that allows a linear factor to act as a nonlinear factor
 * @author Alex Cunningham
 */

#include <gtsam_unstable/nonlinear/LinearizedFactor.h>
#include <boost/foreach.hpp>
#include <iostream>

namespace gtsam {

/* ************************************************************************* */
LinearizedGaussianFactor::LinearizedGaussianFactor(const GaussianFactor::shared_ptr& gaussian, const Ordering& ordering, const Values& lin_points) {
  // Extract the keys and linearization points
  BOOST_FOREACH(const Index& idx, gaussian->keys()) {
    // find full symbol
    if (idx < ordering.size()) {
      Key key = ordering.key(idx);

      // extract linearization point
      assert(lin_points.exists(key));
      this->lin_points_.insert(key, lin_points.at(key));

      // store keys
      this->keys_.push_back(key);
    } else {
      throw std::runtime_error("LinearizedGaussianFactor: could not find index in decoder!");
    }
  }
}



/* ************************************************************************* */
LinearizedJacobianFactor::LinearizedJacobianFactor() : Ab_(matrix_) {
}

/* ************************************************************************* */
LinearizedJacobianFactor::LinearizedJacobianFactor(const JacobianFactor::shared_ptr& jacobian,
    const Ordering& ordering, const Values& lin_points)
: Base(jacobian, ordering, lin_points), Ab_(matrix_) {

  // Get the Ab matrix from the Jacobian factor, with any covariance baked in
  AbMatrix fullMatrix = jacobian->matrix_augmented(true);

  // Create the dims array
  size_t *dims = (size_t *)alloca(sizeof(size_t) * (jacobian->size() + 1));
  size_t index = 0;
  for(JacobianFactor::const_iterator iter = jacobian->begin(); iter != jacobian->end(); ++iter) {
    dims[index++] = jacobian->getDim(iter);
  }
  dims[index] = 1;

  // Update the BlockInfo accessor
  BlockAb Ab(fullMatrix, dims, dims+jacobian->size()+1);
  Ab.swap(Ab_);
}

/* ************************************************************************* */
void LinearizedJacobianFactor::print(const std::string& s, const KeyFormatter& keyFormatter) const {

  std::cout << s << std::endl;

  std::cout << "Nonlinear Keys: ";
  BOOST_FOREACH(const Key& key, this->keys())
    std::cout << keyFormatter(key) << " ";
  std::cout << std::endl;

  for(const_iterator key=begin(); key!=end(); ++key)
    std::cout << boost::format("A[%1%]=\n")%keyFormatter(*key) << A(*key) << std::endl;
  std::cout << "b=\n" << b() << std::endl;

  lin_points_.print("Linearization Point: ");
}

/* ************************************************************************* */
bool LinearizedJacobianFactor::equals(const NonlinearFactor& expected, double tol) const {

  const This *e = dynamic_cast<const This*> (&expected);
  if (e) {

    Matrix thisMatrix = this->Ab_.range(0, Ab_.nBlocks());
    Matrix rhsMatrix = e->Ab_.range(0, Ab_.nBlocks());

    return Base::equals(expected, tol)
        && lin_points_.equals(e->lin_points_, tol)
        && equal_with_abs_tol(thisMatrix, rhsMatrix, tol);
  } else {
    return false;
  }
}

/* ************************************************************************* */
double LinearizedJacobianFactor::error(const Values& c) const {
  Vector errorVector = error_vector(c);
  return 0.5 * errorVector.dot(errorVector);
}

/* ************************************************************************* */
boost::shared_ptr<GaussianFactor>
LinearizedJacobianFactor::linearize(const Values& c, const Ordering& ordering) const {

  // Create the 'terms' data structure for the Jacobian constructor
  std::vector<std::pair<Index, Matrix> > terms;
  BOOST_FOREACH(Key key, keys()) {
    terms.push_back(std::make_pair(ordering[key], this->A(key)));
  }

  // compute rhs
  Vector b = -error_vector(c);

  return boost::shared_ptr<GaussianFactor>(new JacobianFactor(terms, b, noiseModel::Unit::Create(dim())));
}

/* ************************************************************************* */
Vector LinearizedJacobianFactor::error_vector(const Values& c) const {

  Vector errorVector = -b();

  BOOST_FOREACH(Key key, this->keys()) {
    const Value& newPt = c.at(key);
    const Value& linPt = lin_points_.at(key);
    Vector d = linPt.localCoordinates_(newPt);
    const constABlock A = this->A(key);
    errorVector += A * d;
  }

  return errorVector;
}






/* ************************************************************************* */
LinearizedHessianFactor::LinearizedHessianFactor() : info_(matrix_) {
}

/* ************************************************************************* */
LinearizedHessianFactor::LinearizedHessianFactor(const HessianFactor::shared_ptr& hessian,
      const Ordering& ordering, const Values& lin_points)
: Base(hessian, ordering, lin_points), info_(matrix_) {

  // Copy the augmented matrix holding G, g, and f
  Matrix fullMatrix = hessian->info();

  // Create the dims array
  size_t *dims = (size_t*)alloca(sizeof(size_t)*(hessian->size() + 1));
  size_t index = 0;
  for(HessianFactor::const_iterator iter = hessian->begin(); iter != hessian->end(); ++iter) {
    dims[index++] = hessian->getDim(iter);
  }
  dims[index] = 1;

  // Update the BlockInfo accessor
  BlockInfo infoMatrix(fullMatrix, dims, dims+hessian->size()+1);
  infoMatrix.swap(info_);
}

/* ************************************************************************* */
void LinearizedHessianFactor::print(const std::string& s, const KeyFormatter& keyFormatter) const {

  std::cout << s << std::endl;

  std::cout << "Nonlinear Keys: ";
  BOOST_FOREACH(const Key& key, this->keys())
    std::cout << keyFormatter(key) << " ";
  std::cout << std::endl;

  gtsam::print(Matrix(info_.range(0,info_.nBlocks(), 0,info_.nBlocks()).selfadjointView<Eigen::Upper>()), "Ab^T * Ab: ");

  lin_points_.print("Linearization Point: ");
}

/* ************************************************************************* */
bool LinearizedHessianFactor::equals(const NonlinearFactor& expected, double tol) const {

  const This *e = dynamic_cast<const This*> (&expected);
  if (e) {

    Matrix thisMatrix = this->info_.full().selfadjointView<Eigen::Upper>();
    thisMatrix(thisMatrix.rows()-1, thisMatrix.cols()-1) = 0.0;
    Matrix rhsMatrix = e->info_.full().selfadjointView<Eigen::Upper>();
    rhsMatrix(rhsMatrix.rows()-1, rhsMatrix.cols()-1) = 0.0;

    return Base::equals(expected, tol)
        && lin_points_.equals(e->lin_points_, tol)
        && equal_with_abs_tol(thisMatrix, rhsMatrix, tol);
  } else {
    return false;
  }
}

/* ************************************************************************* */
double LinearizedHessianFactor::error(const Values& c) const {

  // Construct an error vector in key-order from the Values
  Vector dx = zero(dim());
  size_t index = 0;
  for(unsigned int i = 0; i < this->size(); ++i){
    Key key = this->keys()[i];
    const Value& newPt = c.at(key);
    const Value& linPt = lin_points_.at(key);
    dx.segment(index, linPt.dim()) = linPt.localCoordinates_(newPt);
    index += linPt.dim();
  }

  // error 0.5*(f - 2*x'*g + x'*G*x)
  double f = constantTerm();
  double xtg = dx.dot(linearTerm());
  double xGx = dx.transpose() * squaredTerm().selfadjointView<Eigen::Upper>() * dx;

  return 0.5 * (f - 2.0 * xtg +  xGx);
}

/* ************************************************************************* */
boost::shared_ptr<GaussianFactor>
LinearizedHessianFactor::linearize(const Values& c, const Ordering& ordering) const {

  // Use the ordering to convert the keys into indices;
  std::vector<Index> js;
  BOOST_FOREACH(Key key, this->keys()){
    js.push_back(ordering.at(key));
  }

  // Make a copy of the info matrix
  Matrix newMatrix;
  SymmetricBlockView<Matrix> newInfo(newMatrix);
  newInfo.assignNoalias(info_);

  // Construct an error vector in key-order from the Values
  Vector dx = zero(dim());
  size_t index = 0;
  for(unsigned int i = 0; i < this->size(); ++i){
    Key key = this->keys()[i];
    const Value& newPt = c.at(key);
    const Value& linPt = lin_points_.at(key);
    dx.segment(index, linPt.dim()) = linPt.localCoordinates_(newPt);
    index += linPt.dim();
  }

  // f2 = f1 - 2*dx'*g1 + dx'*G1*dx
  //newInfo(this->size(), this->size())(0,0) += -2*dx.dot(linearTerm()) + dx.transpose() * squaredTerm().selfadjointView<Eigen::Upper>() * dx;
  double f = constantTerm() - 2*dx.dot(linearTerm()) + dx.transpose() * squaredTerm().selfadjointView<Eigen::Upper>() * dx;

  // g2 = g1 - G1*dx
  //newInfo.rangeColumn(0, this->size(), this->size(), 0) -= squaredTerm().selfadjointView<Eigen::Upper>() * dx;
  Vector g = linearTerm() - squaredTerm().selfadjointView<Eigen::Upper>() * dx;
  std::vector<Vector> gs;
  for(size_t i = 0; i < info_.nBlocks()-1; ++i) {
    gs.push_back(g.segment(info_.offset(i), info_.offset(i+1) - info_.offset(i)));
  }

  // G2 = G1
  // Do Nothing
  std::vector<Matrix> Gs;
  for(size_t i = 0; i < info_.nBlocks()-1; ++i) {
    for(size_t j = i; j < info_.nBlocks()-1; ++j) {
      Gs.push_back(info_(i,j));
    }
  }

  // Create a Hessian Factor from the modified info matrix
  //return boost::shared_ptr<GaussianFactor>(new HessianFactor(js, newInfo));
  return boost::shared_ptr<GaussianFactor>(new HessianFactor(js, Gs, gs, f));
}

} // \namespace aspn