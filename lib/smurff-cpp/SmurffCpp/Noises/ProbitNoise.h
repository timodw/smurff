#pragma once

#include <string>
#include <iostream>

#include "INoiseModel.h"

namespace smurff {

   class NoiseFactory;

   // Gaussian noise that adapts to the data
   class ProbitNoise : public INoiseModel
   {
      friend class NoiseFactory;
      
   private:
      double threshold;
      
   protected:
      ProbitNoise(double threshold = 0.0);

   public:
      double sample(const SubModel& model, const PVec<> &pos, double val) override;

      std::ostream& info(std::ostream& os, std::string indent) override;
      std::string getStatus() override;
   };

}
