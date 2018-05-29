/**
 * @file cli_quickstart.hpp
 * @author Ryan Curtin

@page cli_quickstart mlpack command-line quickstart guide

@section cli_quickstart_intro Introduction

This page describes how you can quickly get started using mlpack from the
command-line and gives a few examples of usage, and pointers to deeper
documentation.

This quickstart guide is also available for @ref python_quickstart "Python".

@section cli_quickstart_install Installing mlpack

Installing the mlpack is straightforward and can be done with your system's
package manager.

For instance, for Ubuntu or Debian the command is simply

@code{.sh}
sudo apt-get install mlpack-bin
@endcode

On Fedora or Red Hat:

@code{.sh}
sudo dnf install mlpack
@endcode

If you use a different distribution, mlpack may be packaged under a different
name.  And if it is not packaged, you can use a Docker image from Dockerhub:

@code{.sh}
docker run -it mlpack/mlpack /bin/bash
@endcode

This Docker image has mlpack already built and installed.

If you prefer to build mlpack from scratch, see @ref build.

@section cli_quickstart_example Simple mlpack quickstart example

As a really simple example of how to use mlpack from the command-line, let's do
some simple classification on a subset of the standard machine learning @c
covertype dataset.  We'll first split the dataset into a training set and a
testing set, then we'll train an mlpack random forest on the training data, and
finally we'll print the accuracy of the random forest on the test dataset.

You can copy-paste this code directly into your shell to run it.

@code{.sh}
# Get the dataset and unpack it.
wget http://www.mlpack.org/datasets/covertype-small.data.csv.gz
wget http://www.mlpack.org/datasets/covertype-small.labels.csv.gz
gunzip covertype-small.data.csv.gz covertype-small.labels.csv.gz

# Split the dataset; 70% into a training set and 30% into a test set.
# Each of these options has a shorthand single-character option but here we type
# it all out for clarity.
mlpack_preprocess_split                                       \
    --input_file covertype-small.data.csv                     \
    --input_labels_file covertype-small.labels.csv            \
    --training_file covertype-small.train.csv                 \
    --training_labels_file covertype-small.train.labels.csv   \
    --test_file covertype-small.test.csv                      \
    --test_labels_file covertype-small.test.labels.csv        \
    --test_ratio 0.3                                          \
    --verbose

# Train a random forest.
mlpack_random_forest                                  \
    --training_file covertype-small.train.csv         \
    --labels_file covertype-small.train.labels.csv    \
    --num_trees 10                                    \
    --minimum_leaf_size 3                             \
    --print_training_accuracy                         \
    --output_model_file rf-model.bin                  \
    --verbose

# Now predict the labels of the test points and print the accuracy.
# Also, save the test set predictions to the file 'predictions.csv'.
mlpack_random_forest                                    \
    --input_model_file rf-model.bin                     \
    --test_file covertype-small.test.csv                \
    --test_labels_file covertype-small.test.labels.csv  \
    --predictions_file predictions.csv                  \
    --verbose
@endcode

We can see by looking at the output that we achieve reasonably good accuracy on
the test dataset (80%+).  The file @c predictions.csv could also be used by
other tools; for instance, we can easily calculate the number of points that
were predicted incorrectly:

@code{.sh}
$ diff -U 0 predictions.csv covertype-test.labels.csv | grep '^@@' | wc -l
@endcode

It's easy to modify the code above to do more complex things, or to use
different mlpack learners, or to interface with other machine learning toolkits.

@section cli_quickstart_whatelse What else does mlpack implement?

The example above has only shown a little bit of the functionality of mlpack.
Lots of other commands are available with different functionality.  Below is a
list of all the mlpack functionality offered through the command-line, split
into some categories.

 - Classification techniques: <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_adaboost.html">mlpack_adaboost</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_decision_stump.html">mlpack_decision_stump</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_decision_tree.html">mlpack_decision_tree</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_hmm_train.html">mlpack_hmm_train</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_hmm_generate.html">mlpack_hmm_generate</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_hmm_loglik.html">mlpack_hmm_loglik</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_hmm_viterbi.html">mlpack_hmm_viterbi</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_hoeffding_tree.html">mlpack_hoeffding_tree</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_logistic_regression.html">mlpack_logistic_regression</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_nbc.html">mlpack_nbc</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_perceptron.html">mlpack_perceptron</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_random_forest.html">mlpack_random_forest</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_softmax_regression.html">mlpack_softmax_regression</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_cf.html">mlpack_cf</a></tt>

 - Distance-based problems: <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_approx_kfn.html">mlpack_approx_kfn</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_emst.html">mlpack_emst</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_fastmks.html">mlpack_fastmks</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_kfn.html">mlpack_kfn</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_knn.html">mlpack_knn</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_krann.html">mlpack_krann</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_lsh.html">mlpack_lsh</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_det.html">mlpack_det</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_range_search.html">mlpack_range_search</a></tt>

 - Clustering: <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_kmeans.html">mlpack_kmeans</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_mean_shift.html">mlpack_mean_shift</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_gmm_train.html">mlpack_gmm_train</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_gmm_generate.html">mlpack_gmm_generate</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_gmm_probability.html">mlpack_gmm_probability</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_dbscan.html">mlpack_dbscan</a></tt>

 - Transformations: <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_pca.html">mlpack_pca</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_radical.html">mlpack_radical</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_local_coordinate_coding.html">mlpack_local_coordinate_coding</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_sparse_coding.html">mlpack_sparse_coding</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_nca.html">mlpack_nca</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_kernel_pca.html">mlpack_kernel_pca</a></tt>

 - Regression: <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_linear_regression.html">mlpack_linear_regression</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_lars.html">mlpack_lars</a></tt>

 - Preprocessing/other: <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_preprocess_binarize.html">mlpack_preprocess_binarize</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_preprocess_split.html">mlpack_preprocess_split</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_preprocess_describe.html">mlpack_preprocess_describe</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_preprocess_imputer.html">mlpack_preprocess_imputer</a></tt>, <tt><a href="http://www.mlpack.org/docs/mlpack-git/man/mlpack_nmf.html">mlpack_nmf</a></tt>

For more information on what mlpack does, see http://www.mlpack.org/about.html.
Next, let's go through another example for providing movie recommendations with
mlpack.

@section cli_quickstart_movierecs Using mlpack for movie recommendations

In this example, we'll train a collaborative filtering model using mlpack's
@c mlpack_cf program.  We'll train this on the MovieLens dataset from
https://grouplens.org/datasets/movielens/, and then we'll use the model that we
train to give recommendations.

You can copy-paste this code directly into the command line to run it.

@code{.sh}
wget http://www.mlpack.org/datasets/ml-20m/ratings-only.csv.gz
wget http://www.mlpack.org/datasets/ml-20m/movies.csv.gz
gunzip ratings-only.csv.gz
gunzip movies.csv.gz

# Hold out 10% of the dataset into a test set so we can evaluate performance.
mlpack_preprocess_split                 \
    --input_file ratings-only.csv       \
    --training_file ratings-train.csv   \
    --test_file ratings-test.csv        \
    --test_ratio 0.1                    \
    --verbose

# Train the model.  Change the rank to increase/decrease the complexity of the
# model.
mlpack_cf                             \
    --training_file ratings-train.csv \
    --test_file ratings-test.csv      \
    --rank 10                         \
    --algorithm RegSVD                \
    --output_model_file cf-model.bin  \
    --verbose

# Now query the 5 top movies for user 1.
echo "1" > query.csv;
mlpack_cf                             \
    --input_model_file cf-model.bin   \
    --query_file query.csv            \
    --recommendations 10              \
    --output_file recommendations.csv \
    --verbose

# Get the names of the movies for user 1.
echo "Recommendations for user 1:"
for i in `seq 1 10`; do
    item=`cat recommendations.csv | awk -F',' '{ print $'$i' }'`;
    head -n $(($item + 2)) movies.csv | tail -1 | \
        sed 's/^[^,]*,[^,]*,//' | \
        sed 's/\(.*\),.*$/\1/' | sed 's/"//g';
done
@endcode

Here is some example output, showing that user 1 seems to have good taste in
movies:

@code{.unparsed}
Recommendations for user 1:
Casablanca (1942)
Pan's Labyrinth (Laberinto del fauno, El) (2006)
Godfather, The (1972)
Answer This! (2010)
Life Is Beautiful (La Vita è bella) (1997)
Adventures of Tintin, The (2011)
Dark Knight, The (2008)
Out for Justice (1991)
Dr. Strangelove or: How I Learned to Stop Worrying and Love the Bomb (1964)
Schindler's List (1993)
@endcode

@section cli_quickstart_nextsteps Next steps with mlpack

Now that you have done some simple work with mlpack, you have seen how it can
easily plug into a data science production workflow for the command line.  A
great thing to do next would be to look at more documentation for the mlpack
command-line programs:

 - <a href="http://www.mlpack.org/docs/mlpack-git/man.html">mlpack
   command-line program documentation</a>

Also, mlpack is much more flexible from C++ and allows much greater
functionality.  So, more complicated tasks are possible if you are willing to
write C++.  To get started learning about mlpack in C++, the following resources
might be helpful:

 - <a href="http://www.mlpack.org/docs/mlpack-git/doxygen/tutorials.html">mlpack
   C++ tutorials</a>
 - <a href="http://www.mlpack.org/docs/mlpack-git/doxygen/build.html">mlpack
   build and installation guide</a>
 - <a href="http://www.mlpack.org/docs/mlpack-git/doxygen/sample.html">Simple
   sample C++ mlpack programs</a>
 - <a href="http://www.mlpack.org/docs/mlpack-git/doxygen/index.html">mlpack
   Doxygen documentation homepage</a>

 */
