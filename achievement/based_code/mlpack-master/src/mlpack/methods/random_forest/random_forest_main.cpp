/**
 * @file random_forest_main.cpp
 * @author Ryan Curtin
 *
 * A program to build and evaluate random forests.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#include <mlpack/core.hpp>
#include <mlpack/methods/random_forest/random_forest.hpp>
#include <mlpack/methods/decision_tree/random_dimension_select.hpp>
#include <mlpack/core/util/mlpack_main.hpp>

using namespace mlpack;
using namespace mlpack::tree;
using namespace mlpack::util;
using namespace std;

PROGRAM_INFO("Random forests",
    "This program is an implementation of the standard random forest "
    "classification algorithm by Leo Breiman.  A random forest can be "
    "trained and saved for later use, or a random forest may be loaded "
    "and predictions or class probabilities for points may be generated."
    "\n\n"
    "This documentation will be rewritten once #880 is merged.");

PARAM_MATRIX_IN("training", "Training dataset.", "t");
PARAM_UROW_IN("labels", "Labels for training dataset.", "l");
PARAM_MATRIX_IN("test", "Test dataset to produce predictions for.", "T");
PARAM_UROW_IN("test_labels", "Test dataset labels, if accuracy calculation is "
    "desired.", "L");

PARAM_FLAG("print_training_accuracy", "If set, then the accuracy of the model "
    "on the training set will be predicted (verbose must also be specified).",
    "a");

PARAM_INT_IN("num_trees", "Number of trees in the random forest.", "N", 10);
PARAM_INT_IN("minimum_leaf_size", "Minimum number of points in each leaf "
    "node.", "n", 20);

PARAM_MATRIX_OUT("probabilities", "Predicted class probabilities for each "
    "point in the test set.", "P");
PARAM_UROW_OUT("predictions", "Predicted classes for each point in the test "
    "set.", "p");

/**
 * This is the class that we will serialize.  It is a pretty simple wrapper
 * around DecisionTree<>.  In order to support categoricals, it will need to
 * also hold and serialize a DatasetInfo.
 */
class RandomForestModel
{
 public:
  // The tree itself, left public for direct access by this program.
  RandomForest<> rf;

  // Create the model.
  RandomForestModel() { /* Nothing to do. */ }

  // Serialize the model.
  template<typename Archive>
  void serialize(Archive& ar, const unsigned int /* version */)
  {
    ar & BOOST_SERIALIZATION_NVP(rf);
  }
};

PARAM_MODEL_IN(RandomForestModel, "input_model", "Pre-trained random forest to "
    "use for classification.", "m");
PARAM_MODEL_OUT(RandomForestModel, "output_model", "Model to save trained "
    "random forest to.", "M");

static void mlpackMain()
{
  // Check for incompatible input parameters.
  RequireOnlyOnePassed({ "training", "input_model" }, true);

  ReportIgnoredParam({{ "training", false }}, "print_training_accuracy");

  if (CLI::HasParam("test"))
  {
    RequireAtLeastOnePassed({ "probabilities", "predictions" }, "no test output"
        " will be saved");
  }

  ReportIgnoredParam({{ "test", false }}, "test_labels");

  RequireAtLeastOnePassed({ "test", "output_model", "print_training_accuracy" },
      "the trained forest model will not be used or saved");

  if (CLI::HasParam("training"))
  {
    RequireAtLeastOnePassed({ "labels" }, true, "must pass labels when training"
        " set given");
  }

  RequireParamValue<int>("num_trees", [](int x) { return x > 0; }, true,
      "number of trees in forest must be positive");

  ReportIgnoredParam({{ "test", false }}, "predictions");
  ReportIgnoredParam({{ "test", false }}, "probabilities");

  RequireParamValue<int>("minimum_leaf_size", [](int x) { return x > 0; }, true,
      "minimum leaf size must be greater than 0");

  ReportIgnoredParam({{ "training", false }}, "num_trees");
  ReportIgnoredParam({{ "training", false }}, "minimum_leaf_size");

  RandomForestModel* rfModel;
  if (CLI::HasParam("training"))
  {
    rfModel = new RandomForestModel();

    // Train the model on the given input data.
    arma::mat data = std::move(CLI::GetParam<arma::mat>("training"));
    arma::Row<size_t> labels =
        std::move(CLI::GetParam<arma::Row<size_t>>("labels"));
    const size_t numTrees = (size_t) CLI::GetParam<int>("num_trees");
    const size_t minimumLeafSize =
        (size_t) CLI::GetParam<int>("minimum_leaf_size");

    Log::Info << "Training random forest with " << numTrees << " trees..."
        << endl;

    const size_t numClasses = arma::max(labels) + 1;

    // Train the model.
    rfModel->rf.Train(data, labels, numClasses, numTrees, minimumLeafSize);

    // Did we want training accuracy?
    if (CLI::HasParam("print_training_accuracy"))
    {
      arma::Row<size_t> predictions;
      rfModel->rf.Classify(data, predictions);

      const size_t correct = arma::accu(predictions == labels);

      Log::Info << correct << " of " << labels.n_elem << " correct on training"
          << " set (" << (double(correct) / double(labels.n_elem) * 100) << ")."
          << endl;
    }
  }
  else
  {
    // Then we must be loading a model.
    rfModel = CLI::GetParam<RandomForestModel*>("input_model");
  }

  if (CLI::HasParam("test"))
  {
    arma::mat testData = std::move(CLI::GetParam<arma::mat>("test"));

    // Get predictions and probabilities.
    arma::Row<size_t> predictions;
    arma::mat probabilities;
    rfModel->rf.Classify(testData, predictions, probabilities);

    // Did we want to calculate test accuracy?
    if (CLI::HasParam("test_labels"))
    {
      arma::Row<size_t> testLabels =
          std::move(CLI::GetParam<arma::Row<size_t>>("test_labels"));

      const size_t correct = arma::accu(predictions == testLabels);

      Log::Info << correct << " of " << testLabels.n_elem << " correct on test"
          << " set (" << (double(correct) / double(testLabels.n_elem) * 100)
          << ")." << endl;
    }

    // Save the outputs.
    CLI::GetParam<arma::mat>("probabilities") = std::move(probabilities);
    CLI::GetParam<arma::Row<size_t>>("predictions") = std::move(predictions);
  }

  // Save the output model.
  CLI::GetParam<RandomForestModel*>("output_model") = rfModel;
}
