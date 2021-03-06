#pragma once

#include <memory>

#include <Eigen/Sparse>
#include <Eigen/Core>

#include <SmurffCpp/Utils/PVec.hpp>
#include <SmurffCpp/Sessions/ISession.h>

namespace smurff {

class RootFile;
class Result;
struct ResultItem;

class PredictSession : public ISession
{
private:
    std::shared_ptr<RootFile> m_model_rootfile;
    std::shared_ptr<RootFile> m_pred_rootfile;
    Config m_config;
    bool m_has_config;

    std::shared_ptr<Result> m_result;
    std::vector<std::shared_ptr<StepFile>>::reverse_iterator m_pos;

    double m_secs_per_iter;
    double m_secs_total;

    std::vector<std::shared_ptr<StepFile>> m_stepfiles;

    int m_num_latent;
    PVec<> m_dims;
    bool m_is_init;

    std::shared_ptr<Model> restoreModel(const std::shared_ptr<StepFile> &);

public:
    int getNumSteps() const
    {
        return m_stepfiles.size();
    }

    PVec<> getModelDims() const
    {
       return m_dims;
    }

    int getNumLatent() const
    {
        return m_num_latent;
    }

    void run() override;
    bool step() override;
    void init() override;

    void save();

    std::shared_ptr<StatusItem> getStatus() const override;

    std::shared_ptr<Result> getResult() const override;

    std::shared_ptr<RootFile> getRootFile() const override {
        return m_pred_rootfile;
    }


    std::shared_ptr<RootFile> getModelRoot() const {
        return m_model_rootfile;
    }

  public:
    PredictSession(const Config &config);
    PredictSession(std::shared_ptr<RootFile> rf, const Config &config);
    PredictSession(std::shared_ptr<RootFile> rf);

    std::ostream& info(std::ostream &os, std::string indent) const override;

    // predict one element - based on position only
    ResultItem predict(PVec<> Ytest);
    
    ResultItem predict(PVec<> Ytest, const StepFile &sf);

    // predict one element - based on ResultItem
    void predict(ResultItem &);
    void predict(ResultItem &, const StepFile &sf);

    // predict all elements in Ytest
    std::shared_ptr<Result> predict(std::shared_ptr<TensorConfig> Y);
    void predict(Result &, const StepFile &);

    // predict element or elements based on sideinfo
    template<class Feat>
    std::shared_ptr<Result> predict(std::vector<std::shared_ptr<Feat>>);
    template<class Feat>
    std::shared_ptr<Result> predict(std::vector<std::shared_ptr<Feat>>, int sample);
    
};

}
