/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 2001, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */
//
// Written: MHS
// Created: April 2002

// Description: This file contains the class implementation for 
// SecantAccelerator3. 

#include <SecantAccelerator3.h>

#include <Vector.h>
#include <LinearSOE.h>
#include <IncrementalIntegrator.h>

SecantAccelerator3::SecantAccelerator3(int maxIter, int tangent)
  :Accelerator(ACCELERATOR_TAGS_Secant),
   iteration(0), numEqns(0), R1(0.0), R2(0.0),
   vOld(0), rOld(0), r_1(0), maxIterations(maxIter), theTangent(tangent),
   cutOut(false)
{

}

SecantAccelerator3::SecantAccelerator3(int maxIter, int tangent,
				       double r1, double r2)
  :Accelerator(ACCELERATOR_TAGS_Secant),
   iteration(0), numEqns(0), R1(r1), R2(r2),
   vOld(0), rOld(0), r_1(0), maxIterations(maxIter), theTangent(tangent),
   cutOut(true)
{

}

SecantAccelerator3::~SecantAccelerator3()
{
  if (vOld != 0)
    delete vOld;

  if (rOld != 0)
    delete rOld;

  if (r_1 != 0)
    delete r_1;
}

int 
SecantAccelerator3::newStep(LinearSOE &theSOE)
{
  int newNumEqns = theSOE.getNumEqn();

  if (vOld != 0 && vOld->Size() != newNumEqns) {
    delete vOld;
    vOld = 0;
  }

  if (rOld != 0 && rOld->Size() != newNumEqns) {
    delete rOld;
    rOld = 0;
  }

  if (r_1 != 0 && r_1->Size() != newNumEqns) {
    delete r_1;
    r_1 = 0;
  }

  numEqns = newNumEqns;

  if (vOld == 0)
    vOld = new Vector(numEqns);

  if (rOld == 0)
    rOld = new Vector(numEqns);

  if (r_1 == 0)
    r_1 = new Vector(numEqns);

  // Reset iteration counter
  iteration = 0;

  return 0;
}

int
SecantAccelerator3::accelerate(Vector &vStar, LinearSOE &theSOE, 
			       IncrementalIntegrator &theIntegrator)
{
  // Current right hand side
  const Vector &rNew  = theSOE.getB();

  // Store for next iteration
  *r_1 = vStar;

  // No acceleration on first iteration
  if (iteration == 0) {
    // do nothing
  }
  else {
    // Store gamma in rOld ... \gamma = R_i - R_{i-1}
    rOld->addVector(-1.0, rNew, 1.0);

    // Store alpha in r_1 ... \alpha = r_i - r_{i-1}
    r_1->addVector(-1.0, vStar, 1.0);
    
    double den = 1.0 / ((*vOld)^(*rOld));
    double C   = ((*vOld)^rNew) * den;
    double A   = 1.0-C;
    double B   = -C - ((*r_1)^rNew) * den + C*((*r_1)^(*rOld)) * den;
    double D   = -C - A*(vStar^(*rOld))*den;
    double DA  = D/A;

    // Check "cut-out" criteria
    if (cutOut && (A > R1 || A < 1.0/R1 || DA > R2 || DA < -0.5*R2)) {
      // do nothing
      //opserr << "SecantAccelerator3::accelerate() -- cut out, A = " << A
      //	   << ", D/A = " << DA << "\n";
    }
    else {
      vStar.addVector(A, *vOld, B);
      vStar.addVector(1.0, *rOld, C);
    }
  }

  // Store old values for next iteration
  *rOld = rNew;
  *vOld = vStar;

  iteration++;

  return 0; 
}

int
SecantAccelerator3::updateTangent(IncrementalIntegrator &theIntegrator)
{
  if (iteration < maxIterations)
    return 0;

  switch (theTangent) {
  case CURRENT_TANGENT:
    iteration = 0;
    theIntegrator.formTangent(CURRENT_TANGENT);
    return 1;
    break;
  case INITIAL_TANGENT:
    iteration = 0;
    theIntegrator.formTangent(INITIAL_TANGENT);
    return 0;
    break;
  case NO_TANGENT:
    //iteration = 0;
    return 0;
    break;
  default:
    return 0;
  }

  /*
  if (iteration > maxIterations) {
    //opserr << "SecantAccelerator3::updateTangent() tangent formed" << "\n";
    iteration = 0;
    if (theTangent != NO_TANGENT) {
      theIntegrator.formTangent(theTangent);
      return 1;
    }
    else
      return 0;
  }
  else
    return 0;
  */
}

void
SecantAccelerator3::Print(OPS_Stream &s, int flag)
{
  s << "SecantAccelerator3" << "\n";
  s << "\tIterations till reform tangent: " << maxIterations << "\n";
  if (cutOut)
    s << "\tCut-out factors -- R1: " << R1 << ", R2: " << R2 << "\n";
  else
    s << "\tNo cut-outs" << "\n";
}

int
SecantAccelerator3::sendSelf(int commitTag, Channel &theChannel)
{
  return -1;
}

int
SecantAccelerator3::recvSelf(int commitTag, Channel &theChannel, 
			     FEM_ObjectBroker &theBroker)
{
  return -1;
}
