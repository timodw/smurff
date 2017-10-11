#pragma once

#include "Session.h"

namespace smurff {

class PythonSession : public Session 
{
private:
   static bool keepRunning;

public:
   PythonSession() 
   {
      name = "PythonSession";
      keepRunning = true;
   }

   void step() override;

private:
   static void intHandler(int);
};

}