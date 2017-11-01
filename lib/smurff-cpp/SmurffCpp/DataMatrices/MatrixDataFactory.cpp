#include "MatrixDataFactory.h"

#include <SmurffCpp/Configs/NoiseConfig.h>

#include <SmurffCpp/Utils/MatrixUtils.h>
#include <SmurffCpp/Utils/PVec.hpp>

//matrix classes
#include <SmurffCpp/DataMatrices/SparseMatrixData.h>
#include <SmurffCpp/DataMatrices/ScarceBinaryMatrixData.h>
#include <SmurffCpp/DataMatrices/DenseMatrixData.h>
#include <SmurffCpp/DataMatrices/MatricesData.h>

//noise classes
#include <SmurffCpp/Noises/AdaptiveGaussianNoise.h>
#include <SmurffCpp/Noises/FixedGaussianNoise.h>
#include <SmurffCpp/Noises/ProbitNoise.h>
#include <SmurffCpp/Noises/Noiseless.h>
#include <SmurffCpp/Noises/UnusedNoise.h>

using namespace smurff;

void setNoiseModel(const NoiseConfig &config, Data* data)
{
    if (config.name == "fixed")
    {
        data->setNoiseModel(new FixedGaussianNoise(config.precision));
    }
    else if (config.name == "adaptive")
    {
        data->setNoiseModel(new AdaptiveGaussianNoise(config.sn_init, config.sn_max));
    }
    else if (config.name == "probit")
    {
        data->setNoiseModel(new ProbitNoise());
    }
    else if (config.name == "noiseless")
    {
        data->setNoiseModel(new Noiseless());
    }
    else
    {
        die("Unknown noise model; " + config.name);
    }
}

std::unique_ptr<MatrixData> matrix_config_to_matrix(const MatrixConfig &config, bool scarce)
{
   std::unique_ptr<MatrixData> local_data_ptr;

   if (config.isDense())
   {
      Eigen::MatrixXd Ytrain = matrix_utils::dense_to_eigen(config);
      local_data_ptr = std::unique_ptr<MatrixData>(new DenseMatrixData(Ytrain));
   }
   else
   {
      Eigen::SparseMatrix<double> Ytrain = matrix_utils::sparse_to_eigen(config);
      if (!scarce)
      {
         local_data_ptr = std::unique_ptr<MatrixData>(new SparseMatrixData(Ytrain));
      }
      else if (matrix_utils::is_binary(Ytrain))
      {
         local_data_ptr = std::unique_ptr<MatrixData>(new ScarceBinaryMatrixData(Ytrain));
      }
      else
      {
         local_data_ptr = std::unique_ptr<MatrixData>(new ScarceMatrixData(Ytrain));
      }
   }

   setNoiseModel(config.getNoiseConfig(), local_data_ptr.get());

   return local_data_ptr;
}

std::unique_ptr<MatrixData> create_matrix(const MatrixConfig &train, const std::vector<MatrixConfig> &row_features, const std::vector<MatrixConfig> &col_features)
{
   if (row_features.empty() && col_features.empty())
      return ::matrix_config_to_matrix(train, true);

   // multiple matrices
   MatricesData* local_data_ptr = new MatricesData();

   local_data_ptr->setNoiseModel(new UnusedNoise());
   local_data_ptr->add(PVec<>({0,0}), ::matrix_config_to_matrix(train, true));

   for(size_t i = 0; i < row_features.size(); ++i)
   {
      local_data_ptr->add(PVec<>({0, static_cast<int>(i + 1)}), ::matrix_config_to_matrix(row_features[i], false));
   }

   for(size_t i = 0; i < col_features.size(); ++i)
   {
      local_data_ptr->add(PVec<>({static_cast<int>(i + 1), 0}), ::matrix_config_to_matrix(col_features[i], false));
   }

   return std::unique_ptr<MatrixData>(local_data_ptr);
}

std::unique_ptr<MatrixData> MatrixDataFactory::create_matrix(std::shared_ptr<Session> session)
{
   //row_matrices and col_matrices are selected if prior is not macau and not macauone
   std::vector<MatrixConfig> row_matrices;
   std::vector<MatrixConfig> col_matrices;

   if (session->config.row_prior_type != PriorTypes::macau && session->config.row_prior_type != PriorTypes::macauone)
      row_matrices = session->config.row_features;

   if (session->config.col_prior_type != PriorTypes::macau && session->config.col_prior_type != PriorTypes::macauone)
      col_matrices = session->config.col_features;

   return ::create_matrix(session->config.train, row_matrices, col_matrices);
}