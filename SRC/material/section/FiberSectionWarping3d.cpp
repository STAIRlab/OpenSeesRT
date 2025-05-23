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
// Description: This file contains the class implementation of FiberSectionWarping3d.
// Modified by Xi Zhang from University of Sydney, Australia (include warping degrees of freedom). Refer to 
// Formulation and Implementation of Three-dimensional Doubly Symmetric Beam-Column Analyses with Warping Effects in OpenSees
// Research Report R917, School of Civil Engineering, University of Sydney.
//
// Written: fmk
// Created: 04/04
//
#include <stdlib.h>

#include <Channel.h>
#include <Vector.h>
#include <Matrix.h>
#include <classTags.h>
#include <FiberSectionWarping3d.h>
#include <ID.h>
#include <FEM_ObjectBroker.h>
#include <Information.h>
#include <UniaxialMaterial.h>
#include <ElasticMaterial.h>
#include <math.h>
#include <fstream>
#include <string.h>
#include <elementAPI.h>
#include <SensitiveResponse.h>
typedef SensitiveResponse<FrameSection> SectionResponse;


ID FiberSectionWarping3d::code(6);

void * OPS_ADD_RUNTIME_VPV(OPS_FiberSectionWarping3d)
{
    int numData = OPS_GetNumRemainingInputArgs();
    if(numData < 1) {
            opserr<<"insufficient arguments for FiberSectionWarping3d\n";
            return 0;
    }
    
    numData = 1;
    int tag;
    if (OPS_GetIntInput(&numData, &tag) < 0) return 0;

    if (OPS_GetNumRemainingInputArgs() < 2) {
      opserr << "WARNING torsion not specified for FiberSection\n";
      opserr << "Use either -GJ $GJ or -torsion $matTag\n";
      opserr << "\nFiberSection3d section: " << tag << endln;
      return 0;
    }
    
    UniaxialMaterial *torsion = 0;
    bool deleteTorsion = false;
    bool computeCentroid = true;
    while (OPS_GetNumRemainingInputArgs() > 0) {
      const char* opt = OPS_GetString();
      if (strcmp(opt,"-noCentroid") == 0) {
        computeCentroid = false;
      }
      if (strcmp(opt, "-GJ") == 0 && OPS_GetNumRemainingInputArgs() > 0) {
        numData = 1;
        double GJ;
        if (OPS_GetDoubleInput(&numData, &GJ) < 0) {
          opserr << "WARNING: failed to read GJ\n";
          return 0;
        }
        torsion = new ElasticMaterial(0,GJ);
        deleteTorsion = true;
      }
      if (strcmp(opt, "-torsion") == 0 && OPS_GetNumRemainingInputArgs() > 0) {
        numData = 1;
        int torsionTag;
        if (OPS_GetIntInput(&numData, &torsionTag) < 0) {
          opserr << "WARNING: failed to read torsion\n";
          return 0;
        }
        torsion = OPS_getUniaxialMaterial(torsionTag);
      }
    }

    if (torsion == 0) {
      opserr << "WARNING torsion not specified for FiberSection\n";
      opserr << "\nFiberSection3d section: " << tag << endln;
      return 0;
    }
    
    int num = 30;
    FrameSection *section = new FiberSectionWarping3d(tag, num, *torsion);
    if (deleteTorsion)
      delete torsion;
    return section;
}

#if 0
// constructors:
FiberSectionWarping3d::FiberSectionWarping3d(int tag, int num, Fiber **fibers,
                                             UniaxialMaterial &torsion): 
  FrameSection(tag, SEC_TAG_FiberSectionWarping3d),
  numFibers(num), sizeFibers(num),  theMaterials(0), matData(0),
  yBar(0.0), zBar(0.0), e(8), eCommit(8), s(0), ks(0), theTorsion(0)
{
  if (numFibers != 0) {
    theMaterials = new UniaxialMaterial *[numFibers];

    matData = new double [numFibers*4];

    double Qz = 0.0;
    double Qy = 0.0;
    double A  = 0.0;
    double Heightt;
    
    for (int i = 0; i < numFibers; i++) {
      UniaxialFiber3d *theFiber = (UniaxialFiber3d *)fibers[i];
      double yLoc, zLoc, Area;
      theFiber->getFiberLocation(yLoc, zLoc);
      Area = theFiber->getArea();
          Heightt = theFiber->getd();

      Qz += yLoc*Area;
      Qy += zLoc*Area;
      A  += Area;

      matData[i*4] = yLoc;
      matData[i*4+1] = zLoc;
      matData[i*4+2] = Area;
          matData[i*4+3] = Heightt;
      UniaxialMaterial *theMat = theFiber->getMaterial();
      theMaterials[i] = theMat->getCopy();

      if (theMaterials[i] == 0) {
        opserr << "FiberSectionWarping3d::FiberSectionWarping3d -- failed to get copy of a Material\n";
        exit(-1);
      }
    }

    yBar = -Qz/A;
    zBar = Qy/A;
  }

  theTorsion = torsion.getCopy();
  if (theTorsion == 0)
    opserr << "FiberSectionWarping3d::FiberSectionWarping3d -- failed to get copy of torsion material\n";
  
  s = new Vector(sData, 6);
  ks = new Matrix(kData, 6, 6);

  sData[0] = 0.0;
  sData[1] = 0.0;
  sData[2] = 0.0;
  sData[3] = 0.0;
  sData[4] = 0.0;
  sData[5] = 0.0;  

  for (int i=0; i<36; i++)
    kData[i] = 0.0;

  code(0) = SECTION_RESPONSE_P;
  code(1) = SECTION_RESPONSE_MZ;
  code(2) = SECTION_RESPONSE_MY;
  code(3) = SECTION_RESPONSE_W;
  code(4) = SECTION_RESPONSE_B;  
  code(5) = SECTION_RESPONSE_T;  

 // AddingSensitivity:BEGIN ////////////////////////////////////
  parameterID = 0;
  SHVs=0;
  // AddingSensitivity:END //////////////////////////////////////
}
#endif

FiberSectionWarping3d::FiberSectionWarping3d(int tag, int num, UniaxialMaterial &torsion): 
    FrameSection(tag, SEC_TAG_FiberSectionWarping3d),
    numFibers(0), sizeFibers(num), theMaterials(0), matData(0),
    yBar(0.0), zBar(0.0),
    e(8), eCommit(8), s(0), ks(0), theTorsion(0)
{
    if (sizeFibers != 0) {
        theMaterials = new UniaxialMaterial *[sizeFibers]{};

        matData = new double [sizeFibers*4]{};
    }

    theTorsion = torsion.getCopy();
    if (theTorsion == 0) 
      opserr << "FiberSectionWarping3d::FiberSectionWarping3d -- failed to get copy of torsion material\n";

    s = new Vector(sData, 6);
    ks = new Matrix(kData, 6, 6);

    sData[0] = 0.0;
    sData[1] = 0.0;
    sData[2] = 0.0;
    sData[3] = 0.0;
    sData[4] = 0.0;
    sData[5] = 0.0;        

    for (int i=0; i<36; i++)
        kData[i] = 0.0;

    code(0) = SECTION_RESPONSE_P;
    code(1) = SECTION_RESPONSE_MZ;
    code(2) = SECTION_RESPONSE_MY;
    code(3) = SECTION_RESPONSE_W;
    code(4) = SECTION_RESPONSE_B;  
    code(5) = SECTION_RESPONSE_T;    
}

// constructor for blank object that recvSelf needs to be invoked upon
FiberSectionWarping3d::FiberSectionWarping3d():
  FrameSection(0, SEC_TAG_FiberSectionWarping3d),
  numFibers(0), theMaterials(nullptr), matData(nullptr),
  yBar(0.0), zBar(0.0), e(8), eCommit(8), s(0), ks(0), theTorsion(0)
{
  s = new Vector(sData, 6);
  ks = new Matrix(kData, 6, 6);

  sData[0] = 0.0;
  sData[1] = 0.0;
  sData[2] = 0.0;
  sData[3] = 0.0;
  sData[4] = 0.0;
  sData[5] = 0.0;  

  for (int i=0; i<36; i++)
    kData[i] = 0.0;

  code(0) = SECTION_RESPONSE_P;
  code(1) = SECTION_RESPONSE_MZ;
  code(2) = SECTION_RESPONSE_MY;
  code(3) = SECTION_RESPONSE_W;
  code(4) = SECTION_RESPONSE_B;  
  code(5) = SECTION_RESPONSE_T;  

 // AddingSensitivity:BEGIN ////////////////////////////////////
  parameterID = 0;
  SHVs=0;
  // AddingSensitivity:END //////////////////////////////////////
}

int
FiberSectionWarping3d::addFiber(UniaxialMaterial &theMat, const double Area, const double yLoc, const double zLoc, const double omega)
{
  // need to create a larger array
  int newSize = numFibers+1;

  UniaxialMaterial **newArray = new UniaxialMaterial *[newSize]; 
  double *newMatData = new double [4 * newSize];

  // copy the old pointers
  int i;
  for (i = 0; i < numFibers; i++) {
    newArray[i]       = theMaterials[i];
    newMatData[4*i]   = matData[4*i];
    newMatData[4*i+1] = matData[4*i+1];
    newMatData[4*i+2] = matData[4*i+2];
    newMatData[4*i+3] = matData[4*i+3];
  }
  // set the new pointers
//double yLoc, zLoc, Area, Height;
//newFiber.getFiberLocation(yLoc, zLoc);
//Area = newFiber.getArea();
//Height = newFiber.getd();

//// calculate sectorial area
//double omig=0.0;
//if (y>0.0)
//  omig = -zLoc*(yLoc-Height);
//else
//  omig = -zLoc*(yLoc+Height);

  newMatData[numFibers*4]   = -yLoc;
  newMatData[numFibers*4+1] =  zLoc;
  newMatData[numFibers*4+2] =  Area;
  newMatData[numFibers*4+3] =  omega;
//UniaxialMaterial *theMat = newFiber.getMaterial();
  newArray[numFibers] = theMat.getCopy();

  if (newArray[numFibers] == nullptr) {
    opserr << "FiberSectionWarping3d::addFiber -- failed to get copy of a Material\n";
    delete [] newArray;
    delete [] newMatData;
    return -1;
  }

  numFibers++;
  
  if (theMaterials != nullptr) {
    delete [] theMaterials;
    delete [] matData;
  }

  theMaterials = newArray;
  matData = newMatData;

  double Qz = 0.0;
  double Qy = 0.0;
  double A  = 0.0;

  // Recompute centroid
  for (i = 0; i < numFibers; i++) {
    double yLoc  = -matData[4*i];
    double zLoc  =  matData[4*i+1];
    double Area  =  matData[4*i+2];

    A  += Area;
    Qz += yLoc*Area;
    Qy += zLoc*Area;
  }

  yBar = -Qz/A;
  zBar = Qy/A;

  return 0;
}



// destructor:
FiberSectionWarping3d::~FiberSectionWarping3d()
{
  if (theMaterials != 0) {
    for (int i = 0; i < numFibers; i++)
      if (theMaterials[i] != 0)
        delete theMaterials[i];
      
    delete [] theMaterials;
  }

  if (matData != nullptr)
    delete [] matData;

  if (s != 0)
    delete s;

  if (ks != 0)
    delete ks;

  if (theTorsion != 0)
    delete theTorsion;  
}

int
FiberSectionWarping3d::setTrialSectionDeformation(const Vector &deforms)
{
  int res = 0;
  e = deforms;

  for (int i = 0; i < 36; i++)
    kData[i] = 0.0;

  sData[0] = 0.0;
  sData[1] = 0.0;
  sData[2] = 0.0;
  sData[3] = 0.0;
  sData[4] = 0.0;
  sData[5] = 0.0;  

  int loc = 0;

  double d0 = deforms(0);
  double d1 = deforms(1);
  double d2 = deforms(2);
  double d3 = deforms(3);
  double d4 = deforms(4);
  double d5 = deforms(5);
  double d6 = deforms(6);
  double d7 = deforms(7);

  for (int i = 0; i < numFibers; i++) {
    UniaxialMaterial *theMat = theMaterials[i];
    double y    = matData[loc++] - yBar;
    double z    = matData[loc++] - zBar;
    double A    = matData[loc++];
    double omig = matData[loc++];         // SV warping function

    // determine material strain and set it, include second order terms
    double strain = d0 - y*d1 - z*d2 - omig*d3 
                  + 0.5*d5*d5 + 0.5*d6*d6 + 0.5*(y*y + z*z)*d4*d4 
                  - y*d7*d2 + z*d7*d1;
    double tangent, stress;
    res += theMat->setTrial(strain, stress, tangent);

    //
    // section force vector D, refer to Alemdar
    //
    double fs0 = stress * A;
    sData[0] += fs0;                // N
    sData[1] += -y*fs0;             // Mz
    sData[2] += -z*fs0;             // My
    sData[3] +=  fs0 * (y*y + z*z); // W
    sData[4] += -fs0 * omig;        // B

    // 
    // Section stiffness matrix k, refer to Alemdar
    // 
    double EA = tangent * A;
    double vas1 = y*EA;
    double vas2 = z*EA;
    double vas1as2 = vas1*z;

    kData[ 0] += EA;
    kData[ 1] += -vas1;
    kData[ 2] += -vas2;
    kData[ 3] += (y*y + z*z)*EA;
    kData[ 4] += omig*EA;
    
    kData[ 7] += vas1 * y;
    kData[ 8] += y*z*EA;
    kData[ 9] += -y*(y*y + z*z)*EA;
    kData[10] += -y*omig*EA;
      
    kData[14] += vas2 * z;
    kData[15] += -z*(y*y+z*z)*EA;
    kData[16] += -z*omig*EA;
    
    kData[21] += (y*y+z*z)*(y*y+z*z)*EA;
    kData[22] += omig*(y*y+z*z)*EA;
      
    kData[28] += omig*omig*EA;

  }
  
  kData[ 6] = kData[1];
  kData[12] = kData[2];
  kData[18] = kData[3];
  kData[24] = kData[4];
  
  kData[13] = kData[8];
  kData[19] = kData[9];
  kData[25] = kData[10];

  kData[20] = kData[15];
  kData[26] = kData[16];  

  kData[27] = kData[22];

  if (theTorsion != nullptr) {
    double stress, tangent;
    res += theTorsion->setTrial(d4, stress, tangent);
    sData[5] = stress;
    kData[35] = tangent;
  }
  
  return res;
}

const Matrix&
FiberSectionWarping3d::getInitialTangent()
{
  static double kInitialData[36];
  static Matrix kInitial(kInitialData, 6, 6);
  for (int i=0; i<36; i++)
    kInitialData[i]=0.0;

  int loc = 0;

  for (int i = 0; i < numFibers; i++) {
    UniaxialMaterial *theMat = theMaterials[i];
    double y    = matData[loc++] - yBar;
    double z    = matData[loc++] - zBar;
    double A    = matData[loc++];
    double omig = matData[loc++];

    
    double tangent = theMat->getInitialTangent();

    double value   = tangent * A;
    double vas1    = y*value;
    double vas2    = z*value;
    double vas1as2 = vas1*z;

    // section stiffness matrix k, refer to Alemdar 
    kInitialData[0] += value;
    kInitialData[3] += (y*y+z*z)*value;
    kInitialData[7] += vas1 * y;
    kInitialData[14] += vas2 * z; 
    //kInitialData[18] += (y*y+z*z)*value;
    kInitialData[21] += (y*y+z*z)*(y*y+z*z)*value;
    kInitialData[28] += omig*omig*value;
  }
  kInitialData[18] = kInitialData[3];

  if (theTorsion != nullptr)
    kInitialData[35] = theTorsion->getInitialTangent();

  return kInitial;
}

const Vector&
FiberSectionWarping3d::getSectionDeformation()
{
  return e;
}

const Matrix&
FiberSectionWarping3d::getSectionTangent()
{
  return *ks;
}

const Vector&
FiberSectionWarping3d::getStressResultant()
{
  return *s;
}

FrameSection*
FiberSectionWarping3d::getFrameCopy()
{
  FiberSectionWarping3d *theCopy = new FiberSectionWarping3d();
  theCopy->setTag(this->getTag());

  theCopy->numFibers = numFibers;

  if (numFibers != 0) {
    theCopy->theMaterials = new UniaxialMaterial *[numFibers];

    theCopy->matData = new double [numFibers*4];

    for (int i = 0; i < numFibers; i++) {
      theCopy->matData[i*4]    = matData[i*4];
      theCopy->matData[i*4+1]  = matData[i*4+1];
      theCopy->matData[i*4+2]  = matData[i*4+2];
      theCopy->matData[i*4+3]  = matData[i*4+3];
      theCopy->theMaterials[i] = theMaterials[i]->getCopy();
    }
  }

  theCopy->eCommit = eCommit;
  theCopy->e = e;
  theCopy->yBar = yBar;
  theCopy->zBar = zBar;

  for (int i=0; i<36; i++)
    theCopy->kData[i] = kData[i];

  theCopy->sData[0] = sData[0];
  theCopy->sData[1] = sData[1];
  theCopy->sData[2] = sData[2];
  theCopy->sData[3] = sData[3];
  theCopy->sData[4] = sData[4];
  theCopy->sData[5] = sData[5];  

  if (theTorsion != nullptr)
    theCopy->theTorsion = theTorsion->getCopy();
  else
    theCopy->theTorsion = nullptr;
  
  return theCopy;
}

const ID&
FiberSectionWarping3d::getType()
{
  return code;
}

int
FiberSectionWarping3d::getOrder() const
{
  return 6;
}

int
FiberSectionWarping3d::commitState()
{
  int err = 0;

  for (int i = 0; i < numFibers; i++)
    err += theMaterials[i]->commitState();

  if (theTorsion != 0)
    err += theTorsion->commitState();
  
  eCommit = e;

  return err;
}

int
FiberSectionWarping3d::revertToLastCommit()
{
  int err = 0;

  // Last committed section deformations
  e = eCommit;

  for (int i = 0; i < 36; i++)
    kData[i] = 0.0;

  sData[0] = 0.0;
  sData[1] = 0.0;
  sData[2] = 0.0;
  sData[3] = 0.0;
  sData[4] = 0.0;
  sData[5] = 0.0;

  int loc = 0;
  for (int i = 0; i < numFibers; i++) {
    UniaxialMaterial *theMat = theMaterials[i];
    double y    = matData[loc++] - yBar;
    double z    = matData[loc++] - zBar;
    double A    = matData[loc++];
    double omig = matData[loc++];

    // invoke revertToLast on the material
    err += theMat->revertToLastCommit();

    double tangent = theMat->getTangent();
    double stress = theMat->getStress();

    double value = tangent * A;
    double vas1 = y*value;
    double vas2 = z*value;
    double vas1as2 = vas1*z;

    
    kData[0] += value;
    kData[3] += (y*y+z*z)*value;
    kData[7] += vas1 * y;
    kData[14] += vas2 * z; 
    //kData[18] += (y*y+z*z)*value;
    kData[21] += (y*y+z*z)*(y*y+z*z)*value;
    kData[28] += omig*omig*value;
    
    double fs0 = stress * A;
    
    sData[0] += fs0;
    sData[1] += -1.0 * fs0 * y;
    sData[2] += -1.0 * fs0 * z;
    sData[3] += fs0 * (y*y+z*z);
    sData[4] += -fs0 * omig;
  }
  kData[18] = kData[3];

  if (theTorsion != 0) {
    err += theTorsion->revertToLastCommit();
    sData[5] = theTorsion->getStress();
    kData[35] = theTorsion->getTangent();
  } else {
    sData[5] = 0.0;
    kData[35] = 0.0;
  }
  
  return err;
}

int
FiberSectionWarping3d::revertToStart()
{
  // revert the fibers to start    
  int err = 0;

  for (int i = 0; i < 36; i++)
    kData[i] = 0.0;

  sData[0] = 0.0;
  sData[1] = 0.0;
  sData[2] = 0.0;
  sData[3] = 0.0;
  sData[4] = 0.0;
  sData[5] = 0.0;  
  
  int loc = 0;

  for (int i = 0; i < numFibers; i++) {
    UniaxialMaterial *theMat = theMaterials[i];
    double y    = matData[loc++] - yBar;
    double z    = matData[loc++] - zBar;
    double A    = matData[loc++];
    double omig = matData[loc++];

    // invoke revertToStart on the material
    err += theMat->revertToStart();

    double tangent = theMat->getTangent();
    double stress = theMat->getStress();

    double value = tangent * A;
    double vas1 = y*value;
    double vas2 = z*value;
    double vas1as2 = vas1*z;
    
    kData[0] += value;
    kData[3] += (y*y+z*z)*value;
    kData[7] += vas1 * y;
    kData[14] += vas2 * z; 
    //kData[18] += (y*y+z*z)*value;
    kData[21] += (y*y+z*z)*(y*y+z*z)*value;
    kData[28] += omig*omig*value;

    
    double fs0 = stress * A;

    sData[0] += fs0;
    sData[1] += -1.0 * fs0 * y;
    sData[2] += -1.0 * fs0 * z;
    sData[3] += fs0 * (y*y+z*z);
    sData[4] += -fs0 * omig;
  }
  kData[18] = kData[3];

  if (theTorsion != 0) {
    err += theTorsion->revertToStart();
    kData[35] = theTorsion->getTangent();
    sData[5] = theTorsion->getStress();
  } else {
    kData[35] = 0.0;
    sData[5] = 0.0;
  }  

  return err;
}

int
FiberSectionWarping3d::sendSelf(int commitTag, Channel &theChannel)
{
  int res = 0;

  // create an id to send objects tag and numFibers, 
  //     size 5 so no conflict with matData below if just 1 fiber
  static ID data(5);
  data(0) = this->getTag();
  data(1) = numFibers;
  data(2) = (theTorsion != 0) ? 1 : 0;  
  int dbTag = this->getDbTag();
  if (theTorsion != 0) {
    theTorsion->setDbTag(dbTag);
    data(3) = theTorsion->getClassTag();
  }  
  res += theChannel.sendID(dbTag, commitTag, data);
  if (res < 0) {
    opserr << "FiberSectionWarping3d::sendSelf - failed to send ID data\n";
    return res;
  }    

  if (theTorsion != 0)
    theTorsion->sendSelf(commitTag, theChannel);    
  
  if (numFibers != 0) {
    
    // create an id containingg classTag and dbTag for each material & send it
    ID materialData(2*numFibers);
    for (int i=0; i<numFibers; i++) {
      UniaxialMaterial *theMat = theMaterials[i];
      materialData(2*i) = theMat->getClassTag();
      int matDbTag = theMat->getDbTag();
      if (matDbTag == 0) {
        matDbTag = theChannel.getDbTag();
        if (matDbTag != 0)
          theMat->setDbTag(matDbTag);
      }
      materialData(2*i+1) = matDbTag;
    }    
    
    res += theChannel.sendID(dbTag, commitTag, materialData);
    if (res < 0) {
     opserr << "FiberSectionWarping3d::sendSelf - failed to send material data\n";
     return res;
    }    

    // send the fiber data, i.e. area and loc
    Vector fiberData(matData, 4*numFibers);
    res += theChannel.sendVector(dbTag, commitTag, fiberData);
    if (res < 0) {
     opserr << "FiberSectionWarping3d::sendSelf - failed to send material data\n";
     return res;
    }    

    // now invoke send(0 on all the materials
    for (int j=0; j<numFibers; j++)
      theMaterials[j]->sendSelf(commitTag, theChannel);
  }

  return res;
}

int
FiberSectionWarping3d::recvSelf(int commitTag, Channel &theChannel,
                         FEM_ObjectBroker &theBroker)
{
  int res = 0;

  static ID data(5);
  
  int dbTag = this->getDbTag();
  res += theChannel.recvID(dbTag, commitTag, data);

  if (res < 0) {
   opserr << "FiberSectionWarping3d::sendSelf - failed to recv ID data\n";
   return res;
  } 
   
  this->setTag(data(0));

  if (data(2) == 1 && theTorsion == 0) {        
    int cTag = data(3);
    theTorsion = theBroker.getNewUniaxialMaterial(cTag);
    if (theTorsion == 0) {
      opserr << "FiberSectionWarping3d::recvSelf - failed to get torsion material \n";
      return -1;
    }
    theTorsion->setDbTag(dbTag);
  }

  if (theTorsion->recvSelf(commitTag, theChannel, theBroker) < 0) {
    opserr << "FiberSectionWarping3d::recvSelf - torsion failed to recvSelf \n";
    return -2;
  }
  
  // recv data about materials objects, classTag and dbTag
  if (data(1) != 0) {
    ID materialData(2*data(1));
    res += theChannel.recvID(dbTag, commitTag, materialData);
    if (res < 0) {
     opserr << "FiberSectionWarping3d::sendSelf - failed to send material data\n";
     return res;
    }    

    // if current arrays not of correct size, release old and resize
    if (theMaterials == 0 || numFibers != data(1)) {
      // delete old stuff if outa date
      if (theMaterials != 0) {
        for (int i=0; i<numFibers; i++)
          delete theMaterials[i];
        delete [] theMaterials;
        if (matData != 0)
          delete [] matData;
        matData = 0;
        theMaterials = 0;
      }

      // create memory to hold material pointers and fiber data
      numFibers = data(1);
      sizeFibers = data(1);
      if (numFibers != 0) {

        theMaterials = new UniaxialMaterial *[numFibers];
        
        if (theMaterials == 0) {
          opserr << "FiberSectionWarping3d::recvSelf -- failed to allocate Material pointers\n";
          exit(-1);
        }

        for (int j=0; j<numFibers; j++)
          theMaterials[j] = 0;
        
        matData = new double [numFibers*4];

        if (matData == 0) {
          opserr << "FiberSectionWarping3d::recvSelf  -- failed to allocate double array for material data\n";
          exit(-1);
        }
      }
    }

    Vector fiberData(matData, 4*numFibers);
    res += theChannel.recvVector(dbTag, commitTag, fiberData);
    if (res < 0) {
     opserr << "FiberSectionWarping3d::sendSelf - failed to send material data\n";
     return res;
    }    
    
    int i;
    for (i=0; i<numFibers; i++) {
      int classTag = materialData(2*i);
      int dbTag = materialData(2*i+1);

      // if material pointed to is blank or not of corrcet type, 
      // release old and create a new one
      if (theMaterials[i] == 0)
        theMaterials[i] = theBroker.getNewUniaxialMaterial(classTag);

      else if (theMaterials[i]->getClassTag() != classTag) {
        delete theMaterials[i];
        theMaterials[i] = theBroker.getNewUniaxialMaterial(classTag);      
      }

      if (theMaterials[i] == 0) {
        opserr << "FiberSectionWarping3d::recvSelf -- failed to allocate double array for material data\n";
        exit(-1);
      }

      theMaterials[i]->setDbTag(dbTag);
      res += theMaterials[i]->recvSelf(commitTag, theChannel, theBroker);
    }

    double Qz = 0.0;
    double Qy = 0.0;
    double A  = 0.0;

    // Recompute centroid
    for (i = 0; i < numFibers; i++) {
      double yLoc = -matData[4*i];
      double zLoc = matData[4*i+1];
      double Area = matData[4*i+2];
      A  += Area;
      Qz += yLoc*Area;
      Qy += zLoc*Area;
    }
    
    yBar = -Qz/A;
    zBar = Qy/A;
  }    

  return res;
}

void
FiberSectionWarping3d::Print(OPS_Stream &s, int flag)
{
  if (flag == 2) {
    for (int i = 0; i < numFibers; i++) {
      s << -matData[4*i] << " "  << matData[4*i+1] << " "  << matData[4*i+2] << " " ;
      s << theMaterials[i]->getStress() << " "  << theMaterials[i]->getStrain() << endln;
    } 
  } else {
    s << "\nFiberSectionWarping3d, tag: " << this->getTag() << endln;
    s << "\tSection code: " << code;
    s << "\tNumber of Fibers: " << numFibers << endln;
    s << "\tCentroid: (" << -yBar << ", " << zBar << ')' << endln;
    
    if (flag == 1) {
      for (int i = 0; i < numFibers; i++) {
        s << "\nLocation (y, z) = (" << -matData[4*i] << ", " << matData[4*i+1] << ")";
        s << "\nArea = " << matData[4*i+2] << endln;
      theMaterials[i]->Print(s, flag);
      }
    }
  }
}

Response*
FiberSectionWarping3d::setResponse(const char **argv, int argc, OPS_Stream &output)
{

  const ID &type = this->getType();
  int typeSize = this->getOrder();

  Response *theResponse =0;

  output.tag("SectionOutput");
  output.attr("secType", this->getClassType());
  output.attr("secTag", this->getTag());

  // deformations
  if (strcmp(argv[0],"deformations") == 0 || strcmp(argv[0],"deformation") == 0) {
    for (int i=0; i<typeSize; i++) {
      int code = type(i);
      switch (code){
      case SECTION_RESPONSE_MZ:
        output.tag("ResponseType","kappaZ");
        break;
      case SECTION_RESPONSE_P:
        output.tag("ResponseType","eps");
        break;
      case SECTION_RESPONSE_VY:
        output.tag("ResponseType","gammaY");
        break;
      case SECTION_RESPONSE_MY:
        output.tag("ResponseType","kappaY");
        break;
      case SECTION_RESPONSE_VZ:
        output.tag("ResponseType","gammaZ");
        break;
      case SECTION_RESPONSE_T:
        output.tag("ResponseType","theta");
        break;
      default:
        output.tag("ResponseType","Unknown");
      }
    }
    theResponse =  new SectionResponse(*this, 1, this->getSectionDeformation());
  
  // forces
  } else if (strcmp(argv[0],"forces") == 0 || strcmp(argv[0],"force") == 0) {
    for (int i=0; i<typeSize; i++) {
      int code = type(i);
      switch (code){
      case SECTION_RESPONSE_MZ:
        output.tag("ResponseType","Mz");
        break;
      case SECTION_RESPONSE_P:
        output.tag("ResponseType","P");
        break;
      case SECTION_RESPONSE_VY:
        output.tag("ResponseType","Vy");
        break;
      case SECTION_RESPONSE_MY:
        output.tag("ResponseType","My");
        break;
      case SECTION_RESPONSE_VZ:
        output.tag("ResponseType","Vz");
        break;
      case SECTION_RESPONSE_T:
        output.tag("ResponseType","T");
        break;
      default:
        output.tag("ResponseType","Unknown");
      }
    }
    theResponse =  new SectionResponse(*this, 2, this->getStressResultant());
  
  // force and deformation
  } else if (strcmp(argv[0],"forceAndDeformation") == 0) { 
    for (int i=0; i<typeSize; i++) {
      int code = type(i);
      switch (code){
      case SECTION_RESPONSE_MZ:
        output.tag("ResponseType","kappaZ");
        break;
      case SECTION_RESPONSE_P:
        output.tag("ResponseType","eps");
        break;
      case SECTION_RESPONSE_VY:
        output.tag("ResponseType","gammaY");
        break;
      case SECTION_RESPONSE_MY:
        output.tag("ResponseType","kappaY");
        break;
      case SECTION_RESPONSE_VZ:
        output.tag("ResponseType","gammaZ");
        break;
      case SECTION_RESPONSE_T:
        output.tag("ResponseType","theta");
        break;
      default:
        output.tag("ResponseType","Unknown");
      }
    }
    for (int j=0; j<typeSize; j++) {
      int code = type(j);
      switch (code){
      case SECTION_RESPONSE_MZ:
        output.tag("ResponseType","Mz");
        break;
      case SECTION_RESPONSE_P:
        output.tag("ResponseType","P");
        break;
      case SECTION_RESPONSE_VY:
        output.tag("ResponseType","Vy");
        break;
      case SECTION_RESPONSE_MY:
        output.tag("ResponseType","My");
        break;
      case SECTION_RESPONSE_VZ:
        output.tag("ResponseType","Vz");
        break;
      case SECTION_RESPONSE_T:
        output.tag("ResponseType","T");
        break;
      default:
        output.tag("ResponseType","Unknown");
      }
    }

    theResponse =  new SectionResponse(*this, 4, Vector(2*this->getOrder()));
  
  }  
  
  else {
    if (argc > 2 || strcmp(argv[0],"fiber") == 0) {

      int key = numFibers;
      int passarg = 2;
      
      
      if (argc <= 3)        {  // fiber number was input directly
        
        key = atoi(argv[1]);
        
      } else if (argc > 4) {         // find fiber closest to coord. with mat tag
        int matTag = atoi(argv[3]);
        double yCoord = atof(argv[1]);
        double zCoord = atof(argv[2]);
        double closestDist;
        double ySearch, zSearch, dy, dz;
        double distance;
        int j;
        
        // Find first fiber with specified material tag
        for (j = 0; j < numFibers; j++) {
          if (matTag == theMaterials[j]->getTag()) {
            ySearch = -matData[4*j];
            zSearch =  matData[4*j+1];
            dy = ySearch-yCoord;
            dz = zSearch-zCoord;
            closestDist = sqrt(dy*dy + dz*dz);
            key = j;
            break;
          }
        }
        
        // Search the remaining fibers
        for ( ; j < numFibers; j++) {
          if (matTag == theMaterials[j]->getTag()) {
            ySearch = -matData[4*j];
            zSearch =  matData[4*j+1];
            dy = ySearch-yCoord;
            dz = zSearch-zCoord;
            distance = sqrt(dy*dy + dz*dz);
            if (distance < closestDist) {
              closestDist = distance;
              key = j;
            }
          }
        }
        passarg = 4;
      }
      
      else {                  // fiber near-to coordinate specified
        double yCoord = atof(argv[1]);
        double zCoord = atof(argv[2]);
        double closestDist;
        double ySearch, zSearch, dy, dz;
        double distance;
        ySearch = -matData[0];
        zSearch =  matData[1];
        dy = ySearch-yCoord;
        dz = zSearch-zCoord;
        closestDist = sqrt(dy*dy + dz*dz);
        key = 0;
        for (int j = 1; j < numFibers; j++) {
          ySearch = -matData[4*j];
          zSearch =  matData[4*j+1];
          dy = ySearch-yCoord;
          dz = zSearch-zCoord;
          distance = sqrt(dy*dy + dz*dz);
          if (distance < closestDist) {
            closestDist = distance;
            key = j;
          }
        }
        passarg = 3;
      }
      
      if (key < numFibers && key >= 0) {
        output.tag("FiberOutput");
        output.attr("yLoc",-matData[4*key]);
        output.attr("zLoc",matData[4*key+1]);
        output.attr("area",matData[4*key+2]);
        
        theResponse =  theMaterials[key]->setResponse(&argv[passarg], argc-passarg, output);
        
        output.endTag();
      }
    }
  }

  output.endTag();
  return theResponse;
}


int 
FiberSectionWarping3d::getResponse(int responseID, Information &sectInfo)
{
  // Just call the base class method ... don't need to define
  // this function, but keeping it here just for clarity
  return FrameSection::getResponse(responseID, sectInfo);
}

int
FiberSectionWarping3d::setParameter(const char **argv, int argc, Parameter &param)
{
  if (argc < 3)
    return -1;


  int result = 0;

  // A material parameter
  if (strstr(argv[0],"material") != 0) {

    // Get the tag of the material
    int paramMatTag = atoi(argv[1]);

    // Loop over fibers to find the right material(s)
    int ok = 0;
    for (int i = 0; i < numFibers; i++)
      if (paramMatTag == theMaterials[i]->getTag()) {
        ok = theMaterials[i]->setParameter(&argv[2], argc-2, param);
        if (ok != -1)
          result = ok;
      }
    
    return result;
  }    

  int ok = 0;
  
  // loop over every material
  for (int i = 0; i < numFibers; i++) {
    ok = theMaterials[i]->setParameter(argv, argc, param);
    if (ok != -1)
      result = ok;
  }

  return result;
}

const Vector &
FiberSectionWarping3d::getSectionDeformationSensitivity(int gradIndex)
{
  static Vector dummy(3);
  dummy.Zero();
  if (SHVs !=0) {
    dummy(0) = (*SHVs)(0,gradIndex);
    dummy(1) = (*SHVs)(1,gradIndex);
    dummy(2) = (*SHVs)(2,gradIndex);
  }
  return dummy;
}

   
const Vector &
FiberSectionWarping3d::getStressResultantSensitivity(int gradIndex, bool conditional)
{
  
  static Vector ds(3);
  
  ds.Zero();
  
  double  stressGradient;
  int loc = 0;
  
  
  for (int i = 0; i < numFibers; i++) {
    UniaxialMaterial *theMat = theMaterials[i];
    double y = matData[loc++] - yBar;
    double z = matData[loc++] - zBar;
    double A = matData[loc++];
    stressGradient = theMaterials[i]->getStressSensitivity(gradIndex,conditional);
    stressGradient *=  A;
    ds(0) += stressGradient;
    ds(1) += stressGradient * y;
    ds(2) += stressGradient * z;
    
  } 
  
  return ds;
}

const Matrix &
FiberSectionWarping3d::getSectionTangentSensitivity(int gradIndex)
{
  static Matrix something(2,2);
  
  something.Zero();
  
  return something;
}

int
FiberSectionWarping3d::commitSensitivity(const Vector& defSens, int gradIndex, int numGrads)
{

  // here add SHVs to store the strain sensitivity.

  if (SHVs == 0) {
    SHVs = new Matrix(3,numGrads);
  }
  
  (*SHVs)(0,gradIndex) = defSens(0);
  (*SHVs)(1,gradIndex) = defSens(1);
  (*SHVs)(2,gradIndex) = defSens(2);

  int loc = 0;

  double d0 = defSens(0);
  double d1 = defSens(1);
  double d2 = defSens(2);

  for (int i = 0; i < numFibers; i++) {
    UniaxialMaterial *theMat = theMaterials[i];
    double y = matData[loc++] - yBar;
    double z = matData[loc++] - zBar;
    loc++;   // skip A data.
    
    double strainSens = d0 + y*d1 + z*d2;
    
    theMat->commitSensitivity(strainSens,gradIndex,numGrads);
  }

  return 0;
}

// AddingSensitivity:END ///////////////////////////////////


