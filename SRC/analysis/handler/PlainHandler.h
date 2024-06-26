/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
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
// Description: This file contains the class definition for PlainHandler.
// PlainHandler is a simple ConstraintHandler; simple in that it can only
// handle 0 boundary constraints, i.e. no non-zero SP_Constraints and no 
// MP_Constraints can be handled by this type of ConstraintHandler.
//
// Written: fmk 
// Created: 11/96
// Revision: A
//
#ifndef PlainHandler_h
#define PlainHandler_h

#include <ConstraintHandler.h>

class FE_Element;
class DOF_Group;

class PlainHandler : public ConstraintHandler
{
  public:
    PlainHandler();
    ~PlainHandler();

    int handle(const ID *nodesNumberedLast =nullptr);
    void clearAll(void);    

    int sendSelf(int commitTag, Channel &theChannel);
    int recvSelf(int commitTag, Channel &theChannel, 
		 FEM_ObjectBroker &theBroker);
    
  protected:
    
  private:

};

#endif

