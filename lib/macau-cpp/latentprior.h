#ifndef LATENTPRIOR_H
#define LATENTPRIOR_H

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <memory>

#include "mvnormal.h"
#include "linop.h"
#include "model.h"

class INoiseModel;

 // forward declarationsc
typedef Eigen::SparseMatrix<double> SparseMatrixD;

/** interface */
template<typename FactorType>
class ILatentPrior {
  public:
      // c-tor
      ILatentPrior(FactorType &, INoiseModel &);

      // utility
      int num_latent() const { return fac.num_latent; }
      Eigen::MatrixXd::ConstColXpr col(int i) const { return fac.col(i); }
      virtual void savePriorInfo(std::string prefix) = 0;

      // work
      virtual void update_prior() = 0;
      virtual double getLinkNorm() { return NAN; };
      virtual double getLinkLambda() { return NAN; };
      virtual bool run_slave() { return false; } // returns true if some work happened...

      void sample_latents(const FactorType &other);
      virtual void sample_latent(int n, const FactorType &other) = 0;

  protected:
      FactorType &fac;
      INoiseModel &noise;
};

typedef ILatentPrior<Factor> SparseLatentPrior;
typedef ILatentPrior<DenseFactor> DenseLatentPrior;

//class DenseLatentPrior : public ILatentPrior<DenseFactor>
//{
//     // c-tor
//     DenseLatentPrior(DenseFactor &f, INoiseModel &n)
//         : ILatentPrior<DenseFactor>(f,n) {}
//
//     void update_precision() {
//         UU = fac.U * fac.U.transpose();
//     }
//
//     Eigen::MatrixXd UU;
//};


/** Prior without side information (pure BPMF) */

template<typename FactorType>
class NormalPrior : public ILatentPrior<FactorType> {
  public:
    Eigen::VectorXd mu; 
    Eigen::MatrixXd Lambda;
    Eigen::MatrixXd WI;
    Eigen::VectorXd mu0;

    int b0;
    int df;

  public:
    NormalPrior(FactorType &d, INoiseModel &noise);

    void update_prior() override;
    void savePriorInfo(std::string prefix) override;

    virtual const Eigen::VectorXd getMu(int) const { return mu; }
    virtual const Eigen::MatrixXd getLambda(int) const { return Lambda; }
};

class SparseNormalPrior : public NormalPrior<Factor> {
  public:
    SparseNormalPrior(Factor &d, INoiseModel &noise);
    void sample_latent(int n, const Factor &other) override;
};

class DenseNormalPrior : public NormalPrior<DenseFactor> {
  public:
    DenseNormalPrior(DenseFactor &d, INoiseModel &noise);
    void sample_latent(int n, const DenseFactor &other) override;
};

/** Prior with side information */
template<class FType>
class MacauPrior : public SparseNormalPrior {
  public:
    Eigen::MatrixXd Uhat;
    std::unique_ptr<FType> F;  /* side information */
    Eigen::MatrixXd FtF;       /* F'F */
    Eigen::MatrixXd beta;      /* link matrix */
    bool use_FtF;
    double lambda_beta;
    double lambda_beta_mu0; /* Hyper-prior for lambda_beta */
    double lambda_beta_nu0; /* Hyper-prior for lambda_beta */

    double tol = 1e-6;

  public:
    MacauPrior(Factor &d, INoiseModel &noise);
            
    void addSideInfo(std::unique_ptr<FType> &Fmat, bool comp_FtF = false);

    void update_prior() override;
    double getLinkNorm() override;
    double getLinkLambda() override { return lambda_beta; };
    const Eigen::VectorXd getMu(int n) const override { return this->mu + Uhat.col(n); }
    const Eigen::MatrixXd getLambda(int) const override { return this->Lambda; }

    void compute_Ft_y_omp(Eigen::MatrixXd &);
    virtual void sample_beta();
    void setLambdaBeta(double lb) { lambda_beta = lb; };
    void setTol(double t) { tol = t; };
    void savePriorInfo(std::string prefix) override;
};


/** Prior with side information */
template<class FType>
class MacauMPIPrior : public MacauPrior<FType> {
  public:
    MacauMPIPrior(Factor &d, INoiseModel &noise);

    void addSideInfo(std::unique_ptr<FType> &Fmat, bool comp_FtF = false);

    virtual void sample_beta();
    virtual bool run_slave() { sample_beta(); return true; }

    int world_rank;
    int world_size;

    int rhs() const { return rhs_for_rank[world_rank]; }

  private:
    int* rhs_for_rank = NULL;
    double* rec     = NULL;
    int* sendcounts = NULL;
    int* displs     = NULL;
};


typedef Eigen::VectorXd VectorNd;
typedef Eigen::MatrixXd MatrixNNd;
typedef Eigen::ArrayXd ArrayNd;

/** Spike and slab prior */
class SpikeAndSlabPrior : public SparseLatentPrior {
  public:
    VectorNd Zcol, W2col, Zkeep;
    ArrayNd alpha;
    VectorNd r;

    //-- hyper params
    const double prior_beta = 1; //for r
    const double prior_alpha_0 = 1.; //for alpha
    const double prior_beta_0 = 1.; //for alpha

    
 
  public:
    SpikeAndSlabPrior(Factor &d, INoiseModel &noise);
    void update_prior() override;
    void savePriorInfo(std::string prefix) override;
    void sample_latent(int n, const Factor &other) override;
};

std::pair<double,double> posterior_lambda_beta(Eigen::MatrixXd & beta, Eigen::MatrixXd & Lambda_u, double nu, double mu);
double sample_lambda_beta(Eigen::MatrixXd & beta, Eigen::MatrixXd & Lambda_u, double nu, double mu);

Eigen::MatrixXd A_mul_B(Eigen::MatrixXd & A, Eigen::MatrixXd & B);
Eigen::MatrixXd A_mul_B(Eigen::MatrixXd & A, SparseFeat & B);
Eigen::MatrixXd A_mul_B(Eigen::MatrixXd & A, SparseDoubleFeat & B);

#endif /* LATENTPRIOR_H */
