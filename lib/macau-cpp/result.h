#ifndef RESULT_H
#define RESULT_H

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <memory>

#include "matrix_io.h"
#include "utils.h"

namespace Macau {

struct Model;

struct Result {
    //-- test set
    struct Item {
        int row, col;
        double val, pred, pred_avg, var, stds;
    };
    std::vector<Item> predictions;
    int nrows, ncols;
    void set(Eigen::SparseMatrix<double> Y);


    //-- prediction metrics
    void update(const Model &, bool burnin);
    double rmse_avg = NAN;
    double rmse = NAN;
    double auc = NAN; 
    int sample_iter = 0;
    int burnin_iter = 0;

    // general
    void save(std::string fname_prefix);
    void init();
    std::ostream &info(std::ostream &os, std::string indent);

    //-- for binary classification
    int total_pos;
    bool classify = false;
    double threshold;
    void update_auc();
    void setThreshold(double t) { threshold = t; classify = true; } 
};

}; // end namespace Macau

#endif
