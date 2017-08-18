#include "noisemodels.h"
#include "session.h"
#include "data.h"

using namespace Eigen;

namespace smurff {

SpikeAndSlabPrior::SpikeAndSlabPrior(BaseSession &m, int p)
    : ILatentPrior(m, p, "SpikeAndSlabPrior") {}


void SpikeAndSlabPrior::init() {
    const int K = num_latent();
    const int D = num_cols();
    const int V = data().nview(mode);
    assert(D > 0);

    Zcol.init(MatrixXd::Zero(K,V));
    W2col.init(MatrixXd::Zero(K,V));
    
    //-- prior params
    alpha = ArrayNNd::Ones(K,V);
    Zkeep = MatrixNNd::Constant(K, V, D);
    r = MatrixNNd::Constant(K,V,.5);
}

void SpikeAndSlabPrior::sample_latent(int d)
{
    const int K = num_latent();
    const int v = data().view(mode, d);

    auto &W = U(); // alias
    VectorNd Wcol = W.col(d); // local copy
    
    std::default_random_engine generator;
    std::uniform_real_distribution<double> udist(0,1);
    ArrayNd log_alpha = alpha.col(v).log();
    ArrayNd log_r = - r.col(v).array().log() + (VectorNd::Ones(K) - r.col(v)).array().log();

    MatrixNNd XX = MatrixNNd::Zero(K, K);
    VectorNd yX = VectorNd::Zero(K);
    data().get_pnm(model(),mode,d,yX,XX);
    double t = noise().getAlpha();

    for(int k=0;k<K;++k) {
        double lambda = t * XX(k,k) + alpha(k,v);
        double mu = t / lambda * (yX(k) - Wcol.transpose() * XX.col(k) + Wcol(k) * XX(k,k));
        double z1 = log_r(k) -  0.5 * (lambda * mu * mu - log(lambda) + log_alpha(k));
        double z = 1 / (1 + exp(z1));
        double p = udist(generator);
        if (Zkeep(k,v) > 0 && p < z) {
            Zcol.local()(k)++;
            double var = randn() / sqrt(lambda);
            Wcol(k) = mu + var;
        } else {
            Wcol(k) = .0;
        }
    }

    W.col(d) = Wcol;
    W2col.local() += Wcol.array().square().matrix();
}

void SpikeAndSlabPrior::sample_latents() {
    ILatentPrior::sample_latents();

    const int D = num_cols();
    auto Zc = Zcol.combine();
    auto W2c = W2col.combine();
 
    // update hyper params
    r = ( Zc.array() + prior_beta ) / ( D + prior_beta * D ) ;
    auto ww = W2c.array() / 2 + prior_beta_0;
    auto tmpz = Zc.array() / 2 + prior_alpha_0 ;
    alpha = tmpz.binaryExpr(ww, [](double a, double b)->double {
            std::default_random_engine generator;
            std::gamma_distribution<double> distribution(a, 1/b);
            return distribution(generator) + 1e-7;
    });


    Zkeep = Zc.array();
    Zcol.reset();
    W2col.reset();
}

std::ostream &SpikeAndSlabPrior::status(std::ostream &os, std::string indent) const {
    int Zcount = (Zkeep.array() > 0).count();
    os << indent << name << ": Z = " << Zcount << "/" << num_latent() << "\n";
    return os;
}

} // end namespace smurff