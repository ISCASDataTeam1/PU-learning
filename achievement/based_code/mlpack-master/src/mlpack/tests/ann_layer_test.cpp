/**
 * @file ann_layer_test.cpp
 * @author Marcus Edel
 * @author Praveen Ch
 *
 * Tests the ann layer modules.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#include <mlpack/core.hpp>

#include <mlpack/methods/ann/layer/layer.hpp>
#include <mlpack/methods/ann/layer/layer_types.hpp>
#include <mlpack/methods/ann/loss_functions/mean_squared_error.hpp>
#include <mlpack/methods/ann/loss_functions/sigmoid_cross_entropy_error.hpp>
#include <mlpack/methods/ann/init_rules/random_init.hpp>
#include <mlpack/methods/ann/init_rules/const_init.hpp>
#include <mlpack/methods/ann/init_rules/nguyen_widrow_init.hpp>
#include <mlpack/methods/ann/ffn.hpp>
#include <mlpack/methods/ann/rnn.hpp>

#include <boost/test/unit_test.hpp>
#include "test_tools.hpp"

using namespace mlpack;
using namespace mlpack::ann;

BOOST_AUTO_TEST_SUITE(ANNLayerTest);

// Helper function which calls the Reset function of the given module.
template<class T>
void ResetFunction(
    T& layer,
    typename std::enable_if<HasResetCheck<T, void(T::*)()>::value>::type* = 0)
{
  layer.Reset();
}

template<class T>
void ResetFunction(
    T& /* layer */,
    typename std::enable_if<!HasResetCheck<T, void(T::*)()>::value>::type* = 0)
{
  /* Nothing to do here */
}

// Approximate Jacobian and supposedly-true Jacobian, then compare them
// similarly to before.
template<typename ModuleType>
double JacobianTest(ModuleType& module,
                  arma::mat& input,
                  const double minValue = -2,
                  const double maxValue = -1,
                  const double perturbation = 1e-6)
{
  arma::mat output, outputA, outputB, jacobianA, jacobianB;

  // Initialize the input matrix.
  RandomInitialization init(minValue, maxValue);
  init.Initialize(input, input.n_rows, input.n_cols);

  // Initialize the module parameters.
  ResetFunction(module);

  // Initialize the jacobian matrix.
  module.Forward(std::move(input), std::move(output));
  jacobianA = arma::zeros(input.n_elem, output.n_elem);

  // Share the input paramter matrix.
  arma::mat sin = arma::mat(input.memptr(), input.n_rows, input.n_cols,
      false, false);

  for (size_t i = 0; i < input.n_elem; ++i)
  {
    double original = sin(i);
    sin(i) = original - perturbation;
    module.Forward(std::move(input), std::move(outputA));
    sin(i) = original + perturbation;
    module.Forward(std::move(input), std::move(outputB));
    sin(i) = original;

    outputB -= outputA;
    outputB /= 2 * perturbation;
    jacobianA.row(i) = outputB.t();
  }

  // Initialize the derivative parameter.
  arma::mat deriv = arma::zeros(output.n_rows, output.n_cols);

  // Share the derivative parameter.
  arma::mat derivTemp = arma::mat(deriv.memptr(), deriv.n_rows, deriv.n_cols,
      false, false);

  // Initialize the jacobian matrix.
  jacobianB = arma::zeros(input.n_elem, output.n_elem);

  for (size_t i = 0; i < derivTemp.n_elem; ++i)
  {
    deriv.zeros();
    derivTemp(i) = 1;

    arma::mat delta;
    module.Backward(std::move(input), std::move(deriv), std::move(delta));

    jacobianB.col(i) = delta;
  }

  return arma::max(arma::max(arma::abs(jacobianA - jacobianB)));
}

// Approximate Jacobian and supposedly-true Jacobian, then compare them
// similarly to before.
template<typename ModuleType>
double JacobianPerformanceTest(ModuleType& module,
                               arma::mat& input,
                               arma::mat& target,
                               const double eps = 1e-6)
{
  module.Forward(std::move(input), std::move(target));

  arma::mat delta;
  module.Backward(std::move(input), std::move(target), std::move(delta));

  arma::mat centralDifference = arma::zeros(delta.n_rows, delta.n_cols);
  arma::mat inputTemp = arma::mat(input.memptr(), input.n_rows, input.n_cols,
      false, false);

  arma::mat centralDifferenceTemp = arma::mat(centralDifference.memptr(),
      centralDifference.n_rows, centralDifference.n_cols, false, false);

  for (size_t i = 0; i < input.n_elem; ++i)
  {
    inputTemp(i) = inputTemp(i) + eps;
    double outputA = module.Forward(std::move(input), std::move(target));
    inputTemp(i) = inputTemp(i) - (2 * eps);
    double outputB = module.Forward(std::move(input), std::move(target));

    centralDifferenceTemp(i) = (outputA - outputB) / (2 * eps);
    inputTemp(i) = inputTemp(i) + eps;
  }

  return arma::max(arma::max(arma::abs(centralDifference - delta)));
}

// Simple numerical gradient checker.
template<class FunctionType>
double CheckGradient(FunctionType& function, const double eps = 1e-7)
{
  // Get gradients for the current parameters.
  arma::mat orgGradient, gradient, estGradient;
  function.Gradient(orgGradient);

  estGradient = arma::zeros(orgGradient.n_rows, orgGradient.n_cols);

  // Compute numeric approximations to gradient.
  for (size_t i = 0; i < orgGradient.n_elem; ++i)
  {
    double tmp = function.Parameters()(i);

    // Perturb parameter with a positive constant and get costs.
    function.Parameters()(i) += eps;
    double costPlus = function.Gradient(gradient);

    // Perturb parameter with a negative constant and get costs.
    function.Parameters()(i) -= (2 * eps);
    double costMinus = function.Gradient(gradient);

    // Restore the parameter value.
    function.Parameters()(i) = tmp;

    // Compute numerical gradients using the costs calculated above.
    estGradient(i) = (costPlus - costMinus) / (2 * eps);
  }

  // Estimate error of gradient.
  return arma::norm(orgGradient - estGradient) /
      arma::norm(orgGradient + estGradient);
}

/**
 * Simple add module test.
 */
BOOST_AUTO_TEST_CASE(SimpleAddLayerTest)
{
  arma::mat output, input, delta;
  Add<> module(10);
  module.Parameters().randu();

  // Test the Forward function.
  input = arma::zeros(10, 1);
  module.Forward(std::move(input), std::move(output));
  BOOST_REQUIRE_EQUAL(arma::accu(module.Parameters()), arma::accu(output));

  // Test the Backward function.
  module.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(output), arma::accu(delta));

  // Test the forward function.
  input = arma::ones(10, 1);
  module.Forward(std::move(input), std::move(output));
  BOOST_REQUIRE_CLOSE(10 + arma::accu(module.Parameters()),
      arma::accu(output), 1e-3);

  // Test the backward function.
  module.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_CLOSE(arma::accu(output), arma::accu(delta), 1e-3);
}

/**
 * Jacobian add module test.
 */
BOOST_AUTO_TEST_CASE(JacobianAddLayerTest)
{
  for (size_t i = 0; i < 5; i++)
  {
    const size_t elements = math::RandInt(2, 1000);
    arma::mat input;
    input.set_size(elements, 1);

    Add<> module(elements);
    module.Parameters().randu();

    double error = JacobianTest(module, input);
    BOOST_REQUIRE_LE(error, 1e-5);
  }
}

/**
 * Add layer numerically gradient test.
 */
BOOST_AUTO_TEST_CASE(GradientAddLayerTest)
{
  // Add function gradient instantiation.
  struct GradientFunction
  {
    GradientFunction()
    {
      input = arma::randu(10, 1);
      target = arma::mat("1");

      model = new FFN<NegativeLogLikelihood<>, NguyenWidrowInitialization>();
      model->Predictors() = input;
      model->Responses() = target;
      model->Add<IdentityLayer<> >();
      model->Add<Add<> >(10);
      model->Add<LogSoftMax<> >();
    }

    ~GradientFunction()
    {
      delete model;
    }

    double Gradient(arma::mat& gradient) const
    {
      arma::mat output;
      double error = model->Evaluate(model->Parameters(), 0, 1);
      model->Gradient(model->Parameters(), 0, gradient, 1);
      return error;
    }

    arma::mat& Parameters() { return model->Parameters(); }

    FFN<NegativeLogLikelihood<>, NguyenWidrowInitialization>* model;
    arma::mat input, target;
  } function;

  BOOST_REQUIRE_LE(CheckGradient(function), 1e-4);
}

/**
 * Simple constant module test.
 */
BOOST_AUTO_TEST_CASE(SimpleConstantLayerTest)
{
  arma::mat output, input, delta;
  Constant<> module(10, 3.0);

  // Test the Forward function.
  input = arma::zeros(10, 1);
  module.Forward(std::move(input), std::move(output));
  BOOST_REQUIRE_EQUAL(arma::accu(output), 30.0);

  // Test the Backward function.
  module.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 0);

  // Test the forward function.
  input = arma::ones(10, 1);
  module.Forward(std::move(input), std::move(output));
  BOOST_REQUIRE_EQUAL(arma::accu(output), 30.0);

  // Test the backward function.
  module.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 0);
}

/**
 * Jacobian constant module test.
 */
BOOST_AUTO_TEST_CASE(JacobianConstantLayerTest)
{
  for (size_t i = 0; i < 5; i++)
  {
    const size_t elements = math::RandInt(2, 1000);
    arma::mat input;
    input.set_size(elements, 1);

    Constant<> module(elements, 1.0);

    double error = JacobianTest(module, input);
    BOOST_REQUIRE_LE(error, 1e-5);
  }
}

/**
 * Simple dropout module test.
 */
BOOST_AUTO_TEST_CASE(SimpleDropoutLayerTest)
{
  // Initialize the probability of setting a value to zero.
  const double p = 0.2;

  // Initialize the input parameter.
  arma::mat input(1000, 1);
  input.fill(1 - p);

  Dropout<> module(p);
  module.Deterministic() = false;

  // Test the Forward function.
  arma::mat output;
  module.Forward(std::move(input), std::move(output));
  BOOST_REQUIRE_LE(
      arma::as_scalar(arma::abs(arma::mean(output) - (1 - p))), 0.05);

  // Test the Backward function.
  arma::mat delta;
  module.Backward(std::move(input), std::move(input), std::move(delta));
  BOOST_REQUIRE_LE(
      arma::as_scalar(arma::abs(arma::mean(delta) - (1 - p))), 0.05);

  // Test the Forward function.
  module.Deterministic() = true;
  module.Forward(std::move(input), std::move(output));
  BOOST_REQUIRE_EQUAL(arma::accu(input), arma::accu(output));
}

/**
 * Perform dropout x times using ones as input, sum the number of ones and
 * validate that the layer is producing approximately the correct number of
 * ones.
 */
BOOST_AUTO_TEST_CASE(DropoutProbabilityTest)
{
  arma::mat input = arma::ones(1500, 1);
  const size_t iterations = 10;

  double probability[5] = { 0.1, 0.3, 0.4, 0.7, 0.8 };
  for (size_t trial = 0; trial < 5; ++trial)
  {
    double nonzeroCount = 0;
    for (size_t i = 0; i < iterations; ++i)
    {
      Dropout<> module(probability[trial]);
      module.Deterministic() = false;

      arma::mat output;
      module.Forward(std::move(input), std::move(output));

      // Return a column vector containing the indices of elements of X that
      // are non-zero, we just need the number of non-zero values.
      arma::uvec nonzero = arma::find(output);
      nonzeroCount += nonzero.n_elem;
    }
    const double expected = input.n_elem * (1 - probability[trial]) *
        iterations;
    const double error = fabs(nonzeroCount - expected) / expected;

    BOOST_REQUIRE_LE(error, 0.15);
  }
}

/*
 * Perform dropout with probability 1 - p where p = 0, means no dropout.
 */
BOOST_AUTO_TEST_CASE(NoDropoutTest)
{
  arma::mat input = arma::ones(1500, 1);
  Dropout<> module(0);
  module.Deterministic() = false;

  arma::mat output;
  module.Forward(std::move(input), std::move(output));

  BOOST_REQUIRE_EQUAL(arma::accu(output), arma::accu(input));
}

/*
 * Perform test to check whether mean and variance remain nearly same
 * after AlphaDropout.
 */
BOOST_AUTO_TEST_CASE(SimpleAlphaDropoutLayerTest)
{
  // Initialize the probability of setting a value to alphaDash.
  const double p = 0.2;

  // Initialize the input parameter having a mean nearabout 0
  // and variance nearabout 1.
  arma::mat input = arma::randn<arma::mat>(1000, 1);

  AlphaDropout<> module(p);
  module.Deterministic() = false;

  // Test the Forward function when training phase.
  arma::mat output;
  module.Forward(std::move(input), std::move(output));
  // Check whether mean remains nearly same.
  BOOST_REQUIRE_LE(
      arma::as_scalar(arma::abs(arma::mean(input) - arma::mean(output))), 0.1);

  // Check whether variance remains nearly same.
  BOOST_REQUIRE_LE(
      arma::as_scalar(arma::abs(arma::var(input) - arma::var(output))), 0.1);

  // Test the Backward function when training phase.
  arma::mat delta;
  module.Backward(std::move(input), std::move(input), std::move(delta));
  BOOST_REQUIRE_LE(
      arma::as_scalar(arma::abs(arma::mean(delta) - 0)), 0.05);

  // Test the Forward function when testing phase.
  module.Deterministic() = true;
  module.Forward(std::move(input), std::move(output));
  BOOST_REQUIRE_EQUAL(arma::accu(input), arma::accu(output));
}

/**
 * Perform AlphaDropout x times using ones as input, sum the number of ones
 * and validate that the layer is producing approximately the correct number
 * of ones.
 */
BOOST_AUTO_TEST_CASE(AlphaDropoutProbabilityTest)
{
  arma::mat input = arma::ones(1500, 1);
  const size_t iterations = 10;

  double probability[5] = { 0.1, 0.3, 0.4, 0.7, 0.8 };
  for (size_t trial = 0; trial < 5; ++trial)
  {
    double nonzeroCount = 0;
    for (size_t i = 0; i < iterations; ++i)
    {
      AlphaDropout<> module(probability[trial]);
      module.Deterministic() = false;

      arma::mat output;
      module.Forward(std::move(input), std::move(output));

      // Return a column vector containing the indices of elements of X
      // that are not alphaDash, we just need the number of
      // nonAlphaDash values.
      arma::uvec nonAlphaDash = arma::find(module.Mask());
      nonzeroCount += nonAlphaDash.n_elem;
    }

    const double expected = input.n_elem * (1-probability[trial]) * iterations;

    const double error = fabs(nonzeroCount - expected) / expected;

    BOOST_REQUIRE_LE(error, 0.15);
  }
}

/**
 * Perform AlphaDropout with probability 1 - p where p = 0,
 * means no AlphaDropout.
 */
BOOST_AUTO_TEST_CASE(NoAlphaDropoutTest)
{
  arma::mat input = arma::ones(1500, 1);
  AlphaDropout<> module(0);
  module.Deterministic() = false;

  arma::mat output;
  module.Forward(std::move(input), std::move(output));

  BOOST_REQUIRE_EQUAL(arma::accu(output), arma::accu(input));
}

/**
 * Simple linear module test.
 */
BOOST_AUTO_TEST_CASE(SimpleLinearLayerTest)
{
  arma::mat output, input, delta;
  Linear<> module(10, 10);
  module.Parameters().randu();
  module.Reset();

  // Test the Forward function.
  input = arma::zeros(10, 1);
  module.Forward(std::move(input), std::move(output));
  BOOST_REQUIRE_CLOSE(arma::accu(
      module.Parameters().submat(100, 0, module.Parameters().n_elem - 1, 0)),
      arma::accu(output), 1e-3);

  // Test the Backward function.
  module.Backward(std::move(input), std::move(input), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 0);
}

/**
 * Jacobian linear module test.
 */
BOOST_AUTO_TEST_CASE(JacobianLinearLayerTest)
{
  for (size_t i = 0; i < 5; i++)
  {
    const size_t inputElements = math::RandInt(2, 1000);
    const size_t outputElements = math::RandInt(2, 1000);

    arma::mat input;
    input.set_size(inputElements, 1);

    Linear<> module(inputElements, outputElements);
    module.Parameters().randu();

    double error = JacobianTest(module, input);
    BOOST_REQUIRE_LE(error, 1e-5);
  }
}

/**
 * Linear layer numerically gradient test.
 */
BOOST_AUTO_TEST_CASE(GradientLinearLayerTest)
{
  // Linear function gradient instantiation.
  struct GradientFunction
  {
    GradientFunction()
    {
      input = arma::randu(10, 1);
      target = arma::mat("1");

      model = new FFN<NegativeLogLikelihood<>, NguyenWidrowInitialization>();
      model->Predictors() = input;
      model->Responses() = target;
      model->Add<IdentityLayer<> >();
      model->Add<Linear<> >(10, 2);
      model->Add<LogSoftMax<> >();
    }

    ~GradientFunction()
    {
      delete model;
    }

    double Gradient(arma::mat& gradient) const
    {
      arma::mat output;
      double error = model->Evaluate(model->Parameters(), 0, 1);
      model->Gradient(model->Parameters(), 0, gradient, 1);
      return error;
    }

    arma::mat& Parameters() { return model->Parameters(); }

    FFN<NegativeLogLikelihood<>, NguyenWidrowInitialization>* model;
    arma::mat input, target;
  } function;

  BOOST_REQUIRE_LE(CheckGradient(function), 1e-4);
}

/**
 * Simple linear no bias module test.
 */
BOOST_AUTO_TEST_CASE(SimpleLinearNoBiasLayerTest)
{
  arma::mat output, input, delta;
  LinearNoBias<> module(10, 10);
  module.Parameters().randu();
  module.Reset();

  // Test the Forward function.
  input = arma::zeros(10, 1);
  module.Forward(std::move(input), std::move(output));
  BOOST_REQUIRE_EQUAL(0, arma::accu(output));

  // Test the Backward function.
  module.Backward(std::move(input), std::move(input), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 0);
}

/**
 * Jacobian linear no bias module test.
 */
BOOST_AUTO_TEST_CASE(JacobianLinearNoBiasLayerTest)
{
  for (size_t i = 0; i < 5; i++)
  {
    const size_t inputElements = math::RandInt(2, 1000);
    const size_t outputElements = math::RandInt(2, 1000);

    arma::mat input;
    input.set_size(inputElements, 1);

    LinearNoBias<> module(inputElements, outputElements);
    module.Parameters().randu();

    double error = JacobianTest(module, input);
    BOOST_REQUIRE_LE(error, 1e-5);
  }
}

/**
 * LinearNoBias layer numerically gradient test.
 */
BOOST_AUTO_TEST_CASE(GradientLinearNoBiasLayerTest)
{
  // LinearNoBias function gradient instantiation.
  struct GradientFunction
  {
    GradientFunction()
    {
      input = arma::randu(10, 1);
      target = arma::mat("1");

      model = new FFN<NegativeLogLikelihood<>, NguyenWidrowInitialization>();
      model->Predictors() = input;
      model->Responses() = target;
      model->Add<IdentityLayer<> >();
      model->Add<LinearNoBias<> >(10, 2);
      model->Add<LogSoftMax<> >();
    }

    ~GradientFunction()
    {
      delete model;
    }

    double Gradient(arma::mat& gradient) const
    {
      arma::mat output;
      double error = model->Evaluate(model->Parameters(), 0, 1);
      model->Gradient(model->Parameters(), 0, gradient, 1);
      return error;
    }

    arma::mat& Parameters() { return model->Parameters(); }

    FFN<NegativeLogLikelihood<>, NguyenWidrowInitialization>* model;
    arma::mat input, target;
  } function;

  BOOST_REQUIRE_LE(CheckGradient(function), 1e-4);
}

/**
 * Jacobian negative log likelihood module test.
 */
BOOST_AUTO_TEST_CASE(JacobianNegativeLogLikelihoodLayerTest)
{
  for (size_t i = 0; i < 5; i++)
  {
    NegativeLogLikelihood<> module;
    const size_t inputElements = math::RandInt(5, 100);
    arma::mat input;
    RandomInitialization init(0, 1);
    init.Initialize(input, inputElements, 1);

    arma::mat target(1, 1);
    target(0) = math::RandInt(1, inputElements - 1);

    double error = JacobianPerformanceTest(module, input, target);
    BOOST_REQUIRE_LE(error, 1e-5);
  }
}

/**
 * Jacobian LeakyReLU module test.
 */
BOOST_AUTO_TEST_CASE(JacobianLeakyReLULayerTest)
{
  for (size_t i = 0; i < 5; i++)
  {
    const size_t inputElements = math::RandInt(2, 1000);

    arma::mat input;
    input.set_size(inputElements, 1);

    LeakyReLU<> module;

    double error = JacobianTest(module, input);
    BOOST_REQUIRE_LE(error, 1e-5);
  }
}

/**
 * Jacobian FlexibleReLU module test.
 */
BOOST_AUTO_TEST_CASE(JacobianFlexibleReLULayerTest)
{
  for (size_t i = 0; i < 5; i++)
  {
    const size_t inputElements = math::RandInt(2, 1000);

    arma::mat input;
    input.set_size(inputElements, 1);

    FlexibleReLU<> module;

    double error = JacobianTest(module, input);
    BOOST_REQUIRE_LE(error, 1e-5);
  }
}

/**
 * Flexible ReLU layer numerically gradient test.
 */
BOOST_AUTO_TEST_CASE(GradientFlexibleReLULayerTest)
{
  // Add function gradient instantiation.
  struct GradientFunction
  {
    GradientFunction()
    {
      input = arma::randu(2, 1);
      target = arma::mat("1");

      model = new FFN<NegativeLogLikelihood<>, RandomInitialization>(
          NegativeLogLikelihood<>(), RandomInitialization(0.1, 0.5));

      model->Predictors() = input;
      model->Responses() = target;
      model->Add<LinearNoBias<> >(2, 5);
      model->Add<FlexibleReLU<> >(0.05);
      model->Add<LogSoftMax<> >();
    }

    ~GradientFunction()
    {
      delete model;
    }

    double Gradient(arma::mat& gradient) const
    {
      arma::mat output;
      double error = model->Evaluate(model->Parameters(), 0, 1);
      model->Gradient(model->Parameters(), 0, gradient, 1);
      return error;
    }

    arma::mat& Parameters() { return model->Parameters(); }

    FFN<NegativeLogLikelihood<>, RandomInitialization>* model;
    arma::mat input, target;
  } function;

  BOOST_REQUIRE_LE(CheckGradient(function), 1e-4);
}

/**
 * Jacobian MultiplyConstant module test.
 */
BOOST_AUTO_TEST_CASE(JacobianMultiplyConstantLayerTest)
{
  for (size_t i = 0; i < 5; i++)
  {
    const size_t inputElements = math::RandInt(2, 1000);

    arma::mat input;
    input.set_size(inputElements, 1);

    MultiplyConstant<> module(3.0);

    double error = JacobianTest(module, input);
    BOOST_REQUIRE_LE(error, 1e-5);
  }
}

/**
 * Jacobian HardTanH module test.
 */
BOOST_AUTO_TEST_CASE(JacobianHardTanHLayerTest)
{
  for (size_t i = 0; i < 5; i++)
  {
    const size_t inputElements = math::RandInt(2, 1000);

    arma::mat input;
    input.set_size(inputElements, 1);

    HardTanH<> module;

    double error = JacobianTest(module, input);
    BOOST_REQUIRE_LE(error, 1e-5);
  }
}

/**
 * Simple select module test.
 */
BOOST_AUTO_TEST_CASE(SimpleSelectLayerTest)
{
  arma::mat outputA, outputB, input, delta;

  input = arma::ones(10, 5);
  for (size_t i = 0; i < input.n_cols; ++i)
  {
    input.col(i) *= i;
  }

  // Test the Forward function.
  Select<> moduleA(3);
  moduleA.Forward(std::move(input), std::move(outputA));
  BOOST_REQUIRE_EQUAL(30, arma::accu(outputA));

  // Test the Forward function.
  Select<> moduleB(3, 5);
  moduleB.Forward(std::move(input), std::move(outputB));
  BOOST_REQUIRE_EQUAL(15, arma::accu(outputB));

  // Test the Backward function.
  moduleA.Backward(std::move(input), std::move(outputA), std::move(delta));
  BOOST_REQUIRE_EQUAL(30, arma::accu(delta));

  // Test the Backward function.
  moduleB.Backward(std::move(input), std::move(outputA), std::move(delta));
  BOOST_REQUIRE_EQUAL(15, arma::accu(delta));
}

/**
 * Simple join module test.
 */
BOOST_AUTO_TEST_CASE(SimpleJoinLayerTest)
{
  arma::mat output, input, delta;
  input = arma::ones(10, 5);

  // Test the Forward function.
  Join<> module;
  module.Forward(std::move(input), std::move(output));
  BOOST_REQUIRE_EQUAL(50, arma::accu(output));

  bool b = output.n_rows == 1 || output.n_cols == 1;
  BOOST_REQUIRE_EQUAL(b, true);

  // Test the Backward function.
  module.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(50, arma::accu(delta));

  b = delta.n_rows == input.n_rows && input.n_cols;
  BOOST_REQUIRE_EQUAL(b, true);
}

/**
 * Simple add merge module test.
 */
BOOST_AUTO_TEST_CASE(SimpleAddMergeLayerTest)
{
  arma::mat output, input, delta;
  input = arma::ones(10, 1);

  for (size_t i = 0; i < 5; ++i)
  {
    AddMerge<> module;
    const size_t numMergeModules = math::RandInt(2, 10);
    for (size_t m = 0; m < numMergeModules; ++m)
    {
      IdentityLayer<> identityLayer;
      identityLayer.Forward(std::move(input),
          std::move(identityLayer.OutputParameter()));

      module.Add(identityLayer);
    }

    // Test the Forward function.
    module.Forward(std::move(input), std::move(output));
    BOOST_REQUIRE_EQUAL(10 * numMergeModules, arma::accu(output));

    // Test the Backward function.
    module.Backward(std::move(input), std::move(output), std::move(delta));
    BOOST_REQUIRE_EQUAL(arma::accu(output), arma::accu(delta));
  }
}

/**
 * Test the LSTM layer with a user defined rho parameter and without.
 */
BOOST_AUTO_TEST_CASE(LSTMRrhoTest)
{
  const size_t rho = 5;
  arma::cube input = arma::randu(1, 1, 5);
  arma::cube target = arma::ones(1, 1, 5);
  RandomInitialization init(0.5, 0.5);

  // Create model with user defined rho parameter.
  RNN<NegativeLogLikelihood<>, RandomInitialization> modelA(
      rho, false, NegativeLogLikelihood<>(), init);
  modelA.Add<IdentityLayer<> >();
  modelA.Add<Linear<> >(1, 10);

  // Use LSTM layer with rho.
  modelA.Add<LSTM<> >(10, 3, rho);
  modelA.Add<LogSoftMax<> >();

  // Create model without user defined rho parameter.
  RNN<NegativeLogLikelihood<> > modelB(
      rho, false, NegativeLogLikelihood<>(), init);
  modelB.Add<IdentityLayer<> >();
  modelB.Add<Linear<> >(1, 10);

  // Use LSTM layer with rho = MAXSIZE.
  modelB.Add<LSTM<> >(10, 3);
  modelB.Add<LogSoftMax<> >();

  optimization::StandardSGD opt(0.1, 1, 5, -100, false);
  modelA.Train(input, target, opt);
  modelB.Train(input, target, opt);

  CheckMatrices(modelB.Parameters(), modelA.Parameters());
}

/**
 * LSTM layer numerical gradient test.
 */
BOOST_AUTO_TEST_CASE(GradientLSTMLayerTest)
{
  // LSTM function gradient instantiation.
  struct GradientFunction
  {
    GradientFunction()
    {
      input = arma::randu(1, 1, 5);
      target.ones(1, 1, 5);
      const size_t rho = 5;

      model = new RNN<NegativeLogLikelihood<> >(rho);
      model->Predictors() = input;
      model->Responses() = target;
      model->Add<IdentityLayer<> >();
      model->Add<Linear<> >(1, 10);
      model->Add<LSTM<> >(10, 3, rho);
      model->Add<LogSoftMax<> >();
    }

    ~GradientFunction()
    {
      delete model;
    }

    double Gradient(arma::mat& gradient) const
    {
      arma::mat output;
      double error = model->Evaluate(model->Parameters(), 0, 1);
      model->Gradient(model->Parameters(), 0, gradient, 1);
      return error;
    }

    arma::mat& Parameters() { return model->Parameters(); }

    RNN<NegativeLogLikelihood<> >* model;
    arma::cube input, target;
  } function;

  BOOST_REQUIRE_LE(CheckGradient(function), 1e-4);
}

/**
 * Test the FastLSTM layer with a user defined rho parameter and without.
 */
BOOST_AUTO_TEST_CASE(FastLSTMRrhoTest)
{
  const size_t rho = 5;
  arma::cube input = arma::randu(1, 1, 5);
  arma::cube target = arma::ones(1, 1, 5);
  RandomInitialization init(0.5, 0.5);

  // Create model with user defined rho parameter.
  RNN<NegativeLogLikelihood<>, RandomInitialization> modelA(
      rho, false, NegativeLogLikelihood<>(), init);
  modelA.Add<IdentityLayer<> >();
  modelA.Add<Linear<> >(1, 10);

  // Use FastLSTM layer with rho.
  modelA.Add<FastLSTM<> >(10, 3, rho);
  modelA.Add<LogSoftMax<> >();

  // Create model without user defined rho parameter.
  RNN<NegativeLogLikelihood<> > modelB(
      rho, false, NegativeLogLikelihood<>(), init);
  modelB.Add<IdentityLayer<> >();
  modelB.Add<Linear<> >(1, 10);

  // Use FastLSTM layer with rho = MAXSIZE.
  modelB.Add<FastLSTM<> >(10, 3);
  modelB.Add<LogSoftMax<> >();

  optimization::StandardSGD opt(0.1, 1, 5, -100, false);
  modelA.Train(input, target, opt);
  modelB.Train(input, target, opt);

  CheckMatrices(modelB.Parameters(), modelA.Parameters());
}

/**
 * FastLSTM layer numerical gradient test.
 */
BOOST_AUTO_TEST_CASE(GradientFastLSTMLayerTest)
{
  // Fast LSTM function gradient instantiation.
  struct GradientFunction
  {
    GradientFunction()
    {
      input = arma::randu(1, 1, 5);
      target = arma::ones(1, 1, 5);
      const size_t rho = 5;

      model = new RNN<NegativeLogLikelihood<> >(rho);
      model->Predictors() = input;
      model->Responses() = target;
      model->Add<IdentityLayer<> >();
      model->Add<Linear<> >(1, 10);
      model->Add<FastLSTM<> >(10, 3, rho);
      model->Add<LogSoftMax<> >();
    }

    ~GradientFunction()
    {
      delete model;
    }

    double Gradient(arma::mat& gradient) const
    {
      arma::mat output;
      double error = model->Evaluate(model->Parameters(), 0, 1);
      model->Gradient(model->Parameters(), 0, gradient, 1);
      return error;
    }

    arma::mat& Parameters() { return model->Parameters(); }

    RNN<NegativeLogLikelihood<> >* model;
    arma::cube input, target;
  } function;

  // The threshold should be << 0.1 but since the Fast LSTM layer uses an
  // approximation of the sigmoid function the estimated gradient is not
  // correct.
  BOOST_REQUIRE_LE(CheckGradient(function), 0.2);
}

/**
 * Check if the gradients computed by GRU cell are close enough to the
 * approximation of the gradients.
 */
BOOST_AUTO_TEST_CASE(GradientGRULayerTest)
{
  // GRU function gradient instantiation.
  struct GradientFunction
  {
    GradientFunction()
    {
      input = arma::randu(1, 1, 5);
      target = arma::ones(1, 1, 5);
      const size_t rho = 5;

      model = new RNN<NegativeLogLikelihood<> >(rho);
      model->Predictors() = input;
      model->Responses() = target;
      model->Add<IdentityLayer<> >();
      model->Add<Linear<> >(1, 10);
      model->Add<GRU<> >(10, 3, rho);
      model->Add<LogSoftMax<> >();
    }

    ~GradientFunction()
    {
      delete model;
    }

    double Gradient(arma::mat& gradient) const
    {
      arma::mat output;
      double error = model->Evaluate(model->Parameters(), 0, 1);
      model->Gradient(model->Parameters(), 0, gradient, 1);
      return error;
    }

    arma::mat& Parameters() { return model->Parameters(); }

    RNN<NegativeLogLikelihood<> >* model;
    arma::cube input, target;
  } function;

  BOOST_REQUIRE_LE(CheckGradient(function), 1e-4);
}

/**
 * GRU layer manual forward test.
 */
BOOST_AUTO_TEST_CASE(ForwardGRULayerTest)
{
  GRU<> gru(3, 3, 5);

  // Initialize the weights to all ones.
  NetworkInitialization<ConstInitialization>
    networkInit(ConstInitialization(1));
  networkInit.Initialize(gru.Model(), gru.Parameters());

  // Provide input of all ones.
  arma::mat input = arma::ones(3, 1);
  arma::mat output;

  gru.Forward(std::move(input), std::move(output));

  // Compute the z_t gate output.
  arma::mat expectedOutput = arma::ones(3, 1);
  expectedOutput *= -4;
  expectedOutput = arma::exp(expectedOutput);
  expectedOutput = arma::ones(3, 1) / (arma::ones(3, 1) + expectedOutput);
  expectedOutput = (arma::ones(3, 1)  - expectedOutput) % expectedOutput;

  // For the first input the output should be equal to the output of
  // gate z_t as the previous output fed to the cell is all zeros.
  BOOST_REQUIRE_LE(arma::as_scalar(arma::trans(output) * expectedOutput), 1e-2);

  expectedOutput = output;

  gru.Forward(std::move(input), std::move(output));

  double s = arma::as_scalar(arma::sum(expectedOutput));

  // Compute the value of z_t gate for the second input.
  arma::mat z_t = arma::ones(3, 1);
  z_t *= -(s + 4);
  z_t = arma::exp(z_t);
  z_t = arma::ones(3, 1) / (arma::ones(3, 1) + z_t);

  // Compute the value of o_t gate for the second input.
  arma::mat o_t = arma::ones(3, 1);
  o_t *= -(arma::as_scalar(arma::sum(expectedOutput % z_t)) + 4);
  o_t = arma::exp(o_t);
  o_t = arma::ones(3, 1) / (arma::ones(3, 1) + o_t);

  // Expected output for the second input.
  expectedOutput = z_t % expectedOutput + (arma::ones(3, 1) - z_t) % o_t;

  BOOST_REQUIRE_LE(arma::as_scalar(arma::trans(output) * expectedOutput), 1e-2);
}

/**
 * Simple concat module test.
 */
BOOST_AUTO_TEST_CASE(SimpleConcatLayerTest)
{
  arma::mat output, input, delta, error;

  Linear<> moduleA(10, 10);
  moduleA.Parameters().randu();
  moduleA.Reset();

  Linear<> moduleB(10, 10);
  moduleB.Parameters().randu();
  moduleB.Reset();

  Concat<> module;
  module.Add(moduleA);
  module.Add(moduleB);

  // Test the Forward function.
  input = arma::zeros(10, 1);
  module.Forward(std::move(input), std::move(output));

  BOOST_REQUIRE_CLOSE(arma::accu(
      moduleA.Parameters().submat(100, 0, moduleA.Parameters().n_elem - 1, 0)),
      arma::accu(output.col(0)), 1e-3);

  BOOST_REQUIRE_CLOSE(arma::accu(
      moduleB.Parameters().submat(100, 0, moduleB.Parameters().n_elem - 1, 0)),
      arma::accu(output.col(1)), 1e-3);

  // Test the Backward function.
  error = arma::zeros(10, 2);
  module.Backward(std::move(input), std::move(error), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 0);
}

/**
 * Concat layer numerically gradient test.
 */
BOOST_AUTO_TEST_CASE(GradientConcatLayerTest)
{
  // Concat function gradient instantiation.
  struct GradientFunction
  {
    GradientFunction()
    {
      input = arma::randu(10, 1);
      target = arma::mat("1");

      model = new FFN<NegativeLogLikelihood<>, NguyenWidrowInitialization>();
      model->Predictors() = input;
      model->Responses() = target;
      model->Add<IdentityLayer<> >();

      concat = new Concat<>();
      concat->Add<Linear<> >(10, 2);
      model->Add(concat);

      model->Add<LogSoftMax<> >();
    }

    ~GradientFunction()
    {
      delete model;
    }

    double Gradient(arma::mat& gradient) const
    {
      arma::mat output;
      double error = model->Evaluate(model->Parameters(), 0, 1);
      model->Gradient(model->Parameters(), 0, gradient, 1);
      return error;
    }

    arma::mat& Parameters() { return model->Parameters(); }

    FFN<NegativeLogLikelihood<>, NguyenWidrowInitialization>* model;
    Concat<>* concat;
    arma::mat input, target;
  } function;

  BOOST_REQUIRE_LE(CheckGradient(function), 1e-4);
}

/**
 * Simple lookup module test.
 */
BOOST_AUTO_TEST_CASE(SimpleLookupLayerTest)
{
  arma::mat output, input, delta, gradient;
  Lookup<> module(10, 5);
  module.Parameters().randu();

  // Test the Forward function.
  input = arma::zeros(2, 1);
  input(0) = 1;
  input(1) = 3;

  module.Forward(std::move(input), std::move(output));

  // The Lookup module uses index - 1 for the cols.
  const double outputSum = arma::accu(module.Parameters().col(0)) +
      arma::accu(module.Parameters().col(2));

  BOOST_REQUIRE_CLOSE(outputSum, arma::accu(output), 1e-3);

  // Test the Backward function.
  module.Backward(std::move(input), std::move(input), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(input), arma::accu(input));

  // Test the Gradient function.
  arma::mat error = arma::ones(2, 5);
  error = error.t();
  error.col(1) *= 0.5;

  module.Gradient(std::move(input), std::move(error), std::move(gradient));

  // The Lookup module uses index - 1 for the cols.
  const double gradientSum = arma::accu(gradient.col(0)) +
      arma::accu(gradient.col(2));

  BOOST_REQUIRE_CLOSE(gradientSum, arma::accu(error), 1e-3);
  BOOST_REQUIRE_CLOSE(arma::accu(gradient), arma::accu(error), 1e-3);
}

/**
 * Simple LogSoftMax module test.
 */
BOOST_AUTO_TEST_CASE(SimpleLogSoftmaxLayerTest)
{
  arma::mat output, input, error, delta;
  LogSoftMax<> module;

  // Test the Forward function.
  input = arma::mat("0.5; 0.5");
  module.Forward(std::move(input), std::move(output));
  BOOST_REQUIRE_SMALL(arma::accu(arma::abs(
    arma::mat("-0.6931; -0.6931") - output)), 1e-3);

  // Test the Backward function.
  error = arma::zeros(input.n_rows, input.n_cols);
  // Assume LogSoftmax layer is always associated with NLL output layer.
  error(1, 0) = -1;
  module.Backward(std::move(input), std::move(error), std::move(delta));
  BOOST_REQUIRE_SMALL(arma::accu(arma::abs(
      arma::mat("1.6487; 0.6487") - delta)), 1e-3);
}

/*
 * Simple test for the BilinearInterpolation layer
 */
BOOST_AUTO_TEST_CASE(SimpleBilinearInterpolationLayerTest)
{
  // Tested output against tensorflow.image.resize_bilinear()
  arma::mat input, output, unzoomedOutput, expectedOutput;
  size_t inRowSize = 2;
  size_t inColSize = 2;
  size_t outRowSize = 5;
  size_t outColSize = 5;
  size_t depth = 1;
  input.zeros(inRowSize * inColSize * depth, 1);
  input[0] = 1.0;
  input[1] = input[2] = 2.0;
  input[3] = 3.0;
  BilinearInterpolation<> layer(inRowSize, inColSize, outRowSize, outColSize,
      depth);
  expectedOutput = arma::mat("1.0000 1.4000 1.8000 2.0000 2.0000 \
      1.4000 1.8000 2.2000 2.4000 2.4000 \
      1.8000 2.2000 2.6000 2.8000 2.8000 \
      2.0000 2.4000 2.8000 3.0000 3.0000 \
      2.0000 2.4000 2.8000 3.0000 3.0000");
  expectedOutput.reshape(25, 1);
  layer.Forward(std::move(input), std::move(output));
  CheckMatrices(output - expectedOutput, arma::zeros(output.n_rows), 1e-12);

  expectedOutput = arma::mat("1.0000 1.9000 1.9000 2.8000");
  expectedOutput.reshape(4, 1);
  layer.Backward(std::move(output), std::move(output),
      std::move(unzoomedOutput));
  CheckMatrices(unzoomedOutput - expectedOutput,
      arma::zeros(input.n_rows), 1e-12);
}

/**
 * Tests the BatchNorm Layer, compares the layers parameters with
 * the values from another implementation.
 * Link to the implementation - http://cthorey.github.io./backpropagation/
 */
BOOST_AUTO_TEST_CASE(BatchNormTest)
{
  arma::mat input, output;
  input << 5.1 << 3.5 << 1.4 << arma::endr
        << 4.9 << 3.0 << 1.4 << arma::endr
        << 4.7 << 3.2 << 1.3 << arma::endr;

  BatchNorm<> model(input.n_rows);
  model.Reset();

  // Non-Deteministic Forward Pass Test.
  model.Deterministic() = false;
  model.Forward(std::move(input), std::move(output));
  arma::mat result;
  result << 1.1658 << 0.1100 << -1.2758 << arma::endr
         << 1.2579 << -0.0699 << -1.1880 << arma::endr
         << 1.1737 << 0.0958 << -1.2695 << arma::endr;

  CheckMatrices(output, result, 1e-1);
  result.clear();

  // Deterministic Forward Pass test.
  output = model.TrainingMean();
  result << 3.33333333 << arma::endr
         << 3.1 << arma::endr
         << 3.06666666 << arma::endr;

  CheckMatrices(output, result, 1e-1);
  result.clear();

  output = model.TrainingVariance();
  result << 2.2956 << arma::endr
         << 2.0467 << arma::endr
         << 1.9356 << arma::endr;

  CheckMatrices(output, result, 1e-1);
  result.clear();

  model.Deterministic() = true;
  model.Forward(std::move(input), std::move(output));

  result << 1.1658 << 0.1100 << -1.2757 << arma::endr
         << 1.2579 << -0.0699 << -1.1880 << arma::endr
         << 1.1737 << 0.0958 << -1.2695 << arma::endr;

  CheckMatrices(output, result, 1e-1);
}

/**
 * BatchNorm layer numerically gradient test.
 */
BOOST_AUTO_TEST_CASE(GradientBatchNormTest)
{
  // Add function gradient instantiation.
  struct GradientFunction
  {
    GradientFunction()
    {
      input = arma::randn(10, 256);
      arma::mat target;
      target.ones(1, 256);

      model = new FFN<NegativeLogLikelihood<>, NguyenWidrowInitialization>();
      model->Predictors() = input;
      model->Responses() = target;
      model->Add<IdentityLayer<> >();
      model->Add<BatchNorm<> >(10);
      model->Add<Linear<> >(10, 2);
      model->Add<LogSoftMax<> >();
    }

    ~GradientFunction()
    {
      delete model;
    }

    double Gradient(arma::mat& gradient) const
    {
      arma::mat output;
      double error = model->Evaluate(model->Parameters(), 0, 256, false);
      model->Gradient(model->Parameters(), 0, gradient, 256);
      return error;
    }

    arma::mat& Parameters() { return model->Parameters(); }

    FFN<NegativeLogLikelihood<>, NguyenWidrowInitialization>* model;
    arma::mat input, target;
  } function;

  BOOST_REQUIRE_LE(CheckGradient(function), 1e-4);
}

/**
 * Simple Transposed Convolution layer test.
 */
BOOST_AUTO_TEST_CASE(SimpleTransposedConvolutionLayerTest)
{
  arma::mat output, input, delta;

  TransposedConvolution<> module1(1, 1, 3, 3, 1, 1, 0, 0, 4, 4);
  // Test the Forward function.
  input = arma::linspace<arma::colvec>(0, 15, 16);
  module1.Parameters() = arma::mat(9 + 1, 1, arma::fill::zeros);
  module1.Parameters()(0) = 1.0;
  module1.Parameters()(8) = 2.0;
  module1.Reset();
  module1.Forward(std::move(input), std::move(output));
  // Value calculated using tensorflow.nn.conv2d_transpose()
  BOOST_REQUIRE_EQUAL(arma::accu(output), 360.0);

  // Test the Backward function.
  module1.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 720);

  TransposedConvolution<> module2(1, 1, 4, 4, 1, 1, 2, 2, 5, 5);
  // Test the forward function.
  input = arma::linspace<arma::colvec>(0, 24, 25);
  module2.Parameters() = arma::mat(16 + 1, 1, arma::fill::zeros);
  module2.Parameters()(0) = 1.0;
  module2.Parameters()(3) = 1.0;
  module2.Parameters()(6) = 1.0;
  module2.Parameters()(9) = 1.0;
  module2.Parameters()(12) = 1.0;
  module2.Parameters()(15) = 2.0;
  module2.Reset();
  module2.Forward(std::move(input), std::move(output));
  // Value calculated using tensorflow.nn.conv2d_transpose()
  BOOST_REQUIRE_EQUAL(arma::accu(output), 2100.0);

  // Test the backward function.
  module2.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 7740);

  TransposedConvolution<> module3(1, 1, 3, 3, 1, 1, 1, 1, 5, 5);
  // Test the forward function.
  input = arma::linspace<arma::colvec>(0, 24, 25);
  module3.Parameters() = arma::mat(9 + 1, 1, arma::fill::zeros);
  module3.Parameters()(1) = 2.0;
  module3.Parameters()(2) = 4.0;
  module3.Parameters()(3) = 3.0;
  module3.Parameters()(8) = 1.0;
  module3.Reset();
  module3.Forward(std::move(input), std::move(output));
  // Value calculated using tensorflow.nn.conv2d_transpose()
  BOOST_REQUIRE_EQUAL(arma::accu(output), 3000.0);

  // Test the backward function.
  module3.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 21480);

  TransposedConvolution<> module4(1, 1, 3, 3, 1, 1, 2, 2, 5, 5);
  // Test the forward function.
  input = arma::linspace<arma::colvec>(0, 24, 25);
  module4.Parameters() = arma::mat(9 + 1, 1, arma::fill::zeros);
  module4.Parameters()(2) = 2.0;
  module4.Parameters()(4) = 4.0;
  module4.Parameters()(6) = 6.0;
  module4.Parameters()(8) = 8.0;
  module4.Reset();
  module4.Forward(std::move(input), std::move(output));
  // Value calculated using tensorflow.nn.conv2d_transpose()
  BOOST_REQUIRE_EQUAL(arma::accu(output), 6000.0);

  // Test the backward function.
  module4.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 86208);

  TransposedConvolution<> module5(1, 1, 3, 3, 2, 2, 0, 0, 5, 5);
  // Test the forward function.
  input = arma::linspace<arma::colvec>(0, 24, 25);
  module5.Parameters() = arma::mat(9 + 1, 1, arma::fill::zeros);
  module5.Parameters()(2) = 8.0;
  module5.Parameters()(4) = 6.0;
  module5.Parameters()(6) = 4.0;
  module5.Parameters()(8) = 2.0;
  module5.Reset();
  module5.Forward(std::move(input), std::move(output));
  // Value calculated using tensorflow.nn.conv2d_transpose()
  BOOST_REQUIRE_EQUAL(arma::accu(output), 6000.0);

  // Test the backward function.
  module5.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 83808);

  TransposedConvolution<> module6(1, 1, 3, 3, 2, 2, 1, 1, 5, 5);
  // Test the forward function.
  input = arma::linspace<arma::colvec>(0, 24, 25);
  module6.Parameters() = arma::mat(9 + 1, 1, arma::fill::zeros);
  module6.Parameters()(0) = 8.0;
  module6.Parameters()(3) = 6.0;
  module6.Parameters()(6) = 2.0;
  module6.Parameters()(8) = 4.0;
  module6.Reset();
  module6.Forward(std::move(input), std::move(output));
  // Value calculated using tensorflow.nn.conv2d_transpose()
  BOOST_REQUIRE_EQUAL(arma::accu(output), 6000.0);

  // Test the backward function.
  module6.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 87264);

  TransposedConvolution<> module7(1, 1, 3, 3, 2, 2, 1, 1, 6, 6);
  // Test the forward function.
  input = arma::linspace<arma::colvec>(0, 35, 36);
  module7.Parameters() = arma::mat(9 + 1, 1, arma::fill::zeros);
  module7.Parameters()(0) = 8.0;
  module7.Parameters()(2) = 6.0;
  module7.Parameters()(4) = 2.0;
  module7.Parameters()(8) = 4.0;
  module7.Reset();
  module7.Forward(std::move(input), std::move(output));
  // Value calculated using tensorflow.nn.conv2d_transpose()
  BOOST_REQUIRE_EQUAL(arma::accu(output), 12600.0);

  // Test the backward function.
  module7.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 185500);
}

/**
 * Transposed Convolution layer numerically gradient test.
 */
BOOST_AUTO_TEST_CASE(GradientTransposedConvolutionLayerTest)
{
  // Add function gradient instantiation.
  struct GradientFunction
  {
    GradientFunction()
    {
      input = arma::linspace<arma::colvec>(0, 35, 36);
      target = arma::mat("1");

      model = new FFN<NegativeLogLikelihood<>, RandomInitialization>();
      model->Predictors() = input;
      model->Responses() = target;
      model->Add<TransposedConvolution<> >(1, 1, 3, 3, 2, 2, 1, 1, 6, 6);
      model->Add<LogSoftMax<> >();
    }

    ~GradientFunction()
    {
      delete model;
    }

    double Gradient(arma::mat& gradient) const
    {
      arma::mat output;
      double error = model->Evaluate(model->Parameters(), 0, 1);
      model->Gradient(model->Parameters(), 0, gradient, 1);
      return error;
    }

    arma::mat& Parameters() { return model->Parameters(); }

    FFN<NegativeLogLikelihood<>, RandomInitialization>* model;
    arma::mat input, target;
  } function;

  BOOST_REQUIRE_LE(CheckGradient(function), 1e-3);
}

/**
 * Simple MultiplyMerge module test.
 */
BOOST_AUTO_TEST_CASE(SimpleMultiplyMergeLayerTest)
{
  arma::mat output, input, delta;
  input = arma::ones(10, 1);

  for (size_t i = 0; i < 5; ++i)
  {
    MultiplyMerge<> module;
    const size_t numMergeModules = math::RandInt(2, 10);
    for (size_t m = 0; m < numMergeModules; ++m)
    {
      IdentityLayer<> identityLayer;
      identityLayer.Forward(std::move(input),
          std::move(identityLayer.OutputParameter()));

      module.Add(identityLayer);
    }

    // Test the Forward function.
    module.Forward(std::move(input), std::move(output));
    BOOST_REQUIRE_EQUAL(10, arma::accu(output));

    // Test the Backward function.
    module.Backward(std::move(input), std::move(output), std::move(delta));
    BOOST_REQUIRE_EQUAL(arma::accu(output), arma::accu(delta));
  }
}

/**
 * Simple Atrous Convolution layer test.
 */
BOOST_AUTO_TEST_CASE(SimpleAtrousConvolutionLayerTest)
{
  arma::mat output, input, delta;

  AtrousConvolution<> module1(1, 1, 3, 3, 1, 1, 0, 0, 7, 7, 2, 2);
  // Test the Forward function.
  input = arma::linspace<arma::colvec>(0, 48, 49);
  module1.Parameters() = arma::mat(9 + 1, 1, arma::fill::zeros);
  module1.Parameters()(0) = 1.0;
  module1.Parameters()(8) = 2.0;
  module1.Reset();
  module1.Forward(std::move(input), std::move(output));
  // Value calculated using tensorflow.nn.atrous_conv2d()
  BOOST_REQUIRE_EQUAL(arma::accu(output), 792.0);

  // Test the Backward function.
  module1.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 2376);

  AtrousConvolution<> module2(1, 1, 3, 3, 2, 2, 0, 0, 7, 7, 2, 2);
  // Test the forward function.
  input = arma::linspace<arma::colvec>(0, 48, 49);
  module2.Parameters() = arma::mat(9 + 1, 1, arma::fill::zeros);
  module2.Parameters()(0) = 1.0;
  module2.Parameters()(3) = 1.0;
  module2.Parameters()(6) = 1.0;
  module2.Reset();
  module2.Forward(std::move(input), std::move(output));
  // Value calculated using tensorflow.nn.conv2d()
  BOOST_REQUIRE_EQUAL(arma::accu(output), 264.0);

  // Test the backward function.
  module2.Backward(std::move(input), std::move(output), std::move(delta));
  BOOST_REQUIRE_EQUAL(arma::accu(delta), 792.0);
}

/**
 * Atrous Convolution layer numerically gradient test.
 */
BOOST_AUTO_TEST_CASE(GradientAtrousConvolutionLayerTest)
{
  // Add function gradient instantiation.
  struct GradientFunction
  {
    GradientFunction()
    {
      input = arma::linspace<arma::colvec>(0, 35, 36);
      target = arma::mat("1");

      model = new FFN<NegativeLogLikelihood<>, RandomInitialization>();
      model->Predictors() = input;
      model->Responses() = target;
      model->Add<AtrousConvolution<> >(1, 1, 3, 3, 1, 1, 0, 0, 6, 6, 2, 2);
      model->Add<LogSoftMax<> >();
    }

    ~GradientFunction()
    {
      delete model;
    }

    double Gradient(arma::mat& gradient) const
    {
      arma::mat output;
      double error = model->Evaluate(model->Parameters(), 0, 1);
      model->Gradient(model->Parameters(), 0, gradient, 1);
      return error;
    }

    arma::mat& Parameters() { return model->Parameters(); }

    FFN<NegativeLogLikelihood<>, RandomInitialization>* model;
    arma::mat input, target;
  } function;

  BOOST_REQUIRE_LE(CheckGradient(function), 1e-3);
}

/**
 * Tests the LayerNorm layer.
 */
BOOST_AUTO_TEST_CASE(LayerNormTest)
{
  arma::mat input, output;
  input << 5.1 << 3.5 << arma::endr
        << 4.9 << 3.0 << arma::endr
        << 4.7 << 3.2 << arma::endr;

  LayerNorm<> model(input.n_cols);
  model.Reset();

  model.Forward(std::move(input), std::move(output));
  arma::mat result;
  result << 1.2247 << 1.2978 << arma::endr
         << 0 << -1.1355 << arma::endr
         << -1.2247 << -0.1622 << arma::endr;

  CheckMatrices(output, result, 1e-1);
  result.clear();

  output = model.Mean();
  result << 4.9000 << 3.2333 << arma::endr;

  CheckMatrices(output, result, 1e-1);
  result.clear();

  output = model.Variance();
  result << 0.0267 << 0.0422 << arma::endr;

  CheckMatrices(output, result, 1e-1);
}

/**
 * LayerNorm layer numerically gradient test.
 */
BOOST_AUTO_TEST_CASE(GradientLayerNormTest)
{
  // Add function gradient instantiation.
  struct GradientFunction
  {
    GradientFunction()
    {
      input = arma::randn(10, 256);
      arma::mat target;
      target.ones(1, 256);

      model = new FFN<NegativeLogLikelihood<>, NguyenWidrowInitialization>();
      model->Predictors() = input;
      model->Responses() = target;
      model->Add<IdentityLayer<> >();
      model->Add<LayerNorm<> >(256);
      model->Add<Linear<> >(10, 2);
      model->Add<LogSoftMax<> >();
    }

    ~GradientFunction()
    {
      delete model;
    }

    double Gradient(arma::mat& gradient) const
    {
      arma::mat output;
      double error = model->Evaluate(model->Parameters(), 0, 256, false);
      model->Gradient(model->Parameters(), 0, gradient, 256);
      return error;
    }

    arma::mat& Parameters() { return model->Parameters(); }

    FFN<NegativeLogLikelihood<>, NguyenWidrowInitialization>* model;
    arma::mat input, target;
  } function;

  BOOST_REQUIRE_LE(CheckGradient(function), 1e-4);
}

BOOST_AUTO_TEST_SUITE_END();
